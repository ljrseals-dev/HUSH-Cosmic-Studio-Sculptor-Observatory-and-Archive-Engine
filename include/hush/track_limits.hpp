#pragma once

#include <cstddef>

// CMake defines HUSH_MAX_TRACKS for hush_core and consumers. The fallback keeps
// standalone header consumers aligned with the documented default.
#ifndef HUSH_MAX_TRACKS
#define HUSH_MAX_TRACKS 1024
#endif

namespace hush {
inline constexpr std::size_t kMaxTracks = static_cast<std::size_t>(HUSH_MAX_TRACKS);
} // namespace hush
