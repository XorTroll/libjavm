
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

            void ProcessConstantPoolUtf8Data(vm::ConstantPool &pool);
            void ProcessFieldInfoArray(std::vector<vm::FieldInfo> &array);

            void Load();

            inline vm::ClassBaseField ConvertFromFieldInfo(const vm::FieldInfo &field) {
                const vm::NameAndTypeData field_nat = {
                    .name_index = field.GetNameIndex(),
                    .processed_name = field.GetName(),
                    .desc_index = field.GetDescriptorIndex(),
                    .processed_desc = field.GetDescriptor()
                };
                return vm::ClassBaseField(field_nat, field.GetAccessFlags(), field.GetAttributes(), this->pool);
            }

        public:
            using File::File;

            JavaClassFileSource(const std::string &path) : File(path) {
                this->Load();
            }
            
            JavaClassFileSource(const u8 *ptr, const size_t ptr_sz, const bool owns = false) : File(ptr, ptr_sz, owns) {
                this->Load();
            }

            inline String GetClassName() {
                return this->class_name;
            }

            virtual Ptr<vm::ClassType> LocateClassType(const String &find_class_name) override;

            inline virtual void ResetCachedClassTypes() override {
                ptr::Destroy(this->cached_class_type);
            }

            virtual std::vector<Ptr<vm::ClassType>> GetClassTypes() override;
    };

}