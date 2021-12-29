
#pragma once
#include <javm/vm/vm_TypeBase.hpp>
#include <functional>

namespace javm::native {

    // Platform/lib-specific native threading functions

    using ThreadHandle = vm::type::Long;
    using Priority = vm::type::Integer;

    using ThreadEntrypoint = void(*)(void*);

    class Thread {
        public:
            static constexpr auto MainThreadName = u"main";
            
            static constexpr Priority MinPriority = 1;
            static constexpr Priority DefaultPriority = 5;
            static constexpr Priority MaxPriority = 10;

        private:
            Ptr<vm::Variable> thread_v;

        public:
            inline void SetThreadVariable(Ptr<vm::Variable> thread_v) {
                this->thread_v = thread_v;
            }

            inline Ptr<vm::Variable> GetThreadVariable() {
                return this->thread_v;
            }

            virtual void Start(ThreadEntrypoint entry_fn) = 0;
            virtual ThreadHandle GetHandle() = 0;
            virtual bool IsAlive() = 0;
    };

    ThreadHandle GetCurrentThreadHandle();

    Ptr<Thread> CreateThread();
    Ptr<Thread> CreateExistingThread(const ThreadHandle handle);

    Priority GetThreadPriority(const ThreadHandle handle);
    void SetThreadPriority(const ThreadHandle handle, const Priority prio);

}