
#pragma once
#include <javm/native/native_NativeSync.hpp>
#include <switch.h>

// Sync implementation

namespace nx {

    class LibnxRecursiveMutex : public native::RecursiveMutex {
        private:
            RMutex nx_mutex;

        public:
            LibnxRecursiveMutex() {
                rmutexInit(&this->nx_mutex);
            }

            virtual void Lock() override {
                rmutexLock(&this->nx_mutex);
            }

            virtual bool TryLock() override {
                return rmutexTryLock(&this->nx_mutex);
            }

            virtual void Unlock() override {
                rmutexUnlock(&this->nx_mutex);
            }

            inline RMutex &GetHandle() {
                return this->nx_mutex;
            }
    };

    class LibnxConditionVariable : public native::ConditionVariable {
        private:
            CondVar nx_cv;

        public:
            LibnxConditionVariable() {
                condvarInit(&this->nx_cv);
            }

            virtual void Wait(Ptr<native::RecursiveMutex> lock) override {
                auto nx_lock = ptr::CastTo<LibnxRecursiveMutex>(lock);
                condvarWait(&this->nx_cv, &nx_lock->GetHandle().lock);
            }

            virtual void WaitFor(Ptr<native::RecursiveMutex> lock, vm::type::Long ms) override {
                auto nx_lock = ptr::CastTo<LibnxRecursiveMutex>(lock);
                condvarWaitTimeout(&this->nx_cv, &nx_lock->GetHandle().lock, ms);
            }

            virtual void Notify() override {
                condvarWakeOne(&this->nx_cv);
            }

            virtual void NotifyAll() override {
                condvarWakeAll(&this->nx_cv);
            }
    };

}

namespace javm::native {

    Ptr<RecursiveMutex> CreateRecursiveMutex() {
        return ptr::New<nx::LibnxRecursiveMutex>();
    }

    Ptr<ConditionVariable> CreateConditionVariable() {
        return ptr::New<nx::LibnxConditionVariable>();
    }

}