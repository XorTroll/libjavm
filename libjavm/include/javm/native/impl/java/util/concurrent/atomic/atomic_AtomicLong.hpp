
#pragma once
#include <javm/native/native_NativeCode.hpp>
#include <javm/native/impl/impl_Base.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <javm/vm/vm_Thread.hpp>
#include <javm/vm/vm_Properties.hpp>
#include <javm/vm/vm_Reflection.hpp>
#include <unistd.h>
#include <csignal>
#include <sys/time.h>

namespace javm::native::impl::java::util::concurrent::atomic {

    using namespace vm;

    class AtomicLong {
        public:
            static ExecutionResult VMSupportsCS8(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.util.concurrent.atomic.AtomicLong.VMSupportsCS8] called");
                return ExecutionResult::ReturnVariable(TypeUtils::False());
            }
    };

}