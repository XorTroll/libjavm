
#pragma once
#include <javm/native/native_NativeSync.hpp>

namespace javm::vm {

    class Monitor {
        private:
            Ptr<native::RecursiveMutex> lock;
            Ptr<native::ConditionVariable> cond_var;

        public:
            Monitor() : lock(native::CreateRecursiveMutex()), cond_var(native::CreateConditionVariable()) {}

            inline void Enter() {
                this->lock->Lock();
            }

            inline void Leave() {
                this->lock->Unlock();
            }

            inline void Wait() {
                this->cond_var->Wait(this->lock);
            }

            inline void WaitFor(const type::Long ms) {
                this->cond_var->WaitFor(this->lock, ms);
            }

            inline void Notify() {
                this->cond_var->Notify();
            }

            inline void NotifyAll() {
                this->cond_var->NotifyAll();
            }

            inline void ForceUnlock() {
                this->lock->TryLock();
                this->lock->Unlock();
            }
    };

    class ScopedMonitorLock {
        private:
            Monitor &monitor_ref;

        public:
            ScopedMonitorLock(Monitor &ref) : monitor_ref(ref) {
                this->monitor_ref.Enter();
            }

            ~ScopedMonitorLock() {
                this->monitor_ref.Leave();
            }
    };

}