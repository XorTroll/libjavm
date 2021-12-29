#include <javm/javm_VM.hpp>
#include <cmath>

namespace javm::vm {

    namespace {

        u32 GetFunctionDescriptorParameterCount(const String &descriptor) {
            auto tmp = descriptor.substr(descriptor.find_first_of(u'(') + 1);
            tmp = tmp.substr(0, tmp.find_last_of(u')'));

            bool parsing_class = false;
            u32 count = 0;
            for(const auto &ch: tmp) {
                if(ch == u'[') {
                    // Array, so not a new parameter
                    continue;
                }
                if(parsing_class) {
                    if(ch == u';') {
                        parsing_class = false;
                    }
                    continue;
                }
                else {
                    if(ch == u'L') {
                        parsing_class = true;
                    }
                }
                count++;
            }
            return count;
        }

        std::vector<Ptr<Variable>> LoadClassMethodParameters(ExecutionFrame &frame, const String &descriptor) {
            std::vector<Ptr<Variable>> params;
            const u32 param_count = GetFunctionDescriptorParameterCount(descriptor);
            for(u32 i = 0; i < param_count; i++) {
                auto param_var = frame.PopStack();
                params.push_back(param_var);
            }
            // Params are read in inverse order!
            std::reverse(params.begin(), params.end());
            return params;
        }

        std::pair<Ptr<Variable>, std::vector<Ptr<Variable>>> LoadInstanceMethodParameters(ExecutionFrame &frame, const String &descriptor) {
            std::vector<Ptr<Variable>> params;
            const u32 param_count = GetFunctionDescriptorParameterCount(descriptor);
            for(u32 i = 0; i < param_count; i++) {
                auto param_var = frame.PopStack();
                params.push_back(param_var);
            }
            auto this_var = frame.PopStack();
            // Params are read in inverse order!
            std::reverse(params.begin(), params.end());
            return { this_var, params };
        }

        void CreatePopulateMultidimensionalArray(const u32 dimensions, const std::vector<u32> &lengths, Ptr<ClassType> class_type, const VariableType type, Ptr<Variable> &cur_array, const u32 cur_dimension_idx = 0) {
            const auto dim_len = lengths.at(cur_dimension_idx);
            if(cur_dimension_idx == 0) {
                if(class_type) {
                    cur_array = NewArrayVariable(dim_len, class_type, dimensions - cur_dimension_idx);
                }
                else {
                    cur_array = NewArrayVariable(dim_len, type, dimensions - cur_dimension_idx);
                }
                CreatePopulateMultidimensionalArray(dimensions, lengths, class_type, type, cur_array, cur_dimension_idx + 1);
            }
            else if(cur_dimension_idx < (dimensions - 1)) {
                auto cur_array_obj = cur_array->GetAs<type::Array>();
                for(u32 i = 0; i < cur_array_obj->GetLength(); i++) {
                    Ptr<Variable> array;
                    if(class_type) {
                        array = NewArrayVariable(dim_len, class_type, dimensions - cur_dimension_idx);
                    }
                    else {
                        array = NewArrayVariable(dim_len, type, dimensions - cur_dimension_idx);
                    }

                    CreatePopulateMultidimensionalArray(dimensions, lengths, class_type, type, array, cur_dimension_idx + 1);
                    cur_array_obj->SetAt(i, array);
                }
            }
            else {
                // Last dimension to populate
                auto cur_array_obj = cur_array->GetAs<type::Array>();
                for(u32 i = 0; i < cur_array_obj->GetLength(); i++) {
                    Ptr<Variable> array;
                    if(class_type) {
                        array = NewArrayVariable(dim_len, class_type);
                    }
                    else {
                        array = NewArrayVariable(dim_len, type);
                    }

                    cur_array_obj->SetAt(i, array);
                }
            }
        }

    }

    namespace {

        ExecutionResult HandleInstruction(ExecutionFrame &frame) {
            const auto base_code_offset = frame.GetCodeOffset();

            const auto inst = static_cast<Instruction>(frame.ReadCode<u8>());
            JAVM_LOG("Got instruction: 0x%X", static_cast<u8>(inst));

            auto &cur_offset = frame.GetCodeOffset();

            #define _JAVM_LOAD_INSTRUCTION(instr, idx) \
            case Instruction::instr: { \
                    const auto index = static_cast<u32>(idx); \
                    auto var = frame.GetLocalAt(index); \
                    JAVM_LOG("[*load] Loaded: '%s' at locals[%d]", str::ToUtf8(FormatVariableType(var)).c_str(), index); \
                    if(var->CanGetAs<VariableType::Integer>()) { \
                        JAVM_LOG("[*load] Integer value: %d", var->GetValue<type::Integer>()); \
                    } \
                    frame.PushStack(var); \
                    break; \
                }

            #define _JAVM_LOAD_INSTRUCTION_INDEX_READ(instr) _JAVM_LOAD_INSTRUCTION(instr, frame.ReadCode<u8>())
            
            #define _JAVM_STORE_INSTRUCTION(instr, idx) \
            case Instruction::instr: { \
                    const auto index = static_cast<u32>(idx); \
                    auto var = frame.PopStack(); \
                    JAVM_LOG("[*store] Stored: '%s' at locals[%d]", str::ToUtf8(FormatVariableType(var)).c_str(), index); \
                    frame.SetLocalAt(index, var); \
                    break; \
                }
            
            #define _JAVM_STORE_INSTRUCTION_INDEX_READ(instr) _JAVM_STORE_INSTRUCTION(instr, frame.ReadCode<u8>())

            #define _JAVM_LDC_BASE(idx) { \
                auto &const_pool = frame.GetThisConstantPool(); \
                auto const_item = const_pool.GetItemAt(idx); \
                if(const_item) { \
                    JAVM_LOG("[ldc-base] Tag: %d", static_cast<u32>(const_item->GetTag())); \
                    switch(const_item->GetTag()) { \
                        case ConstantPoolTag::Integer: { \
                            const auto value = const_item->GetIntegerData().integer; \
                            JAVM_LOG("[ldc-base] Int value: '%d'", value); \
                            auto var = NewPrimitiveVariable<type::Integer>(value); \
                            frame.PushStack(var); \
                            break; \
                        } \
                        case ConstantPoolTag::Float: { \
                            const auto value = const_item->GetFloatData().flt; \
                            JAVM_LOG("[ldc-base] Float value: '%f'", value); \
                            auto var = NewPrimitiveVariable<type::Float>(value); \
                            frame.PushStack(var); \
                            break; \
                        } \
                        case ConstantPoolTag::Long: { \
                            const auto value = const_item->GetLongData().lng; \
                            JAVM_LOG("[ldc-base] Long value: '%ld'", value); \
                            auto var = NewPrimitiveVariable<type::Long>(value); \
                            frame.PushStack(var); \
                            break; \
                        } \
                        case ConstantPoolTag::Double: { \
                            const auto value = const_item->GetDoubleData().dbl; \
                            JAVM_LOG("[ldc-base] Double value: '%f'", value); \
                            auto var = NewPrimitiveVariable<type::Double>(value); \
                            frame.PushStack(var); \
                            break; \
                        } \
                        case ConstantPoolTag::String: { \
                            const auto str = const_item->GetStringData().processed_string; \
                            JAVM_LOG("[ldc-base] String value: '%s'", str::ToUtf8(str).c_str()); \
                            auto str_var = jutil::NewString(str); \
                            frame.PushStack(str_var); \
                            break; \
                        } \
                        case ConstantPoolTag::Class: { \
                            const auto type_name = const_item->GetClassData().processed_name; \
                            JAVM_LOG("[ldc-base] Type name: '%s'", str::ToUtf8(type_name).c_str()); \
                            auto ref_type = ref::FindReflectionTypeByName(type_name); \
                            if(ref_type) { \
                                auto class_v = NewClassTypeVariable(ref_type); \
                                frame.PushStack(class_v); \
                            } \
                            else { \
                                ThrowInternal(u"Invalid or unsupported constant pool item 1"); \
                            } \
                            break; \
                        } \
                        default: \
                            break; \
                    } \
                } \
                else { \
                    ThrowInternal(u"Invalid or unsupported constant pool item 2"); \
                } \
            }

            #define _JAVM_LDC_INSTRUCTION(instr, idx) \
            case Instruction::instr: { \
                    const auto index = idx; \
                    _JAVM_LDC_BASE(index) \
                    break; \
                }

            #define _JAVM_ALOAD_INSTRUCTION(instr) \
            case Instruction::instr: { \
                    auto index_var = frame.PopStack(); \
                    if(index_var->CanGetAs<VariableType::Integer>()) { \
                        const auto index = index_var->GetValue<type::Integer>(); \
                        if(index < 0) { \
                            return Throw(u"java/lang/ArrayIndexOutOfBoundsException", str::From(index)); \
                        } \
                        else { \
                            auto array_var = frame.PopStack(); \
                            if(array_var->CanGetAs<VariableType::Array>()) { \
                                auto array_obj = array_var->GetAs<type::Array>(); \
                                if(index < array_obj->GetLength()) { \
                                    auto inner_var = array_obj->GetAt(index); \
                                    if(inner_var) { \
                                        frame.PushStack(inner_var); \
                                    } \
                                    else { \
                                        return ThrowInternal(u"Invalid array index"); \
                                    } \
                                } \
                                else { \
                                    return Throw(u"java/lang/ArrayIndexOutOfBoundsException", str::From(index)); \
                                } \
                            } \
                            else if(array_var->IsNull()) { \
                                return Throw(u"java/lang/NullPointerException"); \
                            } \
                            else { \
                                return ThrowInternal(u"Invalid array item"); \
                            } \
                        } \
                    } \
                    else { \
                        return ThrowInternal(u"Invalid index var"); \
                    } \
                    break; \
                }

            #define _JAVM_ASTORE_INSTRUCTION(instr) \
            case Instruction::instr: { \
                    auto value = frame.PopStack(); \
                    auto index_var = frame.PopStack(); \
                    if(index_var->CanGetAs<VariableType::Integer>()) { \
                        const auto index = index_var->GetValue<type::Integer>(); \
                        if(index < 0) { \
                            return Throw(u"java/lang/ArrayIndexOutOfBoundsException", str::From(index)); \
                        } \
                        else { \
                            auto array_var = frame.PopStack(); \
                            if(array_var->CanGetAs<VariableType::Array>()) { \
                                auto array_obj = array_var->GetAs<type::Array>(); \
                                if(index < array_obj->GetLength()) { \
                                    if(!array_obj->SetAt(index, value)) { \
                                        return Throw(u"java/lang/ArrayStoreException", FormatVariableType(value)); \
                                    } \
                                } \
                                else { \
                                    return Throw(u"java/lang/ArrayIndexOutOfBoundsException", str::From(index)); \
                                } \
                            } \
                            else if(array_var->IsNull()) { \
                                return Throw(u"java/lang/NullPointerException"); \
                            } \
                            else { \
                                return ThrowInternal(u"Invalid array item"); \
                            } \
                        } \
                    } \
                    else { \
                        return ThrowInternal(u"Invalid index var"); \
                    } \
                    break; \
                }

            #define _JAVM_OPERATOR_INSTRUCTION(instr, typ, op) \
            case Instruction::instr: { \
                    auto var2 = frame.PopStack(); \
                    const auto var2_val = var2->GetValue<typ>(); \
                    auto var1 = frame.PopStack(); \
                    const auto var1_val = var1->GetValue<typ>(); \
                    auto res_var = NewPrimitiveVariable<typ>(var1_val op var2_val); \
                    frame.PushStack(res_var); \
                    break; \
                }

            #define _JAVM_DIV_INSTRUCTION(instr, typ) \
            case Instruction::instr: { \
                    auto var2 = frame.PopStack(); \
                    const auto var2_val = var2->GetValue<typ>(); \
                    if(var2_val == 0) { \
                        return Throw(u"java/lang/ArithmeticException", u"/ by zero"); \
                    } \
                    auto var1 = frame.PopStack(); \
                    const auto var1_val = var1->GetValue<typ>(); \
                    auto res_var = NewPrimitiveVariable<typ>(var1_val / var2_val); \
                    frame.PushStack(res_var); \
                    break; \
                }

            #define _JAVM_NEG_INSTRUCTION(instr, type) \
            case Instruction::instr: { \
                    auto var = frame.PopStack(); \
                    auto var_obj = var->GetAs<type>(); \
                    const auto var_val = ptr::GetValue(var_obj); \
                    auto res_var = NewPrimitiveVariable<type>(-var_val); \
                    frame.PushStack(res_var); \
                    break; \
                }

            #define _JAVM_CONVERSION_INSTRUCTION(instr, t1, t2) \
            case Instruction::instr: { \
                    auto var = frame.PopStack(); \
                    auto var_obj = var->GetAs<t1>(); \
                    const auto var_val = ptr::GetValue(var_obj); \
                    auto res_var = NewPrimitiveVariable<t2>((t2)var_val); \
                    frame.PushStack(res_var); \
                    break; \
                }

            #define _JAVM_CMP_INSTRUCTION(instr, typ) \
            case Instruction::instr: { \
                    auto var2 = frame.PopStack(); \
                    auto var2_obj = var2->GetAs<typ>(); \
                    const auto var2_val = ptr::GetValue(var2_obj); \
                    auto var1 = frame.PopStack(); \
                    auto var1_obj = var1->GetAs<typ>(); \
                    const auto var1_val = ptr::GetValue(var1_obj); \
                    if(var2_val > var1_val) { \
                        JAVM_LOG("[cmp]  %ld > %ld", var1_val, var2_val); \
                        auto res_var = NewPrimitiveVariable<type::Integer>(-1); \
                        frame.PushStack(res_var); \
                    } \
                    else if(var2_val < var1_val) { \
                        JAVM_LOG("[cmp]  %ld < %ld", var1_val, var2_val); \
                        auto res_var = NewPrimitiveVariable<type::Integer>(1); \
                        frame.PushStack(res_var); \
                    } \
                    else { \
                        JAVM_LOG("[cmp]  %ld == %ld", var1_val, var2_val); \
                        auto res_var = NewPrimitiveVariable<type::Integer>(0); \
                        frame.PushStack(res_var); \
                    } \
                    break; \
                }

            #define _JAVM_FDCMP_INSTRUCTION(instr, typ, nanv) \
            case Instruction::instr: { \
                    auto var2 = frame.PopStack(); \
                    auto var2_obj = var2->GetAs<typ>(); \
                    const auto var2_val = ptr::GetValue(var2_obj); \
                    auto var1 = frame.PopStack(); \
                    auto var1_obj = var1->GetAs<typ>(); \
                    const auto var1_val = ptr::GetValue(var1_obj); \
                    if(std::isnan(var1_val) || std::isnan(var2_val)) { \
                        JAVM_LOG("[fdcmp]  %f or %f is NaN!?", var1_val, var2_val); \
                        auto res_var = NewPrimitiveVariable<type::Integer>(nanv); \
                        frame.PushStack(res_var); \
                    } \
                    if(var2_val > var1_val) { \
                        JAVM_LOG("[fdcmp]  %f > %f", var1_val, var2_val); \
                        auto res_var = NewPrimitiveVariable<type::Integer>(-1); \
                        frame.PushStack(res_var); \
                    } \
                    else if(var2_val < var1_val) { \
                        JAVM_LOG("[fdcmp]  %f < %f", var1_val, var2_val); \
                        auto res_var = NewPrimitiveVariable<type::Integer>(1); \
                        frame.PushStack(res_var); \
                    } \
                    else { \
                        JAVM_LOG("[fdcmp]  %f == %f", var1_val, var2_val); \
                        auto res_var = NewPrimitiveVariable<type::Integer>(0); \
                        frame.PushStack(res_var); \
                    } \
                    break; \
                }

            switch(inst) {
                case Instruction::NOP: {
                    // Do nothing :P
                    break;
                }
                case Instruction::ACONST_NULL: {
                    auto null = MakeNull();
                    frame.PushStack(null);
                    break;
                }
                case Instruction::ICONST_M1: {
                    auto im1 = NewPrimitiveVariable<type::Integer>(-1);
                    frame.PushStack(im1);
                    break;
                }
                case Instruction::ICONST_0: {
                    auto i0 = NewPrimitiveVariable<type::Integer>(0);
                    frame.PushStack(i0);
                    break;
                }
                case Instruction::ICONST_1: {
                    auto i1 = NewPrimitiveVariable<type::Integer>(1);
                    frame.PushStack(i1);
                    break;
                }
                case Instruction::ICONST_2: {
                    auto i2 = NewPrimitiveVariable<type::Integer>(2);
                    frame.PushStack(i2);
                    break;
                }
                case Instruction::ICONST_3: {
                    auto i3 = NewPrimitiveVariable<type::Integer>(3);
                    frame.PushStack(i3);
                    break;
                }
                case Instruction::ICONST_4: {
                    auto i4 = NewPrimitiveVariable<type::Integer>(4);
                    frame.PushStack(i4);
                    break;
                }
                case Instruction::ICONST_5: {
                    auto i5 = NewPrimitiveVariable<type::Integer>(5);
                    frame.PushStack(i5);
                    break;
                }
                case Instruction::LCONST_0: {
                    auto l0 = NewPrimitiveVariable<type::Long>(0);
                    frame.PushStack(l0);
                    break;
                }
                case Instruction::LCONST_1: {
                    auto l1 = NewPrimitiveVariable<type::Long>(1);
                    frame.PushStack(l1);
                    break;
                }
                case Instruction::FCONST_0: {
                    auto f0 = NewPrimitiveVariable<type::Float>(0.0f);
                    frame.PushStack(f0);
                    break;
                }
                case Instruction::FCONST_1: {
                    auto f1 = NewPrimitiveVariable<type::Float>(1.0f);
                    frame.PushStack(f1);
                    break;
                }
                case Instruction::FCONST_2: {
                    auto f2 = NewPrimitiveVariable<type::Float>(2.0f);
                    frame.PushStack(f2);
                    break;
                }
                case Instruction::DCONST_0: {
                    auto d0 = NewPrimitiveVariable<type::Double>(0.0f);
                    frame.PushStack(d0);
                    break;
                }
                case Instruction::DCONST_1: {
                    auto d1 = NewPrimitiveVariable<type::Double>(1.0f);
                    frame.PushStack(d1);
                    break;
                }
                case Instruction::BIPUSH: {
                    const auto byte = frame.ReadCode<i8>();
                    auto i_v = NewPrimitiveVariable<type::Integer>(static_cast<type::Integer>(byte));
                    frame.PushStack(i_v);
                    break;
                }
                case Instruction::SIPUSH: {
                    const auto shrt = BE(frame.ReadCode<i16>());
                    auto i_v = NewPrimitiveVariable<type::Integer>(static_cast<type::Integer>(shrt));
                    frame.PushStack(i_v);
                    break;
                }
                _JAVM_LDC_INSTRUCTION(LDC, frame.ReadCode<u8>())
                _JAVM_LDC_INSTRUCTION(LDC_W, BE(frame.ReadCode<u16>()))
                _JAVM_LDC_INSTRUCTION(LDC2_W, BE(frame.ReadCode<u16>()))
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(ILOAD)
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(LLOAD)
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(FLOAD)
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(DLOAD)
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(ALOAD)
                _JAVM_LOAD_INSTRUCTION(ILOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(FLOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(LLOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(DLOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(ALOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(ILOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(FLOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(LLOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(DLOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(ALOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(ILOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(FLOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(LLOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(DLOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(ALOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(ILOAD_3, 3)
                _JAVM_LOAD_INSTRUCTION(FLOAD_3, 3)
                _JAVM_LOAD_INSTRUCTION(LLOAD_3, 3)
                _JAVM_LOAD_INSTRUCTION(DLOAD_3, 3)
                _JAVM_LOAD_INSTRUCTION(ALOAD_3, 3)
                _JAVM_ALOAD_INSTRUCTION(IALOAD)
                _JAVM_ALOAD_INSTRUCTION(LALOAD)
                _JAVM_ALOAD_INSTRUCTION(FALOAD)
                _JAVM_ALOAD_INSTRUCTION(DALOAD)
                _JAVM_ALOAD_INSTRUCTION(AALOAD)
                _JAVM_ALOAD_INSTRUCTION(BALOAD)
                _JAVM_ALOAD_INSTRUCTION(CALOAD)
                _JAVM_ALOAD_INSTRUCTION(SALOAD)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(ISTORE)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(LSTORE)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(FSTORE)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(DSTORE)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(ASTORE)
                _JAVM_STORE_INSTRUCTION(ISTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(FSTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(LSTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(DSTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(ASTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(ISTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(FSTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(LSTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(DSTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(ASTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(ISTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(FSTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(LSTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(DSTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(ASTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(ISTORE_3, 3)
                _JAVM_STORE_INSTRUCTION(FSTORE_3, 3)
                _JAVM_STORE_INSTRUCTION(LSTORE_3, 3)
                _JAVM_STORE_INSTRUCTION(DSTORE_3, 3)
                _JAVM_STORE_INSTRUCTION(ASTORE_3, 3)
                _JAVM_ASTORE_INSTRUCTION(IASTORE)
                _JAVM_ASTORE_INSTRUCTION(LASTORE)
                _JAVM_ASTORE_INSTRUCTION(FASTORE)
                _JAVM_ASTORE_INSTRUCTION(DASTORE)
                _JAVM_ASTORE_INSTRUCTION(AASTORE)
                _JAVM_ASTORE_INSTRUCTION(BASTORE)
                _JAVM_ASTORE_INSTRUCTION(CASTORE)
                _JAVM_ASTORE_INSTRUCTION(SASTORE)
                case Instruction::POP: {
                    frame.PopStack();
                    break;
                }
                case Instruction::POP2: {
                    if(frame.PopStack()->IsBigComputationalType()) {
                        break;
                    }
                    frame.PopStack();
                    break;
                }
                case Instruction::DUP: {
                    auto var = frame.PopStack();
                    frame.PushStack(var);
                    frame.PushStack(var);
                    break;
                }
                case Instruction::DUP_X1: {
                    auto var1 = frame.PopStack();
                    auto var2 = frame.PopStack();
                    frame.PushStack(var1);
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    break;
                }
                case Instruction::DUP_X2: {
                    auto var1 = frame.PopStack();
                    auto var2 = frame.PopStack();
                    if(var1->IsBigComputationalType()) {
                        frame.PushStack(var1);
                        frame.PushStack(var2);
                        frame.PushStack(var1);
                        break;
                    }
                    auto var3 = frame.PopStack();
                    frame.PushStack(var1);
                    frame.PushStack(var3);
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    break;
                }
                case Instruction::DUP2: {
                    auto var1 = frame.PopStack();
                    if(var1->IsBigComputationalType()) {
                        // Special case for long/double
                        frame.PushStack(var1);
                        frame.PushStack(var1);
                        break;
                    }
                    auto var2 = frame.PopStack();
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    break;
                }
                case Instruction::DUP2_X1: {
                    auto var1 = frame.PopStack();
                    auto var2 = frame.PopStack();
                    if(var1->IsBigComputationalType()) {
                        frame.PushStack(var1);
                        frame.PushStack(var2);
                        frame.PushStack(var1);
                        break;
                    }
                    auto var3 = frame.PopStack();
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    frame.PushStack(var3);
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    break;
                }
                case Instruction::DUP2_X2: {
                    auto v1 = frame.PopStack();
                    auto v2 = frame.PopStack();
                    auto v3 = frame.PopStack();
                    if(v1->IsBigComputationalType()) {
                        frame.PushStack(v1);
                        frame.PushStack(v3);
                        frame.PushStack(v2);
                        frame.PushStack(v1);
                        break;
                    }
                    else if(v3->IsBigComputationalType()) {
                        frame.PushStack(v2);
                        frame.PushStack(v1);
                        frame.PushStack(v3);
                        frame.PushStack(v2);
                        frame.PushStack(v1);
                        break;
                    }
                    else if(v2->IsBigComputationalType()) {
                        frame.PushStack(v1);
                        frame.PushStack(v2);
                        frame.PushStack(v1);
                        break;
                    }
                    auto v4 = frame.PopStack();
                    frame.PushStack(v2);
                    frame.PushStack(v1);
                    frame.PushStack(v4);
                    frame.PushStack(v3);
                    frame.PushStack(v2);
                    frame.PushStack(v1);
                    break;
                }
                _JAVM_OPERATOR_INSTRUCTION(IADD, type::Integer, +)
                _JAVM_OPERATOR_INSTRUCTION(LADD, type::Long, +)
                _JAVM_OPERATOR_INSTRUCTION(FADD, type::Float, +)
                _JAVM_OPERATOR_INSTRUCTION(DADD, type::Double, +)
                _JAVM_OPERATOR_INSTRUCTION(ISUB, type::Integer, -)
                _JAVM_OPERATOR_INSTRUCTION(LSUB, type::Long, -)
                _JAVM_OPERATOR_INSTRUCTION(FSUB, type::Float, -)
                _JAVM_OPERATOR_INSTRUCTION(DSUB, type::Double, -)
                _JAVM_OPERATOR_INSTRUCTION(IMUL, type::Integer, *)
                _JAVM_OPERATOR_INSTRUCTION(LMUL, type::Long, *)
                _JAVM_OPERATOR_INSTRUCTION(FMUL, type::Float, *)
                _JAVM_OPERATOR_INSTRUCTION(DMUL, type::Double, *)
                _JAVM_DIV_INSTRUCTION(IDIV, type::Integer)
                _JAVM_DIV_INSTRUCTION(LDIV, type::Long)
                _JAVM_DIV_INSTRUCTION(FDIV, type::Float)
                _JAVM_DIV_INSTRUCTION(DDIV, type::Double)
                _JAVM_OPERATOR_INSTRUCTION(IREM, type::Integer, %)
                _JAVM_OPERATOR_INSTRUCTION(LREM, type::Long, %)
                // Fuck u, decimals
                case Instruction::FREM: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Float>();
                    const auto var2_val = ptr::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Float>();
                    const auto var1_val = ptr::GetValue(var1_obj);
                    auto res_var = NewPrimitiveVariable<type::Float>(fmodf(var1_val, var2_val));
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::DREM: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Double>();
                    const auto var2_val = ptr::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Double>();
                    const auto var1_val = ptr::GetValue(var1_obj);
                    auto res_var = NewPrimitiveVariable<type::Double>(fmod(var1_val, var2_val));
                    frame.PushStack(res_var);
                    break;
                }
                _JAVM_NEG_INSTRUCTION(INEG, type::Integer)
                _JAVM_NEG_INSTRUCTION(LNEG, type::Long)
                _JAVM_NEG_INSTRUCTION(FNEG, type::Float)
                _JAVM_NEG_INSTRUCTION(DNEG, type::Double)
                case Instruction::ISHL: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    const auto var2_val = ptr::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Integer>();
                    const auto var1_val = ptr::GetValue(var1_obj);
                    const auto tmp = var2_val & 0x1F;
                    auto res_var = NewPrimitiveVariable<type::Integer>(var1_val << tmp);
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::LSHL: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    const auto var2_val = ptr::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Long>();
                    const auto var1_val = ptr::GetValue(var1_obj);
                    const auto tmp = var2_val & 0x3F;
                    auto res_var = NewPrimitiveVariable<type::Long>(var1_val << tmp);
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::ISHR: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    const auto var2_val = ptr::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Integer>();
                    const auto var1_val = ptr::GetValue(var1_obj);
                    const auto tmp = var2_val & 0x1F;
                    auto res_var = NewPrimitiveVariable<type::Integer>(var1_val >> tmp);
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::LSHR: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    const auto var2_val = ptr::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Long>();
                    const auto var1_val = ptr::GetValue(var1_obj);
                    const auto tmp = var2_val & 0x3F;
                    auto res_var = NewPrimitiveVariable<type::Long>(var1_val >> tmp);
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::IUSHR: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    const auto var2_val = ptr::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Integer>();
                    const auto var1_val = ptr::GetValue(var1_obj);
                    const auto tmp = var2_val & 0x1F;
                    if(var1_val >= 0) {
                        auto res_var = NewPrimitiveVariable<type::Integer>(var1_val >> tmp);
                        frame.PushStack(res_var);
                    }
                    else {
                        auto res_var = NewPrimitiveVariable<type::Integer>((var1_val >> tmp) + (2 << ~tmp));
                        frame.PushStack(res_var);
                    }
                    break;
                }
                case Instruction::LUSHR: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    const auto var2_val = ptr::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Long>();
                    const auto var1_val = ptr::GetValue(var1_obj);
                    const auto tmp = var2_val & 0x3F;
                    if(var1_val >= 0) {
                        auto res_var = NewPrimitiveVariable<type::Long>(var1_val >> tmp);
                        frame.PushStack(res_var);
                    }
                    else {
                        auto res_var = NewPrimitiveVariable<type::Long>((var1_val >> tmp) + (2 << ~tmp));
                        frame.PushStack(res_var);
                    }
                    break;
                }
                _JAVM_OPERATOR_INSTRUCTION(IAND, type::Integer, &)
                _JAVM_OPERATOR_INSTRUCTION(LAND, type::Long, &)
                _JAVM_OPERATOR_INSTRUCTION(IOR, type::Integer, |)
                _JAVM_OPERATOR_INSTRUCTION(LOR, type::Long, |)
                _JAVM_OPERATOR_INSTRUCTION(IXOR, type::Integer, ^)
                _JAVM_OPERATOR_INSTRUCTION(LXOR, type::Long, ^)
                case Instruction::IINC: {
                    const auto idx = frame.ReadCode<u8>();
                    const auto cnst = frame.ReadCode<i8>();
                    auto var = frame.GetLocalAt((u32)idx);
                    auto var_obj = var->GetAs<type::Integer>();
                    auto var_val = ptr::GetValue(var_obj);
                    var_val += cnst;
                    auto new_var = NewPrimitiveVariable<type::Integer>(var_val);
                    frame.SetLocalAt((u32)idx, new_var);
                    break;
                }
                _JAVM_CONVERSION_INSTRUCTION(I2L, type::Integer, type::Long)
                _JAVM_CONVERSION_INSTRUCTION(I2F, type::Integer, type::Float)
                _JAVM_CONVERSION_INSTRUCTION(I2D, type::Integer, type::Double)
                _JAVM_CONVERSION_INSTRUCTION(L2I, type::Long, type::Integer)
                _JAVM_CONVERSION_INSTRUCTION(L2F, type::Long, type::Float)
                _JAVM_CONVERSION_INSTRUCTION(L2D, type::Long, type::Double)
                _JAVM_CONVERSION_INSTRUCTION(F2I, type::Float, type::Integer)
                _JAVM_CONVERSION_INSTRUCTION(F2L, type::Float, type::Long)
                _JAVM_CONVERSION_INSTRUCTION(F2D, type::Float, type::Double)
                _JAVM_CONVERSION_INSTRUCTION(D2I, type::Double, type::Integer)
                _JAVM_CONVERSION_INSTRUCTION(D2L, type::Double, type::Long)
                _JAVM_CONVERSION_INSTRUCTION(D2F, type::Double, type::Float)
                _JAVM_CONVERSION_INSTRUCTION(I2B, type::Integer, type::Byte)
                _JAVM_CONVERSION_INSTRUCTION(I2C, type::Integer, type::Character)
                _JAVM_CONVERSION_INSTRUCTION(I2S, type::Integer, type::Short)
                _JAVM_CMP_INSTRUCTION(LCMP, type::Long)
                _JAVM_FDCMP_INSTRUCTION(FCMPL, type::Float, -1)
                _JAVM_FDCMP_INSTRUCTION(FCMPG, type::Float, 1)
                _JAVM_FDCMP_INSTRUCTION(DCMPL, type::Double, -1)
                _JAVM_FDCMP_INSTRUCTION(DCMPG, type::Double, 1)

                #define _JAVM_IF_INSTRUCTION(op) \
                const auto rel_code_offset = BE(frame.ReadCode<i16>()); \
                auto var = frame.PopStack(); \
                const auto var_val = var->GetValue<type::Integer>(); \
                if(var_val op 0) { \
                    cur_offset = base_code_offset; \
                    cur_offset += rel_code_offset; \
                } \

                case Instruction::IFEQ: {
                    _JAVM_IF_INSTRUCTION(==)
                    break;
                }
                case Instruction::IFNE: {
                    _JAVM_IF_INSTRUCTION(!=)
                    break;
                }
                case Instruction::IFLT: {
                    _JAVM_IF_INSTRUCTION(<)
                    break;
                }
                case Instruction::IFGE: {
                    _JAVM_IF_INSTRUCTION(>=)
                    break;
                }
                case Instruction::IFGT: {
                    _JAVM_IF_INSTRUCTION(>)
                    break;
                }
                case Instruction::IFLE: {
                    _JAVM_IF_INSTRUCTION(<=)
                    break;
                }

                #define _JAVM_ICMP_INSTRUCTION(op) \
                const auto rel_code_offset = BE(frame.ReadCode<i16>()); \
                auto var2 = frame.PopStack(); \
                const auto var2_val = var2->GetValue<type::Integer>(); \
                auto var1 = frame.PopStack(); \
                const auto var1_val = var1->GetValue<type::Integer>(); \
                JAVM_LOG("[icmp] %d " #op " %d", var1_val, var2_val); \
                if(var1_val op var2_val) { \
                    cur_offset = base_code_offset; \
                    cur_offset += rel_code_offset; \
                } \

                case Instruction::IF_ICMPEQ: {
                    _JAVM_ICMP_INSTRUCTION(==)
                    break;
                }
                case Instruction::IF_ICMPNE: {
                    _JAVM_ICMP_INSTRUCTION(!=)
                    break;
                }
                case Instruction::IF_ICMPLT: {
                    _JAVM_ICMP_INSTRUCTION(<)
                    break;
                }
                case Instruction::IF_ICMPGE: {
                    _JAVM_ICMP_INSTRUCTION(>=)
                    break;
                }
                case Instruction::IF_ICMPGT: {
                    _JAVM_ICMP_INSTRUCTION(>)
                    break;
                }
                case Instruction::IF_ICMPLE: {
                    _JAVM_ICMP_INSTRUCTION(<=)
                    break;
                }
                case Instruction::IF_ACMPEQ: {
                    auto var2 = frame.PopStack();
                    auto var1 = frame.PopStack();
                    JAVM_LOG("[acmpeq] %s == %s", str::ToUtf8(FormatVariable(var1)).c_str(), str::ToUtf8(FormatVariable(var2)).c_str());
                    const auto rel_code_offset = BE(frame.ReadCode<i16>());
                    if(ptr::Equal(var1, var2)) {
                        cur_offset = base_code_offset;
                        cur_offset += rel_code_offset;
                    }
                    break;
                }
                case Instruction::IF_ACMPNE: {
                    auto var2 = frame.PopStack();
                    auto var1 = frame.PopStack();
                    JAVM_LOG("[acmpne] %s != %s", str::ToUtf8(FormatVariable(var1)).c_str(), str::ToUtf8(FormatVariable(var2)).c_str());
                    const auto rel_code_offset = BE(frame.ReadCode<i16>());
                    if(!ptr::Equal(var1, var2)) {
                        cur_offset = base_code_offset;
                        cur_offset += rel_code_offset;
                    }
                    break;
                }
                case Instruction::GOTO: {
                    const auto rel_code_offset = BE(frame.ReadCode<i16>());
                    cur_offset = base_code_offset;
                    cur_offset += rel_code_offset;
                    break;
                }
                case Instruction::JSR: {
                    const auto rel_code_offset = BE(frame.ReadCode<i16>());
                    // Note: technically the variable should be a special "returnAddress", but we use a regular int for simplicity
                    // TODO: be more accurate with that?
                    auto code_offset_v = NewPrimitiveVariable<type::Integer>(cur_offset);
                    frame.PushStack(code_offset_v);

                    cur_offset = base_code_offset;
                    cur_offset += rel_code_offset;
                    break;
                }
                case Instruction::RET: {
                    const auto index = frame.ReadCode<u8>();
                    auto code_offset_v = frame.GetLocalAt(index);

                    const auto code_offset = code_offset_v->GetValue<type::Integer>();
                    cur_offset = code_offset;
                    break;
                }
                // TODO: TABLESWITCH
                case Instruction::LOOKUPSWITCH: {
                    // TODO: cleanup of this implementation
                    auto prev_pos = cur_offset - 1;
                    const auto orig_prev_pos = prev_pos;
                    if((prev_pos % 4) != 0) {
                        prev_pos += (4 - prev_pos % 4);
                    }
                    else {
                        prev_pos += 4;
                    }
                    const auto base_code_offset = cur_offset;
                    // Change position
                    cur_offset = prev_pos;
                    const auto default_byte = BE(frame.ReadCode<u32>());
                    const auto count = BE(frame.ReadCode<u32>());
                    std::map<u32, u32> table;
                    for(u32 i = 0; i < count; i++) {
                        const auto value = BE(frame.ReadCode<u32>());
                        const auto val_pos = BE(frame.ReadCode<u32>());
                        table.insert(std::make_pair(value, val_pos));
                    }
                    // Restore position
                    cur_offset = base_code_offset;
                    auto top_v = frame.PopStack();
                    const auto top = top_v->GetValue<type::Integer>();
                    auto it = table.find(top);
                    if(it != table.end()) {
                        cur_offset = it->second + orig_prev_pos;
                    }
                    else {
                        cur_offset = default_byte + orig_prev_pos;
                    }
                    break;
                }
                case Instruction::IRETURN:
                case Instruction::LRETURN:
                case Instruction::FRETURN:
                case Instruction::DRETURN:
                case Instruction::ARETURN: {
                    auto var = frame.PopStack();
                    JAVM_LOG("[*return] Returning '%s'...", str::ToUtf8(FormatVariableType(var)).c_str());
                    if(var->CanGetAs<VariableType::Integer>()) {
                        JAVM_LOG("[*return] Return int value: %d", var->GetValue<type::Integer>());
                    }
                    if(var->CanGetAs<VariableType::Boolean>()) {
                        JAVM_LOG("[*return] Return boolean value: %s", var->GetValue<type::Boolean>() ? "true" : "false");
                    }
                    return ExecutionResult::ReturnVariable(var);
                }
                case Instruction::RETURN: {
                    // Return nothing (void)
                    JAVM_LOG("[return] Returning void...");
                    return ExecutionResult::Void();
                }
                case Instruction::GETSTATIC: {
                    const auto index = BE(frame.ReadCode<u16>());

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_field_item = const_pool.GetItemAt(index, ConstantPoolTag::FieldRef);
                    if(const_field_item) {
                        auto const_field_data = const_field_item->GetFieldMethodRefData();
                        auto const_class_item = const_pool.GetItemAt(const_field_data.class_index, ConstantPoolTag::Class);
                        if(const_class_item) {
                            auto const_class_data = const_class_item->GetClassData();
                            const auto class_name = const_class_data.processed_name;
                            auto field_nat_item = const_pool.GetItemAt(const_field_data.name_and_type_index, ConstantPoolTag::NameAndType);
                            if(field_nat_item) {
                                auto field_nat_data = field_nat_item->GetNameAndTypeData();
                                const auto field_name = field_nat_data.processed_name;
                                const auto field_desc = field_nat_data.processed_desc;

                                JAVM_LOG("[getstatic] Get static field '%s' ('%s') of '%s'...", str::ToUtf8(field_name).c_str(), str::ToUtf8(field_desc).c_str(), str::ToUtf8(class_name).c_str());

                                auto class_type = rt::LocateClassType(class_name);
                                if(class_type) {
                                    auto var = class_type->GetStaticField(field_name, field_desc);
                                    JAVM_LOG("[getstatic] Static field: '%s'...", str::ToUtf8(FormatVariableType(var)).c_str());
                                    frame.PushStack(var);
                                }
                                else {
                                    return ThrowInternal(str::Format("Invalid class name: '%s'", str::ToUtf8(class_name).c_str()));
                                }
                            }
                            else {
                                return ThrowInternal(u"Invalid const pool item NAT...?");
                            }
                        }
                        else {
                            return ThrowInternal(u"Invalid const pool item Class...?");
                        }
                    }
                    else {
                        return ThrowInternal(u"Invalid const pool item FieldRef...?");
                    }

                    break;
                }
                case Instruction::PUTSTATIC: {
                    const auto index = BE(frame.ReadCode<u16>());
                    auto var = frame.PopStack();
                    
                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_field_item = const_pool.GetItemAt(index, ConstantPoolTag::FieldRef);
                    if(const_field_item) {
                        auto const_field_data = const_field_item->GetFieldMethodRefData();
                        auto const_class_item = const_pool.GetItemAt(const_field_data.class_index, ConstantPoolTag::Class);
                        if(const_class_item) {
                            auto const_class_data = const_class_item->GetClassData();
                            const auto class_name = const_class_data.processed_name;
                            auto field_nat_item = const_pool.GetItemAt(const_field_data.name_and_type_index, ConstantPoolTag::NameAndType);
                            if(field_nat_item) {
                                auto field_nat_data = field_nat_item->GetNameAndTypeData();
                                const auto field_name = field_nat_data.processed_name;
                                const auto field_desc = field_nat_data.processed_desc;

                                JAVM_LOG("[putstatic] Set static field '%s' ('%s') of '%s'...", str::ToUtf8(field_name).c_str(), str::ToUtf8(field_desc).c_str(), str::ToUtf8(class_name).c_str());
                                JAVM_LOG("[putstatic] Static field: '%s'...", str::ToUtf8(FormatVariableType(var)).c_str());

                                auto class_type = rt::LocateClassType(class_name);
                                if(class_type) {
                                    class_type->SetStaticField(field_name, field_desc, var);
                                }
                                else {
                                    return ThrowInternal(str::Format("Invalid class name: '%s'", str::ToUtf8(class_name).c_str()));
                                }
                            }
                            else {
                                return ThrowInternal(u"Invalid const pool item NAT...?");
                            }
                        }
                        else {
                            return ThrowInternal(u"Invalid const pool item Class...?");
                        }
                    }
                    else {
                        return ThrowInternal(u"Invalid const pool item FieldRef...?");
                    }

                    break;
                }
                case Instruction::GETFIELD: {
                    const auto index = BE(frame.ReadCode<u16>());
                    auto var = frame.PopStack();

                    if(var->CanGetAs<VariableType::ClassInstance>()) {
                        auto var_obj = var->GetAs<type::ClassInstance>();

                        auto &const_pool = frame.GetThisConstantPool();
                        auto const_field_item = const_pool.GetItemAt(index, ConstantPoolTag::FieldRef);
                        if(const_field_item) {
                            auto const_field_data = const_field_item->GetFieldMethodRefData();
                            auto const_class_item = const_pool.GetItemAt(const_field_data.class_index, ConstantPoolTag::Class);
                            if(const_class_item) {
                                auto const_class_data = const_class_item->GetClassData();
                                const auto class_name = const_class_data.processed_name;
                                auto field_nat_item = const_pool.GetItemAt(const_field_data.name_and_type_index, ConstantPoolTag::NameAndType);
                                if(field_nat_item) {
                                    auto field_nat_data = field_nat_item->GetNameAndTypeData();
                                    const auto field_name = field_nat_data.processed_name;
                                    const auto field_desc = field_nat_data.processed_desc;

                                    JAVM_LOG("[getfield] Get field '%s' ('%s') of '%s'...", str::ToUtf8(field_name).c_str(), str::ToUtf8(field_desc).c_str(), str::ToUtf8(class_name).c_str());

                                    auto var_obj_c = var_obj->GetInstanceByClassType(var_obj, class_name);
                                    if(var_obj_c) {
                                        auto field_var = var_obj_c->GetField(field_name, field_desc);
                                        JAVM_LOG("[getfield] Field: '%s'...", str::ToUtf8(FormatVariableType(field_var)).c_str());
                                        frame.PushStack(field_var);
                                    }
                                    else {
                                        return ThrowInternal(str::Format("Invalid class name: '%s'", str::ToUtf8(class_name).c_str()));
                                    }
                                }
                                else {
                                    return ThrowInternal(u"Invalid const pool item NAT...?");
                                }
                            }
                            else {
                                return ThrowInternal(u"Invalid const pool item Class...?");
                            }
                        }
                        else {
                            return ThrowInternal(u"Invalid const pool item FieldRef...?");
                        }
                    }
                    else if(var->IsNull()) {
                        // Trying to get a field from a null object
                        return Throw(u"java/lang/NullPointerException");
                    }
                    else {
                        return ThrowInternal(u"Invalid class object...");
                    }

                    break;
                }
                case Instruction::PUTFIELD: {
                    const auto index = BE(frame.ReadCode<u16>());
                    auto field_var = frame.PopStack();
                    auto var = frame.PopStack();

                    if(var->CanGetAs<VariableType::ClassInstance>()) {
                        auto var_obj = var->GetAs<type::ClassInstance>();

                        auto &const_pool = frame.GetThisConstantPool();
                        auto const_field_item = const_pool.GetItemAt(index, ConstantPoolTag::FieldRef);
                        if(const_field_item) {
                            auto const_field_data = const_field_item->GetFieldMethodRefData();
                            auto const_class_item = const_pool.GetItemAt(const_field_data.class_index, ConstantPoolTag::Class);
                            if(const_class_item) {
                                auto const_class_data = const_class_item->GetClassData();
                                const auto class_name = const_class_data.processed_name;
                                auto field_nat_item = const_pool.GetItemAt(const_field_data.name_and_type_index, ConstantPoolTag::NameAndType);
                                if(field_nat_item) {
                                    auto field_nat_data = field_nat_item->GetNameAndTypeData();
                                    const auto field_name = field_nat_data.processed_name;
                                    const auto field_desc = field_nat_data.processed_desc;

                                    JAVM_LOG("[putfield] Set field '%s' ('%s') of '%s'...", str::ToUtf8(field_name).c_str(), str::ToUtf8(field_desc).c_str(), str::ToUtf8(class_name).c_str());
                                    JAVM_LOG("[putfield] Field: '%s'...", str::ToUtf8(FormatVariableType(field_var)).c_str());

                                    auto var_obj_c = var_obj->GetInstanceByClassType(var_obj, class_name);
                                    if(var_obj_c) {
                                        var_obj_c->SetField(field_name, field_desc, field_var);
                                    }
                                    else {
                                        return ThrowInternal(str::Format("Invalid class name: '%s'", str::ToUtf8(class_name).c_str()));
                                    }
                                }
                                else {
                                    return ThrowInternal(u"Invalid const pool item NAT...?");
                                }
                            }
                            else {
                                return ThrowInternal(u"Invalid const pool item Class...?");
                            }
                        }
                        else {
                            return ThrowInternal(u"Invalid const pool item FieldRef...?");
                        }
                    }
                    else {
                        return ThrowInternal(u"Invalid class object...");
                    }

                    break;
                }
                case Instruction::INVOKEVIRTUAL:
                case Instruction::INVOKESPECIAL:
                case Instruction::INVOKEINTERFACE: {
                    const auto index = BE(frame.ReadCode<u16>());
                    const bool is_interface = inst == Instruction::INVOKEINTERFACE;
                    const bool is_virtual = inst == Instruction::INVOKEVIRTUAL;
                    const bool is_special = inst == Instruction::INVOKESPECIAL;
                    if(is_interface) {
                        // TODO: interface has extra items, both unused...?
                        const auto count = frame.ReadCode<u8>();
                        const auto zero = frame.ReadCode<u8>();
                    }
                    
                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_fn_item = const_pool.GetItemAt(index, is_interface ? ConstantPoolTag::InterfaceMethodRef : ConstantPoolTag::MethodRef);
                    if(const_fn_item) {
                        auto const_fn_data = const_fn_item->GetFieldMethodRefData();
                        auto const_class_item = const_pool.GetItemAt(const_fn_data.class_index, ConstantPoolTag::Class);
                        if(const_class_item) {
                            auto const_class_data = const_class_item->GetClassData();
                            const auto class_name = const_class_data.processed_name;
                            auto const_fn_nat_item = const_pool.GetItemAt(const_fn_data.name_and_type_index, ConstantPoolTag::NameAndType);
                            if(const_fn_nat_item) {
                                auto const_fn_nat_data = const_fn_nat_item->GetNameAndTypeData();
                                const auto fn_name = const_fn_nat_data.processed_name;
                                const auto fn_desc = const_fn_nat_data.processed_desc;

                                if(is_virtual) {
                                    JAVM_LOG("[invokevirtual]");
                                }
                                if(is_special) {
                                    JAVM_LOG("[invokespecial]");
                                }
                                if(is_interface) {
                                    JAVM_LOG("[invokeinterface]");
                                }

                                JAVM_LOG("[invoke] Executing '%s'::'%s'::'%s'...", str::ToUtf8(class_name).c_str(), str::ToUtf8(fn_name).c_str(), str::ToUtf8(fn_desc).c_str());

                                auto [this_var, param_vars] = LoadInstanceMethodParameters(frame, fn_desc);
                                JAVM_LOG("[invoke] Parameter count: %ld + this...", param_vars.size());
                                JAVM_LOG("[invoke] T this variable: %s", str::ToUtf8(FormatVariableType(this_var)).c_str());
                                if(this_var->CanGetAs<VariableType::ClassInstance>()) {
                                    auto this_var_obj = this_var->GetAs<type::ClassInstance>();
                                    Ptr<ClassInstance> this_var_obj_c = nullptr;
                                    if(is_special) {
                                        this_var_obj_c = this_var_obj->GetInstanceByClassTypeAndMethodSpecial(this_var_obj, class_name, fn_name, fn_desc);
                                    }
                                    else {
                                        this_var_obj_c = this_var_obj->GetInstanceByClassTypeAndMethodVirtualInterface(this_var_obj, class_name, fn_name, fn_desc);
                                    }
                                    JAVM_LOG("[invoke] This type: '%s'...", str::ToUtf8(this_var_obj->GetClassType()->GetClassName()).c_str());
                                    JAVM_LOG("[invoke] Detected instance type: '%s'...", str::ToUtf8(this_var_obj_c->GetClassType()->GetClassName()).c_str());
                                    if(this_var_obj_c) {
                                        const auto res = this_var_obj_c->CallInstanceMethod(fn_name, fn_desc, this_var, param_vars);
                                        if(res.IsInvalidOrThrown()) {
                                            JAVM_LOG("Invalid/thrown execution of '%s'::'%s'::'%s'...", str::ToUtf8(class_name).c_str(), str::ToUtf8(fn_name).c_str(), str::ToUtf8(fn_desc).c_str());
                                            return res;
                                        }
                                        if(res.Is<ExecutionStatus::VariableReturn>()) {
                                            if(res.var) {
                                                JAVM_LOG("[invoke] Return var type: '%s'...", str::ToUtf8(FormatVariableType(res.var)).c_str());
                                                frame.PushStack(res.var);
                                            }
                                            else {
                                                return ThrowInternal(str::Format("Invalid return var: %s", str::ToUtf8(FormatVariableType(res.var)).c_str()));
                                            }
                                        }
                                    }
                                    else {
                                        return ThrowInternal(str::Format("Invalid this variable 1: %s", str::ToUtf8(FormatVariableType(this_var)).c_str()));
                                    }
                                }
                                else if(this_var->CanGetAs<VariableType::Array>()) {
                                    auto this_array = this_var->GetAs<type::Array>();

                                    JAVM_LOG("[invoke] This array type: '%s'...", str::ToUtf8(FormatVariableType(this_var)).c_str());
                                    const auto res = this_array->CallInstanceMethod(fn_name, fn_desc, this_var, param_vars);
                                    if(res.IsInvalidOrThrown()) {
                                        JAVM_LOG("Invalid/thrown execution of '%s'::'%s'::'%s'...", str::ToUtf8(class_name).c_str(), str::ToUtf8(fn_name).c_str(), str::ToUtf8(fn_desc).c_str());
                                        return res;
                                    }
                                    if(res.Is<ExecutionStatus::VariableReturn>()) {
                                        if(res.var) {
                                            JAVM_LOG("[invoke] Return var type: '%s'...", str::ToUtf8(FormatVariableType(res.var)).c_str());
                                            frame.PushStack(res.var);
                                        }
                                        else {
                                            return ThrowInternal(str::Format("Invalid return var: %s", str::ToUtf8(FormatVariableType(res.var)).c_str()));
                                        }
                                    }
                                }
                                else {
                                    return ThrowInternal(str::Format("Invalid this variable 2: %s", str::ToUtf8(FormatVariableType(this_var)).c_str()));
                                }
                                JAVM_LOG("[invoke] Done '%s'::'%s'::'%s'...", str::ToUtf8(class_name).c_str(), str::ToUtf8(fn_name).c_str(), str::ToUtf8(fn_desc).c_str());
                            }
                            else {
                                return ThrowInternal(u"Invalid const pool NAT item");
                            }
                        }
                        else {
                            return ThrowInternal(u"Invalid const pool Class item");
                        }
                    }
                    else {
                        return ThrowInternal(u"Invalid const pool FieldRef item");
                    }

                    break;
                }
                case Instruction::INVOKESTATIC: {
                    const auto index = BE(frame.ReadCode<u16>());
                    
                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_fn_item = const_pool.GetItemAt(index, ConstantPoolTag::MethodRef);
                    if(const_fn_item) {
                        auto const_fn_data = const_fn_item->GetFieldMethodRefData();
                        auto const_class_item = const_pool.GetItemAt(const_fn_data.class_index, ConstantPoolTag::Class);
                        if(const_class_item) {
                            auto const_class_data = const_class_item->GetClassData();
                            const auto class_name = const_class_data.processed_name;
                            auto const_fn_nat_item = const_pool.GetItemAt(const_fn_data.name_and_type_index, ConstantPoolTag::NameAndType);
                            if(const_fn_nat_item) {
                                auto const_fn_nat_data = const_fn_nat_item->GetNameAndTypeData();
                                const auto fn_name = const_fn_nat_data.processed_name;
                                const auto fn_desc = const_fn_nat_data.processed_desc;

                                JAVM_LOG("[invokestatic] Executing '%s'::'%s'::'%s'...", str::ToUtf8(class_name).c_str(), str::ToUtf8(fn_name).c_str(), str::ToUtf8(fn_desc).c_str());

                                auto class_type = rt::LocateClassType(class_name);
                                if(class_type) {
                                    auto param_vars = LoadClassMethodParameters(frame, fn_desc);
                                    JAVM_LOG("[invokestatic] Parameter count: %ld...", param_vars.size());
                                    const auto res = class_type->CallClassMethod(fn_name, fn_desc, param_vars);
                                    if(res.IsInvalidOrThrown()) {
                                        JAVM_LOG("Invalid/thrown execution of '%s'::'%s'::'%s'...", str::ToUtf8(class_name).c_str(), str::ToUtf8(fn_name).c_str(), str::ToUtf8(fn_desc).c_str());
                                        return res;
                                    }
                                    if(res.Is<ExecutionStatus::VariableReturn>()) {
                                        if(res.var) {
                                            frame.PushStack(res.var);
                                        }
                                        else {
                                            JAVM_LOG("Invalid instance method return value...");
                                        }
                                    }
                                }
                                else {
                                    JAVM_LOG("Invalid class name: '%s'", str::ToUtf8(class_name).c_str());
                                }

                                JAVM_LOG("[invokestatic] Done '%s'::'%s'::'%s'...", str::ToUtf8(class_name).c_str(), str::ToUtf8(fn_name).c_str(), str::ToUtf8(fn_desc).c_str());
                            }
                            else {
                                JAVM_LOG("Invalid const pool item NAT...?");
                            }
                        }
                        else {
                            JAVM_LOG("Invalid const pool item Class...?");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid const pool item FieldRef...?");
                    }

                    break;
                }
                // TODO: INVOKEDYNAMIC
                case Instruction::NEW: {
                    const auto index = BE(frame.ReadCode<u16>());

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_class_item = const_pool.GetItemAt(index, ConstantPoolTag::Class);
                    if(const_class_item) {
                        auto const_class_data = const_class_item->GetClassData();
                        const auto class_name = const_class_data.processed_name;

                        JAVM_LOG("[NEW] new '%s'...", str::ToUtf8(class_name).c_str());

                        auto class_type = rt::LocateClassType(class_name);
                        if(class_type) {
                            const auto res = class_type->EnsureStaticInitializerCalled();
                            if(res.IsInvalidOrThrown()) {
                                return res;
                            }
                            auto class_var = NewClassVariable(class_type);
                            frame.PushStack(class_var);
                        }
                        else {
                            JAVM_LOG("Invalid class type...?");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid const pool class item...?");
                    }

                    break;
                }
                case Instruction::NEWARRAY: {
                    const auto type = frame.ReadCode<u8>();
                    auto len_var = frame.PopStack();

                    if(len_var->CanGetAs<VariableType::Integer>()) {
                        const auto len_val = len_var->GetValue<type::Integer>();
                        if(len_val >= 0) {
                            auto val_type = GetVariableTypeFromNewArrayType(static_cast<NewArrayType>(type));
                            if(val_type != VariableType::Invalid) {
                                auto arr_v = NewArrayVariable(len_val, val_type);
                                frame.PushStack(arr_v);
                            }
                            else {
                                return ThrowInternal(u"Invalid array type...");
                            }
                        }
                        else {
                            return Throw(u"java/lang/NegativeArraySizeException");
                        }
                    }
                    else {
                        return ThrowInternal(u"Invalid length variable...");
                    }

                    break;
                }
                case Instruction::ANEWARRAY: {
                    const auto index = BE(frame.ReadCode<u16>());
                    auto len_var = frame.PopStack();

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_class_item = const_pool.GetItemAt(index, ConstantPoolTag::Class);
                    if(const_class_item) {
                        auto const_class_data = const_class_item->GetClassData();
                        const auto class_name = const_class_data.processed_name;

                        JAVM_LOG("[anewarray] Class name: '%s'", str::ToUtf8(class_name).c_str());
                        auto class_type = rt::LocateClassType(class_name);
                        JAVM_LOG("[anewarray] Got class: '%s'", str::ToUtf8(class_name).c_str());
                        if(class_type) {
                            const auto res = class_type->EnsureStaticInitializerCalled();
                            if(res.IsInvalidOrThrown()) {
                                return res;
                            }
                            if(len_var->CanGetAs<VariableType::Integer>()) {
                                const auto len_val = len_var->GetValue<type::Integer>();
                                if(len_val >= 0) {
                                    JAVM_LOG("[anewarray] Array length: %d", len_val);
                                    auto arr_v = NewArrayVariable(len_val, class_type);
                                    JAVM_LOG("[anewarray] Created array! '%s'", str::ToUtf8(FormatVariableType(arr_v)).c_str());
                                    frame.PushStack(arr_v);
                                }
                                else {
                                    return Throw(u"java/lang/NegativeArraySizeException");
                                }
                            }
                            else {
                                return ThrowInternal(u"Invalid length variable...");
                            }
                        }
                        else {
                            return ThrowInternal(u"Invalid array class type...");
                        }
                    }
                    else {
                        return ThrowInternal(u"Invalid constant pool item...");
                    }

                    break;
                }
                case Instruction::ARRAYLENGTH: {
                    auto arr_var = frame.PopStack();
                    if(arr_var->CanGetAs<VariableType::Array>()) {
                        auto arr_obj = arr_var->GetAs<type::Array>();
                        const auto arr_len = arr_obj->GetLength();

                        JAVM_LOG("[arraylength] -> Array: '%s'...", str::ToUtf8(FormatVariableType(arr_var)).c_str());
                        JAVM_LOG("[arraylength] -> Length: %d...", arr_obj->GetLength());

                        auto len_v = NewPrimitiveVariable<type::Integer>(arr_len);
                        frame.PushStack(len_v);
                    }
                    else if(arr_var->IsNull()) {
                        return Throw(u"java/lang/NullPointerException");
                    }
                    else {
                        return ThrowInternal(u"Invalid array object");
                    }

                    break;
                }
                case Instruction::ATHROW: {
                    auto throwable_var = frame.PopStack();
                    JAVM_LOG("[athrow] Throwable object: '%s'", str::ToUtf8(FormatVariableType(throwable_var)).c_str());

                    if(throwable_var->IsNull()) {
                        return Throw(u"java/lang/NullPointerException");
                    }

                    return ThrowExisting(throwable_var);
                }
                case Instruction::CHECKCAST: {
                    const auto index = BE(frame.ReadCode<u16>());
                    auto var = frame.PopStack();

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_class_item = const_pool.GetItemAt(index, ConstantPoolTag::Class);
                    if(const_class_item) {
                        auto const_class_data = const_class_item->GetClassData();
                        const auto class_name = const_class_data.processed_name;

                        JAVM_LOG("[checkcast] class name: '%s'", str::ToUtf8(class_name).c_str());

                        if(var->CanGetAs<VariableType::ClassInstance>()) {
                            auto var_obj = var->GetAs<type::ClassInstance>();
                            JAVM_LOG("[checkcast] instance class name: '%s'", str::ToUtf8(var_obj->GetClassType()->GetClassName()).c_str());
                            if(var_obj->GetClassType()->CanCastTo(class_name)) {
                                frame.PushStack(var);
                            }
                            else {
                                return Throw(u"java/lang/ClassCastException", str::Format("%s cannot be cast to %s", str::ToUtf8(MakeDotClassName(var_obj->GetClassType()->GetClassName())).c_str(), str::ToUtf8(MakeDotClassName(class_name)).c_str()));
                            }
                        }
                        else if(var->IsNull()) {
                            frame.PushStack(var);
                        }
                        else if(var->CanGetAs<VariableType::Array>()) {
                            auto arr_obj = var->GetAs<type::Array>();
                            auto class_type = arr_obj->GetClassType();

                            auto class_ref_type = ref::FindReflectionTypeByName(class_name);
                            if(class_ref_type->IsArray()) {
                                if(class_type->CanCastTo(GetClassNameFromDescriptor(class_name))) {
                                    frame.PushStack(var);
                                }
                                else {
                                    return ThrowInternal(u"Invalid array cast");
                                }
                            }
                            else {
                                return ThrowInternal(str::Format("Casting array of '%s' to non-array type '%s'...", str::ToUtf8(class_type->GetClassName()).c_str(), str::ToUtf8(class_name).c_str()));
                            }
                        }
                        else {
                            return ThrowInternal(u"Invalid var type");
                        }
                    }
                    else {
                        return ThrowInternal(u"Invalid constant pool item");
                    }

                    break;
                }
                case Instruction::INSTANCEOF: {
                    const auto index = BE(frame.ReadCode<u16>());
                    auto var = frame.PopStack();

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_class_item = const_pool.GetItemAt(index, ConstantPoolTag::Class);
                    if(const_class_item) {
                        auto const_class_data = const_class_item->GetClassData();
                        const auto class_name = const_class_data.processed_name;

                        if(var->CanGetAs<VariableType::ClassInstance>()) {
                            auto var_obj = var->GetAs<type::ClassInstance>();
                            if(var_obj->GetClassType()->CanCastTo(class_name)) {
                                frame.PushStack(MakeTrue());
                            }
                            else {
                                frame.PushStack(MakeFalse());
                            }
                        }
                        else if(var->IsNull()) {
                            frame.PushStack(MakeFalse());
                        }
                    }

                    break;
                }
                case Instruction::MONITORENTER: {
                    auto var = frame.PopStack();
                    if(var->CanGetAs<VariableType::ClassInstance>()) {
                        auto var_obj = var->GetAs<type::ClassInstance>();
                        auto monitor = var_obj->GetMonitor();
                        monitor->Enter();
                    }
                    else if(var->IsNull()) {
                        return Throw(u"java/lang/NullPointerException");
                    }
                    else {
                        return ThrowInternal(u"Invalid monitor enter");
                    }

                    break;
                }
                case Instruction::MONITOREXIT: {
                    auto var = frame.PopStack();
                    if(var->CanGetAs<VariableType::ClassInstance>()) {
                        auto var_obj = var->GetAs<type::ClassInstance>();
                        auto monitor = var_obj->GetMonitor();
                        monitor->Leave();
                    }
                    else if(var->IsNull()) {
                        return Throw(u"java/lang/NullPointerException");
                    }
                    else {
                        return ThrowInternal(u"Invalid monitor leave");
                    }

                    break;
                }
                case Instruction::MULTIANEWARRAY: {
                    const auto index = BE(frame.ReadCode<u16>());
                    const auto dimensions = frame.ReadCode<u8>();

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_class_item = const_pool.GetItemAt(index, ConstantPoolTag::Class);
                    if(const_class_item) {
                        auto const_class_data = const_class_item->GetClassData();
                        const auto class_name = const_class_data.processed_name;
                        const auto base_type_name = GetClassNameFromDescriptor(class_name);

                        std::vector<u32> lens;
                        lens.reserve(dimensions);
                        for(u32 i = 0; i < dimensions; i++) {
                            auto len_var = frame.PopStack();
                            const auto len_val = len_var->GetValue<type::Integer>();
                            if(len_val < 0) {
                                return Throw(u"java/lang/NegativeArraySizeException");
                            }

                            lens.insert(lens.begin(), len_val);
                        }
                        Ptr<Variable> base_arr_v;

                        if(IsPrimitiveType(base_type_name)) {
                            const auto type = GetVariableTypeByDescriptor(base_type_name);
                            JAVM_LOG("[multianewarray] Primitive type name: '%s'", str::ToUtf8(GetPrimitiveTypeName(type)).c_str());

                            CreatePopulateMultidimensionalArray(dimensions, lens, nullptr, type, base_arr_v);
                        }
                        else {
                            JAVM_LOG("[multianewarray] Full array type name: '%s'", str::ToUtf8(class_name).c_str());
                            JAVM_LOG("[multianewarray] Class name: '%s'", str::ToUtf8(base_type_name).c_str());
                            auto class_type = rt::LocateClassType(base_type_name);
                            if(class_type) {
                                const auto res = class_type->EnsureStaticInitializerCalled();
                                if(res.IsInvalidOrThrown()) {
                                    return res;
                                }

                                CreatePopulateMultidimensionalArray(dimensions, lens, class_type, VariableType::Invalid, base_arr_v);
                            }
                            else {
                                return ThrowInternal(u"Invalid array class type...");
                            }
                        }

                        JAVM_LOG("[multianewarray] Created multi array! '%s'", str::ToUtf8(FormatVariableType(base_arr_v)).c_str());
                        frame.PushStack(base_arr_v);
                    }
                    else {
                        return ThrowInternal(u"Invalid constant pool item...");
                    }
                    break;
                }
                case Instruction::IFNULL: {
                    auto var = frame.PopStack();
                    const auto rel_code_offset = BE(frame.ReadCode<i16>());
                    if(var->IsNull()) {
                        cur_offset = base_code_offset;
                        cur_offset += rel_code_offset;
                    }
                    break;
                }
                case Instruction::IFNONNULL: {
                    auto var = frame.PopStack();
                    const auto rel_code_offset = BE(frame.ReadCode<i16>());
                    if(!var->IsNull()) {
                        cur_offset = base_code_offset;
                        cur_offset += rel_code_offset;
                    }
                    break;
                }
                case Instruction::GOTO_W: {
                    const auto rel_code_offset = BE(frame.ReadCode<i32>());
                    cur_offset = base_code_offset;
                    cur_offset += rel_code_offset;
                    break;
                }
                case Instruction::JSR_W: {
                    const auto rel_code_offset = BE(frame.ReadCode<i32>());
                    auto pos_v = NewPrimitiveVariable<type::Integer>(cur_offset);
                    frame.PushStack(pos_v);

                    cur_offset = base_code_offset;
                    cur_offset += rel_code_offset;
                    break;
                }
                
                default:
                    return ThrowInternal(str::Format("Invalid or unimplemented instruction: 0x%X", static_cast<u32>(inst)));
            }
            return ExecutionResult::ContinueCodeExecution();
        }

        ExecutionResult DoExecuteCode(ExecutionFrame &frame) {
            if(IsThrown()) {
                return ThrowAlreadyThrown();
            }
            auto cur_accessor = GetCurrentThread();

            while(true) {
                const auto orig_code_offset = frame.GetCodeOffset();
                if(cur_accessor) {
                    cur_accessor->UpdateCurrentCallCodeOffset(orig_code_offset);
                }

                const auto res = HandleInstruction(frame);
                if(res.Is<ExecutionStatus::Thrown>()) {
                    auto throwable_v = res.var;
                    auto throwable_obj = throwable_v->GetAs<type::ClassInstance>();

                    if(res.catchable_throw) {
                        auto handled_throwable = false;

                        auto &const_pool = frame.GetThisConstantPool();
                        for(const auto &active_exc: frame.GetAvailableExceptionTableEntries(orig_code_offset)) {
                            String class_name;
                            if(active_exc.catch_exc_type_index == 0) {
                                // Catch any exception
                                class_name = u"java/lang/Throwable";
                            }
                            else {
                                auto const_class_item = const_pool.GetItemAt(active_exc.catch_exc_type_index, ConstantPoolTag::Class);
                                if(const_class_item) {
                                    const auto const_class_data = const_class_item->GetClassData();
                                    class_name = const_class_data.processed_name;
                                }
                            }

                            if(!class_name.empty()) {
                                JAVM_LOG("[VM-THROW] Exception table entry class name: '%s'", str::ToUtf8(class_name).c_str());
                                if(throwable_obj->GetClassType()->CanCastTo(class_name)) {
                                    JAVM_LOG("[VM-THROW] Jumping to exception table entry...");
                                    auto &cur_offset = frame.GetCodeOffset();
                                    cur_offset = active_exc.handler_code_offset;
                                    frame.PushStack(throwable_v);
                                    handled_throwable = true;
                                }
                            }
                            else {
                                return ThrowInternal(u"Invalid constant pool item CATCH");
                            }
                        }

                        if(handled_throwable) {
                            ResetThrown();
                            continue;
                        }
                    }

                    // TODO: getMessage is a better option than reading this field?
                    const auto msg_v = throwable_obj->GetField(u"detailMessage", u"Ljava/lang/String;");
                    const auto msg_str = jutil::GetStringValue(msg_v);
                    JAVM_LOG("[VM-THROW] Thrown message: '%s'", str::ToUtf8(msg_str).c_str());

                    if(!IsThrown()) {
                        RegisterThrown(throwable_v);
                    }
                    return res;
                }

                if(!res.Is<ExecutionStatus::ContinueExecution>()) {
                    return res;
                }
            }
        }

        inline void DoSetLocalParameters(ExecutionFrame &frame, const std::vector<Ptr<Variable>> &param_vars, const u32 i_base) {
            auto i = i_base;
            for(const auto &param: param_vars) {
                frame.SetLocalAt(i, param);
                i++;
                if(param->IsBigComputationalType()) {
                    i++;
                }
            }
        }
        
        inline void SetLocalStaticParameters(ExecutionFrame &frame, const std::vector<Ptr<Variable>> &param_vars) {
            DoSetLocalParameters(frame, param_vars, 0);
        }

        inline void SetLocalParameters(ExecutionFrame &frame, Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
            frame.SetLocalAt(0, this_var);
            DoSetLocalParameters(frame, param_vars, 1);
        }

    }

    ExecutionResult ExecuteStaticCode(const u8 *code_ptr, const u16 max_locals, const std::vector<ExceptionTableEntry> &exc_table, ConstantPool &pool, const std::vector<Ptr<Variable>> &param_vars) {
        auto max_locals_val = max_locals;
        for(const auto &param: param_vars) {
            // Longs and doubles take extra spaces
            if(param->IsBigComputationalType()) {
                max_locals_val++;
            }
        }

        ExecutionFrame frame(code_ptr, max_locals_val, exc_table, pool);
        SetLocalStaticParameters(frame, param_vars);
        return DoExecuteCode(frame);
    }

    ExecutionResult ExecuteCode(const u8 *code_ptr, const u16 max_locals, const std::vector<ExceptionTableEntry> &exc_table, Ptr<Variable> this_var, ConstantPool &pool, const std::vector<Ptr<Variable>> &param_vars) {
        auto max_locals_val = max_locals;
        for(const auto &param: param_vars) {
            // Longs and doubles take extra spaces
            if(param->IsBigComputationalType()) {
                max_locals_val++;
            }
        }

        ExecutionFrame frame(code_ptr, max_locals_val, exc_table, pool, this_var);
        SetLocalParameters(frame, this_var, param_vars);
        return DoExecuteCode(frame);
    }

}