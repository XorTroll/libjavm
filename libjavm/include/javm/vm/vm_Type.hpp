
#pragma once
#include <utility>
#include <javm/javm_Base.hpp>
#include <javm/vm/vm_Class.hpp>

namespace javm::vm {

    // TODO: multi-dimensional array support
    // Note: the inner object inside an array is only used to call Object methods and for its monitor

    namespace inner_impl {

        Ptr<ClassType> FindClassTypeImpl(const String &name);
        Ptr<ClassType> GetClassInstanceTypeImpl(Ptr<Variable> &var);
        VariableType GetVariableTypeImpl(Ptr<Variable> &var);

    }

    class Array {
        private:
            VariableType type;
            Ptr<ClassType> class_type;
            std::vector<Ptr<Variable>> inner_array;
            u32 length;
            u32 dimensions;
            Ptr<ClassInstance> inner_object;

            inline Ptr<Variable> MakeDefaultVariable() {
                return inner_impl::NewDefaultVariableImpl(this->type);
            }

            static inline Ptr<ClassInstance> CreateInnerObject() {
                return ptr::New<ClassInstance>(inner_impl::FindClassTypeImpl(u"java/lang/Object"));
            }

        public:
            Array(VariableType type, const u32 length, const u32 dimensions = 1) : type(type), inner_array(length), length(length), dimensions(dimensions), inner_object(CreateInnerObject()) {}

            Array(Ptr<ClassType> type, const u32 length, const u32 dimensions = 1) : type(VariableType::ClassInstance), class_type(type), inner_array(length), length(length), dimensions(dimensions), inner_object(CreateInnerObject()) {}

            inline VariableType GetVariableType() {
                return this->type;
            }

            inline bool IsClassInstanceArray() {
                return ptr::IsValid(this->class_type);
            }

            inline bool IsMultiArray() {
                return this->dimensions > 1;
            }

            inline Ptr<ClassType> GetClassType() {
                return this->class_type;
            }

            inline Ptr<ClassInstance> GetObjectInstance() {
                return this->inner_object;
            }

            inline bool CanCastTo(const String &class_name) {
                if(this->class_type) {
                    if(ClassUtils::EqualClassNames(class_name, this->class_type->GetClassName())) {
                        return true;
                    }
                    auto super_class = this->class_type->GetSuperClassType();
                    while(super_class) {
                        if(ClassUtils::EqualClassNames(class_name, super_class->GetClassName())) {
                            return true;
                        }
                        auto sc = super_class->GetSuperClassType();
                        super_class = sc;
                    }
                }
                return false;
            }

            inline u32 GetLength() {
                return this->length;
            }

            inline u32 GetDimensions() {
                return this->dimensions;
            }

            Ptr<Variable> GetAt(const u32 idx) {
                if(idx < this->length) {
                    auto arr_v = this->inner_array[idx];
                    // If value not set, make a default one and return it
                    if(!arr_v) {
                        arr_v = this->MakeDefaultVariable();
                        this->inner_array[idx] = arr_v;
                    }
                    return arr_v;
                }
                return nullptr;
            }

            inline bool SetAt(const u32 idx, Ptr<Variable> var) {
                if(idx >= this->length) {
                    return false;
                }

                const auto var_type = inner_impl::GetVariableTypeImpl(var);
                if(!TypeTraits::IsVariableTypeConvertibleTo(var_type, this->type)) {
                    return false;
                }

                if(this->IsClassInstanceArray()) {
                    if(var_type == VariableType::ClassInstance) {
                        auto var_class_type = inner_impl::GetClassInstanceTypeImpl(var);
                        if(!var_class_type->CanCastTo(this->class_type->GetClassName())) {
                            return false;
                        }
                    }
                    else if(var_type == VariableType::Array) {
                        // Arrays can only be casted to Object!
                        if(ClassUtils::MakeSlashClassName(this->class_type->GetClassName()) != u"java/lang/Object") {
                            return false;
                        }
                    }
                }

                this->inner_array[idx] = var;
                return true;
            }

            inline ExecutionResult CallInstanceMethod(const String &name, const String &descriptor, Ptr<Variable> this_as_var, const std::vector<Ptr<Variable>> &param_vars) {
                return this->inner_object->CallInstanceMethod(name, descriptor, this_as_var, param_vars);
            }
    };

}