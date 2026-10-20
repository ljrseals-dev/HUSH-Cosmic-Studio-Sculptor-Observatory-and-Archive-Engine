#include "hush/midi_learn.hpp"

#include <cmath>

namespace hush {

bool MidiLearnMap::set_bindings(const MidiLearnBinding* bindings,
                                std::size_t count) noexcept {
    if (count > kMaxBindings || (count != 0 && bindings == nullptr)) {
        return false;
    }

    // Validate before changing live state, so a rejected configuration preserves
    // the previously installed mapping.
    for (std::size_t i = 0; i < count; ++i) {
        const auto& binding = bindings[i];
        if (binding.channel > 15 || binding.controller > 127 ||
            binding.parameter_id == 0 || !std::isfinite(binding.minimum) ||
            !std::isfinite(binding.maximum) || binding.minimum > binding.maximum) {
            return false;
        }
        for (std::size_t j = 0; j < i; ++j) {
            if (bindings[j].channel == binding.channel &&
                bindings[j].controller == binding.controller) {
                return false; // one CC source may map to only one parameter
            }
        }
    }

    for (std::size_t i = 0; i < count; ++i) {
        bindings_[i] = bindings[i];
    }
    count_ = count;
    return true;
}

bool MidiLearnMap::map_cc(const MidiCcEvent& event,
                          MappedParameterValue& output) const noexcept {
    if (event.channel > 15 || event.controller > 127 || event.value > 127) {
        return false;
    }

    for (std::size_t i = 0; i < count_; ++i) {
        const auto& binding = bindings_[i];
        if (binding.channel != event.channel ||
            binding.controller != event.controller) {
            continue;
        }

        const float normalized = static_cast<float>(event.value) / 127.0F;
        const float position = binding.inverted ? 1.0F - normalized : normalized;
        output.parameter_id = binding.parameter_id;
        output.value = binding.minimum +
            (binding.maximum - binding.minimum) * position;
        return true;
    }
    return false;
}

} // namespace hush
