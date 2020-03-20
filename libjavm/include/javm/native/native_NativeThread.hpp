
#pragma once
#include <javm/vm/vm_TypeBase.hpp>
#include <functional>

namespace javm::native {

    using namespace vm::inner_impl;

    // Platform and lib-specific native threading functions

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

            Ptr<vm::Variable> GetJavaThreadVariable() {
                return this->java_thread;
            }

            virtual void Start(ThreadEntrypoint entry_fn) = 0;

            virtual ThreadHandle GetHandle() = 0;

            virtual bool IsAlive() = 0;

    };

    ThreadHandle GetCurrentThreadHandle();

    Ptr<Thread> CreateThread();

    Ptr<Thread> CreateExistingThread(ThreadHandle handle);

    Priority GetThreadPriority(ThreadHandle handle);

    void SetThreadPriority(ThreadHandle handle, Priority prio);

}