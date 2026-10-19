#include "hush/wav_writer.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <type_traits>

namespace hush {
namespace {

void put_u16(std::ostream& out, std::uint16_t value) {
    const char bytes[2] = {
        static_cast<char>(value & 0xffU),
        static_cast<char>((value >> 8U) & 0xffU)
    };
    out.write(bytes, 2);
}

void put_u32(std::ostream& out, std::uint32_t value) {
    const char bytes[4] = {
        static_cast<char>(value & 0xffU),
        static_cast<char>((value >> 8U) & 0xffU),
        static_cast<char>((value >> 16U) & 0xffU),
        static_cast<char>((value >> 24U) & 0xffU)
    };
    out.write(bytes, 4);
}

void put_tag(std::ostream& out, const char (&tag)[5]) {
    out.write(tag, 4);
}

bool fail(std::string* error, const char* message) {
    if (error) *error = message;
    return false;
}

} // namespace

bool write_wav(const std::filesystem::path& path,
               std::span<const float> interleaved_samples,
               std::uint32_t sample_rate,
               std::uint16_t channels,
               WavEncoding encoding,
               std::string* error) {
    if (error) error->clear();
    if (path.empty()) return fail(error, "Output path is empty.");
    if (sample_rate == 0U) return fail(error, "Sample rate must be greater than zero.");
    if (channels == 0U || channels > 8U) return fail(error, "Channel count must be between 1 and 8.");
    if (interleaved_samples.size() % channels != 0U)
        return fail(error, "Sample count is not aligned to the channel count.");

    constexpr std::uint64_t kMaxRiffData = std::numeric_limits<std::uint32_t>::max() - 36ULL;
    const std::uint64_t bytes_per_sample = encoding == WavEncoding::Pcm16 ? 2ULL : 4ULL;
    const auto sample_count = static_cast<std::uint64_t>(interleaved_samples.size());
    if (sample_count > kMaxRiffData / bytes_per_sample)
        return fail(error, "Audio is too large for a standard RIFF/WAVE file.");

    const auto data_bytes = static_cast<std::uint32_t>(sample_count * bytes_per_sample);
    const auto bits_per_sample = static_cast<std::uint16_t>(bytes_per_sample * 8ULL);
    const std::uint64_t byte_rate_64 = static_cast<std::uint64_t>(sample_rate) * channels * bytes_per_sample;
    if (byte_rate_64 > std::numeric_limits<std::uint32_t>::max())
        return fail(error, "Sample rate and channel format exceed the WAV byte-rate limit.");
    const auto byte_rate = static_cast<std::uint32_t>(byte_rate_64);
    const auto block_align = static_cast<std::uint16_t>(channels * bytes_per_sample);
    const std::uint16_t format_code = encoding == WavEncoding::Pcm16 ? 1U : 3U;

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return fail(error, "Could not open output WAV file.");

    put_tag(out, "RIFF");
    put_u32(out, data_bytes + 36U);
    put_tag(out, "WAVE");
    put_tag(out, "fmt ");
    put_u32(out, 16U);
    put_u16(out, format_code);
    put_u16(out, channels);
    put_u32(out, sample_rate);
    put_u32(out, byte_rate);
    put_u16(out, block_align);
    put_u16(out, bits_per_sample);
    put_tag(out, "data");
    put_u32(out, data_bytes);

    for (const float sample : interleaved_samples) {
        const float finite_sample = std::isfinite(sample) ? sample : 0.0F;
        if (encoding == WavEncoding::Pcm16) {
            const float bounded = std::clamp(finite_sample, -1.0F, 1.0F);
            const auto scaled = bounded < 0.0F ? bounded * 32768.0F : bounded * 32767.0F;
            const auto pcm = static_cast<std::int16_t>(scaled);
            put_u16(out, static_cast<std::uint16_t>(pcm));
        } else {
            static_assert(sizeof(float) == sizeof(std::uint32_t));
            static_assert(std::numeric_limits<float>::is_iec559);
            put_u32(out, std::bit_cast<std::uint32_t>(finite_sample));
        }
        if (!out) return fail(error, "Failed while writing WAV sample data.");
    }

    out.flush();
    if (!out) return fail(error, "Failed to flush WAV output.");
    out.close();
    if (out.fail()) return fail(error, "Failed to close WAV output.");
    return true;
}

} // namespace hush
