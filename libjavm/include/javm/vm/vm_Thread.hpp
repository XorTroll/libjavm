
#pragma once
#include <javm/vm/vm_Variable.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <javm/native/native_NativeThread.hpp>

namespace javm::vm {

    using ThreadId = native::ThreadId;
    using ThreadHandle = native::ThreadHandle;

    struct CallInfo {
        Ptr<ClassType> caller_type;
        std::string invokable_name;
        std::string invokable_desc;
    };

    class ThreadAccessor {

        private:
            Ptr<native::Thread> thread_obj;
            std::vector<CallInfo> call_stack;
            bool exception_thrown;

        public:
            ThreadAccessor(Ptr<native::Thread> thr_obj) : thread_obj(thr_obj), exception_thrown(false) {}

            ThreadId GetThreadId() {
                return this->thread_obj->GetId();
            }

            ThreadHandle GetThreadHandle() {
                return this->thread_obj->GetHandle();
            }

            Ptr<native::Thread> GetThreadObject() {
                return this->thread_obj;
            }

            std::string GetThreadName() {
                auto java_thr_v = this->thread_obj->GetJavaThreadVariable();
                auto thr_obj = java_thr_v->GetAs<type::ClassInstance>();
                auto name_res = thr_obj->CallInstanceMethod("getName", "()Ljava/lang/String;", java_thr_v);
                if(name_res.IsInvalidOrThrown()) {
                    return "";
                }
                auto name_v = name_res.ret_var;
                return StringUtils::GetValue(name_v);
            }

            Ptr<Variable> GetJavaThreadInstance() {
                return this->thread_obj->GetJavaThreadVariable();
            }

            void PushNewCall(Ptr<ClassType> caller_type, const std::string &invokable_name, const std::string &invokable_desc) {
                if(this->exception_thrown) {
                    return;
                }
                this->call_stack.push_back({ caller_type, invokable_name, invokable_desc });
            }

            void PopCurrentCall() {
                if(this->exception_thrown) {
                    return;
                }
                this->call_stack.pop_back();
            }

            void NotifyExceptionThrown() {
                this->exception_thrown = true;
            }

            CallInfo GetCurrentCallInfo() {
                return this->call_stack.back();
            }

            std::vector<CallInfo> GetCallStack() {
                return this->call_stack;
            }

            inline std::vector<CallInfo> GetInvertedCallStack() {
                auto stack = this->GetCallStack();
                std::reverse(stack.begin(), stack.end());
                return stack;
            }


    };

    namespace inner_impl {

        static void RegisterThreadImpl(ThreadId id, Ptr<Variable> java_thread);
        static void UnregisterThreadImpl(ThreadId id);

    }

    namespace inner_impl {

        static inline std::vector<Ptr<ThreadAccessor>> g_thread_table;
        static inline Monitor g_thread_table_lock;

        static void RegisterThreadImpl(Ptr<native::Thread> thread_obj) {
            ScopedMonitorLock lk(g_thread_table_lock);
            g_thread_table.push_back(PtrUtils::New<ThreadAccessor>(thread_obj));
        }

        static void UnregisterThreadImpl(Ptr<native::Thread> thread_obj) {
            ScopedMonitorLock lk(g_thread_table_lock);
            g_thread_table.erase(std::remove_if(g_thread_table.begin(), g_thread_table.end(), [&](const Ptr<ThreadAccessor> &thr) -> bool {
                return thr->GetThreadId() == thread_obj->GetId();
            }), g_thread_table.end());
        }

        static void UnregisterSelfImpl() {
            ScopedMonitorLock lk(g_thread_table_lock);
            auto self = native::GetCurrentThreadHandle();
            g_thread_table.erase(std::remove_if(g_thread_table.begin(), g_thread_table.end(), [&](const Ptr<ThreadAccessor> &thr) -> bool {
                return thr->GetThreadHandle() == self;
            }), g_thread_table.end());
        }

        static Ptr<Variable> GetCurrentThreadInstanceImpl() {
            ScopedMonitorLock lk(g_thread_table_lock);
            auto cur_id = native::GetCurrentThreadHandle();
            for(auto &thr: g_thread_table) {
                if(thr->GetThreadHandle() == cur_id) {
                    return thr->GetJavaThreadInstance();
                }
            }
            return nullptr;
        }

        static Ptr<ThreadAccessor> GetCurrentThreadImpl() {
            ScopedMonitorLock lk(g_thread_table_lock);
            auto cur_id = native::GetCurrentThreadHandle();
            for(auto &thr: g_thread_table) {
                if(thr->GetThreadId() == cur_id) {
                    return thr;
                }
            }
            return nullptr;
        }

        static Ptr<ThreadAccessor> GetThreadByHandleImpl(native::ThreadHandle handle) {
            ScopedMonitorLock lk(g_thread_table_lock);
            for(auto &thr: g_thread_table) {
                if(thr->GetThreadHandle() == handle) {
                    return thr;
                }
            }
            return nullptr;
        }

        static u32 GetThreadCountImpl() {
            ScopedMonitorLock lk(g_thread_table_lock);
            return g_thread_table.size();
        }

    }

    class ThreadUtils {

        private:
            static void ThreadEntrypoint(void *thr_ptr) {

                native::Thread *thr_ref = reinterpret_cast<native::Thread*>(thr_ptr);

                auto java_thr_v = thr_ref->GetJavaThreadVariable();
                auto java_thr_obj = java_thr_v->GetAs<type::ClassInstance>();

                auto prio = thr_ref->GetPriority();
                java_thr_obj->SetField("priority", "I", TypeUtils::NewPrimitiveVariable<type::Integer>(prio));
                auto eetop = thr_ref->GetHandle();
                java_thr_obj->SetField("eetop", "J", TypeUtils::NewPrimitiveVariable<type::Long>(eetop));

                java_thr_obj->CallInstanceMethod("run", "()V", java_thr_v);

                UnregisterSelf();

            }

        public:
            static inline Ptr<Variable> GetCurrentThreadInstance() {
                return inner_impl::GetCurrentThreadInstanceImpl();
            }

            static inline Ptr<ThreadAccessor> GetCurrentThread() {
                return inner_impl::GetCurrentThreadImpl();
            }

            static inline Ptr<ThreadAccessor> GetThreadByHandle(native::ThreadHandle handle) {
                return inner_impl::GetThreadByHandleImpl(handle);
            }

            static inline u32 GetThreadCount() {
                return inner_impl::GetThreadCountImpl();
            }

            static inline void RegisterThread(Ptr<native::Thread> thread_obj) {
                inner_impl::RegisterThreadImpl(thread_obj);
            }

            static inline void UnregisterThread(Ptr<native::Thread> thread_obj) {
                inner_impl::UnregisterThreadImpl(thread_obj);
            }

            static inline void UnregisterSelf() {
                inner_impl::UnregisterSelfImpl();
            }

            static void RegisterAndStartThread(Ptr<Variable> thread_var) {
                auto thread_obj = native::CreateThread();
                RegisterThread(thread_obj);
                thread_obj->Start(thread_var, &ThreadEntrypoint);
            }

            static Ptr<Variable> RegisterMainThread() {
                auto handle = native::GetCurrentThreadHandle();
                auto thread_obj = native::CreateExistingThread(handle);
                
                auto thread_class_type = inner_impl::LocateClassTypeImpl("java/lang/Thread");
                auto java_thread_v = TypeUtils::NewClassVariable(thread_class_type);

                thread_class_type->EnsureStaticInitializerCalled();

                auto java_thread_obj = java_thread_v->GetAs<type::ClassInstance>();
                java_thread_obj->SetField("eetop", "J", TypeUtils::NewPrimitiveVariable<type::Long>(handle));
                auto prio = thread_obj->GetPriority();
                java_thread_obj->SetField("priority", "I", TypeUtils::NewPrimitiveVariable<type::Integer>(prio));

                thread_obj->AssignJavaThreadVariable(java_thread_v);

                RegisterThread(thread_obj);

                return java_thread_v;
            }

    };

    namespace inner_impl {

        void ThreadNotifyExecutionStartImpl(Ptr<ClassType> type, const std::string &name, const std::string &descriptor) {
            auto cur_thr = ThreadUtils::GetCurrentThread();
            if(cur_thr) {
                cur_thr->PushNewCall(type, name, descriptor);
            }
        }

        void ThreadNotifyExecutionEndImpl()  {
            auto cur_thr = ThreadUtils::GetCurrentThread();
            if(cur_thr) {
                cur_thr->PopCurrentCall();
            }
        }

        void ThreadNotifyExceptionThrown() {
            auto cur_thr = ThreadUtils::GetCurrentThread();
            if(cur_thr) {
                cur_thr->NotifyExceptionThrown();
            }
        }

    }

}