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
    const auto truncated = root / "hush_wav_reader_truncated_contract.wav";
    const auto bad_alignment = root / "hush_wav_reader_alignment_contract.wav";
    const auto nonfinite = root / "hush_wav_reader_nonfinite_contract.wav";
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
    assert(audio.interleaved_samples.empty());

    // A physically truncated file must be rejected even if its header still
    // advertises the original, longer RIFF container.
    {
        std::ifstream in(pcm, std::ios::binary);
        std::vector<char> bytes{std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>()};
        assert(bytes.size() > 2U);
        bytes.resize(bytes.size() - 2U);
        std::ofstream out(truncated, std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    assert(!hush::read_wav(truncated, audio, &error));
    assert(audio.interleaved_samples.empty());

    // Corrupt the data-chunk length so it is not a whole number of frames.
    {
        std::ifstream in(pcm, std::ios::binary);
        std::vector<char> bytes{std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>()};
        const std::string raw(bytes.begin(), bytes.end());
        const auto pos = raw.find("data");
        assert(pos != std::string::npos && pos + 8U <= bytes.size());
        const auto size_pos = pos + 4U;
        bytes[size_pos] = 5;
        bytes[size_pos + 1U] = 0;
        bytes[size_pos + 2U] = 0;
        bytes[size_pos + 3U] = 0;
        std::ofstream out(bad_alignment, std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    assert(!hush::read_wav(bad_alignment, audio, &error));
    assert(audio.interleaved_samples.empty());

    // IEEE NaN input is sanitized to silence rather than propagated downstream.
    {
        std::ifstream in(floating, std::ios::binary);
        std::vector<char> bytes{std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>()};
        const std::string raw(bytes.begin(), bytes.end());
        const auto pos = raw.find("data");
        assert(pos != std::string::npos && pos + 12U <= bytes.size());
        const auto sample_pos = pos + 8U;
        bytes[sample_pos] = 0;
        bytes[sample_pos + 1U] = 0;
        bytes[sample_pos + 2U] = static_cast<char>(0xC0);
        bytes[sample_pos + 3U] = static_cast<char>(0x7F);
        std::ofstream out(nonfinite, std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    assert(hush::read_wav(nonfinite, audio, &error));
    assert(audio.interleaved_samples.front() == 0.0F);

    std::filesystem::remove(pcm);
    std::filesystem::remove(floating);
    std::filesystem::remove(broken);
    std::filesystem::remove(truncated);
    std::filesystem::remove(bad_alignment);
    std::filesystem::remove(nonfinite);
    return 0;
}
