
#pragma once
#include <javm/vm/vm_TypeBase.hpp>
#include <functional>

namespace javm::native {

    using namespace vm::inner_impl;

    // Platform and lib-specific native threading functions

    using ThreadId = vm::type::Long;
    using ThreadHandle = vm::type::Long;
    using Priority = vm::type::Integer;

    using ThreadEntrypoint = void(*)(void*);

    class Thread {

        public:
            static constexpr const char *MainThreadName = "main";
            
            static constexpr Priority MinPriority = 1;
            static constexpr Priority DefaultPriority = 5;
            static constexpr Priority MaxPriority = 10;

        private:
            Ptr<vm::Variable> java_thread;

        public:
            void AssignJavaThreadVariable(Ptr<vm::Variable> java_thread_v) {
                this->java_thread = java_thread_v;
            }

            void Start(Ptr<vm::Variable> java_thread_v, ThreadEntrypoint entry_fn) {
                this->AssignJavaThreadVariable(java_thread_v);
                this->DoStart(entry_fn);
            }

            Ptr<vm::Variable> GetJavaThreadVariable() {
                return this->java_thread;
            }

            virtual void DoStart(ThreadEntrypoint entry_fn) = 0;

            virtual ThreadId GetId() = 0;

            virtual ThreadHandle GetHandle() = 0;

            virtual bool IsAlive() = 0;

            virtual Priority GetPriority() = 0;

            virtual void SetPriority(Priority prio) = 0;

    };

    ThreadHandle GetCurrentThreadHandle();

    Ptr<Thread> CreateThread();

    Ptr<Thread> CreateExistingThread(ThreadHandle handle);

}