#include <javm/javm_VM.hpp>

namespace javm::vm {

    namespace {

        std::vector<Ptr<ThreadAccessor>> g_ThreadList;
        Monitor g_ThreadListLock;

        Ptr<ThreadAccessor> g_ThrownThread;
        Ptr<Variable> g_ThrownThrowable;
        Monitor g_ThrownLock;
        bool g_ThrownNotified = true;

        void ThreadEntrypoint(void *thread_ptr) {
            auto thread_ref = reinterpret_cast<native::Thread*>(thread_ptr);
            auto thread_v = thread_ref->GetThreadVariable();
            auto thread_obj = thread_v->GetAs<type::ClassInstance>();

            const auto eetop = thread_ref->GetHandle();
            thread_obj->SetField(u"eetop", u"J", NewPrimitiveVariable<type::Long>(eetop));
            const auto prio = native::GetThreadPriority(eetop);
            thread_obj->SetField(u"priority", u"I", NewPrimitiveVariable<type::Integer>(prio));

            const auto res = thread_obj->CallInstanceMethod(u"run", u"()V", thread_v);
            if(res.Is<ExecutionStatus::Thrown>() && !IsThrown()) {
                RegisterThrown(res.var);
            }
            
            UnregisterSelf();
        }

    }

    String ThreadAccessor::GetThreadName() {
        auto thread_v = this->thread_obj->GetThreadVariable();
        auto thread_obj = thread_v->GetAs<type::ClassInstance>();

        const auto name_res = thread_obj->CallInstanceMethod(u"getName", u"()Ljava/lang/String;", thread_v);
        if(!name_res.IsInvalidOrThrown()) {
            return jutil::GetStringValue(name_res.var);
        }

        return u"";
    }

    void ThreadAccessor::SetThreadName(const String &new_name) {
        auto thread_v = this->thread_obj->GetThreadVariable();
        auto thread_obj = thread_v->GetAs<type::ClassInstance>();

        auto name_v = jutil::NewString(new_name);
        thread_obj->CallInstanceMethod(u"setName", u"(Ljava/lang/String;)V", thread_v, name_v);
    }

    void ThreadAccessor::PushNewCall(Ptr<ClassType> caller_type, const String &invokable_name, const String &invokable_desc) {
        this->call_stack.push_back({ caller_type, this->caller_sensitive, invokable_name, invokable_desc, 0 });
    }

    void ThreadAccessor::PopCurrentCall() {
        this->call_stack.pop_back();
    }

    void ThreadAccessor::UpdateCurrentCallCodeOffset(const u16 code_offset) {
        auto &cur_call = this->call_stack.back();
        cur_call.code_offset = code_offset;
    }

    CallerSensitiveGuard::CallerSensitiveGuard() : already_guarded(false) {
        auto cur_accessor = GetCurrentThread();
        if(cur_accessor) {
            if(cur_accessor->IsCallerSensitive()) {
                this->already_guarded = true;
            }
            else {
                cur_accessor->EnableCallerSensitive();
            }
        }
    }

    CallerSensitiveGuard::~CallerSensitiveGuard() {
        if(!this->already_guarded) {
            auto cur_accessor = GetCurrentThread();
            if(cur_accessor) {
                cur_accessor->DisableCallerSensitive();
            }
        }
    }

    Ptr<ThreadAccessor> RegisterThread(Ptr<native::Thread> thread_obj) {
        ScopedMonitorLock lk(g_ThreadListLock);

        auto accessor = ptr::New<ThreadAccessor>(thread_obj);
        g_ThreadList.push_back(accessor);
        return accessor;
    }

    void UnregisterThread(Ptr<native::Thread> thread_obj) {
        ScopedMonitorLock lk(g_ThreadListLock);

        g_ThreadList.erase(std::remove_if(g_ThreadList.begin(), g_ThreadList.end(), [&](const Ptr<ThreadAccessor> &accessor) -> bool {
            return accessor->GetThreadHandle() == thread_obj->GetHandle();
        }), g_ThreadList.end());
    }

    void UnregisterSelf() {
        ScopedMonitorLock lk(g_ThreadListLock);

        const auto cur_handle = native::GetCurrentThreadHandle();
        g_ThreadList.erase(std::remove_if(g_ThreadList.begin(), g_ThreadList.end(), [&](const Ptr<ThreadAccessor> &accessor) -> bool {
            return accessor->GetThreadHandle() == cur_handle;
        }), g_ThreadList.end());
    }

    Ptr<Variable> GetCurrentThreadVariable() {
        ScopedMonitorLock lk(g_ThreadListLock);

        const auto cur_handle = native::GetCurrentThreadHandle();
        for(const auto &accessor: g_ThreadList) {
            if(accessor->GetThreadHandle() == cur_handle) {
                return accessor->GetThreadVariable();
            }
        }
        return nullptr;
    }

    Ptr<ThreadAccessor> GetThreadByHandle(const native::ThreadHandle handle) {
        ScopedMonitorLock lk(g_ThreadListLock);

        for(const auto &accessor: g_ThreadList) {
            if(accessor->GetThreadHandle() == handle) {
                return accessor;
            }
        }
        return nullptr;
    }

    u32 GetThreadCount() {
        ScopedMonitorLock lk(g_ThreadListLock);

        return g_ThreadList.size();
    }

    void RegisterAndStartThread(Ptr<Variable> thread_var) {
        auto thread = native::CreateThread();
        thread->SetThreadVariable(thread_var);

        RegisterThread(thread);
        thread->Start(&ThreadEntrypoint);
    }

    void RegisterThrown(Ptr<Variable> throwable_v) {
        ScopedMonitorLock lk(g_ThrownLock);

        g_ThrownThread = GetCurrentThread();
        g_ThrownThrowable = throwable_v;
        g_ThrownNotified = false;
    }

    Ptr<ThreadAccessor> RetrieveThrownThread() {
        ScopedMonitorLock lk(g_ThrownLock);

        auto thrown_accessor = g_ThrownThread;
        g_ThrownThread = nullptr;
        return thrown_accessor;
    }

    Ptr<Variable> RetrieveThrownThrowable() {
        ScopedMonitorLock lk(g_ThrownLock);

        auto thrown_throwable_v = g_ThrownThrowable;
        g_ThrownThrowable = nullptr;
        return thrown_throwable_v;
    }

    void ResetThrown() {
        ScopedMonitorLock lk(g_ThrownLock);

        g_ThrownThread = nullptr;
        g_ThrownThrowable = nullptr;
        g_ThrownNotified = true;
    }

    bool IsThrown() {
        ScopedMonitorLock lk(g_ThrownLock);

        return ptr::IsValid(g_ThrownThrowable);
    }

    bool IsThrownNotified() {
        ScopedMonitorLock lk(g_ThrownLock);

        return g_ThrownNotified;
    }

    void NotifyThrownNotified() {
        ScopedMonitorLock lk(g_ThrownLock);

        g_ThrownNotified = true;
    }

    ExecutionResult ThrowAlreadyThrown() {
        ScopedMonitorLock lk(g_ThrownLock);

        return ThrowExisting(g_ThrownThrowable, false);
    }

}