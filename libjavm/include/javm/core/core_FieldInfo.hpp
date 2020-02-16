
#pragma once
#include <javm/core/core_AttributeInfo.hpp>
#include <javm/core/core_Values.hpp>

namespace javm::core {

    enum class AccessFlags : u16 {

        Public = 0x0001,
        Private = 0x0002,
        Protected = 0x0004,
        Static = 0x0008,
        Final = 0x0010,
        Synchronized = 0x0020,
        Super = 0x0020,
        Volatile = 0x0040,
        Bridge = 0x0040,
        VariableArgs = 0x0080,
        Transient = 0x0080,
        Native = 0x0100,
        Interface = 0x0200,
        Abstract = 0x0400,
        Strict = 0x0800,
        Synthetic = 0x1000,
        Annotation = 0x2000,
        Enum = 0x4000,
        Miranda = 0x8000,
        ReflectMask = 0xFFFF,
        
    };

    inline constexpr u16 mask = (u16)AccessFlags::Public | (u16)AccessFlags::Private;

    class FieldInfo {

        private:
            u16 access_flags;
            u16 name_index;
            u16 desc_index;
            std::string processed_name;
            std::string processed_desc;
            u16 attribute_count;
            std::vector<AttributeInfo> attributes;
            Value val_holder;

        public:
            FieldInfo(MemoryReader &reader) : access_flags(0), name_index(0), desc_index(0), val_holder(CreateVoidValue()) {
                this->access_flags = BE(reader.Read<u16>());
                this->name_index = BE(reader.Read<u16>());
                this->desc_index = BE(reader.Read<u16>());
                this->attribute_count = BE(reader.Read<u16>());
                this->attributes.reserve(this->attribute_count);

                for(u32 i = 0; i < this->attribute_count; i++) {
                    this->attributes.emplace_back(reader);
                }
            }

            std::vector<AttributeInfo> &GetAttributes() {
                return this->attributes;
            }

            u16 GetAccessFlags() {
                return this->access_flags;
            }

            u16 GetNameIndex() {
                return this->name_index;
            }

            u16 GetDescIndex() {
                return this->desc_index;
            }
            
            void SetValue(Value value) {
                this->val_holder = value;
            }

            core::Value GetValue() {
                return this->val_holder;
            }

            void SetName(const std::string &name) {
                this->processed_name = name;
            }

            std::string GetName() {
                return this->processed_name;
            }

            void SetDesc(const std::string &desc) {
                this->processed_desc = desc;
            }

            std::string GetDesc() {
                return this->processed_desc;
            }

            template<AccessFlags flag>
            bool Is() {
                return this->access_flags & (u16)flag;
            }
    };

}