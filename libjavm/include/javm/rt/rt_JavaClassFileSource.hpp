
#pragma once
#include <javm/rt/rt_ClassSource.hpp>

namespace javm::rt {

    class JavaClassFileSource : public ClassSource, public File, public vm::AttributesItem {

        public:
            static constexpr u32 Magic = 0xCAFEBABE;

        private:
            u32 magic;
            u16 access_flags;
            u16 minor;
            u16 major;
            vm::ConstantPool pool;
            String class_name;
            String super_class_name;
            std::vector<String> interfaces;
            std::vector<vm::FieldInfo> fields;
            std::vector<vm::FieldInfo> methods;
            Ptr<vm::ClassType> cached_class_type;

            void ProcessConstantPoolUtf8Data(vm::ConstantPool &pool) {
                pool.ForEachItem([&](Ptr<vm::ConstantPoolItem> item_ref) {
                    switch(item_ref->GetTag()) {
                        case vm::ConstantPoolTag::Class: {
                            auto &data = item_ref->GetClassData();
                            auto name_data_item = this->pool.GetItemAt(data.name_index, vm::ConstantPoolTag::Utf8);
                            if(name_data_item) {
                                auto &name_data = name_data_item->GetUtf8Data();
                                data.processed_name = StrUtils::FromUtf8(name_data.utf8_str);
                            }
                            break;
                        }
                        case vm::ConstantPoolTag::NameAndType: {
                            auto &data = item_ref->GetNameAndTypeData();
                            auto name_data_item = this->pool.GetItemAt(data.name_index, vm::ConstantPoolTag::Utf8);
                            if(name_data_item) {
                                auto &name_data = name_data_item->GetUtf8Data();
                                data.processed_name = StrUtils::FromUtf8(name_data.utf8_str);
                            }
                            auto desc_data_item = this->pool.GetItemAt(data.desc_index, vm::ConstantPoolTag::Utf8);
                            if(desc_data_item) {
                                auto &desc_data = desc_data_item->GetUtf8Data();
                                data.processed_desc = StrUtils::FromUtf8(desc_data.utf8_str);
                            }
                            break;
                        }
                        case vm::ConstantPoolTag::String: {
                            auto &data = item_ref->GetStringData();
                            auto str_data_item = this->pool.GetItemAt(data.string_index, vm::ConstantPoolTag::Utf8);
                            if(str_data_item) {
                                auto &str_data = str_data_item->GetUtf8Data();
                                data.processed_string = StrUtils::FromUtf8(str_data.utf8_str);
                            }
                            break;
                        }
                        case vm::ConstantPoolTag::InstanceMethodType: {
                            auto &data = item_ref->GetInstanceMethodTypeData();
                            auto desc_data_item = this->pool.GetItemAt(data.desc_index, vm::ConstantPoolTag::Utf8);
                            if(desc_data_item) {
                                auto &desc_data = desc_data_item->GetUtf8Data();
                                data.processed_desc = StrUtils::FromUtf8(desc_data.utf8_str);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }, true);
            }

            void ProcessFieldInfoArray(std::vector<vm::FieldInfo> &array) {
                for(auto &info: array) {
                    auto name_data_item = this->pool.GetItemAt(info.GetNameIndex(), vm::ConstantPoolTag::Utf8);
                    if(name_data_item) {
                        auto &name_data = name_data_item->GetUtf8Data();
                        info.SetName(StrUtils::FromUtf8(name_data.utf8_str));
                    }
                    auto desc_data_item = this->pool.GetItemAt(info.GetDescriptorIndex(), vm::ConstantPoolTag::Utf8);
                    if(desc_data_item) {
                        auto &desc_data = desc_data_item->GetUtf8Data();
                        info.SetDescriptor(StrUtils::FromUtf8(desc_data.utf8_str));
                    }
                }
            }

            void Load() {
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
                u16 const_count = BE(reader.Read<u16>());
                this->pool.SetExpectedCount(const_count);

                for(u16 i = 1; i < const_count; i++) {
                    auto info = PtrUtils::New<vm::ConstantPoolItem>(reader);
                    bool extra_item = (info->GetTag() == vm::ConstantPoolTag::Long) || (info->GetTag() == vm::ConstantPoolTag::Double);
                    this->pool.InsertItem(info);
                    if(extra_item) {
                        // This ones take an extra spot
                        this->pool.InsertEmptyItem();
                        i++;
                    }
                }

                this->ProcessConstantPoolUtf8Data(this->pool);

                this->access_flags = BE(reader.Read<u16>());

                u16 this_class_index = BE(reader.Read<u16>());
                auto this_class_data_item = this->pool.GetItemAt(this_class_index, vm::ConstantPoolTag::Class);
                if(!this_class_data_item) {
                    return;
                }
                auto this_class_data = this_class_data_item->GetClassData();
                this->class_name = this_class_data.processed_name;

                u16 super_class_index = BE(reader.Read<u16>());
                auto super_class_data_item = this->pool.GetItemAt(super_class_index, vm::ConstantPoolTag::Class);
                if(super_class_data_item) {
                    auto super_class_data = super_class_data_item->GetClassData();
                    this->super_class_name = super_class_data.processed_name;
                }

                u16 iface_count = BE(reader.Read<u16>());
                this->interfaces.reserve(iface_count);
                for(u16 i = 0; i < iface_count; i++) {
                    u16 iface_index = BE(reader.Read<u16>());
                    auto iface_class_item = this->pool.GetItemAt(iface_index, vm::ConstantPoolTag::Class);
                    if(iface_class_item) {
                        auto iface_class_data = iface_class_item->GetClassData();
                        this->interfaces.push_back(iface_class_data.processed_name);
                    }
                }

                u16 field_count = BE(reader.Read<u16>());
                this->fields.reserve(field_count);
                for(u16 i = 0; i < field_count; i++) {
                    auto &info_ref = this->fields.emplace_back(reader, this->pool);
                }

                u16 method_count = BE(reader.Read<u16>());
                this->methods.reserve(method_count);
                for(u16 i = 0; i < method_count; i++) {
                    auto &info_ref = this->methods.emplace_back(reader, this->pool);
                }

                std::vector<vm::AttributeInfo> attrs;

                u16 attr_count = BE(reader.Read<u16>());
                attrs.reserve(attr_count);
                for(u16 i = 0; i < attr_count; i++) {
                    attrs.emplace_back(reader);
                }

                this->ProcessFieldInfoArray(this->fields);
                this->ProcessFieldInfoArray(this->methods);
                this->SetAttributes(attrs, this->pool);
            }

            vm::ClassBaseField ConvertFromFieldInfo(vm::FieldInfo &field) {
                vm::NameAndTypeData field_nat = {};
                field_nat.name_index = field.GetNameIndex();
                field_nat.desc_index = field.GetDescriptorIndex();
                field_nat.processed_name = field.GetName();
                field_nat.processed_desc = field.GetDescriptor();
                return vm::ClassBaseField(field_nat, field.GetAccessFlags(), field.GetAttributes(), this->pool);
            }

        public:
            using File::File;

            JavaClassFileSource(const std::string &path) : File(path) {
                this->Load();
            }
            
            JavaClassFileSource(u8 *ptr, size_t ptr_sz, bool owns = false) : File(ptr, ptr_sz, owns) {
                this->Load();
            }

            String GetClassName() {
                return this->class_name;
            }

            virtual Ptr<vm::ClassType> LocateClassType(const String &find_class_name) override {
                // Class files have a single class :P
                if(vm::ClassUtils::EqualClassNames(this->class_name, find_class_name)) {
                    // If the class is already loaded, just return it
                    // Creating multiple type instances would make no sense (for static fields)
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
                    this->cached_class_type = PtrUtils::New<vm::ClassType>(this->class_name, this->super_class_name, this->interfaces, new_fields, invokables, this->access_flags, this->pool);
                    return this->cached_class_type;
                }
                return nullptr;
            }

            virtual void ResetCachedClassTypes() override {
                PtrUtils::Destroy(this->cached_class_type);
            }

            virtual std::vector<Ptr<vm::ClassType>> GetClassTypes() override {
                // Create a list with our type (if it's cached)
                std::vector<Ptr<vm::ClassType>> list;
                if(this->cached_class_type) {
                    list.push_back(this->cached_class_type);
                }
                return list;
            }

    };

}