
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::io {

    using namespace vm;

    class FileDescriptor {
        public:
            static ExecutionResult initIDs(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult set(const std::vector<Ptr<Variable>> &param_vars);
    };

}