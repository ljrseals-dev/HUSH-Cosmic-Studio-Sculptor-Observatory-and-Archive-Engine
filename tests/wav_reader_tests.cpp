#include "hush/wav_reader.hpp"
#include "hush/wav_writer.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
std::vector<unsigned char> read_bytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

bool write_bytes(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(out);
}

void put_u32(std::vector<unsigned char>& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4U; ++i)
        bytes.at(offset + i) = static_cast<unsigned char>((value >> (i * 8U)) & 0xffU);
}
} // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path();
    const auto pcm = root / "hush_wav_reader_pcm_contract.wav";
    const auto floating = root / "hush_wav_reader_float_contract.wav";
    const auto broken = root / "hush_wav_reader_broken_contract.wav";
    const auto bad_riff = root / "hush_wav_reader_bad_riff_contract.wav";
    const auto bad_rate = root / "hush_wav_reader_bad_rate_contract.wav";
    const auto non_finite = root / "hush_wav_reader_non_finite_contract.wav";
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
    assert(!error.empty() && audio.interleaved_samples.empty());

    // The declared RIFF boundary must not extend beyond the actual file.
    auto bytes = read_bytes(floating);
    assert(bytes.size() >= 44U);
    put_u32(bytes, 4U, 0xfffffff0U);
    assert(write_bytes(bad_riff, bytes));
    assert(!hush::read_wav(bad_riff, audio, &error));
    assert(!error.empty() && audio.interleaved_samples.empty());

    // A file with a valid block alignment but an inconsistent byte rate is invalid.
    bytes = read_bytes(floating);
    put_u32(bytes, 28U, 1U);
    assert(write_bytes(bad_rate, bytes));
    assert(!hush::read_wav(bad_rate, audio, &error));
    assert(!error.empty() && audio.interleaved_samples.empty());

    // Float WAVs containing NaN must decode to finite samples.
    bytes = read_bytes(floating);
    bytes.at(44U) = 0x00U;
    bytes.at(45U) = 0x00U;
    bytes.at(46U) = 0xc0U;
    bytes.at(47U) = 0x7fU;
    assert(write_bytes(non_finite, bytes));
    assert(hush::read_wav(non_finite, audio, &error));
    assert(audio.interleaved_samples[0] == 0.0F);

    hush::WavReadOptions tiny_limit;
    tiny_limit.max_data_bytes = 1U;
    assert(!hush::read_wav(pcm, audio, &error, tiny_limit));
    assert(!hush::read_wav({}, audio, &error));
    assert(!hush::read_wav(pcm, audio, &error, hush::WavReadOptions{0U}));

    std::filesystem::remove(pcm);
    std::filesystem::remove(floating);
    std::filesystem::remove(broken);
    std::filesystem::remove(bad_riff);
    std::filesystem::remove(bad_rate);
    std::filesystem::remove(non_finite);
    return 0;
}
