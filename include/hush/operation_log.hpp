#pragma once
#include "hush/project.hpp"
#include <string>
#include <vector>
namespace hush {
enum class OperationKind { SetGain, SetPan, SetMute, SetSolo };
struct Operation { std::string entity_id; OperationKind kind{}; float before_value{}; float after_value{}; };
class OperationLog {
public:
    bool apply_and_append(Project& project, Operation op);
    void append(Operation op);
    bool undo(Project& project);
    bool redo(Project& project);
    std::size_t size() const noexcept { return operations_.size(); }
    // Read-only history access for synchronizing the exact operation touched by undo/redo.
    [[nodiscard]] const Operation* operation_at(std::size_t index) const noexcept {
        return index < operations_.size() ? &operations_[index] : nullptr;
    }
    std::size_t cursor() const noexcept { return cursor_; }
    [[nodiscard]] bool dirty() const noexcept { return dirty_; }
    void mark_dirty() noexcept { dirty_ = true; }
    // Call only after the canonical project save and recovery-journal save both succeed.
    void mark_saved() noexcept { dirty_ = false; }
private:
    bool dirty_{false};
    std::vector<Operation> operations_;
    std::size_t cursor_{0};
    static bool apply_value(Project&, const Operation&, bool forward);
};
}