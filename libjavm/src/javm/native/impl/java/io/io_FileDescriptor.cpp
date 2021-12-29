#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/io/io_FileDescriptor.hpp>

namespace javm::native::impl::java::io {

    using namespace vm;

    ExecutionResult FileDescriptor::initIDs(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.io.FileDescriptor.initIDs] called");
        return ExecutionResult::Void();
    }

    ExecutionResult FileDescriptor::set(const std::vector<Ptr<Variable>> &param_vars) {
        auto fd_v = param_vars[0];
        const auto fd_i = fd_v->GetValue<type::Integer>();
        JAVM_LOG("[java.io.FileDescriptor.set] called - fd: %d", fd_i);
        auto fd_l_v = NewPrimitiveVariable<type::Long>(static_cast<type::Long>(fd_i));
        return ExecutionResult::ReturnVariable(fd_l_v);
    }

}