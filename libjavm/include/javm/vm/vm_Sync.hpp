
#pragma once
#include <javm/native/native_NativeSync.hpp>

namespace javm::vm {

    class Monitor {
        private:
            Ptr<native::RecursiveMutex> lock;
            Ptr<native::ConditionVariable> cond_var;

        public:
            Monitor() : lock(native::CreateRecursiveMutex()), cond_var(native::CreateConditionVariable()) {}

            void Enter() {
                this->lock->Lock();
            }

            void Leave() {
                this->lock->Unlock();
            }

            void Wait() {
                this->cond_var->Wait(this->lock);
            }

            void WaitFor(const type::Long ms) {
                this->cond_var->WaitFor(this->lock, ms);
            }

            void Notify() {
                this->cond_var->Notify();
            }

            void NotifyAll() {
                this->cond_var->NotifyAll();
            }

            void ForceUnlock() {
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