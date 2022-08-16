#include "aes.h"

/**
 * @brief Encrypt
 * @param input
 * @param output
 * @param length
 * @param key
 * @param ivec
 */
void Encrypt(uint8_t* input, uint8_t* output, size_t length, uint8_t* key, uint8_t* ivec) {
    // data structure that contains the key itself
    AES_KEY keyEn;

    // set the encryption key
    AES_set_encrypt_key(key, 128, &keyEn);

    // set where on the 128 bit encrypted block to begin encryption
    int num = 0;
    AES_cfb128_encrypt(input, output, length, &keyEn, ivec, &num, AES_ENCRYPT);

}

/**
 * @brief Decrypt
 * @param input
 * @param output
 * @param length
 * @param key
 * @param ivec
 */
void Decrypt(uint8_t* input, uint8_t* output, size_t length, uint8_t* key, uint8_t* ivec) {
    // data structure that contains the key itself
    AES_KEY keyEn;

    // set the encryption key
    AES_set_encrypt_key(key, 128, &keyEn);

    // set where on the 128 bit encrypted block to begin encryption
    int num = 0;
    AES_cfb128_encrypt(input, output, length, &keyEn, ivec, &num, AES_DECRYPT);

}
