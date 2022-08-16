#pragma once

#include <cstdlib>
#include <cstdint>
#include <openssl/aes.h>

void Encrypt(uint8_t* input, uint8_t* output, size_t length, uint8_t* key, uint8_t* vec);
void Decrypt(uint8_t* input, uint8_t* output, size_t length, uint8_t* key, uint8_t* vec);
