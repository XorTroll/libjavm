
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

namespace javm::native::impl::java::io {

    using namespace vm;

    class FileDescriptor {
        public:
            static ExecutionResult initIDs(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.io.FileDescriptor.initIDs] called");
                return ExecutionResult::Void();
            }

            static ExecutionResult set(const std::vector<Ptr<Variable>> &param_vars) {
                auto fd_v = param_vars[0];
                const auto fd_i = fd_v->GetValue<type::Integer>();
                JAVM_LOG("[java.io.FileDescriptor.set] called - fd: %d", fd_i);
                auto fd_l_v = TypeUtils::NewPrimitiveVariable<type::Long>(static_cast<type::Long>(fd_i));
                return ExecutionResult::ReturnVariable(fd_l_v);
            }
    };

}