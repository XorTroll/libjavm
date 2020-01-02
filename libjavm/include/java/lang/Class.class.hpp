
#pragma once
#include <java/lang/String.class.hpp>

namespace java::lang {

    class Class : public Object {

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

            core::Value getName(core::Frame &frame, core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisReference<Class>(this_param);
                auto class_def = this_ref->GetClassDefinition();
                auto class_name = class_def->GetName();

                auto str_obj = frame.CreateNewClass<true>("java.lang.String");
                auto str_ref = str_obj->GetReference<String>();
                str_ref->SetString(core::ClassObject::GetPresentableClassName(class_name));
                return str_obj;
            }

    };
}