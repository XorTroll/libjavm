#include <javm/javm_VM.hpp>

namespace javm::vm {

    ConstantPoolItem::ConstantPoolItem(MemoryReader &reader) : tag(ConstantPoolTag::Invalid), empty(true) {
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

    Ptr<ConstantPoolItem> ConstantPool::GetItemAt(const u16 index, const ConstantPoolTag expected_tag) {
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
                JAVM_LOG("Tag mismatch - %d and %d...", static_cast<u32>(expected_tag), static_cast<u32>(item->GetTag()));
            }
            else {
                return item;
            }
        }
        return nullptr;
    }

    void ConstantPool::ForEachItem(std::function<void(Ptr<ConstantPoolItem>)> fn, const bool skip_empty) {
        for(auto &item: this->inner_pool) {
            if(skip_empty && !ptr::IsValid(item)) {
                continue;
            }
            fn(item);
        }
    }

}