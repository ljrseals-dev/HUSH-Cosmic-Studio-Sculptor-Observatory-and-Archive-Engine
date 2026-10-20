#include "hush/recording_session.hpp"
#include "hush/wav_reader.hpp"
#include <array>
#include <cassert>
#include <filesystem>
int main(){
 using namespace hush;const auto path=std::filesystem::temp_directory_path()/"hush-recording-session-test.wav";
 RecordingSession session;std::string error;assert(session.start(path,48000U,2U,64U,8U,&error));
 const std::array<float,8> samples{0.1F,-0.1F,0.2F,-0.2F,0.3F,-0.3F,0.4F,-0.4F};
 assert(session.capture(samples)==4U);assert(session.stop(&error));assert(session.frames_written()==4U);assert(session.dropped_frames()==0U);assert(std::filesystem::file_size(path)==44U+samples.size()*4U);std::filesystem::remove(path);return 0;
}