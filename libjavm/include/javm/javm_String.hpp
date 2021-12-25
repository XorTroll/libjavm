
#pragma once
#include <javm/javm_Base.hpp>
#include <locale>
#include <codecvt>

namespace javm {

    namespace inner_impl {

        static inline std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> g_str_convert;

    }

    // TODO: change all/most of *Utils static classes into namespaces...

    namespace str {

        inline String FromUtf8(const std::string &str) {
            return inner_impl::g_str_convert.from_bytes(str);
        }

        inline std::string ToUtf8(const String &str) {
            return inner_impl::g_str_convert.to_bytes(str);
        }

        template<typename T>
        inline String From(const T t) {
            return FromUtf8(std::to_string(t));
        }

        template<typename ...Args>
        inline String Format(const std::string &fmt, Args &&...args) {
            const auto fmt_size = std::snprintf(nullptr, 0, fmt.c_str(), args...);
            auto fmt_buf = new char[fmt_size + 1]();
            std::sprintf(fmt_buf, fmt.c_str(), args...);
            fmt_buf[fmt_size] = '\0';

            const auto fmt_str = FromUtf8(fmt_buf);
            delete[] fmt_buf;
            return fmt_str;
        }

    }

}