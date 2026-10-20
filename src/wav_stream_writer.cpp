#include "hush/wav_stream_writer.hpp"
#include <bit>
#include <cmath>
#include <limits>
namespace hush { namespace {
void u16(std::ostream& o, std::uint16_t v) { char b[2]{char(v&255U),char((v>>8U)&255U)}; o.write(b,2); }
void u32(std::ostream& o, std::uint32_t v) { char b[4]{char(v&255U),char((v>>8U)&255U),char((v>>16U)&255U),char((v>>24U)&255U)}; o.write(b,4); }
void tag(std::ostream& o,const char (&v)[5]) { o.write(v,4); }
bool fail(std::string* e,const char* m) { if(e)*e=m; return false; }
constexpr std::uint64_t max_data=std::numeric_limits<std::uint32_t>::max()-36ULL;
}
WavStreamWriter::~WavStreamWriter(){ if(file_.is_open()){std::string ignored;(void)finalize(&ignored);} }
bool WavStreamWriter::open(const std::filesystem::path& p,std::uint32_t rate,std::uint16_t ch,std::string* e){
 if(e)e->clear(); if(file_.is_open())return fail(e,"WAV stream already open.");
 if(p.empty()||rate==0U||ch==0U||ch>8U)return fail(e,"Invalid path, sample rate, or channel count.");
 if(static_cast<std::uint64_t>(rate)*ch*4ULL>std::numeric_limits<std::uint32_t>::max())return fail(e,"WAV byte rate overflow.");
 file_.open(p,std::ios::binary|std::ios::in|std::ios::out|std::ios::trunc);
 if(!file_)return fail(e,"Could not open WAV output.");
 sample_rate_=rate;channels_=ch;frames_written_=0;failed_=false;
 tag(file_,"RIFF");u32(file_,36U);tag(file_,"WAVE");tag(file_,"fmt ");u32(file_,16U);u16(file_,3U);u16(file_,ch);u32(file_,rate);u32(file_,rate*static_cast<std::uint32_t>(ch)*4U);u16(file_,static_cast<std::uint16_t>(ch*4U));u16(file_,32U);tag(file_,"data");u32(file_,0U);
 if(!file_){failed_=true;return fail(e,"Could not write initial WAV header.");} return true;
}
bool WavStreamWriter::write_frames(const float* p,std::uint64_t frames,std::string* e){
 if(e)e->clear();if(!file_.is_open()||failed_)return fail(e,"WAV stream is not writable.");
 if(frames&&p==nullptr)return fail(e,"Audio buffer is null.");
 const auto bytes_per_frame=static_cast<std::uint64_t>(channels_)*4ULL;
 if(frames_written_>max_data/bytes_per_frame||frames>(max_data/bytes_per_frame)-frames_written_)return fail(e,"RIFF/WAVE size limit reached; RF64 is not implemented.");
 static_assert(sizeof(float)==sizeof(std::uint32_t));static_assert(std::numeric_limits<float>::is_iec559);
 for(std::uint64_t i=0;i<frames*channels_;++i){float v=std::isfinite(p[i])?p[i]:0.0F;u32(file_,std::bit_cast<std::uint32_t>(v));if(!file_){failed_=true;return fail(e,"Disk write failed.");}}
 frames_written_+=frames;return true;
}
bool WavStreamWriter::finalize(std::string* e){
 if(e)e->clear();if(!file_.is_open())return !failed_||fail(e,"WAV stream previously failed.");
 const auto bytes=frames_written_*channels_*4ULL;file_.seekp(4,std::ios::beg);u32(file_,static_cast<std::uint32_t>(36ULL+bytes));file_.seekp(40,std::ios::beg);u32(file_,static_cast<std::uint32_t>(bytes));file_.flush();bool ok=bool(file_)&&!failed_;file_.close();if(!ok){failed_=true;return fail(e,"Could not finalize WAV header or close file.");}return true;
}
}