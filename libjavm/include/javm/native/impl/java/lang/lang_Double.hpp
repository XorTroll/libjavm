
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class Double {
        public:
            static ExecutionResult doubleToRawLongBits(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult longBitsToDouble(const std::vector<Ptr<Variable>> &param_vars);
    };

}