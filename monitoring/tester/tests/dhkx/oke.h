#pragma once

#include <vector>

#include "ustring.h"


// openssl-key-exchange
namespace oke {

//using namespace unsigned_types;

/* -----------Type definition for request and response dependencies----------- */
enum KeyExchangeType {
    UNKNOWN_REQUEST_TYPE = 0,
    // Used to initialize the key exchange sequence
    KEY_EXCHANGE_INITIATE,
    // Used to end the keyexchange sequence and indicate the execution result
    KEY_EXCHANGE_FINALIZE
};

struct KeyInfo {
    // Random digit
    std::string salt_32bytes;
    // Public-key of EC NIST P-256
    std::string ec_public_key_65bytes;
};

struct Token {
    // random at each request
    std::string salt_3bytes;
    // calculated by salt_3bytes and its public-key
    std::string hmac_3bytes;
};

struct Plaintext {
    // Custom defined message structure, you can
    // customize it to your own needs
    std::string param1;
    int32_t param2;

    inline bool ParseFromString(const std::string& data) {
        param1 = data.data();
        param2 = static_cast<int>(data.size());
        return true;
      }
};

struct ResponseStatus {
    enum StatusCode {
        UNKNOWN_RESPONSE_STATUS  = 0,
        OK,
        NOT_SUPPORTED,
        INVALID_REQUEST,
        ERROR
    };

    StatusCode  status_code;
    // A human readable string
    std::string error_message;
};


struct KeyExchangeRequest {
    // Which type of KeyExchangeType to request
    KeyExchangeType key_exchange_type;
    // It has value when key_exchanage_type=KEY_EXCHANGE_INITIATE
    KeyInfo*        key_info;
};

struct KeyExchangeResponse {
    // Which of KeyExchangeType does it match
    KeyExchangeType  key_exchange_type;
    // The status of KeyExchangeRequest execution result
    ResponseStatus   response_status;
    // It has value when key_exchanage_type=KEY_EXCHANGE_INITIATE
    KeyInfo          key_info;
};

struct Ciphertext {
    // default 1
    int32_t  cipher_version = 1;
    // randomly generated each time
    std::string  aes_iv_12bytes;
    // PlainText message serialized and encrypted
    std::string  ciphertext_nbytes;
    // generated after AES encryption
    std::string  aes_tag_16bytes;
};

struct EncryptedRequest {
    // The token to verify the identify of the client
    Token            token;
    // It can be decrypted into PlainText message
    Ciphertext       ciphertext;
};

struct EncryptedResponse {
    // The status of EncryptedRequest execution result
    ResponseStatus   response_status;
    // It can be decrypted into PlainText message
    Ciphertext       ciphertext;
};


}
