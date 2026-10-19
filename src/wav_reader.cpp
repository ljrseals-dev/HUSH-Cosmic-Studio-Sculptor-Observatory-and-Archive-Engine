#include "hush/wav_reader.hpp"

#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

namespace hush {
namespace {

bool fail(std::string* error, const char* message) {
    if (error) *error = message;
    return false;
}

bool read_exact(std::istream& in, char* data, std::streamsize size) {
    in.read(data, size);
    return in.good() || in.gcount() == size;
}

std::uint16_t u16(const unsigned char* p) {
    return static_cast<std::uint16_t>(p[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8U);
}

std::uint32_t u32(const unsigned char* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8U) |
           (static_cast<std::uint32_t>(p[2]) << 16U) |
           (static_cast<std::uint32_t>(p[3]) << 24U);
}

bool tag_is(const unsigned char* p, const char* tag) {
    return p[0] == static_cast<unsigned char>(tag[0]) &&
           p[1] == static_cast<unsigned char>(tag[1]) &&
           p[2] == static_cast<unsigned char>(tag[2]) &&
           p[3] == static_cast<unsigned char>(tag[3]);
}

} // namespace

bool read_wav(const std::filesystem::path& path, WavAudioFile& out,
              std::string* error, WavReadOptions options) {
    if (error) error->clear();
    out = {};
    if (path.empty()) return fail(error, "Input path is empty.");

    std::ifstream in(path, std::ios::binary);
    if (!in) return fail(error, "Could not open input WAV file.");

    unsigned char header[12]{};
    if (!read_exact(in, reinterpret_cast<char*>(header), 12))
        return fail(error, "WAV header is truncated.");
    if (!tag_is(header, "RIFF") || !tag_is(header + 8, "WAVE"))
        return fail(error, "File is not a little-endian RIFF/WAVE file.");

    const std::uint64_t riff_end = static_cast<std::uint64_t>(u32(header + 4)) + 8ULL;
    if (riff_end < 12ULL) return fail(error, "RIFF size is invalid.");

    bool have_fmt = false;
    bool have_data = false;
    std::uint16_t format = 0;
    std::uint16_t channels = 0;
    std::uint16_t bits = 0;
    std::uint16_t block_align = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t data_size = 0;
    std::streampos data_start{};

    while (true) {
        const auto chunk_header_pos = in.tellg();
        if (chunk_header_pos < 0) break;
        const auto chunk_header_offset = static_cast<std::uint64_t>(chunk_header_pos);
        if (chunk_header_offset > riff_end || riff_end - chunk_header_offset < 8ULL) break;
        unsigned char chunk[8]{};
        if (!read_exact(in, reinterpret_cast<char*>(chunk), 8))
            return fail(error, "WAV chunk header is truncated.");
        const std::uint32_t chunk_size = u32(chunk + 4);
        const auto payload_start = in.tellg();
        if (payload_start < 0) return fail(error, "Could not locate WAV chunk.");
        const std::uint64_t payload_pos = static_cast<std::uint64_t>(payload_start);
        const std::uint64_t padded_size = static_cast<std::uint64_t>(chunk_size) + (chunk_size & 1U);
        if (payload_pos > riff_end || padded_size > riff_end - payload_pos)
            return fail(error, "WAV chunk exceeds the declared RIFF boundary.");

        if (tag_is(chunk, "fmt ")) {
            if (chunk_size < 16U) return fail(error, "WAV format chunk is too short.");
            unsigned char fmt[16]{};
            if (!read_exact(in, reinterpret_cast<char*>(fmt), 16))
                return fail(error, "WAV format chunk is truncated.");
            format = u16(fmt);
            channels = u16(fmt + 2);
            sample_rate = u32(fmt + 4);
            block_align = u16(fmt + 12);
            bits = u16(fmt + 14);
            have_fmt = true;
        } else if (tag_is(chunk, "data") && !have_data) {
            data_start = payload_start;
            data_size = chunk_size;
            have_data = true;
        }

        in.seekg(static_cast<std::streamoff>(payload_pos + padded_size), std::ios::beg);
        if (!in) return fail(error, "Could not seek to the next WAV chunk.");
        if (have_fmt && have_data) break;
    }

    if (!have_fmt) return fail(error, "WAV format chunk is missing.");
    if (!have_data) return fail(error, "WAV data chunk is missing.");
    if (channels == 0U || channels > 8U)
        return fail(error, "WAV channel count must be between 1 and 8.");
    if (sample_rate == 0U) return fail(error, "WAV sample rate must be greater than zero.");
    if (!((format == 1U && bits == 16U) || (format == 3U && bits == 32U)))
        return fail(error, "Only PCM16 and IEEE float32 WAV files are supported.");
    const std::uint16_t bytes_per_sample = bits / 8U;
    const std::uint32_t expected_align = static_cast<std::uint32_t>(channels) * bytes_per_sample;
    if (block_align != expected_align)
        return fail(error, "WAV block alignment does not match its channel format.");
    if (data_size % block_align != 0U)
        return fail(error, "WAV data size is not aligned to complete audio frames.");
    if (data_size > options.max_data_bytes)
        return fail(error, "WAV data exceeds the configured import size limit.");

    const std::uint64_t sample_count_64 = data_size / bytes_per_sample;
    if (sample_count_64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        return fail(error, "WAV sample count exceeds this platform's addressable memory.");

    std::vector<float> decoded;
    decoded.resize(static_cast<std::size_t>(sample_count_64));
    in.clear();
    in.seekg(data_start);
    if (!in) return fail(error, "Could not seek to WAV audio data.");

    unsigned char bytes[4]{};
    for (float& sample : decoded) {
        if (!read_exact(in, reinterpret_cast<char*>(bytes), bytes_per_sample))
            return fail(error, "WAV audio data is truncated.");
        if (format == 1U) {
            const auto raw = u16(bytes);
            const auto signed_value = raw < 0x8000U
                ? static_cast<std::int32_t>(raw)
                : static_cast<std::int32_t>(raw) - 65536;
            sample = static_cast<float>(signed_value) / 32768.0F;
        } else {
            sample = std::bit_cast<float>(u32(bytes));
            if (!std::isfinite(sample)) sample = 0.0F;
        }
    }

    out.sample_rate = sample_rate;
    out.channels = channels;
    out.interleaved_samples = std::move(decoded);
    return true;
}

} // namespace hush
