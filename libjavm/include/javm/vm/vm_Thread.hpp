
#pragma once
#include <javm/vm/vm_Variable.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <javm/native/native_NativeThread.hpp>

namespace javm::vm {

    // TODO: does the eetop field hold anything else? it appears to be an internal VM pointer rather than just a thread handle...

    using ThreadHandle = native::ThreadHandle;

    struct CallInfo {
        Ptr<ClassType> caller_type;
        String invokable_name;
        String invokable_desc;
    };

    class ThreadAccessor {
        private:
            Ptr<native::Thread> thread_obj;
            std::vector<CallInfo> call_stack;
            bool exception_thrown;
            String cached_name;

        public:
            ThreadAccessor(Ptr<native::Thread> thr_obj) : thread_obj(thr_obj), exception_thrown(false) {}

            inline ThreadHandle GetThreadHandle() {
                return this->thread_obj->GetHandle();
            }

            inline Ptr<native::Thread> GetThreadObject() {
                return this->thread_obj;
            }

            void CacheThreadName() {
                auto java_thr_v = this->thread_obj->GetJavaThreadVariable();
                auto thr_obj = java_thr_v->GetAs<type::ClassInstance>();
                const auto name_res = thr_obj->CallInstanceMethod(u"getName", u"()Ljava/lang/String;", java_thr_v);
                if(!name_res.IsInvalidOrThrown()) {
                    auto name_v = name_res.ret_var;
                    this->cached_name = jstr::GetValue(name_v);
                }
            }

            String GetThreadName() {
                if(this->cached_name.empty()) {
                    this->CacheThreadName();
                }
                return this->cached_name;
            }

            inline Ptr<Variable> GetJavaThreadInstance() {
                return this->thread_obj->GetJavaThreadVariable();
            }

            void PushNewCall(Ptr<ClassType> caller_type, const String &invokable_name, const String &invokable_desc) {
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

            inline void NotifyExceptionThrown() {
                this->exception_thrown = true;
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

        friend class ThreadUtils;
    };

    namespace inner_impl {

        static inline std::vector<Ptr<ThreadAccessor>> g_thread_table;
        static inline Monitor g_thread_table_lock;

        static Ptr<ThreadAccessor> RegisterThreadImpl(Ptr<native::Thread> thread_obj) {
            ScopedMonitorLock lk(g_thread_table_lock);
            auto accessor = ptr::New<ThreadAccessor>(thread_obj);
            accessor->CacheThreadName();
            g_thread_table.push_back(accessor);
            return accessor;
        }

        static Ptr<ThreadAccessor> RegisterThreadWithoutNameCacheImpl(Ptr<native::Thread> thread_obj) {
            ScopedMonitorLock lk(g_thread_table_lock);
            auto accessor = ptr::New<ThreadAccessor>(thread_obj);
            g_thread_table.push_back(accessor);
            return accessor;
        }

        static void UnregisterThreadImpl(Ptr<native::Thread> thread_obj) {
            ScopedMonitorLock lk(g_thread_table_lock);
            g_thread_table.erase(std::remove_if(g_thread_table.begin(), g_thread_table.end(), [&](const Ptr<ThreadAccessor> &thr) -> bool {
                return thr->GetThreadHandle() == thread_obj->GetHandle();
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
            auto handle = native::GetCurrentThreadHandle();
            for(const auto &thr: g_thread_table) {
                if(thr->GetThreadHandle() == handle) {
                    return thr->GetJavaThreadInstance();
                }
            }
            return nullptr;
        }

        static Ptr<ThreadAccessor> GetCurrentThreadImpl() {
            ScopedMonitorLock lk(g_thread_table_lock);
            auto handle = native::GetCurrentThreadHandle();
            for(const auto &thr: g_thread_table) {
                if(thr->GetThreadHandle() == handle) {
                    return thr;
                }
            }
            return nullptr;
        }

        static Ptr<ThreadAccessor> GetThreadByHandleImpl(const native::ThreadHandle handle) {
            ScopedMonitorLock lk(g_thread_table_lock);
            for(const auto &thr: g_thread_table) {
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

    namespace inner_impl {

        static inline Ptr<Variable> g_thrown_throwable;
        static inline Ptr<ThreadAccessor> g_thrown_thread;
        static inline Monitor g_thrown_lock;

        static inline bool WasExceptionThrownImpl() {
            ScopedMonitorLock lk(g_thrown_lock);
            return ptr::IsValid(g_thrown_throwable);
        }

        static inline Ptr<ThreadAccessor> GetExceptionThreadImpl() {
            ScopedMonitorLock lk(g_thrown_lock);
            return g_thrown_thread;
        }

        static inline Ptr<Variable> GetThrownExceptionImpl() {
            ScopedMonitorLock lk(g_thrown_lock);
            return g_thrown_throwable;
        }

        static inline void NotifyExceptionThrownImpl(Ptr<Variable> throwable_v) {
            ScopedMonitorLock lk(g_thrown_lock);
            g_thrown_throwable = throwable_v;
            const auto cur_handle = native::GetCurrentThreadHandle();
            g_thrown_thread = GetThreadByHandleImpl(cur_handle);
        }

        // AKA notify that the exception has already been checked, called after getting this info, after execution is done
        static inline void ConsumeThrownExceptionImpl() {
            ScopedMonitorLock lk(g_thrown_lock);
            g_thrown_throwable = nullptr;
        }

    }

    class ThreadUtils {
        private:
            static void ThreadEntrypoint(void *thr_ptr) {
                auto thr_ref = reinterpret_cast<native::Thread*>(thr_ptr);

                auto java_thr_v = thr_ref->GetJavaThreadVariable();
                auto java_thr_obj = java_thr_v->GetAs<type::ClassInstance>();

                const auto eetop = thr_ref->GetHandle();
                java_thr_obj->SetField(u"eetop", u"J", TypeUtils::NewPrimitiveVariable<type::Long>(eetop));
                const auto prio = native::GetThreadPriority(eetop);
                java_thr_obj->SetField(u"priority", u"I", TypeUtils::NewPrimitiveVariable<type::Integer>(prio));

                java_thr_obj->CallInstanceMethod(u"run", u"()V", java_thr_v);

                UnregisterSelf();
            }

        public:
            static inline Ptr<Variable> GetCurrentThreadInstance() {
                return inner_impl::GetCurrentThreadInstanceImpl();
            }

            static inline Ptr<ThreadAccessor> GetCurrentThread() {
                return inner_impl::GetCurrentThreadImpl();
            }

            static inline std::vector<CallInfo> &GetCurrentCallStack() {
                return GetCurrentThread()->GetCallStack();
            }

            static inline Ptr<ThreadAccessor> GetThreadByHandle(const native::ThreadHandle handle) {
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
                thread_obj->AssignJavaThreadVariable(thread_var);
                RegisterThread(thread_obj);
                thread_obj->Start(&ThreadEntrypoint);
            }

            static Ptr<Variable> RegisterMainThread() {
                const auto handle = native::GetCurrentThreadHandle();
                auto thread_obj = native::CreateExistingThread(handle);
                
                auto thread_class_type = inner_impl::LocateClassTypeImpl(u"java/lang/Thread");
                auto java_thread_v = TypeUtils::NewClassVariable(thread_class_type);

                thread_class_type->EnsureStaticInitializerCalled();

                auto java_thread_obj = java_thread_v->GetAs<type::ClassInstance>();
                java_thread_obj->SetField(u"eetop", u"J", TypeUtils::NewPrimitiveVariable<type::Long>(handle));
                const auto prio = native::GetThreadPriority(handle);
                java_thread_obj->SetField(u"priority", u"I", TypeUtils::NewPrimitiveVariable<type::Integer>(prio));

                thread_obj->AssignJavaThreadVariable(java_thread_v);

                auto thr_accessor = inner_impl::RegisterThreadWithoutNameCacheImpl(thread_obj);
                thr_accessor->cached_name = native::Thread::MainThreadName;

                return java_thread_v;
            }

            static std::pair<Ptr<ThreadAccessor>, Ptr<Variable>> GetThrownExceptionInfo() {
                if(inner_impl::WasExceptionThrownImpl()) {
                    auto thr = inner_impl::GetExceptionThreadImpl();
                    auto throwable_v = inner_impl::GetThrownExceptionImpl();
                    inner_impl::ConsumeThrownExceptionImpl();
                    return { thr, throwable_v };
                }

                return {};
            }
    };

    namespace inner_impl {

        void ThreadNotifyExecutionStartImpl(Ptr<ClassType> type, const String &name, const String &descriptor) {
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