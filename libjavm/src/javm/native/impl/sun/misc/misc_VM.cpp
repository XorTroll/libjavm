#include <javm/javm_VM.hpp>
#include <javm/native/impl/sun/misc/misc_VM.hpp>

namespace javm::native::impl::sun::misc {

    using namespace vm;

    ExecutionResult VM::initialize(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.misc.VM.initialize] called");
        return ExecutionResult::Void();
    }

}