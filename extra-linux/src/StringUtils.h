//
// Created by Ivan Kishchenko on 01/02/2024.
//

#pragma once

#include <codecvt>
#include <locale>

class StringUtils {
public:
        inline static std::wstring s2ws(const std::string &str) {
            return std::wstring_convert < std::codecvt_utf8 < wchar_t > , wchar_t > ().from_bytes(str);
        }

        inline static std::string ws2s(const std::wstring &wstr) {
            return std::wstring_convert < std::codecvt_utf8 < wchar_t > , wchar_t > ().to_bytes(wstr);
        }
};
