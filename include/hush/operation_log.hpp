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
    std::size_t cursor() const noexcept { return cursor_; }
private:
    std::vector<Operation> operations_;
    std::size_t cursor_{0};
    static bool apply_value(Project&, const Operation&, bool forward);
};
}