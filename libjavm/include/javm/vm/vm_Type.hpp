
#pragma once
#include <utility>
#include <javm/javm_Base.hpp>
#include <javm/vm/vm_Class.hpp>

namespace javm::vm {

    // TODO: multi arrays

    class Array {

        private:
            VariableType type;
            Ptr<ClassType> class_type;
            std::vector<Ptr<Variable>> inner_array;
            u32 array_length;

            Ptr<Variable> MakeDefaultVariable() {
                return inner_impl::NewDefaultVariableImpl(this->type);
            }

            void Reset() {
                // test
                if(this->array_length > 255) {
                    this->array_length = 255;
                }
                this->inner_array.resize(static_cast<unsigned>(this->array_length));
                JAVM_LOG("Array resize - new size: %ld", this->inner_array.size());
            }

        public:
            Array(VariableType type, u32 length) : type(type), array_length(length) {
                this->Reset();
            }

            Array(Ptr<ClassType> type, u32 length) : type(VariableType::ClassInstance), class_type(type), array_length(length) {
                this->Reset();
            }

            VariableType GetVariableType() {
                return this->type;
            }

            bool IsClassInstanceArray() {
                if(this->type == VariableType::ClassInstance) {
                    if(this->class_type) {
                        return true;
                    }
                }
                return false;
            }

            Ptr<ClassType> GetClassType() {
                return this->class_type;
            }

            bool CanCastTo(const String &class_name) {
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

            u32 GetLength() {
                return this->array_length;
            }

            Ptr<Variable> GetAt(u32 idx) {
                if(idx < this->array_length) {
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

            void SetAt(u32 idx, Ptr<Variable> var) {
                if(idx < this->array_length) {
                    this->inner_array[idx] = var;
                }
            }

    };

}