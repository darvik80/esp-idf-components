/*
 * Copyright (C) 2015-2019 Alibaba Group Holding Limited
 */

#include "AlibabaIotCredentials.h"

#include <cstring>
#include <mbedtls/sha256.h>

const char *ali_ca_cert = "-----BEGIN CERTIFICATE-----\r\n"
                          "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\r\n"
                          "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\r\n"
                          "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\r\n"
                          "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\r\n"
                          "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\r\n"
                          "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\r\n"
                          "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\r\n"
                          "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\r\n"
                          "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\r\n"
                          "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\r\n"
                          "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\r\n"
                          "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\r\n"
                          "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\r\n"
                          "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\r\n"
                          "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\r\n"
                          "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\r\n"
                          "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\r\n"
                          "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\r\n"
                          "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\r\n"
                          "-----END CERTIFICATE-----";

#define TIMESTAMP_VALUE             "2524608000000"
#define MQTT_CLINETID_KV            "|timestamp=2524608000000,_v=paho-c-1.0.0,securemode=3,signmethod=hmacsha256,lan=C|"

#define SHA256_KEY_IOPAD_SIZE   (64)
#define SHA256_DIGEST_SIZE      (32)

bool utils_hmac_sha256(std::string_view msg, std::string_view key, uint8_t output[32]) {
    //iot_sha256_context context{};
    uint8_t k_ipad[SHA256_KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
    uint8_t k_opad[SHA256_KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
    int32_t i;

    if (msg.empty() || key.empty()) {
        return false;
    }

    if (key.size() > SHA256_KEY_IOPAD_SIZE) {
        return false;
    }

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key.data(), key.size());
    memcpy(k_opad, key.data(), key.size());

    /* XOR key with ipad and opad values */
    for (i = 0; i < SHA256_KEY_IOPAD_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    mbedtls_sha256_context context{};
    mbedtls_sha256_init(&context);
    mbedtls_sha256_starts(&context, 0);
    mbedtls_sha256_update(&context, k_ipad, SHA256_KEY_IOPAD_SIZE);
    mbedtls_sha256_update(&context, (const unsigned char *) msg.data(), msg.size());
    mbedtls_sha256_finish(&context, output);

    /* perform outer SHA */
    mbedtls_sha256_init(&context);
    mbedtls_sha256_starts(&context, 0);
    mbedtls_sha256_update(&context, k_opad, SHA256_KEY_IOPAD_SIZE);    /* start with outer pad */
    mbedtls_sha256_update(&context, output, SHA256_DIGEST_SIZE);     /* then results of 1st hash */
    mbedtls_sha256_finish(&context, output);                       /* finish up 2nd pass */

    return true;
}

std::string hex2str(const uint8_t *input, uint16_t input_len) {
    const char *zEncode = "0123456789ABCDEF";

    std::string res;
    res.reserve(input_len * 2);
    for (int i = 0; i < input_len; i++) {
        res += zEncode[(input[i] >> 4) & 0xf];
        res += zEncode[(input[i]) & 0xf];
    }

    return res;
}

AlibabaIotCredentials::AlibabaIotCredentials(std::string_view product, std::string_view device,
                                             std::string_view secret) {
    /* setup username */
    _username = device;
    _username.append("&").append(product);

    /* setup password */
    std::string data("clientId");
    data.append(username())
            .append("deviceName").append(device)
            .append("productKey").append(product)
            .append("timestamp").append(TIMESTAMP_VALUE);

    uint8_t macRes[32];
    utils_hmac_sha256(data, secret, macRes);
    _password = hex2str(macRes, 32);

    /* setup client-id */
    _clientId = _username + MQTT_CLINETID_KV;

    _uri = "wss://";
    _uri.append(product).append(".iot-as-mqtt.ap-southeast-1.aliyuncs.com:443");
}

std::string_view AlibabaIotCredentials::caCertificate() {
    return ali_ca_cert;
}
