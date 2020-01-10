
#pragma once
#include <java/lang/String.class.hpp>

namespace java::lang {

    class Class final : public native::Class {

        private:
            std::shared_ptr<core::ClassObject> class_def;

        public:
            void SetClassDefinition(std::shared_ptr<core::ClassObject> def) {
                this->class_def = def;
            }

            std::shared_ptr<core::ClassObject> GetClassDefinition() {
                return this->class_def;
            }

        public:
            JAVM_NATIVE_CLASS_CTOR(Class) {

                JAVM_NATIVE_CLASS_NAME("java.lang.Class")

                JAVM_NATIVE_CLASS_REGISTER_METHOD(getName)

            }

            core::Value getName(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<Class>(this_v);
                auto class_def = this_ref->GetClassDefinition();
                auto class_name = class_def->GetName();

                auto str_obj = core::CreateNewClassWith<true>(frame, "java.lang.String", [&](auto *ref) {
                    reinterpret_cast<String*>(ref)->SetNativeString(core::ClassObject::GetPresentableClassName(class_name));
                });

                return str_obj;
            }

    };

    // Defined in Object's header
    core::Value CreateClassInstanceFromClassDefinition(core::Frame &frame, std::shared_ptr<core::ClassObject> def) {
        return core::CreateNewClassWith<true>(frame, "java.lang.Class", [&](auto *ref) {
            reinterpret_cast<Class*>(ref)->SetClassDefinition(def);
        });
    }
}