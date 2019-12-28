
#pragma once
#include <javm/core/core_AttributeInfo.hpp>
#include <javm/core/core_Values.hpp>

namespace javm::core {

    class FieldInfo {

        private:
            u16 access_flags;
            u16 name_index;
            u16 desc_index;
            std::string processed_name;
            std::string processed_desc;
            u16 attribute_count;
            std::vector<AttributeInfo> attributes;
            ValuePointerHolder val_holder;

        public:
            FieldInfo(MemoryReader &reader) : access_flags(0), name_index(0), desc_index(0), val_holder(ValuePointerHolder::CreateVoid()) {
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

            u16 GetNameIndex() {
                return this->name_index;
            }

            u16 GetDescIndex() {
                return this->desc_index;
            }
            
            void SetValue(ValuePointerHolder holder) {
                this->val_holder = holder;
            }

            core::ValuePointerHolder GetValue() {
                return this->val_holder;
            }

            void SetName(std::string name) {
                this->processed_name = name;
            }

            std::string GetName() {
                return this->processed_name;
            }

            void SetDesc(std::string desc) {
                this->processed_desc = desc;
            }

            std::string GetDesc() {
                return this->processed_desc;
            }
    };

}