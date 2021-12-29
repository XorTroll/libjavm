#include <javm/javm_VM.hpp>
#include <javm/native/impl/sun/misc/misc_Unsafe.hpp>
#include <javm/native/impl/impl_Base.hpp>

namespace javm::native::impl::sun::misc {

    using namespace vm;

    namespace {

        template<typename T>
        inline type::Boolean DoCompareAndSwap(Ptr<T> ptr, const T old_v, const T new_v) {
            if(ptr::GetValue(ptr) == old_v) {
                ptr::SetValue(ptr, new_v);
                return true;
            }
            else {
                ptr::SetValue(ptr, old_v);
                return false;
            }
        }

        constexpr inline std::pair<type::Integer, bool> DecodeUnsafeOffset(const type::Long raw_off) {
            union {
                struct {
                    type::Integer is_static;
                    type::Integer offset;
                };
                type::Long raw_offset;
            } offset{};
            offset.raw_offset = raw_off;
            return { offset.offset, static_cast<bool>(offset.is_static) };
        }

        constexpr inline type::Long EncodeUnsafeOffset(const type::Integer off, const bool is_static) {
            union {
                struct {
                    type::Integer is_static;
                    type::Integer offset;
                };
                type::Long raw_offset;
            } offset{};
            offset.offset = off;
            offset.is_static = static_cast<type::Integer>(is_static);
            return offset.raw_offset;
        }

    }

    ExecutionResult Unsafe::registerNatives(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.misc.Unsafe.registerNatives] called");
        return ExecutionResult::Void();
    }

    ExecutionResult Unsafe::arrayBaseOffset(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.misc.Unsafe.arrayBaseOffset] called");
        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Integer>(0));
    }

    ExecutionResult Unsafe::arrayIndexScale(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.misc.Unsafe.arrayIndexScale] called");
        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Integer>(sizeof(intptr_t)));
    }

    ExecutionResult Unsafe::addressSize(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.misc.Unsafe.addressSize] called");
        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Integer>(sizeof(intptr_t)));
    }

    ExecutionResult Unsafe::objectFieldOffset(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.misc.Unsafe.objectFieldOffset] called");
        auto field_v = param_vars[0];
        auto field_obj = field_v->GetAs<type::ClassInstance>();
        auto field_name_v = field_obj->GetField(u"name", u"Ljava/lang/String;");
        const auto field_name = jutil::GetStringValue(field_name_v);
        auto field_desc_v = field_obj->GetField(u"signature", u"Ljava/lang/String;");
        const auto field_desc = jutil::GetStringValue(field_desc_v);
        auto class_type_v = field_obj->GetField(u"clazz", u"Ljava/lang/Class;");
        auto ref_type = GetReflectionTypeFromClassVariable(class_type_v);
        if(ref_type) {
            JAVM_LOG("[sun.misc.Unsafe.objectFieldOffset] reflection type name: '%s'", str::ToUtf8(ref_type->GetTypeName()).c_str());
            if(ref_type->IsClassInstance()) {
                auto class_type = ref_type->GetClassType();
                const auto offset = class_type->GetRawFieldUnsafeOffset(field_name, field_desc);
                const auto is_static = class_type->IsRawFieldStatic(field_name, field_desc);
                const auto raw_offset = EncodeUnsafeOffset(offset, is_static);
                JAVM_LOG("[sun.misc.Unsafe.objectFieldOffset] field: '%s' - '%s', offset: %d", str::ToUtf8(field_name).c_str(), str::ToUtf8(field_desc).c_str(), offset);
                ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Long>(raw_offset));
            }
        }

        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Long>(0));
    }

    ExecutionResult Unsafe::getIntVolatile(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        auto obj_v = param_vars[0];
        auto obj_obj = obj_v->GetAs<type::ClassInstance>();
        auto obj_type = obj_obj->GetClassType();
        auto raw_off_v = param_vars[1];
        const auto raw_off = raw_off_v->GetValue<type::Long>();
        const auto [off, is_static] = DecodeUnsafeOffset(raw_off);
        JAVM_LOG("[sun.misc.Unsafe.getIntVolatile] called - offset: %d, static: %s", off, is_static ? "true" : "false");

        if(is_static) {
            auto var = obj_type->GetStaticFieldByUnsafeOffset(off);
            JAVM_LOG("[sun.misc.Unsafe.getIntVolatile] returning '%s'...", str::ToUtf8(FormatVariableType(var)).c_str());
            return ExecutionResult::ReturnVariable(var);
        }
        else {
            auto var = obj_obj->GetFieldByUnsafeOffset(off);
            JAVM_LOG("[sun.misc.Unsafe.getIntVolatile] returning '%s'...", str::ToUtf8(FormatVariableType(var)).c_str());
            return ExecutionResult::ReturnVariable(var);
        }

        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Integer>(sizeof(intptr_t)));
    }

    ExecutionResult Unsafe::compareAndSwapInt(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.misc.Unsafe.compareAndSwapInt] called");
        auto var_v = param_vars[0];
        auto var_obj = var_v->GetAs<type::ClassInstance>();
        auto raw_off_v = param_vars[1];
        const auto raw_off = raw_off_v->GetValue<type::Long>();
        auto expected_v = param_vars[2];
        const auto expected = expected_v->GetValue<type::Integer>();
        auto newv_v = param_vars[3];
        const auto newv = newv_v->GetValue<type::Integer>();
        const auto [off, is_static] = DecodeUnsafeOffset(raw_off);
        Ptr<Variable> field_v;
        if(is_static) {
            field_v = var_obj->GetClassType()->GetStaticFieldByUnsafeOffset(off);
        }
        else {
            field_v = var_obj->GetFieldByUnsafeOffset(off);
        }
        if(field_v) {
            auto field_ref = field_v->GetAs<type::Integer>();
            JAVM_LOG("[sun.misc.Unsafe.compareAndSwapInt] Offset: %d, Static: %s, Value: %d, Expected: %d, New: %d", off, is_static ? "true" : "false", ptr::GetValue(field_ref), expected, newv);
            const auto ret = DoCompareAndSwap(field_ref, expected, newv);
            field_v->SetAs<type::Integer>(field_ref);
            if(ret) {
                return ExecutionResult::ReturnVariable(MakeTrue());
            }
        }

        return ExecutionResult::ReturnVariable(MakeFalse());
    }

    ExecutionResult Unsafe::allocateMemory(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        auto size_v = param_vars[0];
        const auto size = size_v->GetValue<type::Long>();
        auto ptr = new u8[size]();
        JAVM_LOG("[sun.misc.Unsafe.allocateMemory] called - Size: %ld, Ptr: %p", size, ptr);
        const auto ptr_val = reinterpret_cast<intptr_t>(ptr);
        auto ptr_v = NewPrimitiveVariable<type::Long>(ptr_val);
        return ExecutionResult::ReturnVariable(ptr_v);
    }

    ExecutionResult Unsafe::putLong(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        auto addr_v = param_vars[0];
        const auto addr = addr_v->GetValue<type::Long>();
        auto val_v = param_vars[1];
        const auto val = val_v->GetValue<type::Long>();
        auto ptr = reinterpret_cast<type::Long*>(addr);
        JAVM_LOG("[sun.misc.Unsafe.putLong] called - Addr: %p, Value: %ld", ptr, val);
        *ptr = val;
        return ExecutionResult::Void();
    }

    ExecutionResult Unsafe::getByte(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        auto addr_v = param_vars[0];
        const auto addr = addr_v->GetValue<type::Long>();
        auto ptr = reinterpret_cast<u8*>(addr);
        JAVM_LOG("[sun.misc.Unsafe.getByte] called - Addr: %p", ptr);
        const auto byte = *ptr;
        auto byte_v = NewPrimitiveVariable<type::Byte>(static_cast<type::Byte>(byte));
        return ExecutionResult::ReturnVariable(byte_v);
    }

    ExecutionResult Unsafe::freeMemory(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        auto addr_v = param_vars[0];
        const auto addr = addr_v->GetValue<type::Long>();
        auto ptr = reinterpret_cast<u8*>(addr);
        JAVM_LOG("[sun.misc.Unsafe.freeMemory] called - Addr: %p", ptr);
        delete[] ptr;
        return ExecutionResult::Void();
    }

}