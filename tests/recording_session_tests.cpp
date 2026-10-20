#include "hush/recording_session.hpp"
#include "hush/wav_reader.hpp"
#include <array>
#include <cassert>
#include <filesystem>
#include <string>
int main(){
 const auto path=std::filesystem::temp_directory_path()/"hush_recording_session_contract.wav";std::string error;
 hush::RecordingSession session(2U,8U,4U);const std::array<float,2> before{0.1F,0.2F};assert(!session.capture(before));
 assert(session.start(path,48000U,hush::WavEncoding::Float32,&error));
 const std::array<float,6>a{0.1F,-0.1F,0.2F,-0.2F,0.3F,-0.3F};const std::array<float,4>b{0.4F,-0.4F,0.5F,-0.5F};
 assert(session.capture(a));assert(session.drain(&error));assert(session.capture(b));assert(session.drain(&error));assert(session.frames_written()==5U);
 assert(session.stop(&error));assert(!session.is_recording());hush::WavAudioFile decoded;assert(hush::read_wav(path,decoded,&error));
 assert(decoded.sample_rate==48000U&&decoded.channels==2U&&decoded.frame_count()==5U);
 assert(decoded.interleaved_samples[0]==a[0]&&decoded.interleaved_samples[5]==a[5]&&decoded.interleaved_samples[6]==b[0]);
 hush::RecordingRingBuffer ring(1U,2U);const std::array<float,2> first{1.0F,2.0F};const std::array<float,1> over{3.0F};
 assert(ring.push(first));assert(!ring.push(over));assert(ring.dropped_frames()==1U);std::array<float,2> popped{};assert(ring.pop(popped)==2U&&popped==first);
 std::filesystem::remove(path);return 0;
}
