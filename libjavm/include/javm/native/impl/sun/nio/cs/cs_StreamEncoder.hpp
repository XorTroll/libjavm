
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::sun::nio::cs {

    using namespace vm;

    class StreamEncoder {
        public:
            static ExecutionResult forOutputStreamWriter(const std::vector<Ptr<Variable>> &param_vars);
    };

}