
#pragma once
#include <utility>
#include <javm/javm_Base.hpp>
#include <javm/vm/vm_Class.hpp>

namespace javm::vm {

    // TODO: multi arrays
    // Note: the inner object inside an array is only used to call Object methods and for its monitor

    namespace inner_impl {

        Ptr<ClassType> FindClassTypeImpl(const String &name);

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

            void Reset() {
                this->inner_array.resize(static_cast<unsigned>(this->length));
                JAVM_LOG("Array resize - new size: %ld", this->inner_array.size());
            }

            static inline Ptr<ClassInstance> CreateInnerObject() {
                return ptr::New<ClassInstance>(inner_impl::FindClassTypeImpl(u"java/lang/Object"));
            }

        public:
            Array(VariableType type, const u32 length) : type(type), length(length), dimensions(1), inner_object(CreateInnerObject()) {
                this->Reset();
            }

            Array(Ptr<ClassType> type, const u32 length) : type(VariableType::ClassInstance), class_type(type), length(length), dimensions(1), inner_object(CreateInnerObject()) {
                this->Reset();
            }

            inline VariableType GetVariableType() {
                return this->type;
            }

            inline bool IsClassInstanceArray() {
                return (this->type == VariableType::ClassInstance) && this->class_type;
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

            inline void SetAt(const u32 idx, Ptr<Variable> var) {
                if(idx < this->length) {
                    this->inner_array[idx] = var;
                }
            }

            inline ExecutionResult CallInstanceMethod(const String &name, const String &descriptor, Ptr<Variable> this_as_var, const std::vector<Ptr<Variable>> &param_vars) {
                return this->inner_object->CallInstanceMethod(name, descriptor, this_as_var, param_vars);
            }
    };

}