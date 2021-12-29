
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::sun::misc {

    using namespace vm;

    class VM {
        public:
            static ExecutionResult initialize(const std::vector<Ptr<Variable>> &param_vars);
    };

}