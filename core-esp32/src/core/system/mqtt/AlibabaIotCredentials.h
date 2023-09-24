//
// Created by Kishchenko, Ivan on 11/4/21.
//

#pragma once

#include <string>
#include <string_view>

class AlibabaIotCredentials {
    std::string _username;
    std::string _password;
    std::string _clientId;
    std::string _uri;
public:
    AlibabaIotCredentials(std::string_view product, std::string_view device, std::string_view secret);

    std::string_view username() {
        return _username;
    }

    std::string_view password() {
        return _password;
    }

    std::string_view clientId() {
        return _clientId;
    }

    std::string_view uri() {
        return _uri;
    }

    std::string_view caCertificate();
};
