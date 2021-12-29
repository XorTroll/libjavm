
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::vm::jutil {

    Ptr<Variable> NewThrowable(const String &type, const String &msg = u"");

    inline Ptr<Variable> NewInternalThrowable(const String &msg) {
        return NewThrowable(u"java/lang/InternalError", msg);
    }

}