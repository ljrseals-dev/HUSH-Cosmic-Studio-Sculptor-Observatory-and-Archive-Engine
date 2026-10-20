#include "hush/wav_stream_writer.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
namespace hush { namespace {
void put_u16(std::ostream& o,std::uint16_t v){const char b[2]{static_cast<char>(v&255U),static_cast<char>((v>>8U)&255U)};o.write(b,2);}
void put_u32(std::ostream& o,std::uint32_t v){const char b[4]{static_cast<char>(v&255U),static_cast<char>((v>>8U)&255U),static_cast<char>((v>>16U)&255U),static_cast<char>((v>>24U)&255U)};o.write(b,4);}
void put_tag(std::ostream& o,const char(&t)[5]){o.write(t,4);}
bool fail(std::string* e,const char* m){if(e)*e=m;return false;}
}
WavStreamWriter::~WavStreamWriter(){if(out_.is_open()){std::string ignored;(void)finalize(&ignored);}}
bool WavStreamWriter::start(const std::filesystem::path& p,std::uint32_t rate,std::uint16_t ch,WavEncoding enc,std::string* e){
 if(e)e->clear();if(out_.is_open())return fail(e,"A WAV recording is already open.");if(p.empty())return fail(e,"Output path is empty.");
 if(!rate)return fail(e,"Sample rate must be greater than zero.");if(!ch||ch>8U)return fail(e,"Channel count must be between 1 and 8.");
 path_=p;sample_rate_=rate;channels_=ch;encoding_=enc;data_bytes_=0;frames_written_=0;
 out_.open(path_,std::ios::binary|std::ios::trunc|std::ios::in|std::ios::out);if(!out_)return fail(e,"Could not open output WAV file.");
 if(!write_header(0,e)){out_.close();return false;}out_.seekp(44,std::ios::beg);if(!out_){out_.close();return fail(e,"Could not position WAV data writer.");}return true;
}
bool WavStreamWriter::write_header(std::uint32_t data,std::string* e){
 const std::uint64_t bps=encoding_==WavEncoding::Pcm16?2ULL:4ULL,rate=static_cast<std::uint64_t>(sample_rate_)*channels_*bps;
 if(rate>std::numeric_limits<std::uint32_t>::max())return fail(e,"Sample rate and channel format exceed the WAV byte-rate limit.");
 out_.seekp(0,std::ios::beg);put_tag(out_,"RIFF");put_u32(out_,data+36U);put_tag(out_,"WAVE");put_tag(out_,"fmt ");put_u32(out_,16U);
 put_u16(out_,encoding_==WavEncoding::Pcm16?1U:3U);put_u16(out_,channels_);put_u32(out_,sample_rate_);put_u32(out_,static_cast<std::uint32_t>(rate));
 put_u16(out_,static_cast<std::uint16_t>(channels_*bps));put_u16(out_,static_cast<std::uint16_t>(bps*8ULL));put_tag(out_,"data");put_u32(out_,data);
 if(!out_)return fail(e,"Failed to write WAV header.");return true;
}
bool WavStreamWriter::append(std::span<const float> s,std::string* e){
 if(e)e->clear();if(!out_.is_open())return fail(e,"No WAV recording is open.");if(s.size()%channels_)return fail(e,"Sample count is not aligned to the channel count.");
 const std::uint64_t bps=encoding_==WavEncoding::Pcm16?2ULL:4ULL,add=static_cast<std::uint64_t>(s.size())*bps,max=std::numeric_limits<std::uint32_t>::max()-36ULL;
 if(data_bytes_>max||add>max-data_bytes_)return fail(e,"Recording exceeds the standard RIFF/WAVE size limit; stop before 4 GiB.");
 for(float x:s){const float f=std::isfinite(x)?x:0.0F;if(encoding_==WavEncoding::Pcm16){const float b=std::clamp(f,-1.0F,1.0F),v=b<0?b*32768.0F:b*32767.0F;put_u16(out_,static_cast<std::uint16_t>(static_cast<std::int16_t>(v)));}else{static_assert(sizeof(float)==sizeof(std::uint32_t));static_assert(std::numeric_limits<float>::is_iec559);put_u32(out_,std::bit_cast<std::uint32_t>(f));}if(!out_)return fail(e,"Failed while writing WAV recording samples.");}
 data_bytes_+=add;frames_written_+=static_cast<std::uint64_t>(s.size()/channels_);return true;
}
bool WavStreamWriter::finalize(std::string* e){
 if(e)e->clear();if(!out_.is_open())return true;bool ok=true;if(!write_header(static_cast<std::uint32_t>(data_bytes_),e))ok=false;out_.flush();
 if(!out_){if(e&&e->empty())*e="Failed to flush WAV recording.";ok=false;}out_.close();if(out_.fail()){if(e&&e->empty())*e="Failed to close WAV recording.";ok=false;}return ok;
}
} // namespace hush
