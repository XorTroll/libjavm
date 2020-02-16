
#pragma once
#include <java/lang/String.class.hpp>
#include <java/lang/Class.class.hpp>

namespace java::lang {

    class Enum final : public native::Class {

        private:
            std::string item_name;
            int item_ordinal;

        public:
            std::string GetItemName() {
                return this->item_name;
            }

            int GetItemOrdinal() {
                return this->item_ordinal;
            }

            void AssignValue(const std::string &name, int ord) {
                this->item_name = name;
                this->item_ordinal = ord;
            }

        public:
            JAVM_NATIVE_CLASS_CTOR(Enum) {

                JAVM_NATIVE_CLASS_NAME("java.lang.Enum")

                JAVM_NATIVE_CLASS_REGISTER_CTOR(constructor)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(name)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(toString)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(ordinal)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(clone)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(compareTo)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(getDeclaringClass)

                JAVM_NATIVE_CLASS_REGISTER_STATIC_FN(valueOf)

            }

            core::Value constructor(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<Enum>(this_v);

                if(parameters.size() == 2) {
                    auto name_v = parameters[0].value;
                    auto ord_v = parameters[1].value;
                    if(name_v->IsValidCast<String>()) {
                        auto name = name_v->GetReference<String>();
                        auto name_str = name->GetNativeString();
                        if(ord_v->IsValidCast<int>()) {
                            auto ord = ord_v->Get<int>();
                            this_ref->AssignValue(name_str, ord);
                        }
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }

            core::Value name(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<Enum>(this_v);

                auto name = this_ref->GetItemName();

                auto name_obj = core::CreateNewClassWith<true>(frame, "java.lang.String", [&](auto *ref) {
                    reinterpret_cast<String*>(ref)->SetNativeString(core::ClassObject::GetPresentableClassName(name));
                });

                return name_obj;
            }

            // Same thing
            core::Value toString(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                return this->name(frame, this_v, parameters);
            }

            core::Value ordinal(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<Enum>(this_v);

                auto ord = this_ref->GetItemOrdinal();

                return core::CreateNewValue<int>(ord);
            }

            core::Value clone(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                frame.ThrowWithType("java.lang.CloneNotSupportedException", "Clone is not supported for java.lang.Enum");

                return core::CreateNullValue();
            }

            core::Value compareTo(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<Enum>(this_v);

                auto this_ord = this_ref->GetItemOrdinal();
                auto ret_ord = this_ord;

                if(parameters.size() == 1) {
                    if(parameters[0].value->IsValidCast<Enum>()) {
                        auto other_ref = parameters[0].value->GetReference<Enum>();
                        auto other_ord = other_ref->GetItemOrdinal();
                        ret_ord -= other_ord;
                    }
                }

                return core::CreateNewValue<int>(ret_ord);
            }

            core::Value valueOf(core::Frame &frame, std::vector<core::FunctionParameter> parameters) {

                if(parameters.size() == 2) {
                    auto class_v = parameters[0].value->GetReference<lang::Class>();
                    auto class_def = class_v->GetClassDefinition();
                    auto val_v = parameters[1].value->GetReference<String>();
                    auto val_str = val_v->GetNativeString();
                    auto ret = class_def->CallStaticFunction(frame, "values", core::TypeDefinitions::GetArrayDefinition(core::TypeDefinitions::GetClassDefinition(frame, class_def->GetName())));
                    auto ret_arr = ret->GetReference<core::Array>();
                    for(u32 i = 0; i < ret_arr->GetLength(); i++) {
                        auto item = ret_arr->GetAt(i);
                        auto item_enum_obj = item->GetReference<Enum>();
                        auto enum_str = item_enum_obj->CallMethod(frame, "toString", core::TypeDefinitions::GetClassDefinition(frame, "java.lang.String"));
                        auto enum_str_ref = enum_str->GetReference<String>();
                        if(val_str == enum_str_ref->GetNativeString()) {
                            return item;
                        }
                    }
                }

                frame.ThrowWithType("java.lang.IllegalArgumentException", "No enum constant with provided name");
                return core::CreateNullValue();
            }

            core::Value getDeclaringClass(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                // This way we get the derived Enum type instance, and not the Enum super class instance
                // Getting the plain instance instead of the invoker would make getClass() return java.lang.Enum :P
                auto this_ref = this->GetThisInvokerInstance<core::ClassObject>(this_v);

                /* Java equivalent:
                    Class getDeclaringClass() {
                        return this.getClass();
                    }
                */
                // They're not exactly equivalent (getClass and getDeclaringClass), but enough for now
                auto class_v = this_ref->CallMethod(frame, "getClass", core::TypeDefinitions::GetClassDefinition(frame, "java.lang.Class"));

                return class_v;
            }
            
    };

}