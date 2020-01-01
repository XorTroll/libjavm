
#pragma once
#include <javm/javm_File.hpp>
#include <map>
#include <sstream>
#include <iostream>

namespace javm::core {
    #define JAVM_JAR_MANIFEST_FILE "META-INF/MANIFEST.MF"

    class ManifestFile : public File {

        private:
            std::map<std::string, std::string> attributes;

            void Load() {
                if(this->IsValid()) {
                    char *manifest_str = new char[this->GetFileSize() + 1]();
                    memcpy(manifest_str, this->GetFileData(), this->GetFileSize());
                    std::istringstream strm(manifest_str);
                    std::string tmpline;
                    while(std::getline(strm, tmpline)) {
                        if(tmpline.empty()) {
                            continue;
                        }
                        if(tmpline.find(':') == std::string::npos) {
                            continue;
                        }
                        auto attr_name = tmpline.substr(0, tmpline.find_first_of(':'));
                        auto attr_value = tmpline.substr(tmpline.find_first_of(':') + 1);
                        while(attr_value.front() == ' ') {
                            attr_value.erase(attr_value.begin());
                        }
                        attr_value.erase(std::remove(attr_value.begin(), attr_value.end(), '\n'), attr_value.end());
                        attr_value.erase(std::remove(attr_value.begin(), attr_value.end(), '\r'), attr_value.end());
                        this->attributes.insert(std::make_pair(attr_name, attr_value));
                    }
                    delete[] manifest_str;
                }
            }

        public:
            using File::File;

            ManifestFile(std::string path) : File(path) {
                this->Load();
            }
            
            ManifestFile(u8 *ptr, size_t ptr_sz, bool owns = false) : File(ptr, ptr_sz, owns) {
                this->Load();
            }

            std::string FindAttribute(std::string name) {
                auto it = this->attributes.find(name);
                if(it != this->attributes.end()) {
                    return it->second;
                }
                return "";
            }
    };

}