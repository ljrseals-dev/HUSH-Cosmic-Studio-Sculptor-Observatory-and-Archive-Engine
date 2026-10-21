#include "hush/recording_session.hpp"
#include "hush/wav_stream_writer.hpp"
#include <chrono>
#include <span>
namespace hush {
RecordingSession::~RecordingSession(){std::string ignored;(void)stop(&ignored);}
bool RecordingSession::start(const std::filesystem::path& path,std::uint32_t rate,std::uint16_t channels,std::size_t capacity,std::size_t chunk,std::string* error){
 if(error)error->clear();if(worker_.joinable()||accepting_.load()){if(error)*error="Recording session already active.";return false;}
 if(rate==0U||channels==0U||channels>8U||chunk==0U||chunk>static_cast<std::size_t>(-1)/channels){if(error)*error="Invalid recording configuration.";return false;}
 if(!ring_.configure(capacity,channels)){if(error)*error="Could not configure capture ring.";return false;}
 try{scratch_.assign(chunk*channels,0.0F);}catch(...){if(error)*error="Could not allocate worker scratch.";return false;}
 {std::lock_guard lock(mutex_);error_.clear();}
 sample_rate_=rate;channels_=channels;frames_written_.store(0);stopping_.store(false);ready_.store(false);opened_.store(false);worker_failed_.store(false);
 worker_=std::thread(&RecordingSession::worker_main,this,path);
 while(!ready_.load(std::memory_order_acquire))std::this_thread::sleep_for(std::chrono::milliseconds(1));
 if(!opened_.load(std::memory_order_acquire)){worker_.join();if(error)*error=last_error();return false;}
 accepting_.store(true,std::memory_order_release);return true;
}
std::size_t RecordingSession::capture(std::span<const float> samples) noexcept {
 if(!accepting_.load(std::memory_order_acquire))return 0U;
 auto accepted=ring_.capture(samples);return accepted;
}
void RecordingSession::worker_main(std::filesystem::path path) noexcept {
 WavStreamWriter writer;std::string err;const bool opened=writer.open(path,sample_rate_,channels_,&err);
 if(!opened){std::lock_guard lock(mutex_);error_=err;worker_failed_.store(true);}
 opened_.store(opened,std::memory_order_release);ready_.store(true,std::memory_order_release);wake_.notify_all();
 if(!opened)return;
 for(;;){
  const auto frames=ring_.drain(scratch_);
  if(frames){if(!writer.write_frames(scratch_.data(),frames,&err)){std::lock_guard lock(mutex_);error_=err;worker_failed_.store(true);accepting_.store(false);stopping_.store(true);}else frames_written_.fetch_add(frames,std::memory_order_release);}
  if(stopping_.load(std::memory_order_acquire)&&ring_.frames_available()==0U)break;
  if(frames==0U){std::unique_lock lock(mutex_);wake_.wait_for(lock,std::chrono::milliseconds(2),[this]{return stopping_.load(std::memory_order_acquire)||ring_.frames_available()!=0U;});}
 }
 if(!writer.finalize(&err)){std::lock_guard lock(mutex_);if(error_.empty())error_=err;worker_failed_.store(true);}
}
bool RecordingSession::stop(std::string* error){
 if(error)error->clear();accepting_.store(false,std::memory_order_release);stopping_.store(true,std::memory_order_release);wake_.notify_one();if(worker_.joinable())worker_.join();
 bool ok=!worker_failed_.load(std::memory_order_acquire)&&last_error().empty();if(!ok&&error){*error=last_error();if(error->empty())*error="Recording worker failed.";}return ok;
}
std::string RecordingSession::last_error() const{std::lock_guard lock(mutex_);return error_;}
}