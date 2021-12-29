
#pragma once
#include <javm/javm_Base.hpp>

namespace javm {

    namespace str {

        String FromUtf8(const std::string &str);
        std::string ToUtf8(const String &str);

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