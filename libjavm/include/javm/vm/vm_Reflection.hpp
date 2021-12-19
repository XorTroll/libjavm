
#pragma once
#include <javm/vm/vm_Type.hpp>

namespace javm::vm {

    class ReflectionType {
        private:
            VariableType type;
            Ptr<ClassType> base_type;
            u32 array_dimensions;
            String type_name;

            String ComputeTypeName() {
                String base_name;
                const auto is_array = this->array_dimensions > 0;
                for(u32 i = 0; i < this->array_dimensions; i++) {
                    base_name += u'[';
                }

                if(this->IsPrimitiveType()) {
                    if(is_array) {
                        base_name += TypeTraits::GetDescriptorForPrimitiveType(this->type);
                    }
                    else {
                        base_name += TypeTraits::GetNameForPrimitiveType(this->type);
                    }
                }
                else if(this->IsClassInstance()) {
                    const auto class_name = ClassUtils::MakeDotClassName(this->base_type->GetClassName());
                    if(is_array) {
                        base_name += u"L" + class_name + u";";
                    }
                    else {
                        base_name += class_name;
                    }
                }
                else {
                    return u"<wtf>";
                }
                return base_name;
            }

        public:
            ReflectionType(Ptr<ClassType> class_type, const u32 array_dimensions = 0) : type(VariableType::ClassInstance), base_type(class_type), array_dimensions(array_dimensions), type_name(ComputeTypeName()) {}

            ReflectionType(VariableType primitive_type, const u32 array_dimensions = 0) : type(primitive_type), array_dimensions(array_dimensions), type_name(ComputeTypeName()) {}

            inline constexpr bool IsPrimitiveType() {
                return TypeTraits::IsPrimitiveVariableType(this->type);
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

    namespace inner_impl {

        static inline std::vector<Ptr<ReflectionType>> g_reflection_type_table;

        inline std::vector<Ptr<ReflectionType>> &GetReflectionTypeTable() {
            return g_reflection_type_table;
        }

        // TODO: make use of this table to lookup cached reflection types!

    }

    class ReflectionUtils {
        private:
            static Ptr<ReflectionType> EnsureType(Ptr<ReflectionType> ref_type) {
                auto &table = inner_impl::GetReflectionTypeTable();
                for(auto &type: table) {
                    if(EqualTypes(ref_type, type)) {
                        // Already present in the cached types' table
                        return type;
                    }
                }
                // Not present - store it
                table.push_back(ref_type);
                return ref_type;
            }

            static Ptr<ReflectionType> CreateArrayType(const String &name, const u32 dimensions) {
                // Try to find a primitive type
                auto pr_type = TypeTraits::GetFieldAnyType(name);
                if(pr_type != VariableType::Invalid) {
                    return ptr::New<ReflectionType>(pr_type, dimensions);
                }
                // Otherwise, search for a class type
                else {
                    auto class_type = inner_impl::LocateClassTypeImpl(ClassUtils::MakeSlashClassName(name));
                    if(class_type) {
                        return ptr::New<ReflectionType>(class_type, dimensions);
                    }
                }
                return nullptr;
            }
            
            static Ptr<ReflectionType> CreateTypeByName(const String &name) {
                u32 array_dimensions = 0;
                auto name_copy = name;
                while(name_copy.front() == '[') {
                    array_dimensions++;
                    name_copy.erase(0, 1);
                }

                if(name_copy.front() == 'L') {
                    if(name_copy.back() == ';') {
                        name_copy.erase(0, 1);
                        name_copy.pop_back();
                    }
                }

                // Try to find a primitive type
                auto pr_type = TypeTraits::GetFieldAnyType(name_copy);
                if(pr_type != VariableType::Invalid) {
                    return ptr::New<ReflectionType>(pr_type, array_dimensions);
                }
                // Otherwise, search for a class type
                else {
                    auto class_type = inner_impl::LocateClassTypeImpl(ClassUtils::MakeSlashClassName(name_copy));
                    if(class_type) {
                        return ptr::New<ReflectionType>(class_type, array_dimensions);
                    }
                }

                return nullptr;
            }

        public:
            static inline bool EqualTypes(Ptr<ReflectionType> t_a, Ptr<ReflectionType> t_b) {
                return t_a->GetTypeName() == t_b->GetTypeName();
            }

            static Ptr<ReflectionType> FindTypeByName(const String &name) {
                auto ref_type = CreateTypeByName(name);
                if(ref_type) {
                    return EnsureType(ref_type);
                }

                JAVM_LOG("Invalid found reflection type...");
                return nullptr;
            }

            static Ptr<ReflectionType> FindArrayType(Ptr<Array> &array) {
                if(array->IsClassInstanceArray()) {
                    auto ref_type = CreateArrayType(array->GetClassType()->GetClassName(), array->GetDimensions());
                    if(ref_type) {
                        return EnsureType(ref_type);
                    }
                }
                else {
                    auto ref_type = ptr::New<ReflectionType>(array->GetVariableType(), array->GetDimensions());
                    if(ref_type) {
                        return EnsureType(ref_type);
                    }
                }

                JAVM_LOG("Invalid found reflection array type...");
                return nullptr;
            }
    };

    namespace inner_impl {

        Ptr<ClassType> FindClassTypeImpl(const String &name) {
            return ReflectionUtils::FindTypeByName(name)->GetClassType();
        }

    }

}