
#pragma once
#include <javm/native/native_Class.hpp>

namespace java {

    // Let's simplify types
    using namespace javm;

    namespace lang {

        // Any new native class should inherit from Object!
        class Object : public native::Class {

            public:
                Object(std::string name = "java.lang.Object", std::string extends = "java.lang.Object") : Class(name) {
                    this->super_class_name = core::ClassObject::ProcessClassName(extends);
                }

                void RemoveSuperClass() {
                    this->super_class_name = "";
                }

                virtual javm::core::ValuePointerHolder CreateInstanceEx(void *machine_ptr) override {
                    auto class_holder = javm::core::ValuePointerHolder::Create<Object>();
                    auto class_ref = class_holder.GetReference<Object>();
                    // Object is special, and can't inherit from anyone
                    // Not adding this would result in an infinite loop of Object extends Object extends Object... :P
                    class_ref->RemoveSuperClass();
                    return class_holder;
                }

                static std::shared_ptr<Object> CreateDefinitionInstance(void *machine_ptr) {
                    auto class_ref = std::make_shared<Object>();
                    auto super_class_name = class_ref->GetSuperClassName();
                    // Object is special, and can't inherit from anyone
                    // Not adding this would result in an infinite loop of Object extends Object extends Object... :P
                    class_ref->RemoveSuperClass();
                    return class_ref;
                }
        };

    }

}