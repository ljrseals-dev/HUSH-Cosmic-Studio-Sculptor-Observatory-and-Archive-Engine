#pragma once
#include "hush/project.hpp"
#include <cstddef>
#include <string>
#include <vector>
namespace hush {
enum class OperationKind {
    SetGain, SetPan, SetMute, SetSolo,
    SetBusGain, SetBusMute, SetBusSolo,
    AddBusInsert, RemoveBusInsert, SetInsertGain, SetInsertEnabled, SetInsertKind
};
struct Operation {
    std::string entity_id;
    OperationKind kind{};
    float before_value{};
    float after_value{};
    BusInsert before_insert{};
    BusInsert after_insert{};
    std::size_t before_index{0};
    std::size_t after_index{0};
};
class OperationLog {
public:
    bool apply_and_append(Project& project, Operation op);
    void append(Operation op);
    bool undo(Project& project);
    bool redo(Project& project);
    std::size_t size() const noexcept { return operations_.size(); }
    [[nodiscard]] const Operation* operation_at(std::size_t index) const noexcept {
        return index < operations_.size() ? &operations_[index] : nullptr;
    }
    std::size_t cursor() const noexcept { return cursor_; }
    [[nodiscard]] bool dirty() const noexcept { return dirty_; }
    void mark_dirty() noexcept { dirty_ = true; }
    void mark_saved() noexcept { dirty_ = false; }
private:
    std::vector<Operation> operations_;
    std::size_t cursor_{0};
    bool dirty_{false};
    static bool apply_value(Project&, const Operation&, bool forward);
};
}