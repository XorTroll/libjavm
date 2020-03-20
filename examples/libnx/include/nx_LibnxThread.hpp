
#pragma once
#include <javm/native/native_NativeThread.hpp>
#include <switch.h>

// Threading implementation

#define R_TRY(rc) { auto _tmp_rc = (rc); if(R_FAILED(_tmp_rc)) { fatalThrow(_tmp_rc); } }

namespace nx {

    class LibnxThread : public native::Thread {

        private:
            bool existing;
            ::Thread thread;

            inline constexpr vm::type::Long GetHandleImpl() {
                return (vm::type::Long)thread.handle;
            }

        public:
            static constexpr size_t StackSize = 0x8000;
            static constexpr int Priority = 0x2B;
            static constexpr int CpuId = -2;

            LibnxThread() : existing(false) {}

            LibnxThread(native::ThreadHandle handle) : existing(true) {
                this->thread = {};
                this->thread.handle = (Handle)handle;
            }

            virtual void Start(native::ThreadEntrypoint entry_fn) override {
                if(existing) {
                    return;
                }
                R_TRY(threadCreate(&this->thread, entry_fn, reinterpret_cast<void*>(this), nullptr, StackSize, Priority, CpuId));
                R_TRY(threadStart(&this->thread));
            }

            virtual native::ThreadHandle GetHandle() override {
                return this->GetHandleImpl();
            }

            virtual bool IsAlive() override {
                u32 tmp_prio = 0;
                auto res = svcGetThreadPriority(&tmp_prio, this->thread.handle);
                // If the thread has ended, the handle should make this call fail :P
                return R_SUCCEEDED(res);
            }

    };

}

namespace javm::native {

    ThreadHandle GetCurrentThreadHandle() {
        auto handle = threadGetCurHandle();
        return (ThreadHandle)handle;
    }

    Ptr<Thread> CreateThread() {
        return PtrUtils::New<nx::LibnxThread>();
    }

    Ptr<Thread> CreateExistingThread(native::ThreadHandle handle) {
        return PtrUtils::New<nx::LibnxThread>(handle);
    }

    Priority GetThreadPriority(ThreadHandle handle) {
        // Stub
        return Thread::DefaultPriority;
    }

    void SetThreadPriority(ThreadHandle handle, Priority prio) {
        // Stub
    }

}