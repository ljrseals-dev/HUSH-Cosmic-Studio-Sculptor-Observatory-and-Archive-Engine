#include "hush/recording_session.hpp"
#include <limits>
namespace hush {
RecordingSession::RecordingSession(std::uint16_t ch,std::size_t cap,std::size_t chunk):ring_(ch,cap){
 if(chunk&&ch&&chunk<=std::numeric_limits<std::size_t>::max()/ch)worker_buffer_.resize(chunk*ch);
}
bool RecordingSession::start(const std::filesystem::path& p,std::uint32_t rate,WavEncoding enc,std::string* e){
 if(recording_){if(e)*e="Recording session is already active.";return false;}
 if(!ring_.valid()||worker_buffer_.empty()){if(e)*e="Recording session has invalid or zero capacity.";return false;}
 if(!writer_.start(p,rate,ring_.channels(),enc,e))return false;recording_=true;return true;
}
bool RecordingSession::capture(std::span<const float> s)noexcept{return recording_&&ring_.push(s);}
bool RecordingSession::drain(std::string* e){
 if(!recording_){if(e)*e="Recording session is not active.";return false;}
 const auto n=ring_.pop(worker_buffer_);if(!n){if(e)e->clear();return true;}
 return writer_.append(std::span<const float>(worker_buffer_.data(),n*ring_.channels()),e);
}
bool RecordingSession::stop(std::string* e){
 if(e)e->clear();if(!recording_)return true;bool ok=true;
 while(ring_.available_frames()!=ring_.capacity_frames()){if(!drain(e)){ok=false;break;}}
 if(!writer_.finalize(e))ok=false;recording_=false;return ok;
}
} // namespace hush
