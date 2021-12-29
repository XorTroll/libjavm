#include <javm/javm_VM.hpp>
#include <cstdio>

namespace javm {

    void File::TryLoad() {
        auto f = fopen(this->file_path.c_str(), "rb");
        if(f) {
            fseek(f, 0, SEEK_END);
            const auto f_size = static_cast<size_t>(ftell(f));
            rewind(f);
            if(f_size > 0) {
                auto f_ptr = new u8[f_size]();
                if(fread(f_ptr, f_size, 1, f) == 1) {
                    this->file_size = f_size;
                    this->file_ptr = f_ptr;
                    this->owns_ptr = true;
                }
                else {
                    delete[] file_ptr;
                    file_ptr = nullptr;
                }
            }
            fclose(f);
        }
    }

}