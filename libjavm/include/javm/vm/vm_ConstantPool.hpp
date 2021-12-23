
#pragma once
#include <javm/javm_Memory.hpp>
#include <javm/vm/vm_Base.hpp>
#include <functional>

namespace javm::vm {

    enum class ConstantPoolTag : u8 {
        Invalid = 0,
        Utf8 = 1,
        Integer = 3,
        Float = 4,
        Long = 5,
        Double = 6,
        Class = 7,
        String = 8,
        FieldRef = 9,
        MethodRef = 10,
        InterfaceMethodRef = 11,
        NameAndType = 12,
        InstanceMethodHandle = 15,
        InstanceMethodType = 16,
        InvokeDynamic = 18,

        Min = 1,
        Max = 18,
    };

    class ConstantNameItem {
        private:
            u16 name_idx;
            String name;

        public:
            inline u16 GetNameIndex() const {
                return this->name_idx;
            }

            inline void SetNameIndex(const u16 idx) {
                this->name_idx = idx;
            }

            inline String GetName() const {
                return this->name;
            }

            inline bool HasName() const {
                return !this->name.empty();
            }

            inline void SetName(const String &new_name) {
                this->name = new_name;
            }
    };

    class ConstantDescriptorItem {
        private:
            u16 desc_idx;
            String desc;

        public:
            inline u16 GetDescriptorIndex() {
                return this->desc_idx;
            }

            inline void SetDescriptorIndex(const u16 idx) {
                this->desc_idx = idx;
            }

            inline String GetDescriptor() {
                return this->desc;
            }

            inline bool HasDescriptor() {
                return !this->desc.empty();
            }

            inline void SetDescriptor(const String &new_desc) {
                this->desc = new_desc;
            }
    };

    class ConstantStringItem {
        private:
            u16 str_idx;
            String str;

        public:
            inline u16 GetStringIndex() {
                return this->str_idx;
            }

            inline void SetStringIndex(const u16 idx) {
                this->str_idx = idx;
            }

            inline String GetString() {
                return this->str;
            }

            inline bool HasString() {
                return !this->str.empty();
            }

            inline void SetString(const String &new_str) {
                this->str = new_str;
            }
    };

    class AccessFlagsItem {

        private:
            u16 access_flags;

        public:
            inline u16 GetAccessFlags() const {
                return this->access_flags;
            }

            inline void SetAccessFlags(const u16 flags) {
                this->access_flags = flags;
            }

            template<AccessFlags Flag>
            inline constexpr bool HasFlag() const {
                return this->access_flags & static_cast<u16>(Flag);
            }

    };

    struct Utf8Data {
        u16 length;
        std::string utf8_str;
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
        String processed_name;
    };

    struct StringData {
        u16 string_index;
        String processed_string;
    };

    struct FieldMethodRefData {
        u16 class_index;
        u16 name_and_type_index;
    };

    struct NameAndTypeData {
        u16 name_index;
        String processed_name;
        u16 desc_index;
        String processed_desc;
    };

    struct InstanceMethodHandleData {
        u8 ref_kind;
        u16 ref_index;
    };

    struct InstanceMethodTypeData {
        u16 desc_index;
        String processed_desc;
    };

    struct InvokeDynamicData {
        u16 bootstrap_method_attr_index;
        u16 name_and_type_index;
    };

    class ConstantPoolItem {
        private:
            ConstantPoolTag tag;
            bool empty;
            
            Utf8Data utf8;
            IntegerData integer;
            FloatData flt;
            LongData lng;
            DoubleData dbl;
            ClassData clss;
            StringData string;
            FieldMethodRefData field_method;
            NameAndTypeData name_and_type;
            InstanceMethodHandleData method_handle;
            InstanceMethodTypeData method_type;
            InvokeDynamicData invoke_dynamic;
            
        public:
            ConstantPoolItem() : tag(ConstantPoolTag::Invalid), empty(true) {}

            ConstantPoolItem(MemoryReader &reader) : tag(ConstantPoolTag::Invalid), empty(true) {
                this->tag = static_cast<ConstantPoolTag>(reader.Read<u8>());
                switch(this->tag) {
                    case ConstantPoolTag::Utf8: {
                        this->utf8.length = BE(reader.Read<u16>());
                        if(this->utf8.length > 0) {
                            auto strbuf = new char[this->utf8.length + 1]();
                            reader.ReadPointer(strbuf, this->utf8.length);
                            this->utf8.utf8_str.assign(strbuf, this->utf8.length);
                            delete[] strbuf;
                        }
                        break;
                    }
                    case ConstantPoolTag::Integer: {
                        this->integer.integer = BE(reader.Read<int>());
                        break;
                    }
                    case ConstantPoolTag::Float: {
                        this->flt.flt = BE(reader.Read<float>());
                        break;
                    }
                    case ConstantPoolTag::Long: {
                        this->lng.lng = BE(reader.Read<long>());
                        break;
                    }
                    case ConstantPoolTag::Double: {
                        this->dbl.dbl = BE(reader.Read<double>());
                        break;
                    }
                    case ConstantPoolTag::Class: {
                        this->clss.name_index = BE(reader.Read<u16>());
                        break;
                    }
                    case ConstantPoolTag::String: {
                        this->string.string_index = BE(reader.Read<u16>());
                        break;
                    }
                    case ConstantPoolTag::FieldRef:
                    case ConstantPoolTag::MethodRef:
                    case ConstantPoolTag::InterfaceMethodRef: {
                        this->field_method.class_index = BE(reader.Read<u16>());
                        this->field_method.name_and_type_index = BE(reader.Read<u16>());
                        break;
                    }
                    case ConstantPoolTag::NameAndType: {
                        this->name_and_type.name_index = BE(reader.Read<u16>());
                        this->name_and_type.desc_index = BE(reader.Read<u16>());
                        break;
                    }
                    case ConstantPoolTag::InstanceMethodHandle: {
                        this->method_handle.ref_kind = reader.Read<u8>();
                        this->method_handle.ref_index = BE(reader.Read<u16>());
                        break;
                    }
                    case ConstantPoolTag::InstanceMethodType: {
                        this->method_type.desc_index = BE(reader.Read<u16>());
                        break;
                    }
                    case ConstantPoolTag::InvokeDynamic: {
                        this->invoke_dynamic.bootstrap_method_attr_index = BE(reader.Read<u16>());
                        this->invoke_dynamic.name_and_type_index = BE(reader.Read<u16>());
                        break;
                    }
                    default:
                        // Should never happen, bad tag then
                        this->tag = ConstantPoolTag::Invalid;
                        this->empty = true;
                        break;
                }
            }

            inline ConstantPoolTag GetTag() {
                return this->tag;
            }

            inline Utf8Data &GetUtf8Data() {
                return this->utf8;
            }

            inline IntegerData &GetIntegerData() {
                return this->integer;
            }

            inline FloatData &GetFloatData() {
                return this->flt;
            }
            
            inline LongData &GetLongData() {
                return this->lng;
            }

            inline DoubleData &GetDoubleData() {
                return this->dbl;
            }

            inline ClassData &GetClassData() {
                return this->clss;
            }

            inline StringData &GetStringData() {
                return this->string;
            }

            inline FieldMethodRefData &GetFieldMethodRefData() {
                return this->field_method;
            }

            inline NameAndTypeData &GetNameAndTypeData() {
                return this->name_and_type;
            }

            inline InstanceMethodHandleData &GetInstanceMethodHandleData() {
                return this->method_handle;
            }

            inline InstanceMethodTypeData &GetInstanceMethodTypeData() {
                return this->method_type;
            }

            inline InvokeDynamicData &GetInvokeDynamicData() {
                return this->invoke_dynamic;
            }

            inline constexpr bool IsEmpty() {
                const auto tag_8 = static_cast<u8>(this->tag);
                if((tag_8 < static_cast<u8>(ConstantPoolTag::Min)) || (tag_8 > static_cast<u8>(ConstantPoolTag::Max))) {
                    return true;
                }
                
                return this->empty;
            }
    };

    class ConstantPool {
        private:
            std::vector<Ptr<ConstantPoolItem>> inner_pool;

        public:
            Ptr<ConstantPoolItem> GetItemAt(const u16 index, const ConstantPoolTag expected_tag = ConstantPoolTag::Invalid) {
                if(index == 0) {
                    return nullptr;
                }
                const auto actual_idx = static_cast<u16>(index - 1);
                if(actual_idx < this->inner_pool.size()) {
                    auto item = this->inner_pool.at(actual_idx);
                    // If we want to ensure the tag we expect is the one we find
                    if(expected_tag != ConstantPoolTag::Invalid) {
                        if(item->GetTag() == expected_tag) {
                            return item;
                        }
                        JAVM_LOG("Tag mismatch - %d and %d", static_cast<u32>(expected_tag), static_cast<u32>(item->GetTag()));
                    }
                    else {
                        return item;
                    }
                }
                return nullptr;
            }

            void ForEachItem(std::function<void(Ptr<ConstantPoolItem>)> fn, const bool skip_empty) {
                for(auto &item: this->inner_pool) {
                    if(skip_empty && !ptr::IsValid(item)) {
                        continue;
                    }
                    fn(item);
                }
            }

            inline void SetExpectedCount(const size_t count) {
                this->inner_pool.reserve(count);
            }

            inline size_t GetItemCount() {
                return this->inner_pool.size();
            }

            inline void InsertItem(Ptr<ConstantPoolItem> item) {
                this->inner_pool.push_back(item);
            }

            inline void InsertEmptyItem() {
                this->InsertItem(nullptr);
            }
    };

}