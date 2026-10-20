#include "hush/project.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <regex>
#include <sstream>
#include <limits>
#include <unordered_set>

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

bool unescape_json(const std::string& text, std::string& out) {
    out.clear();
    out.reserve(text.size());
    bool escaped = false;
    for (const char c : text) {
        if (!escaped) {
            if (c == '\\') {
                escaped = true;
            } else {
                out += c;
            }
            continue;
        }

        switch (c) {
            case '\\': out += '\\'; break;
            case '"': out += '"'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            default: return false;
        }
        escaped = false;
    }
    return !escaped;
}

bool parse_double_token(const std::string& text, double& value) {
    try {
        std::size_t consumed = 0;
        const double parsed = std::stod(text, &consumed);
        if (consumed != text.size()) return false;
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_float_token(const std::string& text, float& value) {
    try {
        std::size_t consumed = 0;
        const float parsed = std::stof(text, &consumed);
        if (consumed != text.size()) return false;
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

std::string law_name(PanLaw law) {
    return law == PanLaw::LegacyMinus4_5dB ? "legacy_minus_4_5_db" : "equal_power_minus_3_db";
}
std::string tap_name(SendTap tap) {
    return tap == SendTap::PreFader ? "pre_fader" : "post_fader";
}
bool extract_array(const std::string& s, const std::string& key, std::string& body, bool required, Error& e) {
    const auto key_pos=s.find("\"" + key + "\"");
    if (key_pos==std::string::npos) {
        if (required) e={"array_missing","",key + " array is missing","error"};
        body.clear();
        return !required;
    }
    const auto left=s.find('[',key_pos);
    if (left==std::string::npos) { e={"array_invalid","",key + " array malformed","error"}; return false; }
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
}
std::string to_json(const Project& p) {
    std::ostringstream o;
    o << std::setprecision(std::numeric_limits<double>::max_digits10);
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
          << ", \"gain\": " << clip.gain << "}";
    }
    if (!p.clips.empty()) o << "\n  ";
    o << "],\n  \"tempo_map\": [";
    for (std::size_t i=0;i<p.tempo_map.size();++i) {
        const auto& point=p.tempo_map[i];
        o << (i ? ",\n" : "\n") << "    {\"beat\": " << point.beat
          << ", \"bpm\": " << point.bpm << "}";
    }
    if (!p.tempo_map.empty()) o << "\n  ";
    o << "],\n  \"automation_lanes\": [";
    for (std::size_t i=0;i<p.automation_lanes.size();++i) {
        const auto& lane=p.automation_lanes[i];
        o << (i ? ",\n" : "\n") << "    {\"id\": \"" << escape_json(lane.id)
          << "\", \"target\": \"" << escape_json(lane.target)
          << "\", \"interpolation\": \""
          << (lane.interpolation==ProjectAutomationInterpolation::Step ? "step" : "linear")
          << "\", \"points\": [";
        for (std::size_t j=0;j<lane.points.size();++j) {
            const auto& point=lane.points[j];
            o << (j ? ", " : "") << "{\"beat\": " << point.beat
              << ", \"value\": " << point.value << "}";
        }
        o << "]}";
    }
    if (!p.automation_lanes.empty()) o << "\n  ";
    o << "]\n}\n";
    return o.str();
}

// Strict reader for the schema-v1 model subset emitted by to_json. Bus and route arrays
// are optional for compatibility with earlier schema-v1 project files.
bool from_json(const std::string& s, Project& out, Error& e) {
    std::smatch m;
    const std::regex version_re(R"("schema_version"\s*:\s*(\d+))");
    const std::regex id_re(R"REGEX("id"\s*:\s*"((?:\\.|[^"\\])*)")REGEX");
    const std::regex law_re(R"REGEX("pan_law"\s*:\s*"([^"]+)")REGEX");
    if (!std::regex_search(s,m,version_re) || m[1].str()!="1") {
        e={"schema_version_unsupported","","Expected schema_version 1","error"}; return false;
    }
    Project p;
    p.schema_version=1;
    if (!std::regex_search(s,m,id_re)) { e={"project_id_missing","","Project id is missing","error"}; return false; }
    if (!unescape_json(m[1].str(), p.id)) { e={"project_id_escape_invalid","","Project ID contains an unsupported or incomplete escape","error"}; return false; }
    if (!std::regex_search(s,m,law_re)) { e={"pan_law_missing","","Pan law is missing","error"}; return false; }
    if (m[1].str()=="legacy_minus_4_5_db") p.pan_law=PanLaw::LegacyMinus4_5dB;
    else if (m[1].str()=="equal_power_minus_3_db") p.pan_law=PanLaw::EqualPowerMinus3dB;
    else { e={"pan_law_invalid","","Unknown pan law","error"}; return false; }

    std::string body;
    if (!extract_array(s,"tracks",body,true,e)) return false;
    const std::regex track_re(R"json(\{\s*"id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"name"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"gain"\s*:\s*(-?[0-9.eE+]+)\s*,\s*"pan"\s*:\s*(-?[0-9.eE+]+)\s*,\s*"mute"\s*:\s*(true|false)\s*,\s*"solo"\s*:\s*(true|false)\s*\})json");
    auto begin=std::sregex_iterator(body.begin(),body.end(),track_re), end=std::sregex_iterator();
    for (auto it=begin;it!=end;++it) {
        Track t; const auto& x=*it;
        if (!unescape_json(x[1].str(), t.id) || !unescape_json(x[2].str(), t.name)) {
            e={"track_string_escape_invalid","","Track ID or name contains an unsupported or incomplete escape","error"}; return false;
        }
        if (!parse_float_token(x[3].str(), t.gain) || !parse_float_token(x[4].str(), t.pan)) {
            e={"track_number_invalid",t.id,"Gain or pan is invalid","error"}; return false;
        }
        t.mute=x[5].str()=="true"; t.solo=x[6].str()=="true";
        if (t.id.empty() || !std::isfinite(t.gain) || !std::isfinite(t.pan) || t.pan < -1.0F || t.pan > 1.0F) {
            e={"track_value_invalid",t.id,"Track id, gain, or pan is invalid","error"}; return false;
        }
        p.tracks.push_back(t);
    }
    if (body.find('{')!=std::string::npos && p.tracks.empty()) {
        e={"track_parse_failed","","Track object did not match schema-v1 fields","error"}; return false;
    }

    if (!extract_array(s,"buses",body,false,e)) return false;
    const std::regex bus_re(R"json(\{\s*"id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"name"\s*:\s*"((?:\\.|[^"\\])*)"\s*\})json");
    begin=std::sregex_iterator(body.begin(),body.end(),bus_re);
    for (auto it=begin;it!=end;++it) {
        Bus b;
        if (!unescape_json((*it)[1].str(), b.id) || !unescape_json((*it)[2].str(), b.name)) {
            e={"bus_string_escape_invalid","","Bus ID or name contains an unsupported or incomplete escape","error"}; return false;
        }
        if (b.id.empty()) { e={"bus_id_empty","","Bus ID must not be empty","error"}; return false; }
        p.buses.push_back(std::move(b));
    }
    if (body.find('{')!=std::string::npos && p.buses.empty()) {
        e={"bus_parse_failed","","Bus object did not match schema-v1 fields","error"}; return false;
    }

    if (!extract_array(s,"routes",body,false,e)) return false;
    const std::regex route_re(R"json(\{\s*"source_bus_id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"destination_bus_id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"tap"\s*:\s*"(pre_fader|post_fader)"\s*,\s*"gain"\s*:\s*(-?[0-9.eE+]+)\s*\})json");
    begin=std::sregex_iterator(body.begin(),body.end(),route_re);
    for (auto it=begin;it!=end;++it) {
        try {
            const auto& x=*it;
            float gain = 0.0F;
            std::string source_id;
            std::string destination_id;
            if (!unescape_json(x[1].str(), source_id) ||
                !unescape_json(x[2].str(), destination_id)) {
                e={"route_string_escape_invalid","","Route bus IDs contain an unsupported or incomplete escape","error"}; return false;
            }
            if (!parse_float_token(x[4].str(), gain)) {
                e={"route_gain_invalid","","Route gain is invalid","error"}; return false;
            }
            p.routes.push_back({std::move(source_id),std::move(destination_id),
                x[3].str()=="pre_fader"?SendTap::PreFader:SendTap::PostFader,gain});
        } catch (...) { e={"route_gain_invalid","","Route gain is invalid","error"}; return false; }
    }
    if (body.find('{')!=std::string::npos && p.routes.empty()) {
        e={"route_parse_failed","","Route object did not match schema-v1 fields","error"}; return false;
    }
    if (!p.buses.empty() || !p.routes.empty()) {
        BusGraph graph; RoutingError routing_error;
        if (!graph.configure(p.buses,p.routes,routing_error)) {
            e={routing_error.code,routing_error.entity_id,routing_error.message,"error"}; return false;
        }
    }

    if (!extract_array(s,"media_sources",body,false,e)) return false;
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
            if (!unescape_json(x[1].str(), source_id) ||
                !unescape_json(x[2].str(), relative_path)) {
                e={"media_source_string_escape_invalid","","Media source ID or path contains an unsupported or incomplete escape","error"}; return false;
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
    if (body.find('{')!=std::string::npos && p.media_sources.empty()) {
        e={"media_source_parse_failed","","Media source object did not match schema-v1 fields","error"}; return false;
    }

    if (!extract_array(s,"clips",body,false,e)) return false;
    const std::regex clip_re(R"json(\{\s*"id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"track_id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"source_id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"timeline_start_frame"\s*:\s*(\d+)\s*,\s*"source_start_frame"\s*:\s*(\d+)\s*,\s*"frame_count"\s*:\s*(\d+)\s*,\s*"gain"\s*:\s*(-?[0-9.eE+]+)\s*\})json");
    begin=std::sregex_iterator(body.begin(),body.end(),clip_re);
    std::unordered_set<std::string> clip_ids;
    for (auto it=begin;it!=end;++it) {
        const auto& x=*it;
        try {
            AudioClip clip;
            if (!unescape_json(x[1].str(), clip.id) ||
                !unescape_json(x[2].str(), clip.track_id) ||
                !unescape_json(x[3].str(), clip.source_id)) {
                e={"audio_clip_string_escape_invalid","","Clip IDs contain an unsupported or incomplete escape","error"}; return false;
            }
            clip.timeline_start_frame=std::stoull(x[4].str());
            clip.source_start_frame=std::stoull(x[5].str());
            clip.frame_count=std::stoull(x[6].str());
            if (!parse_float_token(x[7].str(), clip.gain)) {
                e={"audio_clip_number_invalid",clip.id,"Clip gain is invalid","error"}; return false;
            }
            const auto track=std::find_if(p.tracks.begin(),p.tracks.end(),
                [&](const Track& t){ return t.id==clip.track_id; });
            const auto source=std::find_if(p.media_sources.begin(),p.media_sources.end(),
                [&](const MediaSource& m){ return m.id==clip.source_id; });
            if (clip.id.empty() || clip.track_id.empty() || clip.source_id.empty() ||
                !clip_ids.insert(clip.id).second || track==p.tracks.end() ||
                source==p.media_sources.end() || clip.frame_count==0U ||
                !std::isfinite(clip.gain) || clip.gain<0.0F ||
                clip.source_start_frame>source->frame_count ||
                clip.frame_count>source->frame_count-clip.source_start_frame ||
                clip.timeline_start_frame>std::numeric_limits<std::uint64_t>::max()-clip.frame_count) {
                e={"audio_clip_invalid",clip.id,"Clip IDs, references, gain, or frame bounds are invalid","error"}; return false;
            }
            p.clips.push_back(std::move(clip));
        } catch (...) { e={"audio_clip_number_invalid",x[1].str(),"Clip frame positions or gain are invalid","error"}; return false; }
    }
    if (body.find('{')!=std::string::npos && p.clips.empty()) {
        e={"audio_clip_parse_failed","","Clip object did not match schema-v1 fields","error"}; return false;
    }
    if (!extract_array(s,"tempo_map",body,false,e)) return false;
    const std::regex tempo_re(R"json(\{\s*"beat"\s*:\s*(-?[0-9.eE+]+)\s*,\s*"bpm"\s*:\s*(-?[0-9.eE+]+)\s*\})json");
    begin=std::sregex_iterator(body.begin(),body.end(),tempo_re);
    double previous_tempo_beat=-1.0;
    for (auto it=begin;it!=end;++it) {
        double beat = 0.0;
        double bpm = 0.0;
        if (!parse_double_token((*it)[1].str(), beat) ||
            !parse_double_token((*it)[2].str(), bpm)) {
            e={"tempo_point_number_invalid","","Tempo point number is invalid","error"}; return false;
        }
        if (!std::isfinite(beat) || !std::isfinite(bpm) || beat<0.0 ||
            beat<=previous_tempo_beat || bpm<1.0 || bpm>1000.0 ||
            (p.tempo_map.empty() && beat!=0.0)) {
            e={"tempo_point_invalid","","Tempo map points must start at beat zero, be strictly ordered, and use 1–1000 BPM","error"}; return false;
        }
        p.tempo_map.push_back({beat,bpm});
        previous_tempo_beat=beat;
    }
    if (body.find('{')!=std::string::npos && p.tempo_map.empty()) {
        e={"tempo_map_parse_failed","","Tempo map point did not match schema-v1 fields","error"}; return false;
    }

    if (!extract_array(s,"automation_lanes",body,false,e)) return false;
    const std::regex lane_re(R"json(\{\s*"id"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"target"\s*:\s*"((?:\\.|[^"\\])*)"\s*,\s*"interpolation"\s*:\s*"(step|linear)"\s*,\s*"points"\s*:\s*\[([\s\S]*?)\]\s*\})json");
    const std::regex automation_point_re(R"json(\{\s*"beat"\s*:\s*(-?[0-9.eE+]+)\s*,\s*"value"\s*:\s*(-?[0-9.eE+]+)\s*\})json");
    begin=std::sregex_iterator(body.begin(),body.end(),lane_re);
    std::unordered_set<std::string> lane_ids;
    for (auto it=begin;it!=end;++it) {
        const auto& x=*it;
        ProjectAutomationLane lane;
        if (!unescape_json(x[1].str(), lane.id) ||
            !unescape_json(x[2].str(), lane.target)) {
            e={"automation_lane_string_invalid","","Automation lane strings contain an unsupported or incomplete escape","error"}; return false;
        }
        lane.interpolation=x[3].str()=="step" ? ProjectAutomationInterpolation::Step
                                               : ProjectAutomationInterpolation::Linear;
        if (lane.id.empty() || lane.target.empty() || !lane_ids.insert(lane.id).second) {
            e={"automation_lane_invalid",lane.id,"Automation lane ID and target must be non-empty and IDs unique","error"}; return false;
        }
        const std::string points_body=x[4].str();
        auto point_it=std::sregex_iterator(points_body.begin(),points_body.end(),automation_point_re);
        double previous_beat=-1.0;
        for (;point_it!=end;++point_it) {
            double beat = 0.0;
            float value = 0.0F;
            if (!parse_double_token((*point_it)[1].str(), beat) ||
                !parse_float_token((*point_it)[2].str(), value)) {
                e={"automation_point_number_invalid",lane.id,"Automation point number is invalid","error"}; return false;
            }
            if (!std::isfinite(beat) || !std::isfinite(value) || beat<0.0 || beat<=previous_beat) {
                e={"automation_point_invalid",lane.id,"Automation points must have finite values and strictly increasing non-negative beats","error"}; return false;
            }
            lane.points.push_back({beat,value});
            previous_beat=beat;
        }
        if (lane.points.empty()) {
            e={"automation_points_missing",lane.id,"Automation lane requires at least one valid point","error"}; return false;
        }
        p.automation_lanes.push_back(std::move(lane));
    }
    if (body.find('{')!=std::string::npos && p.automation_lanes.empty()) {
        e={"automation_lane_parse_failed","","Automation lane object did not match schema-v1 fields","error"}; return false;
    }

    out=std::move(p); e={}; return true;
}
}
