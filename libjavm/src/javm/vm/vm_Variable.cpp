#include <javm/javm_VM.hpp>

namespace javm::vm {

    namespace {

        Ptr<Variable> g_NullObjectVariable;
        std::vector<Ptr<Variable>> g_CachedClassTypeVariableList;

        Ptr<Variable> TryFindCachedClassTypeVariable(const String &class_name) {
            for(auto &class_v: g_CachedClassTypeVariableList) {
                auto class_obj = class_v->GetAs<type::ClassInstance>();
                auto name_v = class_obj->GetField(u"name", u"Ljava/lang/String;");
                const auto name = jutil::GetStringValue(name_v);
                if(class_name == name) {
                    return class_v;
                }
            }

            return nullptr;
        }

    }

    Ptr<Variable> NewClassTypeVariable(Ptr<ref::ReflectionType> ref_type) {
        const auto class_name = ref_type->GetTypeName();
        auto cached_class_v = TryFindCachedClassTypeVariable(class_name);
        if(cached_class_v) {
            return cached_class_v;
        }

        auto class_class_type = rt::LocateClassType(u"java/lang/Class");

        // No need to call ctor, we set classLoader to null manually to avoid having to execute code
        // TODO: actually set a valid classLoader?
        auto class_v = NewClassVariable(class_class_type);
        auto class_obj = class_v->GetAs<type::ClassInstance>();

        class_obj->SetField(u"classLoader", u"Ljava/lang/ClassLoader;", MakeNull());

        auto class_name_v = jutil::NewString(class_name);
        class_obj->SetField(u"name", u"Ljava/lang/String;", class_name_v);

        // We've created a new class type variable, cache it
        g_CachedClassTypeVariableList.push_back(class_v);
        return class_v;
    }

    String FormatVariableType(Ptr<Variable> var) {
        if(!var) {
            return u"<invalid>";
        }

        const auto type = var->GetType();
        if(var->IsNull()) {
            return u"<null>";
        }

        if(IsPrimitiveVariableType(type)) {
            return GetPrimitiveTypeName(type);
        }
        else {
            if(type == VariableType::ClassInstance) {
                auto class_obj = var->GetAs<type::ClassInstance>();
                return MakeDotClassName(class_obj->GetClassType()->GetClassName());
            }
            else if(type == VariableType::Array) {
                auto arr_obj = var->GetAs<type::Array>();
                const auto arr_dims = arr_obj->GetDimensions();

                String base_s;
                if(arr_obj->IsClassInstanceArray()) {
                    auto cls_type = arr_obj->GetClassType();
                    base_s += MakeDotClassName(cls_type->GetClassName());
                }
                else {
                    base_s += GetPrimitiveTypeName(arr_obj->GetVariableType());
                }

                Ptr<Array> cur_array_obj = nullptr;
                for(u32 i = 0; i < arr_dims; i++) {
                    if(i == 0) {
                        cur_array_obj = arr_obj;
                    }
                    else {
                        cur_array_obj = cur_array_obj->GetAt(0)->GetAs<type::Array>();
                    }

                    base_s += u"[" + str::From(cur_array_obj->GetLength()) + u"]";
                }
                
                return base_s;
            }
        }

        return u"<unknown - type: " + str::From(static_cast<u32>(type)) + u">";
    }

    String FormatVariable(Ptr<Variable> var) {
        if(!var) {
            return u"<invalid>";
        }
        if(var->IsNull()) {
            return u"<null>";
        }

        const auto type = var->GetType();
        if(IsPrimitiveVariableType(type)) {
            switch(type) {
                case VariableType::Integer:
                case VariableType::Character:
                case VariableType::Boolean:
                case VariableType::Byte:
                case VariableType::Short: {
                    auto val = var->GetValue<type::Integer>();
                    return str::From(val);
                }
                case VariableType::Float: {
                    auto val = var->GetValue<type::Float>();
                    return str::From(val);
                }
                case VariableType::Double: {
                    auto val = var->GetValue<type::Double>();
                    return str::From(val);
                }
                case VariableType::Long: {
                    auto val = var->GetValue<type::Long>();
                    return str::From(val);
                }
                default:
                    return u"<wtf>";
            }
        }
        else {
            if(type == VariableType::ClassInstance) {
                auto class_obj = var->GetAs<type::ClassInstance>();
                const auto res = class_obj->CallInstanceMethod(u"toString", u"()Ljava/lang/String;", var);
                return jutil::GetStringValue(res.var);
            }
            if(type == VariableType::Array) {
                return u"<array>";
            }
        }

        return u"<unknown - type: " + str::From(static_cast<u32>(type)) + u">";
    }

}