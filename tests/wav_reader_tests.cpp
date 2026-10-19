#include "hush/wav_reader.hpp"
#include "hush/wav_writer.hpp"

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main() {
    const auto root = std::filesystem::temp_directory_path();
    const auto pcm = root / "hush_wav_reader_pcm_contract.wav";
    const auto floating = root / "hush_wav_reader_float_contract.wav";
    const auto broken = root / "hush_wav_reader_broken_contract.wav";
    const std::array<float, 6> samples{-1.0F, -0.5F, 0.0F, 0.25F, 0.5F, 1.0F};
    std::string error;

    assert(hush::write_wav(pcm, samples, 48000U, 2U, hush::WavEncoding::Pcm16, &error));
    hush::WavAudioFile audio;
    assert(hush::read_wav(pcm, audio, &error));
    assert(audio.sample_rate == 48000U && audio.channels == 2U);
    assert(audio.frame_count() == 3U && audio.interleaved_samples.size() == samples.size());
    assert(audio.interleaved_samples[0] == -1.0F);
    assert(audio.interleaved_samples[1] == -0.5F);
    assert(audio.interleaved_samples[3] > 0.24F && audio.interleaved_samples[3] < 0.26F);

    assert(hush::write_wav(floating, samples, 44100U, 2U, hush::WavEncoding::Float32, &error));
    assert(hush::read_wav(floating, audio, &error));
    assert(audio.sample_rate == 44100U && audio.channels == 2U);
    assert(audio.interleaved_samples.size() == samples.size());
    assert(audio.interleaved_samples[3] == samples[3]);

    {
        std::ofstream out(broken, std::ios::binary | std::ios::trunc);
        out.write("RIFF", 4);
        out.write("\0\0\0\0", 4);
        out.write("WAVE", 4);
    }
    assert(!hush::read_wav(broken, audio, &error));
    assert(!error.empty());
    assert(audio.interleaved_samples.empty());

    hush::WavReadOptions tiny_limit;
    tiny_limit.max_data_bytes = 1U;
    assert(!hush::read_wav(pcm, audio, &error, tiny_limit));

    std::filesystem::remove(pcm);
    std::filesystem::remove(floating);
    std::filesystem::remove(broken);
    return 0;
}
