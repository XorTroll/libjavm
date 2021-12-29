
#pragma once
#include <javm/rt/rt_JavaClassFileSource.hpp>
#include <andyzip/zipfile_reader.hpp>

namespace javm::rt {

    class ManifestFile : public File {
        private:
            std::map<std::string, std::string> attributes;

            void Load();

        public:
            using File::File;

            ManifestFile(const std::string &path) : File(path) {
                this->Load();
            }
            
            ManifestFile(const u8 *ptr, const size_t ptr_sz, const bool owns = false) : File(ptr, ptr_sz, owns) {
                this->Load();
            }

            inline bool HasAttribute(const std::string &name) {
                return this->attributes.find(name) != this->attributes.end();
            }

            inline String FindAttribute(const std::string &name) {
                auto it = this->attributes.find(name);
                if(it != this->attributes.end()) {
                    return str::FromUtf8(it->second);
                }

                // TODO
                return u"<no-attr>";
            }
    };

    class JavaArchiveSource : public ClassSource, public File {
        private:
            bool archive_valid;
            String main_class_name;
            std::vector<Ptr<JavaClassFileSource>> cached_class_files;

            inline zipfile_reader OpenSelf() {
                return zipfile_reader(this->GetFileData(), this->GetFileData() + this->GetFileSize());
            }

            void Load();

        public:
            using File::File;

            JavaArchiveSource(const std::string &path) : File(path), archive_valid(false) {
                this->Load();
            }
            
            JavaArchiveSource(const u8 *ptr, const size_t ptr_sz, const bool owns = false) : File(ptr, ptr_sz, owns), archive_valid(false) {
                this->Load();
            }

            inline String GetMainClass() {
                return this->main_class_name;
            }

            inline bool CanBeExecuted() {
                if(!this->IsValid()) {
                    return false;
                }
                return !this->main_class_name.empty();
            }

            inline bool IsArchiveValid() {
                return this->IsValid() && this->archive_valid;
            }

            virtual Ptr<vm::ClassType> LocateClassType(const String &find_class_name) override;
            virtual void ResetCachedClassTypes() override;
            virtual std::vector<Ptr<vm::ClassType>> GetClassTypes() override;
    };

}