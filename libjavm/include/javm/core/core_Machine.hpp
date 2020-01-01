
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

    struct ThrownInfo {

        bool info_valid;
        std::string class_type;
        std::string message;

        // TODO: more fields

    };

    class Machine {

        private:
            std::shared_ptr<ClassObject> empty_class;
            std::vector<std::shared_ptr<Archive>> loaded_archives;
            std::vector<std::shared_ptr<ClassObject>> class_files;
            std::vector<std::shared_ptr<ClassObject>> native_classes;
            ThrownInfo thrown_info;

            void ThrowClassNotFound(std::string class_name) {
                this->ThrowExceptionWithType("java.lang.NoClassDefFoundError", ClassObject::ProcessClassName(class_name));
            }

            void ThrowRuntimeException(std::string message) {
                this->ThrowExceptionWithType("java.lang.RuntimeException", message);
            }

            template<typename Arg>
            void HandlePushArgument(Frame &frame, u32 &index, Arg &arg) {
                static_assert(std::is_pointer_v<Arg> || std::is_same_v<Arg, Value>, "Arguments must be pointers to variables or core::Value");
                if constexpr(std::is_pointer_v<Arg>) {
                    frame.SetLocalReference(index, arg);
                }
                else if constexpr(std::is_same_v<Arg, Value>) {
                    frame.SetLocal(index, arg);
                }
                index++;
            }

            Value HandleInstruction(Frame &frame, bool &should_ret, Instruction inst) {
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
                                auto str_obj = CreateNewValue<java::lang::String>(); \
                                auto str_ref = str_obj->GetReference<java::lang::String>(); \
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

                // printf(" -- Read instruction: 0x%X\n", (u32)inst);
                switch(inst) {
                    case Instruction::NOP: {
                        // Do nothing :P
                        break;
                    }
                    case Instruction::ACONST_NULL: {
                        frame.Push(CreateNullValue());
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
                        if((v1->GetAddress() == v2->GetAddress()) && (v1->GetTypeHashCode() == v2->GetTypeHashCode()) && (v1->GetValueType() == v2->GetValueType())) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ACMPNE: {
                        auto v2 = frame.Pop();
                        auto v1 = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if((v1->GetAddress() != v2->GetAddress()) || (v1->GetTypeHashCode() != v2->GetTypeHashCode()) || (v1->GetValueType() != v2->GetValueType())) {
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
                        return CreateNewValue<int>(ret);
                    }
                    case Instruction::LRETURN: {
                        long ret = frame.PopValue<long>();
                        should_ret = true;
                        return CreateNewValue<long>(ret);
                    }
                    case Instruction::ARETURN: {
                        should_ret = true;
                        return frame.Pop();
                    }
                    case Instruction::DRETURN: {
                        double ret = frame.PopValue<double>();
                        should_ret = true;
                        return CreateNewValue<double>(ret);
                    }
                    case Instruction::RETURN: {
                        should_ret = true;
                        // Return nothing - a void value
                        return CreateVoidValue();    
                    }
                    case Instruction::GETSTATIC: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        
                        if(this->HasClass(class_name)) {
                            auto &class_ref = this->FindClass(class_name);
                            if(class_ref->CanHandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, JAVM_EMPTY_METHOD_DESCRIPTOR, frame)) {
                                class_ref->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, JAVM_EMPTY_METHOD_DESCRIPTOR, frame);
                            }
                            auto obj = class_ref->GetStaticField(fld_nat_data.processed_name);
                            if(obj->IsVoid()) {
                                this->ThrowRuntimeException("Invalid static field - " + fld_nat_data.processed_name);
                            }
                            else {
                                frame.Push(obj);
                            }
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::PUTSTATIC: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        
                        
                        if(this->HasClass(class_name)) {
                            auto &class_ref = this->FindClass(class_name);
                            if(class_ref->CanHandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, JAVM_EMPTY_METHOD_DESCRIPTOR, frame)) {
                                class_ref->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, JAVM_EMPTY_METHOD_DESCRIPTOR, frame);
                            }
                            auto obj = frame.Pop();
                            class_ref->SetStaticField(fld_nat_data.processed_name, obj);
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
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
                        if(obj->IsVoid()) {
                            this->ThrowRuntimeException("Invalid field - " + fld_nat_data.processed_name);
                        }
                        else {
                            frame.Push(obj);
                        }
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

                        if(this->HasClass(class_name)) {
                            auto &class_ref = this->FindClass(class_name);
                            if(class_ref->CanAllHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame)) {
                                bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                auto ret = class_ref->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                if(should_ret) {
                                    if(ret->IsVoid()) {
                                        this->ThrowRuntimeException("Invalid method return - " + fn_nat_data.processed_name);
                                    }
                                    else {
                                        frame.Push(ret);
                                    }
                                }
                            }
                            else {
                                this->ThrowRuntimeException("Invalid method - " + fn_nat_data.processed_name);
                            }
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::INVOKESPECIAL: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fn_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();

                        if(this->HasClass(class_name)) {
                            auto &class_ref = this->FindClass(class_name);
                            if(class_ref->CanAllHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame)) {
                                class_ref->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                            }
                            else {
                                this->ThrowRuntimeException("Invalid method - " + fn_nat_data.processed_name);
                            }
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::INVOKESTATIC: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fn_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();

                        if(this->HasClass(class_name)) {
                            auto &class_ref = this->FindClass(class_name);
                            bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                            if(class_ref->CanAllHandleStaticFunction(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame)) {
                                auto ret = class_ref->HandleStaticFunction(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame);
                                if(should_ret) {
                                    if(ret->IsVoid()) {
                                        this->ThrowRuntimeException("Invalid static function return - " + fn_nat_data.processed_name);
                                    }
                                    else {
                                        frame.Push(ret);
                                    }
                                }
                            }
                            else {
                                this->ThrowRuntimeException("Invalid static function - " + fn_nat_data.processed_name);
                            }
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    // TODO: INVOKEINTERFACE, INVOKEDYNAMIC
                    case Instruction::NEW: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto class_name = constant_pool[index - 1].GetClassData().processed_name;

                        if(this->HasClass(class_name)) {
                            auto &class_ref = this->FindClass(class_name);
                            auto obj = class_ref->CreateInstance(frame);
                            frame.Push(obj);
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::NEWARRAY: {
                        u8 type = BE(frame.Read<u8>());

                        int len = frame.PopValue<int>();
                        auto arr_holder = CreateNewValue<Array>();
                        auto arr_ref = arr_holder->GetReference<Array>();
                        for(int i = 0; i < len; i++) {
                            arr_ref->emplace_back(); // Push empty (null) holders
                        }
                        frame.Push(arr_holder);
                        break;
                    }
                    case Instruction::ANEWARRAY: {
                        u16 idx = BE(frame.Read<u16>());

                        int len = frame.PopValue<int>();
                        auto arr_holder = CreateNewValue<Array>();
                        auto arr_ref = arr_holder->GetReference<Array>();
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
                    case Instruction::ATHROW: {
                        this->ThrowExceptionWithInstance(frame, frame.Pop());
                        should_ret = true;
                        break;
                    }
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
                        if(holder->IsNull()) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFNONNULL: {
                        auto holder = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if(!holder->IsNull()) {
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

                return CreateVoidValue();
            }

        public:
            Machine() {
                this->Reset();
            }

            void Reset() {
                this->loaded_archives.clear();
                this->class_files.clear();
                this->native_classes.clear();
                this->thrown_info = {};
                this->thrown_info.info_valid = false;
            }

            ThrownInfo GetExceptionInfo() {
                return this->thrown_info;
            }

            bool WasExceptionThrown() {
                return this->thrown_info.info_valid;
            }

            void ThrowExceptionWithInstance(Frame &frame, Value holder) {
                auto obj = holder->GetReference<ClassObject>();
                auto str = obj->CallMethod(frame, "getMessage"); // Throwable will be a super class, so calls Throwable.getMessage()
                if(str->IsVoid()) {
                    this->ThrowRuntimeException("Invalid exception type");
                }
                else {
                    this->thrown_info = {};
                    this->thrown_info.info_valid = true;
                    this->thrown_info.class_type = ClassObject::GetPresentableClassName(obj->GetName());
                    this->thrown_info.message = "<null message>";
                    if(str->IsValidCast<java::lang::String>()) {
                        auto str_ref = str->GetReference<java::lang::String>();
                        this->thrown_info.message = str_ref->GetString();
                    }
                }
            }

            void ThrowExceptionWithType(std::string class_name, std::string message) {
                if(this->HasClass(class_name)) {
                    auto &err_class = this->FindClass(class_name);
                    Frame frame(reinterpret_cast<void*>(this));
                    auto err_instance = err_class->CreateInstance(frame);
                    auto err_ref = err_instance->GetReference<java::lang::Throwable>();
                    err_ref->SetMessage(message);
                    
                    this->ThrowExceptionWithInstance(frame, err_instance);
                }
                else {
                    this->ThrowClassNotFound(class_name);
                }
            }

            void ThrowExceptionWithMessage(std::string message) {
                this->ThrowExceptionWithType("java.lang.Exception", message);
            }

            // Simplify getting the machine pointer from a frame
            static Machine *GetFrameMachinePointer(Frame &frame) {
                return reinterpret_cast<Machine*>(frame.GetMachinePointer());
            }

            template<typename ...Args>
            std::shared_ptr<ClassFile> LoadClassFile(Args &&...args) {
                std::shared_ptr<ClassObject> classptr = ClassFile::CreateDefinitionInstance(reinterpret_cast<void*>(this), args...);
                this->class_files.push_back(std::move(classptr));
                return std::dynamic_pointer_cast<ClassFile>(this->class_files.back());
            }

            template<typename C, typename ...Args>
            void LoadNativeClass(Args &&...args) {
                static_assert(std::is_same_v<java::lang::Object, C> || std::is_base_of_v<java::lang::Object, C>, "Native classes must be or inherit from from java::lang::Object");

                std::shared_ptr<native::Class> classptr = C::CreateDefinitionInstance(reinterpret_cast<void*>(this));
                this->native_classes.push_back(std::move(classptr));
            }

            void LoadBuiltinNativeClasses() {
                this->LoadNativeClass<java::lang::Object>();
                this->LoadNativeClass<java::lang::String>();
                this->LoadNativeClass<java::lang::StringBuilder>();
                this->LoadNativeClass<java::io::PrintStream>();
                this->LoadNativeClass<java::lang::System>();
                this->LoadNativeClass<java::lang::Throwable>();
                this->LoadNativeClass<java::lang::Exception>();
                this->LoadNativeClass<java::lang::RuntimeException>();
                this->LoadNativeClass<java::lang::Error>();
                this->LoadNativeClass<java::lang::LinkageError>();
                this->LoadNativeClass<java::lang::NoClassDefFoundError>();
            }

            template<typename ...Args>
            std::shared_ptr<Archive> &LoadJavaArchive(Args &&...args) {
                auto archiveptr = std::make_shared<Archive>(args...);
                this->loaded_archives.push_back(std::move(archiveptr));
                return this->loaded_archives.back();
            }

            bool HasClass(std::string name) {
                auto cls_name = ClassObject::ProcessClassName(name);
                for(auto &native: this->native_classes) {
                    if(native->GetName() == cls_name) {
                        return true;
                    }
                }
                // Then, loaded JARs
                for(auto &archive: this->loaded_archives) {
                    for(auto &class_file: archive->GetLoadedClasses()) {
                        if(class_file->GetName() == cls_name) {
                            return true;
                        }
                    }
                }
                // Finally, plain .class files
                for(auto &class_file: this->class_files) {
                    if(class_file->GetName() == cls_name) {
                        return true;
                    }
                }
                return false;
            }
            
            std::shared_ptr<ClassObject> &FindClass(std::string name) {
                auto cls_name = ClassObject::ProcessClassName(name);
                for(auto &native: this->native_classes) {
                    if(native->GetName() == cls_name) {
                        return native;
                    }
                }
                // Then, loaded JARs
                for(auto &archive: this->loaded_archives) {
                    for(auto &class_file: archive->GetLoadedClasses()) {
                        if(class_file->GetName() == cls_name) {
                            return class_file;
                        }
                    }
                }
                // Finally, plain .class files
                for(auto &class_file: this->class_files) {
                    if(class_file->GetName() == cls_name) {
                        return class_file;
                    }
                }
                return this->empty_class;
            }

            Value ExecuteCode(Frame &frame) {
                while(true) {
                    if(this->WasExceptionThrown()) {
                        return CreateVoidValue();
                    }

                    auto inst = static_cast<Instruction>(frame.Read<u8>());

                    if(frame.StackMaximum()) {
                        // (...)
                        // printf("Max stack!\n");
                        break;
                    }

                    bool should_ret = false;
                    auto ret = this->HandleInstruction(frame, should_ret, inst);
                    if(should_ret) {
                        return ret;
                    }
                }
                
                return CreateVoidValue();
            }

            template<typename ...Args>
            Value CallFunction(std::string class_name, std::string fn_name, Args &&...args) {
                if(this->WasExceptionThrown()) {
                    return CreateVoidValue();
                }
                
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
                    for(auto &class_ref: archive->GetLoadedClasses()) {
                        auto class_file = std::dynamic_pointer_cast<ClassFile>(class_ref);
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
                for(auto &class_ref: this->class_files) {
                    auto class_file = std::dynamic_pointer_cast<ClassFile>(class_ref);
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
                return CreateVoidValue();
            }
    };

    Value HandleClassFileMethod(ClassFile *class_file, std::string name, std::string desc, Frame &frame) {
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
        auto super_class = class_file->GetSuperClassInstance();
        if(super_class) {
            if(super_class->IsClassObject()) {
                auto super_class_ref = super_class->GetReference<ClassObject>();
                return super_class_ref->HandleMethod(name, desc, frame);
            }
        }
        return CreateNullValue();
    }

    Value HandleClassFileStaticFunction(ClassFile *class_file, std::string name, std::string desc, Frame &frame) {
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
        auto super_class = class_file->GetSuperClassInstance();
        if(super_class) {
            if(super_class->IsClassObject()) {
                auto super_class_ref = super_class->GetReference<ClassObject>();
                return super_class_ref->HandleStaticFunction(name, desc, frame);
            }
        }
        return CreateNullValue();
    }

    std::shared_ptr<ClassObject> &FindClassByNameEx(void *machine, std::string name) {
        auto mach = reinterpret_cast<Machine*>(machine);
        return mach->FindClass(name);
    }

    std::shared_ptr<ClassObject> &FindClassByName(Frame &frame, std::string name) {
        return FindClassByNameEx(frame.GetMachinePointer(), name);
    }

    void MachineThrowExceptionWithMessage(void *machine, std::string message) {
        return reinterpret_cast<Machine*>(machine)->ThrowExceptionWithMessage(message);
    }

    void MachineThrowExceptionWithType(void *machine, std::string class_name, std::string message) {
        return reinterpret_cast<Machine*>(machine)->ThrowExceptionWithType(class_name, message);
    }

    void MachineThrowExceptionWithInstance(void *machine, Value holder) {
        Frame frame(machine);
        return reinterpret_cast<Machine*>(machine)->ThrowExceptionWithInstance(frame, holder);
    }
}