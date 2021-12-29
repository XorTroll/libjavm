#include <javm/javm_VM.hpp>

namespace javm::vm {

    void ExecutionFrame::VariableStack::ResetFill(Ptr<Variable> this_var, const u16 length) {
        if(this->IsGrowable()) {
            return;
        }

        auto len = length;
        if(this_var) {
            len++;
        }
        this->inner_list.clear();
        this->inner_list.reserve(len);
        for(u16 i = 0; i < len; i++) {
            this->inner_list.push_back(nullptr);
        }
    }

    Ptr<Variable> ExecutionFrame::VariableStack::GetAt(const u32 idx) {
        if(this->IsGrowable()) {
            return nullptr;
        }

        if(idx < this->inner_list.size()) {
            return this->inner_list[idx];
        }
        return nullptr;
    }

    void ExecutionFrame::VariableStack::SetAt(const u32 idx, Ptr<Variable> var) {
        if(this->IsGrowable()) {
            return;
        }
        if(idx < this->inner_list.size()) {
            this->inner_list[idx] = var;
        }
    }

    Ptr<Variable> ExecutionFrame::VariableStack::Pop() {
        if(this->IsFixedSize()) {
            return nullptr;
        }

        if(this->inner_list.empty()) {
            return nullptr;
        }
        auto back_var = this->inner_list.back();
        this->inner_list.pop_back();
        return back_var;
    }

    void ExecutionFrame::VariableStack::Push(Ptr<Variable> var) {
        if(this->IsFixedSize()) {
            return;
        }

        this->inner_list.push_back(var);
    }

    std::vector<ExceptionTableEntry> ExecutionFrame::GetAvailableExceptionTableEntries(const u32 base_code_offset) {
        JAVM_LOG("total count: %ld", this->exc_table.size());
        std::vector<ExceptionTableEntry> active_excs;
        for(const auto &exc_table_entry: this->exc_table) {
            JAVM_LOG("start: %d, cur: %d, end: %d", exc_table_entry.start_code_offset, base_code_offset, exc_table_entry.end_code_offset);
            if((exc_table_entry.start_code_offset <= base_code_offset) && (exc_table_entry.end_code_offset > base_code_offset)) {
                active_excs.push_back(exc_table_entry);
            }
        }
        return active_excs;
    }

    ExecutionScopeGuard::ExecutionScopeGuard(Ptr<ClassType> type, const String &name, const String &descriptor) : self_thrown(false) {
        /*
        // Only push on the call stack if nothing has been thrown
        if(!IsThrown()) {
            auto cur_thr = GetCurrentThread();
            if(cur_thr) {
                cur_thr->PushNewCall(type, name, descriptor);
            }
        }
        */

        auto cur_thr = GetCurrentThread();
        if(cur_thr) {
            cur_thr->PushNewCall(type, name, descriptor);
        }
    }

    ExecutionScopeGuard::~ExecutionScopeGuard() {
        /*
        // Only pop from the call stack if nothing has been thrown or if a throw happened in this execution
        if(!IsThrown() || this->self_thrown) {
            auto cur_thr = GetCurrentThread();
            if(cur_thr) {
                cur_thr->PopCurrentCall();
            }
        }
        */

        auto cur_thr = GetCurrentThread();
        if(cur_thr) {
            cur_thr->PopCurrentCall();
        }
    }

    void ExecutionScopeGuard::NotifyThrown() {
        // Check whether the throw was in this execution (or if it was in a previous execution but wasn't notified, for instance, because it was caller-sensitive)
        if(!IsThrown() || (IsThrown() && !IsThrownNotified())) {
            auto cur_thr = GetCurrentThread();
            if(cur_thr && cur_thr->IsCallerSensitive()) {
                return;
            }

            this->self_thrown = true;
            NotifyThrownNotified();
        }
    }

}