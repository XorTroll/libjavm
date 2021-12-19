
#pragma once
#include <javm/javm_Base.hpp>
#include <locale>
#include <codecvt>

namespace javm {

    namespace inner_impl {

        static inline std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> g_str_convert;

    }

    // TODO: change to a javm::str::* namespace?

    class StrUtils {
        public:
            static inline String FromUtf8(const std::string &str) {
                return inner_impl::g_str_convert.from_bytes(str);
            }

            static inline std::string ToUtf8(const String &str) {
                return inner_impl::g_str_convert.to_bytes(str);
            }

            template<typename T>
            static inline String From(const T t) {
                auto utf8_str = std::to_string(t);
                return FromUtf8(utf8_str);
            }
    };

}