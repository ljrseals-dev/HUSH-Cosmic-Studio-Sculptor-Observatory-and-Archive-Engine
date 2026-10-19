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

std::uint32_t u32(const std::vector<unsigned char>& bytes, std::size_t at) {
    return static_cast<std::uint32_t>(bytes.at(at)) |
           (static_cast<std::uint32_t>(bytes.at(at + 1U)) << 8U) |
           (static_cast<std::uint32_t>(bytes.at(at + 2U)) << 16U) |
           (static_cast<std::uint32_t>(bytes.at(at + 3U)) << 24U);
}

std::uint16_t u16(const std::vector<unsigned char>& bytes, std::size_t at) {
    return static_cast<std::uint16_t>(bytes.at(at)) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes.at(at + 1U)) << 8U);
}

std::vector<unsigned char> read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

int main() {
    const auto base = std::filesystem::temp_directory_path() / "hush_wav_writer_contract";
    const auto pcm_path = base.string() + "_pcm.wav";
    const auto float_path = base.string() + "_float.wav";

    const std::array<float, 4> samples{-1.0F, -0.5F, 0.5F, 1.0F};
    std::string error;
    assert(hush::write_wav(pcm_path, samples, 48000U, 2U, hush::WavEncoding::Pcm16, &error));
    auto pcm = read_file(pcm_path);
    assert(pcm.size() == 52U);
    assert(std::string(pcm.begin(), pcm.begin() + 4) == "RIFF");
    assert(u32(pcm, 4U) == 44U);
    assert(std::string(pcm.begin() + 8, pcm.begin() + 12) == "WAVE");
    assert(u16(pcm, 20U) == 1U);
    assert(u16(pcm, 22U) == 2U);
    assert(u32(pcm, 24U) == 48000U);
    assert(u16(pcm, 34U) == 16U);
    assert(u32(pcm, 40U) == 8U);
    assert(u16(pcm, 44U) == 0x8000U);
    assert(u16(pcm, 46U) == 0xc000U);
    assert(u16(pcm, 48U) == 0x3fffU);
    assert(u16(pcm, 50U) == 0x7fffU);

    assert(hush::write_wav(float_path, samples, 44100U, 2U, hush::WavEncoding::Float32, &error));
    auto fp = read_file(float_path);
    assert(fp.size() == 60U);
    assert(u16(fp, 20U) == 3U);
    assert(u16(fp, 34U) == 32U);
    assert(u32(fp, 40U) == 16U);

    assert(!hush::write_wav(pcm_path, samples, 0U, 2U, hush::WavEncoding::Pcm16, &error));
    assert(!error.empty());
    assert(!hush::write_wav(pcm_path, std::span<const float>(samples.data(), 3U), 48000U, 2U,
                            hush::WavEncoding::Pcm16, &error));
    assert(!hush::write_wav(pcm_path, samples, 48000U, 0U, hush::WavEncoding::Pcm16, &error));

    std::filesystem::remove(pcm_path);
    std::filesystem::remove(float_path);
    return 0;
}
