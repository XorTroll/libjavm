#include <javm/javm_VM.hpp>

namespace javm::vm::ref {

    // TODO: cache reflection types?

    namespace {

        Ptr<ReflectionType> CreateArrayType(const String &name, const u32 dimensions) {
            // Try to find a primitive type
            const auto primitive_type = GetPrimitiveVariableTypeByName(name);
            if(primitive_type != VariableType::Invalid) {
                return ptr::New<ReflectionType>(primitive_type, dimensions);
            }
            // Otherwise, search for a class type
            else {
                auto class_type = rt::LocateClassType(name);
                if(class_type) {
                    return ptr::New<ReflectionType>(class_type, dimensions);
                }
            }
            return nullptr;
        }

        Ptr<ReflectionType> CreateTypeByName(const String &name) {
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
            const auto primitive_type = GetPrimitiveVariableTypeByName(name_copy);
            if(primitive_type != VariableType::Invalid) {
                return ptr::New<ReflectionType>(primitive_type, array_dimensions);
            }
            else {
                // It might be a descriptor rather than a name
                const auto primitive_type = GetVariableTypeByDescriptor(name_copy);
                if(primitive_type != VariableType::Invalid) {
                    return ptr::New<ReflectionType>(primitive_type, array_dimensions);
                }
                // Otherwise, search for a class type
                else {
                    JAVM_LOG("Searching for ref class type '%s' -> '%s'", str::ToUtf8(name).c_str(), str::ToUtf8(name_copy).c_str());
                    auto class_type = rt::LocateClassType(name_copy);
                    if(class_type) {
                        return ptr::New<ReflectionType>(class_type, array_dimensions);
                    }
                }
            }

            return nullptr;
        }

    }

    String ReflectionType::ComputeTypeName() {
        String base_name;
        const auto is_array = this->array_dimensions > 0;
        for(u32 i = 0; i < this->array_dimensions; i++) {
            base_name += u'[';
        }

        if(this->IsPrimitiveType()) {
            if(is_array) {
                base_name += GetPrimitiveTypeDescriptor(this->type);
            }
            else {
                base_name += GetPrimitiveTypeName(this->type);
            }
        }
        else if(this->IsClassInstance()) {
            const auto class_name = MakeDotClassName(this->base_type->GetClassName());
            if(is_array) {
                base_name += u"L" + class_name + u";";
            }
            else {
                base_name += class_name;
            }
        }
        else {
            // TODO
            return u"<wtf>";
        }
        return base_name;
    }

    Ptr<ReflectionType> FindReflectionTypeByName(const String &name) {
        return CreateTypeByName(name);
    }

    Ptr<ReflectionType> FindArrayReflectionType(Ptr<Array> &array) {
        if(array->IsClassInstanceArray()) {
            return CreateArrayType(array->GetClassType()->GetClassName(), array->GetDimensions());
        }
        else {
            return ptr::New<ReflectionType>(array->GetVariableType(), array->GetDimensions());
        }
    }

}