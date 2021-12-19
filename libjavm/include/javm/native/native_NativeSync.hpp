
#pragma once
#include <javm/vm/vm_TypeBase.hpp>

namespace javm::native {

    // These must be implemented, depending on the platform

    class RecursiveMutex {
        public:
            virtual void Lock() = 0;
            virtual bool TryLock() = 0;
            virtual void Unlock() = 0;
    };

    Ptr<RecursiveMutex> CreateRecursiveMutex();

    class ConditionVariable {
        public:
            virtual void Wait(Ptr<RecursiveMutex> lock) = 0;
            virtual void WaitFor(Ptr<RecursiveMutex> lock, const vm::type::Long ms) = 0;
            virtual void Notify() = 0;
            virtual void NotifyAll() = 0;
    };

    Ptr<ConditionVariable> CreateConditionVariable();

}