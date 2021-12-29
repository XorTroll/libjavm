#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/util/concurrent/atomic/atomic_AtomicLong.hpp>

namespace javm::native::impl::java::util::concurrent::atomic {

    using namespace vm;

    ExecutionResult AtomicLong::VMSupportsCS8(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.util.concurrent.atomic.AtomicLong.VMSupportsCS8] called");
        return ExecutionResult::ReturnVariable(MakeFalse());
    }

}