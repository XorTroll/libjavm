#include <javm/rt/rt_JavaClassFileSource.hpp>

namespace javm::rt {

    void JavaClassFileSource::ProcessConstantPoolUtf8Data(vm::ConstantPool &pool) {
        pool.ForEachItem([&](Ptr<vm::ConstantPoolItem> item_ref) {
            switch(item_ref->GetTag()) {
                case vm::ConstantPoolTag::Class: {
                    auto &data = item_ref->GetClassData();
                    auto name_data_item = this->pool.GetItemAt(data.name_index, vm::ConstantPoolTag::Utf8);
                    if(name_data_item) {
                        const auto &name_data = name_data_item->GetUtf8Data();
                        data.processed_name = str::FromUtf8(name_data.utf8_str);
                    }
                    break;
                }
                case vm::ConstantPoolTag::NameAndType: {
                    auto &data = item_ref->GetNameAndTypeData();
                    auto name_data_item = this->pool.GetItemAt(data.name_index, vm::ConstantPoolTag::Utf8);
                    if(name_data_item) {
                        const auto &name_data = name_data_item->GetUtf8Data();
                        data.processed_name = str::FromUtf8(name_data.utf8_str);
                    }
                    auto desc_data_item = this->pool.GetItemAt(data.desc_index, vm::ConstantPoolTag::Utf8);
                    if(desc_data_item) {
                        const auto &desc_data = desc_data_item->GetUtf8Data();
                        data.processed_desc = str::FromUtf8(desc_data.utf8_str);
                    }
                    break;
                }
                case vm::ConstantPoolTag::String: {
                    auto &data = item_ref->GetStringData();
                    auto str_data_item = this->pool.GetItemAt(data.string_index, vm::ConstantPoolTag::Utf8);
                    if(str_data_item) {
                        const auto &str_data = str_data_item->GetUtf8Data();
                        data.processed_string = str::FromUtf8(str_data.utf8_str);
                    }
                    break;
                }
                case vm::ConstantPoolTag::InstanceMethodType: {
                    auto &data = item_ref->GetInstanceMethodTypeData();
                    auto desc_data_item = this->pool.GetItemAt(data.desc_index, vm::ConstantPoolTag::Utf8);
                    if(desc_data_item) {
                        const auto &desc_data = desc_data_item->GetUtf8Data();
                        data.processed_desc = str::FromUtf8(desc_data.utf8_str);
                    }
                    break;
                }
                default:
                    break;
            }
        }, true);
    }

    void JavaClassFileSource::ProcessFieldInfoArray(std::vector<vm::FieldInfo> &array) {
        for(auto &info: array) {
            auto name_data_item = this->pool.GetItemAt(info.GetNameIndex(), vm::ConstantPoolTag::Utf8);
            if(name_data_item) {
                const auto &name_data = name_data_item->GetUtf8Data();
                info.SetName(str::FromUtf8(name_data.utf8_str));
            }
            auto desc_data_item = this->pool.GetItemAt(info.GetDescriptorIndex(), vm::ConstantPoolTag::Utf8);
            if(desc_data_item) {
                auto &desc_data = desc_data_item->GetUtf8Data();
                info.SetDescriptor(str::FromUtf8(desc_data.utf8_str));
            }
        }
    }

    void JavaClassFileSource::Load() {
        if(!this->IsValid()) {
            return;
        }
        MemoryReader reader(this->GetFileData(), this->GetFileSize());
        this->magic = BE(reader.Read<u32>());
        if(this->magic != Magic) {
            return;
        }
        this->minor = BE(reader.Read<u16>());
        this->major = BE(reader.Read<u16>());
        const auto const_count = BE(reader.Read<u16>());
        this->pool.SetExpectedCount(const_count);

        for(auto i = 1; i < const_count; i++) {
            auto info = ptr::New<vm::ConstantPoolItem>(reader);
            const auto has_extra_item = (info->GetTag() == vm::ConstantPoolTag::Long) || (info->GetTag() == vm::ConstantPoolTag::Double);
            this->pool.InsertItem(info);
            if(has_extra_item) {
                // This ones take an extra spot
                this->pool.InsertEmptyItem();
                i++;
            }
        }

        this->ProcessConstantPoolUtf8Data(this->pool);

        this->access_flags = BE(reader.Read<u16>());

        const auto this_class_index = BE(reader.Read<u16>());
        auto this_class_data_item = this->pool.GetItemAt(this_class_index, vm::ConstantPoolTag::Class);
        if(!this_class_data_item) {
            return;
        }
        const auto this_class_data = this_class_data_item->GetClassData();
        this->class_name = this_class_data.processed_name;

        const auto super_class_index = BE(reader.Read<u16>());
        auto super_class_data_item = this->pool.GetItemAt(super_class_index, vm::ConstantPoolTag::Class);
        if(super_class_data_item) {
            const auto super_class_data = super_class_data_item->GetClassData();
            this->super_class_name = super_class_data.processed_name;
        }

        const auto iface_count = BE(reader.Read<u16>());
        this->interfaces.reserve(iface_count);
        for(auto i = 0; i < iface_count; i++) {
            const auto iface_index = BE(reader.Read<u16>());
            auto iface_class_item = this->pool.GetItemAt(iface_index, vm::ConstantPoolTag::Class);
            if(iface_class_item) {
                const auto iface_class_data = iface_class_item->GetClassData();
                this->interfaces.push_back(iface_class_data.processed_name);
            }
        }

        const auto field_count = BE(reader.Read<u16>());
        this->fields.reserve(field_count);
        for(auto i = 0; i < field_count; i++) {
            /* auto &info_ref = */ this->fields.emplace_back(reader, this->pool);
        }

        const auto method_count = BE(reader.Read<u16>());
        this->methods.reserve(method_count);
        for(auto i = 0; i < method_count; i++) {
            /* auto &info_ref = */ this->methods.emplace_back(reader, this->pool);
        }

        std::vector<vm::AttributeInfo> attrs;

        const auto attr_count = BE(reader.Read<u16>());
        attrs.reserve(attr_count);
        for(auto i = 0; i < attr_count; i++) {
            attrs.emplace_back(reader);
        }

        this->ProcessFieldInfoArray(this->fields);
        this->ProcessFieldInfoArray(this->methods);
        this->SetAttributes(attrs, this->pool);
    }

    Ptr<vm::ClassType> JavaClassFileSource::LocateClassType(const String &find_class_name) {
        // Class files have a single class
        if(vm::EqualClassNames(this->class_name, find_class_name)) {
            // If the class has already been loaded, just return it
            if(this->cached_class_type) {
                return this->cached_class_type;
            }

            // Convert fields
            std::vector<vm::ClassBaseField> new_fields;
            for(auto &field: this->fields) {
                new_fields.push_back(this->ConvertFromFieldInfo(field));
            }

            // Get all static fns and methods
            std::vector<vm::ClassBaseField> invokables;
            for(auto &method: this->methods) {
                invokables.push_back(this->ConvertFromFieldInfo(method));
            }

            String source_file;
            for(const auto &attr: this->GetAttributes()) {
                if(attr.GetName() == vm::AttributeName::SourceFile) {
                    auto reader = attr.OpenRead();
                    vm::SourceFileAttributeData source_file_attr(reader);
                    source_file_attr.Process(this->pool);
                    source_file = source_file_attr.GetSourceFile();
                    break;
                } 
            }

            this->cached_class_type = ptr::New<vm::ClassType>(this->class_name, this->super_class_name, source_file, this->interfaces, new_fields, invokables, this->access_flags, this->pool);
            return this->cached_class_type;
        }
        return nullptr;
    }

    std::vector<Ptr<vm::ClassType>> JavaClassFileSource::GetClassTypes() {
        if(this->cached_class_type) {
            return { this->cached_class_type };
        }
        else {
            return {};
        }
    }

}