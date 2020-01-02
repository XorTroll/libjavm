
#pragma once
#include <javm/native/native_Class.hpp>

namespace java {

    // Let's simplify types
    using namespace javm;

    namespace lang {

        // Declared in Class's header
        core::Value CreateClassInstanceFromClassDefinition(core::Frame &frame, std::shared_ptr<core::ClassObject> def);

        class Object final : public native::Class {

            public:
                JAVM_NATIVE_CLASS_CTOR(Object) {

                    JAVM_NATIVE_CLASS_NAME("java.lang.Object")
                    
                    JAVM_NATIVE_CLASS_REGISTER_METHOD(getClass)
                }

                core::Value getClass(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                    auto this_ref = this->GetThisInvokerInstance<Object>(this_v);
                    
                    auto class_name = this_ref->GetName(); // Get the name of the subclass (since 'this' is an Object now, not necessarily the original class)
                    auto class_def = core::FindClassByName(frame, class_name);

                    return CreateClassInstanceFromClassDefinition(frame, class_def);
                }
        };

    }

}