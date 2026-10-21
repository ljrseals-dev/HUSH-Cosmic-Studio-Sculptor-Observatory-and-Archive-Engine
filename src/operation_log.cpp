#include "hush/operation_log.hpp"
#include <algorithm>
namespace hush {
void OperationLog::append(Operation op) {
    if (cursor_ < operations_.size())
        operations_.erase(operations_.begin() + static_cast<std::ptrdiff_t>(cursor_), operations_.end());
    operations_.push_back(std::move(op));
    cursor_ = operations_.size();
    dirty_ = true;
}
bool OperationLog::apply_value(Project& p, const Operation& op, bool forward) {
    const float v = forward ? op.after_value : op.before_value;
    switch (op.kind) {
      case OperationKind::AddBusInsert:
      case OperationKind::RemoveBusInsert: {
        const bool should_exist = (op.kind == OperationKind::AddBusInsert) == forward;
        auto it = std::find_if(p.inserts.begin(), p.inserts.end(),
                               [&](const BusInsert& insert) { return insert.id == op.entity_id; });
        if (should_exist) {
            if (it != p.inserts.end()) return false;
            const auto& insert = forward ? op.after_insert : op.before_insert;
            const auto index = forward ? op.after_index : op.before_index;
            const auto position = p.inserts.begin() +
                static_cast<std::ptrdiff_t>(std::min(index, p.inserts.size()));
            p.inserts.insert(position, insert);
            return true;
        }
        if (it == p.inserts.end()) return false;
        p.inserts.erase(it);
        return true;
      }
      case OperationKind::SetInsertGain:
      case OperationKind::SetInsertEnabled:
      case OperationKind::SetInsertKind: {
        auto it = std::find_if(p.inserts.begin(), p.inserts.end(),
                               [&](const BusInsert& insert) { return insert.id == op.entity_id; });
        if (it == p.inserts.end()) return false;
        if (op.kind == OperationKind::SetInsertGain) it->gain = v;
        else if (op.kind == OperationKind::SetInsertEnabled) it->enabled = (v != 0.0F);
        else it->kind = v == 0.0F ? InsertKind::Gain : InsertKind::SoftClip;
        return true;
      }
      case OperationKind::SetBusGain:
      case OperationKind::SetBusMute:
      case OperationKind::SetBusSolo: {
        auto it = std::find_if(p.buses.begin(), p.buses.end(),
                               [&](const Bus& bus) { return bus.id == op.entity_id; });
        if (it == p.buses.end()) return false;
        switch (op.kind) {
          case OperationKind::SetBusGain: it->gain = v; break;
          case OperationKind::SetBusMute: it->mute = (v != 0.0F); break;
          case OperationKind::SetBusSolo: it->solo = (v != 0.0F); break;
          default: return false;
        }
        return true;
      }
      case OperationKind::SetGain:
      case OperationKind::SetPan:
      case OperationKind::SetMute:
      case OperationKind::SetSolo: {
        auto it = std::find_if(p.tracks.begin(), p.tracks.end(),
                               [&](const Track& t) { return t.id == op.entity_id; });
        if (it == p.tracks.end()) return false;
        switch (op.kind) {
          case OperationKind::SetGain: it->gain = v; break;
          case OperationKind::SetPan: it->pan = v; break;
          case OperationKind::SetMute: it->mute = (v != 0.0F); break;
          case OperationKind::SetSolo: it->solo = (v != 0.0F); break;
          default: return false;
        }
        return true;
      }
    }
    return false;
}
bool OperationLog::apply_and_append(Project& project, Operation op) {
    if (!apply_value(project, op, true)) return false;
    append(std::move(op));
    return true;
}
bool OperationLog::undo(Project& p) {
    if (cursor_ == 0) return false;
    if (!apply_value(p, operations_[cursor_ - 1], false)) return false;
    --cursor_;
    dirty_ = true;
    return true;
}
bool OperationLog::redo(Project& p) {
    if (cursor_ >= operations_.size()) return false;
    if (!apply_value(p, operations_[cursor_], true)) return false;
    ++cursor_;
    dirty_ = true;
    return true;
}
}