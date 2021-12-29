#include <javm/javm_VM.hpp>

namespace javm::vm::jutil {

    Ptr<Variable> NewThrowable(const String &type, const String &msg) {
        auto throwable_class_type = rt::LocateClassType(type);
        if(throwable_class_type) {
            CallerSensitiveGuard guard;

            auto throwable_v = NewClassVariable(throwable_class_type);
            auto throwable_obj = throwable_v->GetAs<type::ClassInstance>();
            if(msg.empty()) {
                const auto ret = throwable_obj->CallConstructor(throwable_v, u"()V");
                if(ret.IsInvalidOrThrown()) {
                    return nullptr;
                }
            }
            else {
                auto msg_v = NewString(msg);
                const auto ret = throwable_obj->CallConstructor(throwable_v, u"(Ljava/lang/String;)V", msg_v);
                if(ret.IsInvalidOrThrown()) {
                    return nullptr;
                }
            }

            return throwable_v;
        }

        return nullptr;
    }

}