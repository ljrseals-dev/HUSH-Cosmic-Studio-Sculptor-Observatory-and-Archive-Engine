#include "hush/recovery_journal.hpp"
#include <cerrno>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace hush {
namespace {
constexpr std::uint64_t kMaxRecordBytes = 64ULL * 1024ULL * 1024ULL;
bool sync_file(const std::filesystem::path& path) {
#ifdef _WIN32
    const int fd = _wopen(path.c_str(), _O_WRONLY | _O_BINARY);
    if (fd < 0) return false;
    const int result = _commit(fd);
    _close(fd);
#else
    const int fd = ::open(path.c_str(), O_WRONLY);
    if (fd < 0) return false;
    const int result = ::fsync(fd);
    ::close(fd);
#endif
    return result == 0;
bool RecoveryJournal::recover_if_different(Project& in_out, bool& applied, Error& error) const {
    Project recovered;
    applied = false;
    if (!recover_latest(recovered, error)) return false;
    if (recovered != in_out) {
        in_out = std::move(recovered);
        applied = true;
    }
    error = {};
    return true;
}

}
}
bool RecoveryJournal::append_snapshot(const Project& project, Error& error) const {
    if (append_writer_) return append_writer_(path_, project, error);
    const std::string payload = to_json(project);
    if (payload.size() > kMaxRecordBytes) {
        error={"journal_record_too_large",project.id,"Snapshot exceeds the 64 MiB journal record limit","error"};
        return false;
    }
    std::error_code ec;
    const auto parent = path_.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent,ec);
    if (ec) { error={"journal_directory_failed","",ec.message(),"error"}; return false; }
    {
        std::ofstream file(path_,std::ios::binary|std::ios::app);
        if (!file) { error={"journal_open_failed","", "Could not open recovery journal for append","error"}; return false; }
        file << "HUSHJ1 " << payload.size() << '\n';
        file.write(payload.data(),static_cast<std::streamsize>(payload.size()));
        file.put('\n');
        file.flush();
        if (!file) { error={"journal_write_failed",project.id,"Could not write complete recovery record","error"}; return false; }
    }
    if (!sync_file(path_)) {
        error={"journal_sync_failed",project.id,"Recovery record was written but stable-storage sync failed","error"};
        return false;
    }
    error={};
    return true;
}
bool RecoveryJournal::recover_latest(Project& out, Error& error) const {
    std::ifstream file(path_,std::ios::binary);
    if (!file) { error={"journal_open_failed","", "Recovery journal does not exist or cannot be opened","error"}; return false; }
    bool found=false;
    Project latest;
    for (;;) {
        std::string header;
        if (!std::getline(file,header)) break;
        constexpr const char* prefix="HUSHJ1 ";
        if (header.rfind(prefix,0)!=0) break;
        std::uint64_t length=0;
        try {
            const std::string number=header.substr(7);
            if (number.empty() || number.find_first_not_of("0123456789")!=std::string::npos) break;
            length=std::stoull(number);
        } catch (...) { break; }
        if (length==0 || length>kMaxRecordBytes || length>static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) break;
        std::string payload(static_cast<std::size_t>(length),'\0');
        file.read(payload.data(),static_cast<std::streamsize>(length));
        if (file.gcount()!=static_cast<std::streamsize>(length)) break;
        char terminator=0;
        if (!file.get(terminator) || terminator!='\n') break;
        Project candidate;
        Error parse_error;
        if (!from_json(payload,candidate,parse_error)) break;
        latest=std::move(candidate);
        found=true;
    }
    if (!found) { error={"journal_no_valid_snapshot","", "No complete valid snapshot was found in the recovery journal","error"}; return false; }
    out=std::move(latest);
    error={};
    return true;
}
}
