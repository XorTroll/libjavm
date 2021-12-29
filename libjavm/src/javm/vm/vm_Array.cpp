#include <javm/javm_VM.hpp>

namespace javm::vm {

    Ptr<Variable> Array::MakeDefaultVariable() {
        return NewDefaultVariable(this->type);
    }

    Ptr<ClassInstance> Array::CreateInnerObject() {
        auto obj_class_type = rt::LocateClassType(u"java/lang/Object");
        return ptr::New<ClassInstance>(obj_class_type);
    }

    bool Array::CanCastTo(const String &class_name) {
        if(this->class_type) {
            if(EqualClassNames(class_name, this->class_type->GetClassName())) {
                return true;
            }
            auto super_class = this->class_type->GetSuperClassType();
            while(super_class) {
                if(EqualClassNames(class_name, super_class->GetClassName())) {
                    return true;
                }
                auto sc = super_class->GetSuperClassType();
                super_class = sc;
            }
        }
        return false;
    }

    Ptr<Variable> Array::GetAt(const u32 idx) {
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

    bool Array::SetAt(const u32 idx, Ptr<Variable> var) {
        if(idx >= this->length) {
            return false;
        }

        const auto var_type = var->GetType();
        if(!IsVariableTypeConvertibleTo(var_type, this->type)) {
            return false;
        }

        if(this->IsClassInstanceArray()) {
            if(var_type == VariableType::ClassInstance) {
                auto var_class_type = var->GetAs<type::ClassInstance>()->GetClassType();
                if(!var_class_type->CanCastTo(this->class_type->GetClassName())) {
                    return false;
                }
            }
            else if(var_type == VariableType::Array) {
                // Arrays can only be casted to Object!
                if(MakeSlashClassName(this->class_type->GetClassName()) != u"java/lang/Object") {
                    return false;
                }
            }
        }

        this->inner_array[idx] = var;
        return true;
    }

}