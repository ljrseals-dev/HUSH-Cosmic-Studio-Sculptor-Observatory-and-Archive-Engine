#include "hush/project.hpp"
#include <cmath>
#include <iomanip>
#include <regex>
#include <sstream>

namespace hush {
namespace {
std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}
std::string law_name(PanLaw law) {
    return law == PanLaw::LegacyMinus4_5dB ? "legacy_minus_4_5_db" : "equal_power_minus_3_db";
}
}
std::string to_json(const Project& p) {
    std::ostringstream o;
    o << std::setprecision(9);
    o << "{\n  \"schema_version\": " << p.schema_version
      << ",\n  \"id\": \"" << escape_json(p.id) << "\""
      << ",\n  \"pan_law\": \"" << law_name(p.pan_law) << "\",\n  \"tracks\": [";
    for (std::size_t i=0; i<p.tracks.size(); ++i) {
        const auto& t=p.tracks[i];
        o << (i ? ",\n" : "\n") << "    {\"id\": \"" << escape_json(t.id)
          << "\", \"name\": \"" << escape_json(t.name) << "\", \"gain\": " << t.gain
          << ", \"pan\": " << t.pan << ", \"mute\": " << (t.mute?"true":"false")
          << ", \"solo\": " << (t.solo?"true":"false") << "}";
    }
    if (!p.tracks.empty()) o << "\n  ";
    o << "]\n}\n";
    return o.str();
}

// Strict reader for the canonical schema-v1 subset emitted by to_json.
// Rejects unknown schema versions and malformed/missing track fields.
bool from_json(const std::string& s, Project& out, Error& e) {
    std::smatch m;
    const std::regex version_re(R"("schema_version"\s*:\s*(\d+))");
    const std::regex id_re(R"("id"\s*:\s*"([^"]*)")");
    const std::regex law_re(R"("pan_law"\s*:\s*"([^"]+)")");
    if (!std::regex_search(s,m,version_re) || m[1].str()!="1") {
        e={"schema_version_unsupported","","Expected schema_version 1","error"}; return false;
    }
    Project p;
    p.schema_version=1;
    if (!std::regex_search(s,m,id_re)) { e={"project_id_missing","","Project id is missing","error"}; return false; }
    p.id=m[1].str();
    if (!std::regex_search(s,m,law_re)) { e={"pan_law_missing","","Pan law is missing","error"}; return false; }
    if (m[1].str()=="legacy_minus_4_5_db") p.pan_law=PanLaw::LegacyMinus4_5dB;
    else if (m[1].str()=="equal_power_minus_3_db") p.pan_law=PanLaw::EqualPowerMinus3dB;
    else { e={"pan_law_invalid","","Unknown pan law","error"}; return false; }

    const auto open=s.find("\"tracks\"");
    if (open==std::string::npos) { e={"tracks_missing","","Tracks array is missing","error"}; return false; }
    const auto left=s.find('[',open), right=s.find(']',left);
    if (left==std::string::npos || right==std::string::npos) { e={"tracks_invalid","","Tracks array malformed","error"}; return false; }
    const std::string body=s.substr(left+1,right-left-1);
    const std::regex track_re(R"json(\{\s*"id"\s*:\s*"([^"]*)"\s*,\s*"name"\s*:\s*"([^"]*)"\s*,\s*"gain"\s*:\s*(-?[0-9.eE+]+)\s*,\s*"pan"\s*:\s*(-?[0-9.eE+]+)\s*,\s*"mute"\s*:\s*(true|false)\s*,\s*"solo"\s*:\s*(true|false)\s*\})json");
    auto begin=std::sregex_iterator(body.begin(),body.end(),track_re), end=std::sregex_iterator();
    for (auto it=begin;it!=end;++it) {
        Track t; const auto& x=*it;
        t.id=x[1].str(); t.name=x[2].str();
        try { t.gain=std::stof(x[3].str()); t.pan=std::stof(x[4].str()); }
        catch (...) { e={"track_number_invalid",t.id,"Gain or pan is invalid","error"}; return false; }
        t.mute=x[5].str()=="true"; t.solo=x[6].str()=="true";
        if (t.id.empty() || !std::isfinite(t.gain) || !std::isfinite(t.pan) || t.pan < -1.0F || t.pan > 1.0F) {
            e={"track_value_invalid",t.id,"Track id, gain, or pan is invalid","error"}; return false;
        }
        p.tracks.push_back(t);
    }
    if (body.find('{')!=std::string::npos && p.tracks.empty()) {
        e={"track_parse_failed","","Track object did not match schema-v1 fields","error"}; return false;
    }
    out=std::move(p); e={}; return true;
}
}
