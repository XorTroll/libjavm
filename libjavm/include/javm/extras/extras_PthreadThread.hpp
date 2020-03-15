
#pragma once
#include <javm/native/native_NativeThread.hpp>
#include <pthread.h>

// Threading implementation - pthread

namespace javm::extras {

    using PthreadEntry = void*(*)(void*);

    inline constexpr vm::type::Long CastFromPthread(pthread_t pthread) {
        return (vm::type::Long)pthread;
    }

    inline constexpr pthread_t CastToPthread(vm::type::Long handle) {
        return (pthread_t)handle;
    }

    class PthreadThread : public native::Thread {

        private:
            bool existing;
            pthread_t pthread;

        private:
            inline constexpr vm::type::Long GetHandleImpl() {
                return CastFromPthread(pthread);
            }

        public:
            PthreadThread() : existing(false) {}

            PthreadThread(native::ThreadHandle handle) : existing(true), pthread(CastToPthread(handle)) {}

            virtual void DoStart(native::ThreadEntrypoint entry_fn) override {
                if(existing) {
                    return;
                }
                pthread_create(&this->pthread, nullptr, (PthreadEntry)entry_fn, reinterpret_cast<void*>(this));
            }

            virtual native::ThreadId GetId() override {
                return this->GetHandleImpl();
            }

            virtual native::ThreadHandle GetHandle() override {
                return this->GetHandleImpl();
            }

            virtual bool IsAlive() override {
                auto ret = pthread_kill(this->pthread, 0);
                if(ret == 0) {
                    return true;
                }
                return false;
            }

            virtual native::Priority GetPriority() override {
                // Stub
                return native::Thread::DefaultPriority;
            }

            virtual void SetPriority(native::Priority prio) override {
                // Stub
            }

    };

}

namespace javm::native {

    ThreadHandle GetCurrentThreadHandle() {
        auto self_thr = pthread_self();
        return extras::CastFromPthread(self_thr);
    }

    Ptr<Thread> CreateThread() {
        return PtrUtils::New<extras::PthreadThread>();
    }

    Ptr<Thread> CreateExistingThread(native::ThreadHandle handle) {
        return PtrUtils::New<extras::PthreadThread>(handle);
    }

}