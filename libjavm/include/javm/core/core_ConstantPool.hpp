
#pragma once
#include <javm/javm_Memory.hpp>

namespace javm::core {

    enum class CPTag {

        UTF8 = 1,
        Integer = 3,
        Float = 4,
        Long = 5,
        Double = 6,
        Class = 7,
        String = 8,
        FieldRef = 9,
        MethodRef = 10,
        InterfaceMethodRef = 11,
        NameAndType = 12

    };

    struct UTF8Data {
        u16 length;
        std::string str;
    };

    struct IntegerData {
        int integer;
    };

    struct FloatData {
        float flt;
    };
    
    struct LongData {
        long lng;
    };

    struct DoubleData {
        double dbl;
    };

    struct ClassData {
        u16 name_index;
        std::string processed_name;
    };

    struct StringData {
        u16 string_index;
        std::string processed_string;
    };

    struct FieldMethodRefData {
        u16 class_index;
        u16 name_and_type_index;
    };

    struct NameAndTypeData {
        u16 name_index;
        std::string processed_name;
        u16 desc_index;
        std::string processed_desc;
    };

    class CPInfo {

        private:
            CPTag tag;
            bool empty;
            
            UTF8Data utf8;
            IntegerData integer;
            FloatData flt;
            LongData lng;
            DoubleData dbl;
            ClassData clss;
            StringData string;
            FieldMethodRefData field_method;
            NameAndTypeData name_and_type;
            
        public:
            CPInfo() : empty(true) {}

            CPInfo(MemoryReader &reader) : empty(false) {
                this->tag = static_cast<CPTag>(reader.Read<u8>());
                switch(tag) {
                    case CPTag::UTF8: {
                        this->utf8.length = BE(reader.Read<u16>());
                        if(this->utf8.length > 0) {
                            char *strbuf = new char[this->utf8.length + 1]();
                            reader.ReadPointer(strbuf, this->utf8.length * sizeof(char));
                            this->utf8.str.assign(strbuf, this->utf8.length);
                            delete[] strbuf;
                        }
                        else {
                            this->utf8.str = "";
                        }
                        break;
                    }
                    case CPTag::Integer: {
                        this->integer.integer = BE(reader.Read<int>());
                        break;
                    }
                    case CPTag::Float: {
                        this->flt.flt = BE(reader.Read<float>());
                        break;
                    }
                    case CPTag::Long: {
                        this->lng.lng = BE(reader.Read<long>());
                        break;
                    }
                    case CPTag::Double: {
                        this->dbl.dbl = BE(reader.Read<double>());
                        break;
                    }
                    case CPTag::Class: {
                        this->clss.name_index = BE(reader.Read<u16>());
                        break;
                    }
                    case CPTag::String: {
                        this->string.string_index = BE(reader.Read<u16>());
                        break;
                    }
                    case CPTag::FieldRef:
                    case CPTag::MethodRef:
                    case CPTag::InterfaceMethodRef: {
                        this->field_method.class_index = BE(reader.Read<u16>());
                        this->field_method.name_and_type_index = BE(reader.Read<u16>());
                        break;
                    }
                    case CPTag::NameAndType: {
                        this->name_and_type.name_index = BE(reader.Read<u16>());
                        this->name_and_type.desc_index = BE(reader.Read<u16>());
                        break;
                    }
                    default:
                        // Should never happen, bad tag then
                        this->empty = true;
                        break;
                }
            }

            CPTag GetTag() {
                return this->tag;
            }

            UTF8Data &GetUTF8Data() {
                return this->utf8;
            }

            IntegerData &GetIntegerData() {
                return this->integer;
            }

            FloatData &GetFloatData() {
                return this->flt;
            }
            
            LongData &GetLongData() {
                return this->lng;
            }

            DoubleData &GetDoubleData() {
                return this->dbl;
            }

            ClassData &GetClassData() {
                return this->clss;
            }

            StringData &GetStringData() {
                return this->string;
            }

            FieldMethodRefData &GetFieldMethodRefData() {
                return this->field_method;
            }

            NameAndTypeData &GetNameAndTypeData() {
                return this->name_and_type;
            }


            bool IsEmpty() {
                return this->empty;
            }
    };

}