#include "hush/project.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <regex>
#include <sstream>
#include <limits>
#include <locale>
#include <system_error>
#include <unordered_set>
#include <unordered_map>

namespace hush {
namespace {
std::string escape_json(const std::string& s) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string out;
    for (const char raw : s) {
        const auto c = static_cast<unsigned char>(raw);
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20U) {
                    out += "\\u00";
                    out += hex[c >> 4U];
                    out += hex[c & 0x0fU];
                } else {
                    out.push_back(raw);
                }
                break;
        }
    }
    return out;
}

bool read_hex4(const std::string& text, std::size_t offset, std::uint32_t& value) {
    if (offset > text.size() || text.size() - offset < 4U) return false;
    value = 0U;
    for (std::size_t i = 0; i < 4U; ++i) {
        const char c = text[offset + i];
        std::uint32_t digit = 0U;
        if (c >= '0' && c <= '9') digit = static_cast<std::uint32_t>(c - '0');
        else if (c >= 'a' && c <= 'f') digit = static_cast<std::uint32_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') digit = static_cast<std::uint32_t>(c - 'A' + 10);
        else return false;
        value = (value << 4U) | digit;
    }
    return true;
}

void append_utf8(std::string& out, std::uint32_t codepoint) {
    if (codepoint <= 0x7fU) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7ffU) {
        out.push_back(static_cast<char>(0xc0U | (codepoint >> 6U)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    } else if (codepoint <= 0xffffU) {
        out.push_back(static_cast<char>(0xe0U | (codepoint >> 12U)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    } else {
        out.push_back(static_cast<char>(0xf0U | (codepoint >> 18U)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3fU)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    }
}

bool unescape_json(const std::string& text, std::string& out) {
    out.clear();
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c != '\\') {
            if (static_cast<unsigned char>(c) < 0x20U) return false;
            out.push_back(c);
            continue;
        }
        if (++i >= text.size()) return false;
        switch (text[i]) {
            case '\\': out.push_back('\\'); break;
            case '"': out.push_back('"'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                std::uint32_t codepoint = 0U;
                if (!read_hex4(text, i + 1U, codepoint)) return false;
                i += 4U;
                if (codepoint >= 0xd800U && codepoint <= 0xdbffU) {
                    if (text.size() - (i + 1U) < 6U ||
                        text[i + 1U] != '\\' || text[i + 2U] != 'u') return false;
                    std::uint32_t low = 0U;
                    if (!read_hex4(text, i + 3U, low) || low < 0xdc00U || low > 0xdfffU) return false;
                    codepoint = 0x10000U + ((codepoint - 0xd800U) << 10U) + (low - 0xdc00U);
                    i += 6U;
                } else if (codepoint >= 0xdc00U && codepoint <= 0xdfffU) {
                    return false;
                }
                append_utf8(out, codepoint);
                break;
            }
            default: return false;
        }
    }
    return true;
}

bool extract_json_string(const std::string& json, std::size_t value_pos, std::string& out) {
    if (value_pos >= json.size() || json[value_pos] != '"') return false;
    bool escaped = false;
    for (std::size_t i = value_pos + 1U; i < json.size(); ++i) {
        const char c = json[i];
        if (escaped) escaped = false;
        else if (c == '\\') escaped = true;
        else if (c == '"') {
            return unescape_json(json.substr(value_pos + 1U, i - value_pos - 1U), out);
        }
    }
    return false;
}

bool find_top_level_key_value(const std::string& json, const std::string& key,
                              std::size_t& value_pos) {
    std::size_t object_depth = 0U;
    std::size_t array_depth = 0U;
    for (std::size_t i = 0; i < json.size(); ++i) {
        const char c = json[i];
        if (c == '"') {
            const std::size_t token_start = i + 1U;
            bool escaped = false;
            std::size_t token_end = token_start;
            for (; token_end < json.size(); ++token_end) {
                const char token_char = json[token_end];
                if (escaped) escaped = false;
                else if (token_char == '\\') escaped = true;
                else if (token_char == '"') break;
            }
            if (token_end == json.size()) return false;
            if (object_depth == 1U && array_depth == 0U &&
                json.compare(token_start, token_end - token_start, key) == 0) {
                std::size_t next = token_end + 1U;
                while (next < json.size() &&
                       std::isspace(static_cast<unsigned char>(json[next])) != 0) ++next;
                if (next < json.size() && json[next] == ':') {
                    ++next;
                    while (next < json.size() &&
                           std::isspace(static_cast<unsigned char>(json[next])) != 0) ++next;
                    value_pos = next;
                    return true;
                }
            }
            i = token_end;
        } else if (c == '{') {
            ++object_depth;
        } else if (c == '}') {
            if (object_depth == 0U) return false;
            --object_depth;
        } else if (c == '[') {
            ++array_depth;
        } else if (c == ']') {
            if (array_depth == 0U) return false;
            --array_depth;
        }
    }
    return false;
}

bool parse_float_token(const std::string& text, float& value) {
    float parsed = 0.0F;
    const auto result = std::from_chars(text.data(), text.data() + text.size(),
                                         parsed, std::chars_format::general);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
    value = parsed;
    return true;
}

std::string law_name(PanLaw law) {
    return law == PanLaw::LegacyMinus4_5dB ? "legacy_minus_4_5_db" : "equal_power_minus_3_db";
}
std::string tap_name(SendTap tap) {
    return tap == SendTap::PreFader ? "pre_fader" : "post_fader";
}
bool extract_array(const std::string& s, const std::string& key, std::string& body, bool required, Error& e) {
    std::size_t value_pos = 0U;
    if (!find_top_level_key_value(s, key, value_pos)) {
        if (required) e={"array_missing","",key + " array is missing","error"};
        body.clear();
        return !required;
    }
    if (value_pos >= s.size() || s[value_pos] != '[') {
        e={"array_invalid","",key + " array must be a JSON array","error"};
        return false;
    }
    const auto left = value_pos;
    bool quoted=false, escaped=false;
    int depth=0;
    for (std::size_t i=left;i<s.size();++i) {
        const char c=s[i];
        if (quoted) {
            if (escaped) escaped=false;
            else if (c=='\\') escaped=true;
            else if (c=='"') quoted=false;
            continue;
        }
        if (c=='"') { quoted=true; continue; }
        if (c=='[') ++depth;
        else if (c==']' && --depth==0) { body=s.substr(left+1,i-left-1); return true; }
    }
    e={"array_invalid","",key + " array is unterminated","error"};
    return false;
}
bool count_object_array_items(const std::string& body, std::size_t& count) {
    count = 0U;
    std::size_t i = 0U;
    const auto skip_space = [&]() {
        while (i < body.size() && std::isspace(static_cast<unsigned char>(body[i])) != 0) ++i;
    };
    skip_space();
    while (i < body.size()) {
        if (body[i] != '{') return false;
        std::size_t depth = 0U;
        bool quoted = false;
        bool escaped = false;
        for (; i < body.size(); ++i) {
            const char c = body[i];
            if (quoted) {
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') quoted = false;
                continue;
            }
            if (c == '"') quoted = true;
            else if (c == '{') ++depth;
            else if (c == '}') {
                if (depth == 0U) return false;
                --depth;
                if (depth == 0U) {
                    ++i;
                    break;
                }
            }
        }
        if (depth != 0U || quoted) return false;
        ++count;
        skip_space();
        if (i == body.size()) return true;
        if (body[i] != ',') return false;
        ++i;
        skip_space();
        if (i == body.size()) return false;
    }
    return true;
}
}
std::string to_json(const Project& p) {
    std::ostringstream o;
    o.imbue(std::locale::classic());
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
    o << "],\n  \"buses\": [";
    for (std::size_t i=0;i<p.buses.size();++i) {
        const auto& b=p.buses[i];
        o << (i ? ",\n" : "\n") << "    {\"id\": \"" << escape_json(b.id)
          << "\", \"name\": \"" << escape_json(b.name) << "\"}";
    }
    if (!p.buses.empty()) o << "\n  ";
    o << "],\n  \"routes\": [";
    for (std::size_t i=0;i<p.routes.size();++i) {
        const auto& r=p.routes[i];
        o << (i ? ",\n" : "\n") << "    {\"source_bus_id\": \"" << escape_json(r.source_bus_id)
          << "\", \"destination_bus_id\": \"" << escape_json(r.destination_bus_id)
          << "\", \"tap\": \"" << tap_name(r.tap) << "\", \"gain\": " << r.gain << "}";
    }
    if (!p.routes.empty()) o << "\n  ";
    o << "],\n  \"media_sources\": [";
    for (std::size_t i=0;i<p.media_sources.size();++i) {
        const auto& source=p.media_sources[i];
        o << (i ? ",\n" : "\n") << "    {\"id\": \"" << escape_json(source.id)
          << "\", \"relative_path\": \"" << escape_json(source.relative_path)
          << "\", \"sample_rate\": " << source.sample_rate
          << ", \"channels\": " << source.channels
          << ", \"frame_count\": " << source.frame_count << "}";
    }
    if (!p.media_sources.empty()) o << "\n  ";
    o << "],\n  \"clips\": [";
    for (std::size_t i=0;i<p.clips.size();++i) {
        const auto& clip=p.clips[i];
        o << (i ? ",\n" : "\n") << "    {\"id\": \"" << escape_json(clip.id)
          << "\", \"track_id\": \"" << escape_json(clip.track_id)
          << "\", \"source_id\": \"" << escape_json(clip.source_id)
          << "\", \"timeline_start_frame\": " << clip.timeline_start_frame
          << ", \"source_start_frame\": " << clip.source_start_frame
          << ", \"frame_count\": " << clip.frame_count
          << ", \"gain\": " << clip.gain
          << ", \"fade_in_frames\": " << clip.fade_in_frames
          << ", \\"fade_out_frames\\": " << clip.fade_out_frames
          << ", \\"take_id\\": \\"" << escape_json(clip.take_id)
          << "\\", \\"take_number\\": " << clip.take_number << "}";
    }
    if (!p.clips.empty()) o << "\n  ";
    o << "]\n}\n";
    return o.str();
}

// Strict reader for the schema-v1 model subset emitted by to_json. Bus and route arrays
// are optional for compatibility with earlier schema-v1 project files.
bool from_json(const std::string& s, Project& out, Error& e) {
    std::size_t value_pos = 0U;
    if (!find_top_level_key_value(s, "schema_version", value_pos) ||
        value_pos >= s.size() || s[value_pos] != '1' ||
        (value_pos + 1U < s.size() &&
         s[value_pos + 1U] != ',' && s[value_pos + 1U] != '}' &&
         std::isspace(static_cast<unsigned char>(s[value_pos + 1U])) == 0)) {
        e={"schema_version_unsupported","","Expected schema_version 1","error"}; return false;
    }
    Project p;
    p.schema_version=1;
    if (!find_top_level_key_value(s, "id", value_pos) ||
        !extract_json_string(s, value_pos, p.id)) {
        e={"project_id_missing","","Project id is missing or contains an invalid JSON escape","error"}; return false;
    }
    if (!find_top_level_key_value(s, "pan_law", value_pos)) {
        e={"pan_law_missing","","Pan law is missing","error"}; return false;
    }
    std::string pan_law;
    if (!extract_json_string(s, value_pos, pan_law)) {
        e={"pan_law_invalid","","Pan law is not a valid JSON string","error"}; return false;
    }
    if (pan_law=="legacy_minus_4_5_db") p.pan_law=PanLaw::LegacyMinus4_5dB;
    else if (pan_law=="equal_power_minus_3_db") p.pan_law=PanLaw::EqualPowerMinus3dB;
    else { e={"pan_law_invalid","","Unknown pan law","error"}; return false; }

    std::string body;
    std::size_t expected_objects = 0U;
    if (!extract_array(s,"tracks",body,true,e)) return false;
    if (!count_object_array_items(body, expected_objects)) {
        e={"track_array_invalid","","Track array must contain complete JSON objects","error"}; return false;
    }
    std::unordered_set<std::string> track_ids;
    const std::regex track_re(R"json(\{\s*"id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"name"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"gain"\s*:\s*(-?[0-9.eE+]+)\s*,\s*"pan"\s*:\s*(-?[0-9.eE+]+)\s*,\s*"mute"\s*:\s*(true|false)\s*,\s*"solo"\s*:\s*(true|false)\s*\})json");
    auto begin=std::sregex_iterator(body.begin(),body.end(),track_re), end=std::sregex_iterator();
    for (auto it=begin;it!=end;++it) {
        Track t; const auto& x=*it;
        if (!unescape_json(x[1].str(), t.id) || !unescape_json(x[2].str(), t.name)) {
            e={"track_string_escape_invalid","","Track ID or name contains an invalid JSON escape","error"}; return false;
        }
        if (!parse_float_token(x[3].str(), t.gain) || !parse_float_token(x[4].str(), t.pan)) {
            e={"track_number_invalid",t.id,"Gain or pan is invalid","error"}; return false;
        }
        t.mute=x[5].str()=="true"; t.solo=x[6].str()=="true";
        if (t.id.empty() || !track_ids.insert(t.id).second) {
            e={"track_id_duplicate",t.id,"Track IDs must be non-empty and unique","error"}; return false;
        }
        if (!std::isfinite(t.gain) || !std::isfinite(t.pan) || t.pan < -1.0F || t.pan > 1.0F) {
            e={"track_value_invalid",t.id,"Track gain or pan is invalid","error"}; return false;
        }
        p.tracks.push_back(t);
    }
    if (p.tracks.size() != expected_objects) {
        e={"track_parse_failed","","One or more track objects did not match schema-v1 fields","error"}; return false;
    }

    if (!extract_array(s,"buses",body,false,e)) return false;
    if (!count_object_array_items(body, expected_objects)) {
        e={"bus_array_invalid","","Bus array must contain complete JSON objects","error"}; return false;
    }
    const std::regex bus_re(R"json(\{\s*"id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"name"\s*:\s*"((?:\\.|[^"\\])*)"\s*\})json");
    begin=std::sregex_iterator(body.begin(),body.end(),bus_re);
    for (auto it=begin;it!=end;++it) {
        Bus b;
        if (!unescape_json((*it)[1].str(), b.id) || !unescape_json((*it)[2].str(), b.name)) {
            e={"bus_string_escape_invalid","","Bus ID or name contains an invalid JSON escape","error"}; return false;
        }
        if (b.id.empty()) { e={"bus_id_empty","","Bus ID must not be empty","error"}; return false; }
        p.buses.push_back(std::move(b));
    }
    if (p.buses.size() != expected_objects) {
        e={"bus_parse_failed","","One or more bus objects did not match schema-v1 fields","error"}; return false;
    }

    if (!extract_array(s,"routes",body,false,e)) return false;
    if (!count_object_array_items(body, expected_objects)) {
        e={"route_array_invalid","","Route array must contain complete JSON objects","error"}; return false;
    }
    const std::regex route_re(R"json(\{\s*"source_bus_id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"destination_bus_id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"tap"\s*:\s*"(pre_fader|post_fader)"\s*,\s*"gain"\s*:\s*(-?[0-9.eE+]+)\s*\})json");
    begin=std::sregex_iterator(body.begin(),body.end(),route_re);
    for (auto it=begin;it!=end;++it) {
        const auto& x=*it;
        float gain = 0.0F;
        std::string source_id;
        std::string destination_id;
        if (!unescape_json(x[1].str(), source_id) || !unescape_json(x[2].str(), destination_id)) {
            e={"route_string_escape_invalid","","Route bus IDs contain an invalid JSON escape","error"}; return false;
        }
        if (!parse_float_token(x[4].str(), gain)) {
            e={"route_gain_invalid","","Route gain is invalid","error"}; return false;
        }
        p.routes.push_back({std::move(source_id),std::move(destination_id),
            x[3].str()=="pre_fader"?SendTap::PreFader:SendTap::PostFader,gain});
    }
    if (p.routes.size() != expected_objects) {
        e={"route_parse_failed","","One or more route objects did not match schema-v1 fields","error"}; return false;
    }
    if (!p.buses.empty() || !p.routes.empty()) {
        BusGraph graph; RoutingError routing_error;
        if (!graph.configure(p.buses,p.routes,routing_error)) {
            e={routing_error.code,routing_error.entity_id,routing_error.message,"error"}; return false;
        }
    }

    if (!extract_array(s,"media_sources",body,false,e)) return false;
    if (!count_object_array_items(body, expected_objects)) {
        e={"media_source_array_invalid","","Media source array must contain complete JSON objects","error"}; return false;
    }
    const std::regex source_re(R"json(\{\s*"id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"relative_path"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"sample_rate"\s*:\s*(\d+)\s*,\s*"channels"\s*:\s*(\d+)\s*,\s*"frame_count"\s*:\s*(\d+)\s*\})json");
    begin=std::sregex_iterator(body.begin(),body.end(),source_re);
    std::unordered_set<std::string> source_ids;
    for (auto it=begin;it!=end;++it) {
        const auto& x=*it;
        try {
            const auto rate=std::stoull(x[3].str());
            const auto channels=std::stoull(x[4].str());
            const auto frames=std::stoull(x[5].str());
            std::string source_id;
            std::string relative_path;
            if (!unescape_json(x[1].str(), source_id) || !unescape_json(x[2].str(), relative_path)) {
                e={"media_source_string_escape_invalid","","Media source ID or path contains an invalid JSON escape","error"}; return false;
            }
            if (rate==0U || rate>std::numeric_limits<std::uint32_t>::max() ||
                channels==0U || channels>8U || frames==0U ||
                source_id.empty() || relative_path.empty() || !source_ids.insert(source_id).second) {
                e={"media_source_invalid",source_id,"Media source metadata or ID is invalid/duplicated","error"}; return false;
            }
            p.media_sources.push_back({std::move(source_id),std::move(relative_path),static_cast<std::uint32_t>(rate),
                static_cast<std::uint16_t>(channels),static_cast<std::uint64_t>(frames)});
        } catch (...) { e={"media_source_number_invalid",x[1].str(),"Media source numeric metadata is invalid","error"}; return false; }
    }
    if (p.media_sources.size() != expected_objects) {
        e={"media_source_parse_failed","","One or more media source objects did not match schema-v1 fields","error"}; return false;
    }

    if (!extract_array(s,"clips",body,false,e)) return false;
    if (!count_object_array_items(body, expected_objects)) {
        e={"audio_clip_array_invalid","","Clip array must contain complete JSON objects","error"}; return false;
    }
    const std::regex clip_re(R"json(\{\s*"id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"track_id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"source_id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"timeline_start_frame"\s*:\s*(-?\d+)\s*,\s*"source_start_frame"\s*:\s*(\d+)\s*,\s*"frame_count"\s*:\s*(\d+)\s*,\s*"gain"\s*:\s*(-?[0-9.eE+]+)(?:\s*,\s*"fade_in_frames"\s*:\s*(\d+))?(?:\s*,\s*"fade_out_frames"\s*:\s*(\d+))?(?:\s*,\s*"take_id"\s*:\s*"((?:\\.|[^"\\])*)")?(?:\s*,\s*"take_number"\s*:\s*(\d+))?\s*\})json");
    begin=std::sregex_iterator(body.begin(),body.end(),clip_re);
    std::unordered_set<std::string> clip_ids;
    std::unordered_map<std::string, std::uint32_t> take_numbers;
    for (auto it=begin;it!=end;++it) {
        const auto& x=*it;
        try {
            AudioClip clip;
            if (!unescape_json(x[1].str(), clip.id) || !unescape_json(x[2].str(), clip.track_id) ||
                !unescape_json(x[3].str(), clip.source_id)) {
                e={"audio_clip_string_escape_invalid","","Clip IDs contain an invalid JSON escape","error"}; return false;
            }
            clip.timeline_start_frame=std::stoll(x[4].str());
            clip.source_start_frame=std::stoull(x[5].str());
            clip.frame_count=std::stoull(x[6].str());
            if (!parse_float_token(x[7].str(), clip.gain)) {
                e={"audio_clip_number_invalid",clip.id,"Clip gain is invalid","error"}; return false;
            }
            clip.fade_in_frames = x[8].matched ? std::stoull(x[8].str()) : 0U;
            clip.fade_out_frames = x[9].matched ? std::stoull(x[9].str()) : 0U;
            if (x[10].matched && !unescape_json(x[10].str(), clip.take_id)) {
                e={"audio_clip_take_id_invalid",clip.id,"Take ID contains an invalid JSON escape","error"}; return false;
            }
            clip.take_number = x[11].matched ? static_cast<std::uint32_t>(std::stoul(x[11].str())) : 0U;
            const auto track=std::find_if(p.tracks.begin(),p.tracks.end(),
                [&](const Track& t){ return t.id==clip.track_id; });
            const auto source=std::find_if(p.media_sources.begin(),p.media_sources.end(),
                [&](const MediaSource& m){ return m.id==clip.source_id; });
            if (clip.id.empty() || clip.track_id.empty() || clip.source_id.empty() ||
                !clip_ids.insert(clip.id).second || track==p.tracks.end() ||
                source==p.media_sources.end() || clip.frame_count==0U ||
                !std::isfinite(clip.gain) || clip.gain<0.0F ||
                clip.fade_in_frames > clip.frame_count || clip.fade_out_frames > clip.frame_count ||
                (clip.take_id.empty() != (clip.take_number == 0U)) ||
                clip.source_start_frame>source->frame_count ||
                clip.frame_count>source->frame_count-clip.source_start_frame ||
                clip.frame_count > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
                clip.timeline_start_frame > std::numeric_limits<std::int64_t>::max() - static_cast<std::int64_t>(clip.frame_count)) {
                e={"audio_clip_invalid",clip.id,"Clip IDs, references, gain, or frame bounds are invalid","error"}; return false;
            }
            if (!clip.take_id.empty()) {
                const auto inserted = take_numbers.emplace(clip.take_id, clip.take_number);
                if (!inserted.second && inserted.first->second != clip.take_number) {
                    e={"audio_clip_take_number_conflict",clip.take_id,"Clips sharing a take ID must share one take number","error"}; return false;
                }
            }
            p.clips.push_back(std::move(clip));
        } catch (...) { e={"audio_clip_number_invalid",x[1].str(),"Clip frame positions or gain are invalid","error"}; return false; }
    }
    if (p.clips.size() != expected_objects) {
        e={"audio_clip_parse_failed","","One or more clip objects did not match schema-v1 fields","error"}; return false;
    }
    out=std::move(p); e={}; return true;
}
}
