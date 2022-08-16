package utils

import (
	"crypto/aes"
	"crypto/cipher"
	"io/ioutil"
	"os"
)

var nonce = []byte{97, 49, 89, 53, 58, 109, 46, 43, 64, 51, 35, 123}

// str: aSq4^y9e:Y+1h%N{
// 16 для 128
var secret = []byte{0x61, 0x53, 0x71, 0x34, 0x5e, 0x79, 0x39, 0x65, 0x3a, 0x59, 0x2b, 0x31, 0x68, 0x25, 0x4e, 0x7b}

func Encrypt(data, pass []byte) []byte {
	p := pass
	if p == nil {
		p = secret
	}
	block, _ := aes.NewCipher(p)
	gcm, err := cipher.NewGCM(block)
	if err != nil {
		panic(err.Error())
	}

	ciphertext := gcm.Seal(nonce, nonce, data, nil)
	return ciphertext
}

func Decrypt(data, pass []byte) []byte {
	p := pass
	if p == nil {
		p = secret
	}
	block, err := aes.NewCipher(p)
	if err != nil {
		panic(err.Error())
	}
	return decrypt(data, block)
}

func DecryptNative(data []byte, key string) []byte {
	block, err := aes.NewCipher([]byte(key))
	if err != nil {
		panic(err.Error())
	}
	return decrypt(data, block)
}

func decrypt(data []byte, block cipher.Block) []byte {
	gcm, err := cipher.NewGCM(block)
	if err != nil {
		panic(err.Error())
	}
	nonceSize := gcm.NonceSize()
	nonce, ciphertext := data[:nonceSize], data[nonceSize:]
	plaintext, err := gcm.Open(nil, nonce, ciphertext, nil)
	if err != nil {
		panic(err.Error())
	}
	return plaintext
}

func encryptFile(filename string, data, passphrase, nonce []byte) {
	f, _ := os.Create(filename)
	defer f.Close()
	f.Write(Encrypt(data, passphrase))
}

func decryptFile(filename string, passphrase []byte) []byte {
	data, _ := ioutil.ReadFile(filename)
	return Decrypt(data, passphrase)
}
