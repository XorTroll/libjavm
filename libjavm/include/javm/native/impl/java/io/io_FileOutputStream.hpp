
#pragma once
#include <javm/native/native_NativeCode.hpp>
#include <javm/native/impl/impl_Base.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <javm/vm/vm_Thread.hpp>
#include <javm/vm/vm_Properties.hpp>
#include <javm/vm/vm_Reflection.hpp>
#include <unistd.h>
#include <csignal>
#include <sys/time.h>

namespace javm::native::impl::java::io {

    using namespace vm;

    class FileOutputStream {
        public:
            static ExecutionResult initIDs(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.io.FileOutputStream.initIDs] called");
                return ExecutionResult::Void();
            }

            static ExecutionResult writeBytes(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                auto byte_arr_v = param_vars[0];
                auto byte_arr = byte_arr_v->GetAs<type::Array>();
                auto off_v = param_vars[1];
                const auto off = off_v->GetValue<type::Integer>();
                auto len_v = param_vars[2];
                const auto len = len_v->GetValue<type::Integer>();
                auto append_v = param_vars[3];
                const auto append = (bool)append_v->GetValue<type::Integer>();

                JAVM_LOG("[java.io.FileOutputStream.writeBytes] called - Array: bytes[%d], Offset: %d, Length: %d, Append: %s", byte_arr->GetLength(), off, len, append ? "true" : "false");

                auto this_obj = this_var->GetAs<type::ClassInstance>();
                auto fd_fd_v = this_obj->GetField(u"fd", u"Ljava/io/FileDescriptor;");
                auto fd_fd_obj = fd_fd_v->GetAs<type::ClassInstance>();
                auto fd_v = fd_fd_obj->GetField(u"fd", u"I");
                const auto fd = fd_v->GetValue<type::Integer>();

                JAVM_LOG("[java.io.FileOutputStream.writeBytes] FD: %d", fd);

                const auto proper_len = std::min(static_cast<u32>(len), byte_arr->GetLength());

                auto tmpbuf = new u8[proper_len]();
                for(u32 i = 0; i < proper_len; i++) {
                    auto byte_v = byte_arr->GetAt(i);
                    const auto byte = static_cast<u8>(byte_v->GetValue<type::Integer>());
                    tmpbuf[i] = byte;
                }

                const auto ret = write(fd, tmpbuf, proper_len);
                JAVM_LOG("[java.io.FileOutputStream.writeBytes] Ret: %ld", ret);

                delete[] tmpbuf;
                return ExecutionResult::Void();
            }
    };

}