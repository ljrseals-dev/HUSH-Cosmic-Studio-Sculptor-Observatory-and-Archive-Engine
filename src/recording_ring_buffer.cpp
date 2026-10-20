#include "hush/recording_ring_buffer.hpp"
#include <algorithm>
#include <limits>
namespace hush {
RecordingRingBuffer::RecordingRingBuffer(std::uint16_t ch,std::size_t cap):channels_(ch),capacity_frames_(cap){
 if(!ch||ch>8U||!cap||cap>std::numeric_limits<std::size_t>::max()/ch)return;samples_.resize(cap*ch);
}
std::size_t RecordingRingBuffer::available_frames()const noexcept{
 const auto w=write_frame_.load(std::memory_order_acquire),r=read_frame_.load(std::memory_order_acquire);const auto q=w-r;
 return q>=capacity_frames_?0U:capacity_frames_-static_cast<std::size_t>(q);
}
bool RecordingRingBuffer::push(std::span<const float> in)noexcept{
 if(!valid()||in.size()%channels_)return false;const auto n=in.size()/channels_;if(!n)return true;
 if(n>capacity_frames_){dropped_frames_.fetch_add(n,std::memory_order_relaxed);return false;}
 const auto w=write_frame_.load(std::memory_order_relaxed),r=read_frame_.load(std::memory_order_acquire),queued=w-r;
 if(queued>capacity_frames_||n>capacity_frames_-static_cast<std::size_t>(queued)){dropped_frames_.fetch_add(n,std::memory_order_relaxed);return false;}
 for(std::size_t f=0;f<n;++f){const auto slot=static_cast<std::size_t>((w+f)%capacity_frames_);for(std::size_t c=0;c<channels_;++c)samples_[slot*channels_+c]=in[f*channels_+c];}
 write_frame_.store(w+n,std::memory_order_release);return true;
}
std::size_t RecordingRingBuffer::pop(std::span<float> out)noexcept{
 if(!valid()||out.size()%channels_)return 0;const auto requested=out.size()/channels_,r=read_frame_.load(std::memory_order_relaxed),w=write_frame_.load(std::memory_order_acquire);
 const auto queued=static_cast<std::size_t>(std::min<std::uint64_t>(w-r,capacity_frames_)),n=std::min(requested,queued);
 for(std::size_t f=0;f<n;++f){const auto slot=static_cast<std::size_t>((r+f)%capacity_frames_);for(std::size_t c=0;c<channels_;++c)out[f*channels_+c]=samples_[slot*channels_+c];}
 read_frame_.store(r+n,std::memory_order_release);return n;
}
} // namespace hush
