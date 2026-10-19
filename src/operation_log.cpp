#include "hush/operation_log.hpp"
#include <algorithm>
namespace hush {
void OperationLog::append(Operation op) {
    if (cursor_ < operations_.size()) operations_.erase(operations_.begin()+static_cast<std::ptrdiff_t>(cursor_),operations_.end());
    operations_.push_back(std::move(op)); cursor_=operations_.size();
}
bool OperationLog::apply_value(Project& p,const Operation& op,bool forward) {
    auto it=std::find_if(p.tracks.begin(),p.tracks.end(),[&](const Track& t){return t.id==op.entity_id;});
    if(it==p.tracks.end()) return false;
    const float v=forward?op.after_value:op.before_value;
    switch(op.kind) {
      case OperationKind::SetGain: it->gain=v; break;
      case OperationKind::SetPan: it->pan=v; break;
      case OperationKind::SetMute: it->mute=(v!=0.0F); break;
      case OperationKind::SetSolo: it->solo=(v!=0.0F); break;
    }
    return true;
}
bool OperationLog::apply_and_append(Project& project, Operation op) {
    if (!apply_value(project, op, true)) return false;
    append(std::move(op)); return true;
}
bool OperationLog::undo(Project& p) {
    if(cursor_==0) return false;
    if(!apply_value(p,operations_[cursor_-1],false)) return false;
    --cursor_; return true;
}
bool OperationLog::redo(Project& p) {
    if(cursor_>=operations_.size()) return false;
    if(!apply_value(p,operations_[cursor_],true)) return false;
    ++cursor_; return true;
}
}