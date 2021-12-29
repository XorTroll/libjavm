#include <javm/javm_VM.hpp>
#include <javm/native/impl/sun/io/io_Win32ErrorMode.hpp>

namespace javm::native::impl::sun::io {

    using namespace vm;

    ExecutionResult Win32ErrorMode::setErrorMode(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.misc.Signal.handle0] setErrorMode...");
        return ExecutionResult::ReturnVariable(param_vars[0]);
    }

}