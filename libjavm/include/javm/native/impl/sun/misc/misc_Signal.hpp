
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::sun::misc {

    using namespace vm;

    class Signal {
        public:
            static ExecutionResult findSignal(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult handle0(const std::vector<Ptr<Variable>> &param_vars);
    };

}