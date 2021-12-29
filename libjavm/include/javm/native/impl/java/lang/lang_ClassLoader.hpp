
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class ClassLoader {
        public:
            static ExecutionResult registerNatives(const std::vector<Ptr<Variable>> &param_vars);
    };

}