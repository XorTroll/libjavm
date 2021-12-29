
#pragma once
#include <javm/vm/vm_Array.hpp>

namespace javm::vm::ref {

    class ReflectionType {
        private:
            VariableType type;
            Ptr<ClassType> base_type;
            u32 array_dimensions;
            String type_name;

            String ComputeTypeName();

        public:
            ReflectionType(Ptr<ClassType> class_type, const u32 array_dimensions = 0) : type(VariableType::ClassInstance), base_type(class_type), array_dimensions(array_dimensions), type_name(ComputeTypeName()) {}
            ReflectionType(VariableType primitive_type, const u32 array_dimensions = 0) : type(primitive_type), array_dimensions(array_dimensions), type_name(ComputeTypeName()) {}

            inline constexpr bool IsPrimitiveType() {
                return IsPrimitiveVariableType(this->type);
            }
            
            inline constexpr bool IsPrimitive() {
                return this->IsPrimitiveType() && !this->IsArray();
            }

            inline constexpr bool IsArray() {
                return this->array_dimensions > 0;
            }

            inline constexpr bool IsMultiArray() {
                return this->array_dimensions > 1;
            }

            inline constexpr u32 GetArrayDimensions() {
                return this->array_dimensions;
            }

            inline constexpr VariableType GetPrimitiveType() {
                return this->type;
            }

            inline bool IsClassInstance() {
                return this->base_type && (!this->IsPrimitive()) && (this->type == VariableType::ClassInstance);
            }

            inline Ptr<ClassType> GetClassType() {
                return this->base_type;
            }

            inline String GetTypeName() {
                return this->type_name;
            }
    };

    inline bool EqualTypes(Ptr<ReflectionType> t_a, Ptr<ReflectionType> t_b) {
        return t_a->GetTypeName() == t_b->GetTypeName();
    }

    Ptr<ReflectionType> FindReflectionTypeByName(const String &name);
    Ptr<ReflectionType> FindArrayReflectionType(Ptr<Array> &array);

}