
#pragma once
#include <javm/vm/vm_Variable.hpp>
#include <javm/vm/jutil/jutil_Throwable.hpp>
#include <javm/vm/jutil/jutil_String.hpp>
#include <javm/native/native_NativeThread.hpp>

namespace javm::vm {

    // TODO: does the eetop field hold anything else? it appears to be an internal VM pointer rather than just a thread handle...

    using ThreadHandle = native::ThreadHandle;

    struct CallInfo {
        Ptr<ClassType> caller_type;
        bool caller_sensitive;
        String invokable_name;
        String invokable_desc;
        u16 code_offset;
    };

    class ThreadAccessor {
        private:
            Ptr<native::Thread> thread_obj;
            Ptr<Variable> throwable_v;
            std::vector<CallInfo> call_stack;
            String cached_name;
            bool caller_sensitive;

        public:
            ThreadAccessor(Ptr<native::Thread> thr_obj) : thread_obj(thr_obj), caller_sensitive(false) {}

            inline ThreadHandle GetThreadHandle() {
                return this->thread_obj->GetHandle();
            }

            inline Ptr<native::Thread> GetThreadObject() {
                return this->thread_obj;
            }

            void CacheThreadName();

            inline void SetCachedThreadName(const String &name) {
                this->cached_name = name;
            }

            inline String GetThreadName() {
                if(this->cached_name.empty()) {
                    this->CacheThreadName();
                }
                return this->cached_name;
            }

            inline Ptr<Variable> GetThreadVariable() {
                return this->thread_obj->GetThreadVariable();
            }

            void PushNewCall(Ptr<ClassType> caller_type, const String &invokable_name, const String &invokable_desc);
            void PopCurrentCall();
            void UpdateCurrentCallCodeOffset(const u16 code_offset);

            inline void EnableCallerSensitive() {
                this->caller_sensitive = true;
            }

            inline void DisableCallerSensitive() {
                this->caller_sensitive = false;
            }

            inline bool IsCallerSensitive() {
                return this->caller_sensitive;
            }

            inline CallInfo GetCurrentCallInfo() {
                return this->call_stack.back();
            }

            inline std::vector<CallInfo> &GetCallStack() {
                return this->call_stack;
            }

            inline std::vector<CallInfo> GetInvertedCallStack() {
                auto stack = this->GetCallStack();
                std::reverse(stack.begin(), stack.end());
                return stack;
            }
    };

    class CallerSensitiveGuard {
        private:
            bool already_guarded;

        public:
            CallerSensitiveGuard();
            ~CallerSensitiveGuard();
    };

    Ptr<ThreadAccessor> RegisterThread(Ptr<native::Thread> thread_obj);
    void UnregisterThread(Ptr<native::Thread> thread_obj);
    void UnregisterSelf();

    Ptr<Variable> GetCurrentThreadVariable();
    Ptr<ThreadAccessor> GetThreadByHandle(const native::ThreadHandle handle);

    inline Ptr<ThreadAccessor> GetCurrentThread() {
        const auto cur_handle = native::GetCurrentThreadHandle();
        return GetThreadByHandle(cur_handle);
    }

    u32 GetThreadCount();

    void RegisterAndStartThread(Ptr<Variable> thread_var);
    Ptr<Variable> RegisterMainThread();

    void RegisterThrown(Ptr<Variable> throwable_v);
    Ptr<ThreadAccessor> RetrieveThrownThread();
    Ptr<Variable> RetrieveThrownThrowable();
    void ResetThrown();
    bool IsThrown();
    bool IsThrownNotified();
    void NotifyThrownNotified();
    ExecutionResult ThrowAlreadyThrown();

}