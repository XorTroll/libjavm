#include <javm/javm_VM.hpp>
#include <locale>
#include <codecvt>

namespace javm {

    namespace {

        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> g_StringConvert;

    }

    namespace str {

        String FromUtf8(const std::string &str) {
            return g_StringConvert.from_bytes(str);
        }

        std::string ToUtf8(const String &str) {
            return g_StringConvert.to_bytes(str);
        }

    }

}