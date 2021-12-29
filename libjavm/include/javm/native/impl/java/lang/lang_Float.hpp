
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class Float {
        public:
            static ExecutionResult floatToRawIntBits(const std::vector<Ptr<Variable>> &param_vars);
    };

}