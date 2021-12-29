
#pragma once
#include <javm/javm_Memory.hpp>
#include <javm/vm/vm_Base.hpp>

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
        Max = 18
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
            inline u16 GetDescriptorIndex() const {
                return this->desc_idx;
            }

            inline void SetDescriptorIndex(const u16 idx) {
                this->desc_idx = idx;
            }

            inline String GetDescriptor() const {
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
            ConstantPoolItem(MemoryReader &reader);

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
                if(this->empty) {
                    return true;
                }

                const auto tag_8 = static_cast<u8>(this->tag);
                if((tag_8 < static_cast<u8>(ConstantPoolTag::Min)) || (tag_8 > static_cast<u8>(ConstantPoolTag::Max))) {
                    return true;
                }
                
                return false;
            }
    };

    class ConstantPool {
        private:
            std::vector<Ptr<ConstantPoolItem>> inner_pool;

        public:
            Ptr<ConstantPoolItem> GetItemAt(const u16 index, const ConstantPoolTag expected_tag = ConstantPoolTag::Invalid);
            void ForEachItem(std::function<void(Ptr<ConstantPoolItem>)> fn, const bool skip_empty);

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