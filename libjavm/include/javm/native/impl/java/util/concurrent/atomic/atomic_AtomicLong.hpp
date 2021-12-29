
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::util::concurrent::atomic {

    using namespace vm;

    class AtomicLong {
        public:
            static ExecutionResult VMSupportsCS8(const std::vector<Ptr<Variable>> &param_vars);
    };

}