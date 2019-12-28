
#pragma once
#include <javm/core/core_Instructions.hpp>
#include <javm/core/core_Archive.hpp>
#include <javm/core/core_CodeAttribute.hpp>
#include <javm/native/native_Class.hpp>
#include <javm/native/native_Standard.hpp>
#include <algorithm>
#include <memory>
#include <cmath>

namespace javm::core {

    class Machine {

        private:
            std::vector<std::unique_ptr<Archive>> loaded_archives;
            std::vector<std::unique_ptr<ClassFile>> class_files;
            std::vector<std::unique_ptr<native::Class>> native_classes;

            template<typename Arg>
            void HandlePushArgument(Frame &frame, u32 &index, Arg &arg) {
                static_assert(std::is_pointer_v<Arg>, "Call arguments must be pointers to variables!");
                frame.SetLocalReference(index, arg);
                index++;
            }

            ValuePointerHolder HandleInstruction(Frame &frame, bool &should_ret, Instruction inst) {
                auto &offset = frame.GetOffset();

                #define _JAVM_LOAD_INSTRUCTION(instr, idx, type) \
                case Instruction::instr: { \
                        u8 index = idx; \
                        auto ref = frame.GetLocalReference<type>((u32)index); \
                        frame.PushReference(ref); \
                        break; \
                    }

                #define _JAVM_LOAD_INSTRUCTION_INDEX_READ(instr, type) _JAVM_LOAD_INSTRUCTION(instr, frame.Read<u8>(), type)
                
                #define _JAVM_STORE_INSTRUCTION(idx, instr, type) \
                case Instruction::instr: { \
                        u8 index = idx; \
                        auto val = frame.PopValue<type>(); \
                        frame.CreateSetLocal<type>((u32)index, val); \
                        break; \
                    }

                #define _JAVM_ASTORE_N_INSTRUCTION(idx) \
                case Instruction::ASTORE_##idx: { \
                        u8 index = idx; \
                        auto holder = frame.Pop(); \
                        frame.SetLocal((u32)index, holder); \
                        break; \
                    }
                
                #define _JAVM_STORE_INSTRUCTION_INDEX_READ(instr, type) _JAVM_STORE_INSTRUCTION(frame.Read<u8>(), instr, type)

                #define _JAVM_LDC_INSTRUCTION(instr, idx) \
                case Instruction::instr: { \
                        auto index = idx; \
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool(); \
                        auto &constant = constant_pool[index - 1]; \
                        switch(constant.GetTag()) { \
                            case CPTag::Integer: { \
                                int value = constant.GetIntegerData().integer; \
                                frame.CreatePush<int>(value); \
                                break; \
                            } \
                            case CPTag::String: { \
                                std::string value = constant.GetStringData().processed_string; \
                                auto str_obj = ValuePointerHolder::Create<java::lang::String>(); \
                                auto str_ref = str_obj.GetReference<java::lang::String>(); \
                                str_ref->SetString(value); \
                                frame.Push(str_obj); \
                                break; \
                            } \
                            default: \
                                break; \
                        } \
                        break; \
                    }

                #define _JAVM_ALOAD_INSTRUCTION(instr) \
                case Instruction::instr: { \
                        int index = frame.PopValue<int>(); \
                        auto array = frame.PopReference<Array>(); \
                        auto holder = array->at(index); \
                        frame.Push(holder); \
                        break; \
                    }

                #define _JAVM_ASTORE_INSTRUCTION(instr) \
                case Instruction::instr: { \
                        auto holder = frame.Pop(); \
                        int idx = frame.PopValue<int>(); \
                        auto arr = frame.PopReference<Array>(); \
                        arr->at(idx) = holder; \
                        break; \
                    }

                #define _JAVM_OPERATOR_INSTRUCTION(instr, type, op) \
                case Instruction::instr: { \
                        type v2 = frame.PopValue<type>(); \
                        type v1 = frame.PopValue<type>(); \
                        frame.CreatePush<type>(v1 op v2); \
                        break; \
                    }

                #define _JAVM_NEG_INSTRUCTION(instr, type) \
                case Instruction::instr: { \
                        type val = frame.PopValue<type>(); \
                        frame.CreatePush<type>(-val); \
                        break; \
                    }

                #define _JAVM_CONVERSION_INSTRUCTION(instr, t1, t2) \
                case Instruction::instr: { \
                        t1 val = frame.PopValue<t1>(); \
                        frame.CreatePush<t2>((t2)val); \
                        break; \
                    }

                #define _JAVM_CMP_INSTRUCTION(instr, type) \
                case Instruction::instr: { \
                        type v2 = frame.PopValue<type>(); \
                        type v1 = frame.PopValue<type>(); \
                        if(v1 > v2) { \
                            frame.CreatePush<int>(-1); \
                        } \
                        else if(v1 < v2) { \
                            frame.CreatePush<int>(1); \
                        } \
                        else { \
                            frame.CreatePush<int>(0); \
                        } \
                        break; \
                    }

                printf(" -- Read instruction: 0x%X\n", (u32)inst);
                switch(inst) {
                    case Instruction::NOP: {
                        // Do nothing :P
                        break;
                    }
                    case Instruction::ACONST_NULL: {
                        frame.Push(ValuePointerHolder::CreateNull());
                        break;
                    }
                    case Instruction::ICONST_M1: {
                        frame.CreatePush<int>(-1);
                        break;
                    }
                    case Instruction::ICONST_0: {
                        frame.CreatePush<int>(0);
                        break;
                    }
                    case Instruction::ICONST_1: {
                        frame.CreatePush<int>(1);
                        break;
                    }
                    case Instruction::ICONST_2: {
                        frame.CreatePush<int>(2);
                        break;
                    }
                    case Instruction::ICONST_3: {
                        frame.CreatePush<int>(3);
                        break;
                    }
                    case Instruction::ICONST_4: {
                        frame.CreatePush<int>(4);
                        break;
                    }
                    case Instruction::ICONST_5: {
                        frame.CreatePush<int>(5);
                        break;
                    }
                    case Instruction::LCONST_0: {
                        frame.CreatePush<long>(0);
                        break;
                    }
                    case Instruction::LCONST_1: {
                        frame.CreatePush<long>(1);
                        break;
                    }
                    case Instruction::FCONST_0: {
                        frame.CreatePush<float>(0.0f);
                        break;
                    }
                    case Instruction::FCONST_1: {
                        frame.CreatePush<float>(1.0f);
                        break;
                    }
                    case Instruction::FCONST_2: {
                        frame.CreatePush<float>(2.0f);
                        break;
                    }
                    case Instruction::DCONST_0: {
                        frame.CreatePush<double>(0.0f);
                        break;
                    }
                    case Instruction::DCONST_1: {
                        frame.CreatePush<double>(1.0f);
                        break;
                    }
                    case Instruction::BIPUSH: {
                        u8 byte = frame.Read<u8>();
                        frame.CreatePush<int>((int)byte);
                        break;
                    }
                    case Instruction::SIPUSH: {
                        i16 shrt = BE(frame.Read<i16>());
                        frame.CreatePush<int>((int)shrt);
                        break;
                    }
                    _JAVM_LDC_INSTRUCTION(LDC, frame.Read<u8>())
                    _JAVM_LDC_INSTRUCTION(LDC_W, frame.Read<u16>())
                    case Instruction::LDC2_W: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto &constant = constant_pool[index - 1];
                        switch(constant.GetTag()) {
                            case CPTag::Long: {
                                long value = constant.GetLongData().lng;
                                frame.CreatePush<long>(value);
                                break;
                            }
                            case CPTag::Double: {
                                double value = constant.GetDoubleData().dbl;
                                frame.CreatePush<double>(value);
                                break;
                            }
                            default:
                                break;
                        }
                        break;
                    }
                    _JAVM_LOAD_INSTRUCTION_INDEX_READ(ILOAD, int)
                    _JAVM_LOAD_INSTRUCTION_INDEX_READ(LLOAD, long)
                    _JAVM_LOAD_INSTRUCTION_INDEX_READ(FLOAD, float)
                    _JAVM_LOAD_INSTRUCTION_INDEX_READ(DLOAD, double)
                    case Instruction::ALOAD: {
                        u8 index = frame.Read<u8>();
                        auto holder = frame.GetLocal((u32)index);
                        frame.Push(holder);
                        break;
                    }
                    _JAVM_LOAD_INSTRUCTION(ILOAD_0, 0, int)
                    _JAVM_LOAD_INSTRUCTION(LLOAD_0, 0, long)
                    _JAVM_LOAD_INSTRUCTION(FLOAD_0, 0, float)
                    _JAVM_LOAD_INSTRUCTION(DLOAD_0, 0, double)
                    case Instruction::ALOAD_0: {
                        auto holder = frame.GetLocal(0);
                        frame.Push(holder);
                        break;
                    }
                    _JAVM_LOAD_INSTRUCTION(ILOAD_1, 1, int)
                    _JAVM_LOAD_INSTRUCTION(LLOAD_1, 1, long)
                    _JAVM_LOAD_INSTRUCTION(FLOAD_1, 1, float)
                    _JAVM_LOAD_INSTRUCTION(DLOAD_1, 1, double)
                    case Instruction::ALOAD_1: {
                        auto holder = frame.GetLocal(1);
                        frame.Push(holder);
                        break;
                    }
                    _JAVM_LOAD_INSTRUCTION(ILOAD_2, 2, int)
                    _JAVM_LOAD_INSTRUCTION(LLOAD_2, 2, long)
                    _JAVM_LOAD_INSTRUCTION(FLOAD_2, 2, float)
                    _JAVM_LOAD_INSTRUCTION(DLOAD_2, 2, double)
                    case Instruction::ALOAD_2: {
                        auto holder = frame.GetLocal(2);
                        frame.Push(holder);
                        break;
                    }
                    _JAVM_LOAD_INSTRUCTION(ILOAD_3, 3, int)
                    _JAVM_LOAD_INSTRUCTION(LLOAD_3, 3, long)
                    _JAVM_LOAD_INSTRUCTION(FLOAD_3, 3, float)
                    _JAVM_LOAD_INSTRUCTION(DLOAD_3, 3, double)
                    case Instruction::ALOAD_3: {
                        auto holder = frame.GetLocal(3);
                        frame.Push(holder);
                        break;
                    }
                    _JAVM_ALOAD_INSTRUCTION(IALOAD)
                    _JAVM_ALOAD_INSTRUCTION(LALOAD)
                    _JAVM_ALOAD_INSTRUCTION(FALOAD)
                    _JAVM_ALOAD_INSTRUCTION(DALOAD)
                    _JAVM_ALOAD_INSTRUCTION(AALOAD)
                    _JAVM_ALOAD_INSTRUCTION(BALOAD)
                    _JAVM_ALOAD_INSTRUCTION(CALOAD)
                    _JAVM_ALOAD_INSTRUCTION(SALOAD)
                    _JAVM_STORE_INSTRUCTION_INDEX_READ(ISTORE, int)
                    _JAVM_STORE_INSTRUCTION_INDEX_READ(LSTORE, long)
                    _JAVM_STORE_INSTRUCTION_INDEX_READ(FSTORE, float)
                    _JAVM_STORE_INSTRUCTION_INDEX_READ(DSTORE, double)
                    case Instruction::ASTORE: {
                        u8 index = frame.Read<u8>();
                        auto holder = frame.Pop();
                        frame.SetLocal((u32)index, holder);
                        break;
                    }
                    _JAVM_STORE_INSTRUCTION(0, ISTORE_0, int)
                    _JAVM_STORE_INSTRUCTION(0, LSTORE_0, long)
                    _JAVM_STORE_INSTRUCTION(0, FSTORE_0, float)
                    _JAVM_STORE_INSTRUCTION(0, DSTORE_0, double)
                    _JAVM_ASTORE_N_INSTRUCTION(0)
                    _JAVM_STORE_INSTRUCTION(1, ISTORE_1, int)
                    _JAVM_STORE_INSTRUCTION(1, LSTORE_1, long)
                    _JAVM_STORE_INSTRUCTION(1, FSTORE_1, float)
                    _JAVM_STORE_INSTRUCTION(1, DSTORE_1, double)
                    _JAVM_ASTORE_N_INSTRUCTION(1)
                    _JAVM_STORE_INSTRUCTION(2, ISTORE_2, int)
                    _JAVM_STORE_INSTRUCTION(2, LSTORE_2, long)
                    _JAVM_STORE_INSTRUCTION(2, FSTORE_2, float)
                    _JAVM_STORE_INSTRUCTION(2, DSTORE_2, double)
                    _JAVM_ASTORE_N_INSTRUCTION(2)
                    _JAVM_STORE_INSTRUCTION(3, ISTORE_3, int)
                    _JAVM_STORE_INSTRUCTION(3, LSTORE_3, long)
                    _JAVM_STORE_INSTRUCTION(3, FSTORE_3, float)
                    _JAVM_STORE_INSTRUCTION(3, DSTORE_3, double)
                    _JAVM_ASTORE_N_INSTRUCTION(3)
                    _JAVM_ASTORE_INSTRUCTION(IASTORE)
                    _JAVM_ASTORE_INSTRUCTION(LASTORE)
                    _JAVM_ASTORE_INSTRUCTION(FASTORE)
                    _JAVM_ASTORE_INSTRUCTION(DASTORE)
                    _JAVM_ASTORE_INSTRUCTION(AASTORE)
                    _JAVM_ASTORE_INSTRUCTION(BASTORE)
                    _JAVM_ASTORE_INSTRUCTION(CASTORE)
                    _JAVM_ASTORE_INSTRUCTION(SASTORE)
                    case Instruction::POP: {
                        frame.Pop();
                        break;
                    }
                    case Instruction::POP2: {
                        frame.Pop();
                        frame.Pop();
                        break;
                    }
                    case Instruction::DUP: {
                        auto val = frame.Pop();
                        frame.Push(val);
                        frame.Push(val);
                        break;
                    }
                    case Instruction::DUP_X1: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        frame.Push(v1);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    case Instruction::DUP_X2: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        auto v3 = frame.Pop();
                        frame.Push(v1);
                        frame.Push(v3);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    case Instruction::DUP2: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        frame.Push(v2);
                        frame.Push(v1);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    case Instruction::DUP2_X1: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        auto v3 = frame.Pop();
                        frame.Push(v2);
                        frame.Push(v1);
                        frame.Push(v3);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    case Instruction::DUP2_X2: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        auto v3 = frame.Pop();
                        auto v4 = frame.Pop();
                        frame.Push(v2);
                        frame.Push(v1);
                        frame.Push(v4);
                        frame.Push(v3);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    _JAVM_OPERATOR_INSTRUCTION(IADD, int, +)
                    _JAVM_OPERATOR_INSTRUCTION(LADD, long, +)
                    _JAVM_OPERATOR_INSTRUCTION(FADD, float, +)
                    _JAVM_OPERATOR_INSTRUCTION(DADD, double, +)
                    _JAVM_OPERATOR_INSTRUCTION(ISUB, int, -)
                    _JAVM_OPERATOR_INSTRUCTION(LSUB, long, -)
                    _JAVM_OPERATOR_INSTRUCTION(FSUB, float, -)
                    _JAVM_OPERATOR_INSTRUCTION(DSUB, double, -)
                    _JAVM_OPERATOR_INSTRUCTION(IMUL, int, *)
                    _JAVM_OPERATOR_INSTRUCTION(LMUL, long, *)
                    _JAVM_OPERATOR_INSTRUCTION(FMUL, float, *)
                    _JAVM_OPERATOR_INSTRUCTION(DMUL, double, *)
                    _JAVM_OPERATOR_INSTRUCTION(IDIV, int, /)
                    _JAVM_OPERATOR_INSTRUCTION(LDIV, long, /)
                    _JAVM_OPERATOR_INSTRUCTION(FDIV, float, /)
                    _JAVM_OPERATOR_INSTRUCTION(DDIV, double, /)
                    _JAVM_OPERATOR_INSTRUCTION(IREM, int, %)
                    _JAVM_OPERATOR_INSTRUCTION(LREM, long, %)
                    // Fuck u, decimals
                    case Instruction::FREM: {
                        float v2 = frame.PopValue<float>();
                        float v1 = frame.PopValue<float>();
                        frame.CreatePush<float>(fmodf(v1, v2));
                        break;
                    }
                    case Instruction::DREM: {
                        double v2 = frame.PopValue<double>();
                        double v1 = frame.PopValue<double>();
                        frame.CreatePush<double>(fmod(v1, v2));
                        break;
                    }
                    _JAVM_NEG_INSTRUCTION(INEG, int)
                    _JAVM_NEG_INSTRUCTION(LNEG, long)
                    _JAVM_NEG_INSTRUCTION(FNEG, float)
                    _JAVM_NEG_INSTRUCTION(DNEG, double)
                    case Instruction::ISHL: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        auto tmp = v2 & 0x1F;
                        frame.CreatePush<int>(v1 << tmp);
                        break;
                    }
                    case Instruction::LSHL: {
                        int v2 = frame.PopValue<int>();
                        long v1 = frame.PopValue<long>();
                        auto tmp = v2 & 0x3F;
                        frame.CreatePush<long>(v1 << tmp);
                        break;
                    }
                    case Instruction::ISHR: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        auto tmp = v2 & 0x1F;
                        frame.CreatePush<int>(v1 >> tmp);
                        break;
                    }
                    case Instruction::LSHR: {
                        int v2 = frame.PopValue<int>();
                        long v1 = frame.PopValue<long>();
                        auto tmp = v2 & 0x3F;
                        frame.CreatePush<long>(v1 >> tmp);
                        break;
                    }
                    case Instruction::IUSHR: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        auto tmp = v2 & 0x1F;
                        if(v1 >= 0) {
                            frame.CreatePush<int>(v1 >> tmp);
                        }
                        else {
                            frame.CreatePush<int>((v1 >> tmp) + (2 << ~tmp));
                        }
                        break;
                    }
                    case Instruction::LUSHR: {
                        int v2 = frame.PopValue<int>();
                        long v1 = frame.PopValue<long>();
                        auto tmp = v2 & 0x3F;
                        if(v1 >= 0) {
                            frame.CreatePush<long>(v1 >> tmp);
                        }
                        else {
                            frame.CreatePush<long>((v1 >> tmp) + (2 << ~tmp));
                        }
                        break;
                    }
                    case Instruction::IAND: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        frame.CreatePush<int>(v1 & v2);
                        break;
                    }
                    case Instruction::LAND: {
                        long v2 = frame.PopValue<long>();
                        long v1 = frame.PopValue<long>();
                        frame.CreatePush<long>(v1 & v2);
                        break;
                    }
                    case Instruction::IOR: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        frame.CreatePush<int>(v1 | v2);
                        break;
                    }
                    case Instruction::LOR: {
                        long v2 = frame.PopValue<long>();
                        long v1 = frame.PopValue<long>();
                        frame.CreatePush<long>(v1 | v2);
                        break;
                    }
                    case Instruction::IXOR: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        frame.CreatePush<int>(v1 ^ v2);
                        break;
                    }
                    case Instruction::LXOR: {
                        long v2 = frame.PopValue<long>();
                        long v1 = frame.PopValue<long>();
                        frame.CreatePush<long>(v1 ^ v2);
                        break;
                    }
                    case Instruction::IINC: {
                        u8 idx = frame.Read<u8>();
                        i8 cnst = frame.Read<i8>();
                        auto value = frame.GetLocalValue<int>((u32)idx);
                        value += cnst;
                        frame.CreateSetLocal<int>((u32)idx, value);
                        break;
                    }
                    _JAVM_CONVERSION_INSTRUCTION(I2L, int, long)
                    _JAVM_CONVERSION_INSTRUCTION(I2F, int, float)
                    _JAVM_CONVERSION_INSTRUCTION(I2D, int, double)
                    _JAVM_CONVERSION_INSTRUCTION(L2I, long, int)
                    _JAVM_CONVERSION_INSTRUCTION(L2F, long, float)
                    _JAVM_CONVERSION_INSTRUCTION(L2D, long, double)
                    _JAVM_CONVERSION_INSTRUCTION(F2I, float, int)
                    _JAVM_CONVERSION_INSTRUCTION(F2L, float, long)
                    _JAVM_CONVERSION_INSTRUCTION(F2D, float, double)
                    _JAVM_CONVERSION_INSTRUCTION(D2I, double, int)
                    _JAVM_CONVERSION_INSTRUCTION(D2L, double, long)
                    _JAVM_CONVERSION_INSTRUCTION(D2F, double, float)
                    _JAVM_CONVERSION_INSTRUCTION(I2B, int, u8)
                    _JAVM_CONVERSION_INSTRUCTION(I2C, int, char)
                    _JAVM_CONVERSION_INSTRUCTION(I2S, int, short)
                    _JAVM_CMP_INSTRUCTION(LCMP, long)
                    _JAVM_CMP_INSTRUCTION(FCMPL, float)
                    _JAVM_CMP_INSTRUCTION(FCMPG, float)
                    _JAVM_CMP_INSTRUCTION(DCMPL, double)
                    _JAVM_CMP_INSTRUCTION(DCMPG, double)

                    case Instruction::IFEQ: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v == 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFNE: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v != 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFLT: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v < 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFGE: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v >= 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFGT: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v > 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFLE: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v <= 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPEQ: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 == v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPNE: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 != v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPLT: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 < v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPGE: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 >= v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPGT: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 > v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPLE: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 <= v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ACMPEQ: {
                        auto v2 = frame.Pop();
                        auto v1 = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if((v1.GetAddress() == v2.GetAddress()) && (v1.GetTypeHashCode() == v2.GetTypeHashCode()) && (v1.GetValueType() == v2.GetValueType())) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ACMPNE: {
                        auto v2 = frame.Pop();
                        auto v1 = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if((v1.GetAddress() != v2.GetAddress()) || (v1.GetTypeHashCode() != v2.GetTypeHashCode()) || (v1.GetValueType() != v2.GetValueType())) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::GOTO: {
                        i16 branch = BE(frame.Read<i16>());
                        offset -= 3;
                        offset += branch;
                        break;
                    }
                    case Instruction::JSR:
                    case Instruction::RET: {
                        // Old instructions, not implementing them
                        break;
                    }
                    // TODO: TABLESWITCH, LOOKUPSWITCH
                    case Instruction::IRETURN: {
                        int ret = frame.PopValue<int>();
                        should_ret = true;
                        return ValuePointerHolder::Create<int>(ret);
                    }
                    case Instruction::LRETURN: {
                        long ret = frame.PopValue<long>();
                        should_ret = true;
                        return ValuePointerHolder::Create<long>(ret);
                    }
                    case Instruction::ARETURN: {
                        should_ret = true;
                        return frame.Pop();
                    }
                    case Instruction::DRETURN: {
                        double ret = frame.PopValue<double>();
                        should_ret = true;
                        return ValuePointerHolder::Create<double>(ret);
                    }
                    case Instruction::RETURN: {
                        should_ret = true;
                        // Return nothing - a void value
                        return ValuePointerHolder::CreateVoid();    
                    }
                    case Instruction::GETSTATIC: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        bool done = false;
                        for(auto &native: this->native_classes) {
                            if(native->GetName() == class_name) {
                                // Call static block in case it hasn't been called yet :P
                                native->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, "()V", frame);
                                auto obj = native->GetField(fld_nat_data.processed_name);
                                frame.Push(obj);
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &archive: this->loaded_archives) {
                            for(auto &class_file: archive->GetLoadedClasses()) {
                                if(class_file->GetName() == class_name) {
                                    // Call static block in case it hasn't been called yet :P
                                    class_file->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, "()V", frame);
                                    auto obj = class_file->GetField(fld_nat_data.processed_name);
                                    frame.Push(obj);
                                    done = true;
                                }
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &class_file: this->class_files) {
                            if(class_file->GetName() == class_name) {
                                // Call static block in case it hasn't been called yet :P
                                class_file->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, "()V", frame);
                                auto obj = class_file->GetField(fld_nat_data.processed_name);
                                frame.Push(obj);
                            }
                        }
                        break;
                    }
                    case Instruction::PUTSTATIC: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        bool done = false;
                        for(auto &native: this->native_classes) {
                            if(native->GetName() == class_name) {
                                // Call static block in case it hasn't been called yet :P
                                native->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, "()V", frame);
                                auto obj = frame.Pop();
                                native->SetField(fld_nat_data.processed_name, obj);
                                done = true;
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &archive: this->loaded_archives) {
                            for(auto &class_file: archive->GetLoadedClasses()) {
                                if(class_file->GetName() == class_name) {
                                    // Call static block in case it hasn't been called yet :P
                                    class_file->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, "()V", frame);
                                    auto obj = frame.Pop();
                                    class_file->SetField(fld_nat_data.processed_name, obj);
                                    done = true;
                                }
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &class_file: this->class_files) {
                            if(class_file->GetName() == class_name) {
                                // Call static block in case it hasn't been called yet :P
                                class_file->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, "()V", frame);
                                auto obj = frame.Pop();
                                class_file->SetField(fld_nat_data.processed_name, obj);
                            }
                        }
                        break;
                    }
                    case Instruction::GETFIELD: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        
                        auto clss = frame.PopReference<ClassObject>();
                        auto obj = clss->GetField(fld_nat_data.processed_name);
                        frame.Push(obj);
                        break;
                    }
                    case Instruction::PUTFIELD: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        
                        auto obj = frame.Pop();
                        auto clss = frame.PopReference<ClassObject>();
                        clss->SetField(fld_nat_data.processed_name, obj);
                        break;
                    }
                    case Instruction::INVOKEVIRTUAL: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fn_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        bool done = false;
                        for(auto &native: this->native_classes) {
                            if(native->GetName() == class_name) {
                                if(native->CanHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc)) {
                                    bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                    auto ret = native->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                    if(should_ret) {
                                        frame.Push(ret);
                                    }
                                }
                                else {
                                    frame.Pop();
                                }
                                done = true;
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &archive: this->loaded_archives) {
                            for(auto &class_file: archive->GetLoadedClasses()) {
                                if(class_file->GetName() == class_name) {
                                    if(class_file->CanHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc)) {
                                        bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                        auto ret = class_file->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                        if(should_ret) {
                                            frame.Push(ret);
                                        }
                                    }
                                    else {
                                        frame.Pop();
                                    }
                                    done = true;
                                }
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &class_file: this->class_files) {
                            if(class_file->GetName() == class_name) {
                                if(class_file->CanHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc)) {
                                    bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                    auto ret = class_file->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                    if(should_ret) {
                                        frame.Push(ret);
                                    }
                                }
                                else {
                                    frame.Pop();
                                }
                                done = true;
                            }
                        }
                        if(!done) {
                            for(u32 i = 0; i < ClassObject::GetFunctionParameterCount(fn_nat_data.processed_desc); i++) {
                                frame.Pop();
                            }
                            frame.Pop();
                        }
                        break;
                    }
                    case Instruction::INVOKESPECIAL: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fn_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        bool done = false;
                        for(auto &native: this->native_classes) {
                            if(native->GetName() == class_name) {
                                if(native->CanHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc)) {
                                    native->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                }
                                else {
                                    frame.Pop();
                                }
                                done = true;
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &archive: this->loaded_archives) {
                            for(auto &class_file: archive->GetLoadedClasses()) {
                                if(class_file->GetName() == class_name) {
                                    if(class_file->CanHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc)) {
                                        class_file->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                    }
                                    else {
                                        frame.Pop();
                                    }
                                    done = true;
                                }
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &class_file: this->class_files) {
                            if(class_file->GetName() == class_name) {
                                if(class_file->CanHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc)) {
                                    class_file->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                }
                                else {
                                    frame.Pop();
                                }
                            }
                        }
                        break;
                    }
                    case Instruction::INVOKESTATIC: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fn_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        bool done = false;
                        for(auto &native: this->native_classes) {
                            if(native->GetName() == class_name) {
                                bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                auto ret = native->HandleStaticFunction(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                if(ret.IsVoid() && should_ret) {
                                    // Throw error - should return and isn't returning...?
                                }
                                frame.Push(std::move(ret));
                                done = true;
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &archive: this->loaded_archives) {
                            for(auto &class_file: archive->GetLoadedClasses()) {
                                if(class_file->GetName() == class_name) {
                                    bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                    auto ret = class_file->HandleStaticFunction(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                    if(should_ret) {
                                        frame.Push(ret);
                                    }
                                    done = true;
                                }
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &class_file: this->class_files) {
                            if(class_file->GetName() == class_name) {
                                bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                auto ret = class_file->HandleStaticFunction(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                if(should_ret) {
                                    frame.Push(ret);
                                }
                            }
                        }
                        break;
                    }
                    // TODO: INVOKEINTERFACE, INVOKEDYNAMIC
                    case Instruction::NEW: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto class_name = constant_pool[index - 1].GetClassData().processed_name;
                        bool done = false;
                        for(auto &native: this->native_classes) {
                            if(native->GetName() == class_name) {
                                auto obj = native->CreateInstance();
                                frame.Push(obj);
                                done = true;
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &archive: this->loaded_archives) {
                            for(auto &class_file: archive->GetLoadedClasses()) {
                                if(class_file->GetName() == class_name) {
                                    auto obj = class_file->CreateInstance();
                                    frame.Push(obj);
                                    done = true;
                                }
                            }
                        }
                        if(done) {
                            break;
                        }
                        for(auto &class_file: this->class_files) {
                            if(class_file->GetName() == class_name) {
                                auto obj = class_file->CreateInstance();
                                frame.Push(obj);
                            }
                        }

                        break;
                    }
                    case Instruction::NEWARRAY: {
                        u8 type = BE(frame.Read<u8>());

                        int len = frame.PopValue<int>();
                        auto arr_holder = ValuePointerHolder::Create<Array>();
                        auto arr_ref = arr_holder.GetReference<Array>();
                        for(int i = 0; i < len; i++) {
                            arr_ref->emplace_back(); // Push empty (null) holders
                        }
                        frame.Push(arr_holder);
                        break;
                    }
                    case Instruction::ANEWARRAY: {
                        u16 idx = BE(frame.Read<u16>());

                        int len = frame.PopValue<int>();
                        auto arr_holder = ValuePointerHolder::Create<Array>();
                        auto arr_ref = arr_holder.GetReference<Array>();
                        for(int i = 0; i < len; i++) {
                            arr_ref->emplace_back(); // Push empty (null) holders
                        }
                        frame.Push(arr_holder);
                        break;
                    }
                    case Instruction::ARRAYLENGTH: {
                        auto arr = frame.PopReference<Array>();
                        frame.CreatePush<int>((int)arr->size());
                        break;
                    }
                    // TODO: ATHROW
                    case Instruction::CHECKCAST:
                    case Instruction::INSTANCEOF: {
                        u16 index = BE(frame.Read<u16>());
                        auto obj = frame.PopReference<ClassObject>();
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto class_name = constant_pool[index - 1].GetClassData().processed_name;
                        if(obj->GetName() == class_name) {
                            frame.CreatePush<int>(1);
                        }
                        else {
                            frame.CreatePush<int>(0);
                        }
                        break;
                    }
                    case Instruction::IFNULL: {
                        auto holder = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if(holder.IsNull()) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFNONNULL: {
                        auto holder = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if(!holder.IsNull()) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::GOTO_W:
                    case Instruction::JSR_W: {
                        // Old instructions, not implementing them
                        break;
                    }
                    
                    default:
                        printf("Unimplemented or unknown instruction!\n");
                        break;
                }

                return ValuePointerHolder::CreateVoid();
            }

        public:
            // Simplify getting the machine pointer from a frame
            static Machine *GetFrameMachinePointer(Frame &frame) {
                return reinterpret_cast<Machine*>(frame.GetMachinePointer());
            }

            template<typename ...Args>
            std::unique_ptr<ClassFile> &LoadClassFile(Args &&...args) {
                auto classptr = std::make_unique<ClassFile>(args...);
                this->class_files.push_back(std::move(classptr));
                return this->class_files.back();
            }

            template<typename C, typename ...Args>
            void LoadNativeClass(Args &&...args) {
                static_assert(std::is_base_of_v<native::Class, C>, "Native classes must inherit from javm::native::Class (or better, from java::lang::Object)");

                std::unique_ptr<native::Class> classptr = std::make_unique<C>(args...);
                this->native_classes.push_back(std::move(classptr));
            }

            void LoadBuiltinNativeClasses() {
                this->LoadNativeClass<java::lang::Object>();
                this->LoadNativeClass<java::lang::String>();
                this->LoadNativeClass<java::lang::StringBuilder>();
            }

            template<typename ...Args>
            std::unique_ptr<Archive> &LoadJavaArchive(Args &&...args) {
                auto archiveptr = std::make_unique<Archive>(args...);
                this->loaded_archives.push_back(std::move(archiveptr));
                return this->loaded_archives.back();
            }

            ValuePointerHolder ExecuteCode(Frame &frame) {
                while(true) {
                    auto inst = static_cast<Instruction>(frame.Read<u8>());
                    if(inst == Instruction::NOP) {
                        continue;
                    }

                    if(frame.StackMaximum()) {
                        // (...)
                        printf("Max stack!\n");
                        break;
                    }

                    bool should_ret = false;
                    auto ret = this->HandleInstruction(frame, should_ret, inst);
                    if(should_ret) {
                        return ret;
                    }
                }
                return ValuePointerHolder::CreateVoid();
            }

            template<typename ...Args>
            ValuePointerHolder CallFunction(std::string class_name, std::string fn_name, Args &&...args) {
                auto proper_class_name = ClassObject::ProcessClassName(class_name);
                // Native classes have preference
                for(auto &native: this->native_classes) {
                    if(native->GetName() == proper_class_name) {
                        Frame frame(native.get(), reinterpret_cast<void*>(this));
                        return native->HandleStaticFunction(fn_name, "", frame);
                    }
                }
                // Then, loaded JARs
                for(auto &archive: this->loaded_archives) {
                    for(auto &class_file: archive->GetLoadedClasses()) {
                        if(class_file->GetName() == proper_class_name) {
                            for(auto &method: class_file->GetMethods()) {
                                if(method.GetName() == fn_name) {
                                    for(auto &attr: method.GetAttributes()) {
                                        if(attr.GetName() == JAVM_CODE_ATTRIBUTE_NAME) {
                                            MemoryReader code_reader(attr.GetInfo(), attr.GetInfoLength());
                                            CodeAttribute code_attr(code_reader);
                                            Frame frame(&code_attr, class_file.get(), reinterpret_cast<void*>(this));
                                            u32 index = 0;
                                            ((this->HandlePushArgument(frame, index, args)), ...);
                                            auto ret = this->ExecuteCode(frame);
                                            code_attr.Dispose();
                                            return ret;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                // Finally, plain .class files
                for(auto &class_file: this->class_files) {
                    if(class_file->GetName() == proper_class_name) {
                        for(auto &method: class_file->GetMethods()) {
                            if(method.GetName() == fn_name) {
                                for(auto &attr: method.GetAttributes()) {
                                    if(attr.GetName() == JAVM_CODE_ATTRIBUTE_NAME) {
                                        MemoryReader code_reader(attr.GetInfo(), attr.GetInfoLength());
                                        CodeAttribute code_attr(code_reader);
                                        Frame frame(&code_attr, class_file.get(), reinterpret_cast<void*>(this));
                                        u32 index = 0;
                                        ((this->HandlePushArgument(frame, index, args)), ...);
                                        auto ret = this->ExecuteCode(frame);
                                        code_attr.Dispose();
                                        return ret;
                                    }
                                }
                            }
                        }
                    }
                }
                return ValuePointerHolder::CreateVoid();
            }

    };

    ValuePointerHolder HandleClassFileMethod(ClassFile *class_file, std::string name, std::string desc, Frame &frame) {
        for(auto &method: class_file->GetMethods()) {
            if(method.GetName() == name) {
                if(method.GetDesc() == desc) {
                    for(auto &attr: method.GetAttributes()) {
                        if(attr.GetName() == JAVM_CODE_ATTRIBUTE_NAME) {
                            MemoryReader code_reader(attr.GetInfo(), attr.GetInfoLength());
                            CodeAttribute code_attr(code_reader);
                            Frame new_frame(&code_attr, class_file, frame.GetMachinePointer());
                            for(u32 i = ClassObject::GetFunctionParameterCount(desc); i > 0; i--) {
                                new_frame.SetLocal(i, frame.Pop());
                            }
                            new_frame.SetLocal(0, frame.Pop());
                            auto ret = Machine::GetFrameMachinePointer(frame)->ExecuteCode(new_frame);
                            code_attr.Dispose();
                            if(ClassObject::ExpectsReturn(desc)) {
                                return ret;
                            }
                        }
                    }
                }
            }
        }
        return ValuePointerHolder::CreateNull();
    }

    ValuePointerHolder HandleClassFileStaticFunction(ClassFile *class_file, std::string name, std::string desc, Frame &frame) {
        for(auto &method: class_file->GetMethods()) {
            if(method.GetName() == name) {
                if(method.GetDesc() == desc) {
                    for(auto &attr: method.GetAttributes()) {
                        if(attr.GetName() == JAVM_CODE_ATTRIBUTE_NAME) {
                            MemoryReader code_reader(attr.GetInfo(), attr.GetInfoLength());
                            CodeAttribute code_attr(code_reader);
                            Frame new_frame(&code_attr, class_file, frame.GetMachinePointer());
                            for(u32 i = 0; i < ClassObject::GetFunctionParameterCount(desc); i++) {
                                new_frame.SetLocal(i, frame.Pop());
                            }
                            auto ret = Machine::GetFrameMachinePointer(frame)->ExecuteCode(new_frame);
                            code_attr.Dispose();
                            if(ClassObject::ExpectsReturn(desc)) {
                                return ret;
                            }
                        }
                    }
                }
            }
        }
        return ValuePointerHolder::CreateNull();
    }

}