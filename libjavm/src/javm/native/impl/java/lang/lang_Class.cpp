#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/lang/lang_Class.hpp>
#include <javm/native/impl/impl_Base.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    ExecutionResult Class::registerNatives(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Class.registerNatives] called...");
        return ExecutionResult::Void();
    }

    ExecutionResult Class::getPrimitiveClass(const std::vector<Ptr<Variable>> &param_vars) {
        auto type_name_v = param_vars[0];
        auto type_name = jutil::GetStringValue(type_name_v);
        JAVM_LOG("[java.lang.Class.getPrimitiveClass] called - primitive type name: '%s'...", str::ToUtf8(type_name).c_str());
        auto ref_type = ref::FindReflectionTypeByName(type_name);
        if(ref_type) {
            return ExecutionResult::ReturnVariable(NewClassTypeVariable(ref_type));
        }
        return ExecutionResult::ReturnVariable(MakeNull());
    }

    ExecutionResult Class::desiredAssertionStatus0(const std::vector<Ptr<Variable>> &param_vars) {
        auto ref_type = GetReflectionTypeFromClassVariable(param_vars[0]);
        if(ref_type) {
            JAVM_LOG("[java.lang.Class.desiredAssertionStatus0] called - Reflection type name: '%s'...", str::ToUtf8(ref_type->GetTypeName()).c_str());
        }
        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Integer>(0));
    }

    ExecutionResult Class::forName0(const std::vector<Ptr<Variable>> &param_vars) {
        auto class_name_v = param_vars[0];
        const auto class_name = jutil::GetStringValue(class_name_v);
        auto init_v = param_vars[1];
        const auto init = init_v->GetAs<type::Boolean>();
        auto loader_v = param_vars[2];
        auto loader = loader_v->GetAs<type::ClassInstance>();

        JAVM_LOG("[java.lang.Class.forName0] called - Class name: '%s'...", str::ToUtf8(class_name).c_str());

        auto ref_type = ref::FindReflectionTypeByName(class_name);
        if(ref_type) {
            JAVM_LOG("[java.lang.Class.desiredAssertionStatus0] called - Processed reflection type: '%s'...", str::ToUtf8(ref_type->GetTypeName()).c_str());
            // Call the static initializer, just in case
            if(ref_type->IsClassInstance()) {
                auto class_type = ref_type->GetClassType();
                const auto ret = class_type->EnsureStaticInitializerCalled();
                if(ret.IsInvalidOrThrown()) {
                    return ret;
                }
            }
            return ExecutionResult::ReturnVariable(NewClassTypeVariable(ref_type));
        }
        else {
            return Throw(u"java/lang/ClassNotFoundException", class_name);
        }
    }

    ExecutionResult Class::getDeclaredFields0(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Class.getDeclaredFields0] called...");
        auto public_only_v = param_vars[0];

        auto ref_type = GetReflectionTypeFromClassVariable(this_var);
        if(ref_type) {
            JAVM_LOG("[java.lang.Class.getDeclaredFields0] reflection type name: '%s'...", str::ToUtf8(ref_type->GetTypeName()).c_str());
            if(ref_type->IsClassInstance()) {
                auto class_type = ref_type->GetClassType();
                auto field_class_type = rt::LocateClassType(u"java/lang/reflect/Field");
                if(field_class_type) {
                    if(class_type) {
                        auto &fields = class_type->GetRawFields();
                        const auto field_count = fields.size();
                        auto field_array = NewArrayVariable(field_count, field_class_type);
                        auto field_array_obj = field_array->GetAs<type::Array>();
                        for(u32 i = 0; i < field_count; i++) {
                            auto &field = fields[i];
                            auto field_v = NewClassVariable(field_class_type);
                            auto field_obj = field_v->GetAs<type::ClassInstance>();

                            auto declaring_class_obj = this_var;
                            JAVM_LOG("[java.lang.Class.getDeclaredFields0] Field[%d] -> %s", i, str::ToUtf8(field.GetName()).c_str());

                            field_obj->SetField(u"clazz", u"Ljava/lang/Class;", declaring_class_obj);

                            auto field_ref_type = ref::FindReflectionTypeByName(field.GetDescriptor());
                            if(field_ref_type) {
                                JAVM_LOG("[java.lang.Class.getDeclaredFields0] Field reflection type: '%s'...", str::ToUtf8(field_ref_type->GetTypeName()).c_str());
                                auto class_v = NewClassTypeVariable(field_ref_type);
                                field_obj->SetField(u"type", u"Ljava/lang/Class;", class_v);
                            }

                            auto offset = class_type->GetRawFieldUnsafeOffset(field.GetName(), field.GetDescriptor());
                            field_obj->SetField(u"slot", u"I", NewPrimitiveVariable<type::Integer>(offset));

                            auto name_v = jutil::NewString(field.GetName());
                            field_obj->SetField(u"name", u"Ljava/lang/String;", name_v);

                            auto modifiers_v = NewPrimitiveVariable<type::Integer>(field.GetAccessFlags());
                            field_obj->SetField(u"modifiers", u"I", modifiers_v);

                            auto signature_v = jutil::NewString(field.GetDescriptor());
                            field_obj->SetField(u"signature", u"Ljava/lang/String;", signature_v);

                            field_obj->SetField(u"override", u"Z", MakeFalse());

                            field_array_obj->SetAt(i, field_v);
                        }
                        JAVM_LOG("[java.lang.Class.getDeclaredFields0] Field count: %d", field_array_obj->GetLength());
                        return ExecutionResult::ReturnVariable(field_array);
                    }
                    else {
                        // No type / no fields (primitive type), let's return an empty array
                        auto field_array_v = NewArrayVariable(0, field_class_type);
                        return ExecutionResult::ReturnVariable(field_array_v);
                    }
                }
            }
        }
        
        return ExecutionResult::ReturnVariable(MakeNull());
    }

    ExecutionResult Class::isInterface(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Class.isInterface] called...");

        auto ref_type = GetReflectionTypeFromClassVariable(this_var);
        if(ref_type) {
            JAVM_LOG("[java.lang.Class.isInterface] reflection type name: '%s'...", str::ToUtf8(ref_type->GetTypeName()).c_str());
            if(ref_type->IsClassInstance()) {
                auto class_type = ref_type->GetClassType();
                if(class_type->HasFlag<AccessFlags::Interface>()) {
                    return ExecutionResult::ReturnVariable(MakeTrue());    
                }
            }
        }
        
        return ExecutionResult::ReturnVariable(MakeFalse());
    }

    ExecutionResult Class::isPrimitive(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Class.isPrimitive] called...");

        auto ref_type = GetReflectionTypeFromClassVariable(this_var);
        if(ref_type) {
            JAVM_LOG("[java.lang.Class.isPrimitive] reflection type name: '%s'...", str::ToUtf8(ref_type->GetTypeName()).c_str());
            if(ref_type->IsPrimitive()) {
                return ExecutionResult::ReturnVariable(MakeTrue());    
            }
        }
        
        return ExecutionResult::ReturnVariable(MakeFalse());
    }

    ExecutionResult Class::isAssignableFrom(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Class.isAssignableFrom] called...");

        auto ref_type_1 = GetReflectionTypeFromClassVariable(this_var);
        if(ref_type_1) {
            JAVM_LOG("[java.lang.Class.isAssignableFrom] reflection 1 type name: '%s'...", str::ToUtf8(ref_type_1->GetTypeName()).c_str());
            auto ref_type_2 = GetReflectionTypeFromClassVariable(param_vars[0]);
            if(ref_type_2) {
                JAVM_LOG("[java.lang.Class.isAssignableFrom] reflection 2 type name: '%s'...", str::ToUtf8(ref_type_2->GetTypeName()).c_str());
                if(ref_type_1->IsClassInstance() && ref_type_2->IsClassInstance()) {
                    auto class_1 = ref_type_1->GetClassType();
                    auto class_2 = ref_type_2->GetClassType();
                    if(class_1->CanCastTo(class_2->GetClassName())) {
                        return ExecutionResult::ReturnVariable(MakeTrue());
                    }
                }
            }
        }
        
        return ExecutionResult::ReturnVariable(MakeFalse());
    }

    ExecutionResult Class::getModifiers(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Class.getModifiers] called");
        return GetClassModifiers(this_var);
    }

}