#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "cryptlib.h"
#include "xed25519.h"
#include "filters.h"
#include "osrng.h"
#include "files.h"
#include "hex.h"

using boost::asio::ip::tcp;

enum { max_length = 1024 };

//const char* address = "127.0.0.1";
//const char* port = "10001";
const char* address = "5.43.224.85";
const char* port = "7133";


/*
bool key_exchange_initiate(const crypto::ownkey_s &ownkey, crypto::peerkey_s &peerkey)
{
    oke::KeyExchangeRequest key_exchange_request;
    oke::KeyInfo           *key_info = new oke::KeyInfo();

    // send ownkey to server
    key_info->ec_public_key_65bytes = std::string((char*)ownkey.ec_pub_key, sizeof(crypto::ownkey_s::ec_pub_key));
    key_info->salt_32bytes = std::string((char*)ownkey.salt, sizeof(crypto::ownkey_s::salt));
    std::cout << ">>>>Send client's own key to server:" << std::endl;
    std::cout << "  ECDH-PUB-KEY:" << std::endl;
    dash::hex_dump(key_info->ec_public_key_65bytes);
    std::cout << "  Salt:" << std::endl;
    dash::hex_dump(key_info->salt_32bytes);

    std::string str_request;
    std::string str_response;
    key_exchange_request.key_info = key_info;
    key_exchange_request.key_exchange_type= oke::KeyExchangeType::KEY_EXCHANGE_INITIATE;
    key_exchange_request.SerializeToString(&str_request);
    bool ret = rpc_client_call("localhost", 7000, 2, 1, std::bind([&str_request, &str_response](rpc::client &cli){
        str_response = cli.call("key_exchange_request", str_request).as<std::string>();
    }, std::placeholders::_1)
                               );
    if (!ret)
    {
        std::cout << "Key exchange request failed." << std::endl;
        return false;
    }


    oke::KeyExchangeResponse response;
    if (!response.ParseFromString(str_response))
    {
        std::cout << "Key exchange parsing response failed." << std::endl;
        return false;
    }
    if (!key_exchange_check_response(response, oke::KeyExchangeType::KEY_EXCHANGE_INITIATE))
        return false;
    if ((response.key_info().salt_32bytes().size() != 32) || (response.key_info().ec_public_key_65bytes().size() != 65))
    {
        std::cout << "Key length does not match." << std::endl;
        return false;
    }

    std::cout << "<<<<Received server's key:" << std::endl;
    std::cout << "  ECDH-PUB-KEY:" << std::endl;
    dash::hex_dump(response.key_info().ec_public_key_65bytes());
    std::cout << "  Salt:" << std::endl;
    dash::hex_dump(response.key_info().salt_32bytes());
    memcpy(peerkey.ec_pub_key, response.key_info().ec_public_key_65bytes().data(), response.key_info().ec_public_key_65bytes().size());
    memcpy(peerkey.salt, response.key_info().salt_32bytes().data(), response.key_info().salt_32bytes().size());

    return true;
}
*/


int main(int argc, char* argv[])
{

#if 0
    crypto::ownkey_s  client_key;
    crypto::peerkey_s server_key;

    /*
        Generate a pair of EC-KEY temporarily or you can load a pre-generated KEY from a file.
        If you are using a pre-generated KEY, you can register it‘s EC-PUBLIC-KEY on the server advance,
          so that the server can identify whether the client is legal.
    */
    if (!crypto::generate_ecdh_keys(client_key.ec_pub_key, client_key.ec_priv_key))
    {
        std::cout << "ECDH-KEY generation failed." << std::endl;
        return -1;
    }

    /* Generate a random number that called salt */
    if (!crypto::rand_salt(client_key.salt, sizeof(crypto::ownkey_s::salt)))
    {
        std::cout << "Random salt generation failed." << std::endl;
        return -1;
    }

    /* Send the client's ECDH-PUB-KEY and salt to server, then wait for the server's ECDH-PUB-KEY and salt  */
    if (!key_exchange_initiate(client_key, server_key))
    {
        std::cout << "Key exchange initialization error." << std::endl;
        return -1;
    }

    /* Calculate the final AES-KEY after receiving the server's key */
    if (!common::key_calculate(client_key, server_key))
    {
        std::cout << "Key calculation error." << std::endl;
        return -1;
    }

    /* Tell the server that the key exchange is over. */
    if (!key_exchange_finalize())
    {
        std::cout << "Key exchange finalize error." << std::endl;
        return -1;
    }

    std::cout << "Encrypted communication:\n\n" << std::endl;

    /* Encrypted communication */
    int32_t sequence = 0;
    while (1)
    {
        std::cout << "\n==================================================" << std::endl;
        if (!encrypted_request(client_key, server_key, "sequence", sequence++))
        {
            std::cout << "Request error." << std::endl;
            return -1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
#endif


//    DH *privkey;
//    int codes;
//    int secret_size;

//    /* Generate the parameters to be used */
//    if(NULL == (privkey = DH_new()))
//        handleErrors();

//    std::cout << "1 state " << std::endl;

//    if(1 != DH_generate_parameters_ex(privkey, 2048, DH_GENERATOR_2, NULL))
//        handleErrors();

//    std::cout << "2 state " << std::endl;

//    if(1 != DH_check(privkey, &codes)) handleErrors();
//    if(codes != 0)
//    {
//        /* Problems have been found with the generated parameters */
//        /* Handle these here - we'll just abort for this example */
//        printf("DH_check failed\n");
//        abort();
//    }

//    std::cout << "3 state " << std::endl;

//    /* Generate the public and private key pair */
//    if(1 != DH_generate_key(privkey))
//        handleErrors();

//    std::cout << "4 state " << std::endl;

//    /* Send the public key to the peer.
//     * How this occurs will be specific to your situation (see main text below) */


//    /* Receive the public key from the peer. In this example we're just hard coding a value */
//    BIGNUM *pubkey = NULL;
//    if(0 == (BN_dec2bn(&pubkey, "01234567890123456789012345678901234567890123456789")))
//        handleErrors();

//    /* Compute the shared secret */
//    unsigned char *secret;
//    if(NULL == (secret = (unsigned char*)OPENSSL_malloc(sizeof(unsigned char) * (DH_size(privkey)))))
//        handleErrors();

//    if(0 > (secret_size = DH_compute_key(secret, pubkey, privkey))) handleErrors();

//    /* Do something with the shared secret */
//    /* Note secret_size may be less than DH_size(privkey) */
//    printf("The shared secret is:\n");
//    BIO_dump_fp(stdout, (const char*)secret, secret_size);

//    /* Clean up */
//    OPENSSL_free(secret);
//    BN_free(pubkey);
//    DH_free(privkey);



    try
    {
        AutoSeededRandomPool rndA;

        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query(tcp::v4(), address, port);
        tcp::resolver::iterator iterator = resolver.resolve(query);

        tcp::socket s(io_service);
        s.connect(*iterator);

        std::cout << "  sizeof key: " << sizeof(client_key.ec_pub_key) << std::endl;
        std::cout << "  write length: " << pubKey.size() << std::endl;
        boost::asio::write(s, boost::asio::buffer(client_key.ec_pub_key,
                                                  sizeof(client_key.ec_pub_key)));

        unsigned char serverPubKeyBuffer[CRYPTO_EC_PUB_KEY_LEN];
        std::string serverPubKey;

        size_t length = boost::asio::read(s,
                                          boost::asio::buffer(serverPubKeyBuffer, max_length));

        serverPubKey = std::string((char*)serverPubKeyBuffer, CRYPTO_EC_PUB_KEY_LEN);
        //        unsigned char reply[max_length];
        //        size_t reply_length = boost::asio::read(s,
        //                                                boost::asio::buffer(reply));

        std::cout << "<<<<<Receive servers's key from server:" << std::endl;
        std::cout << "  reply length: " << length << std::endl;
        std::cout << "  ECDH-PUB-KEY:" << std::endl;

        dash::hex_dump(serverPubKey);

        /* Calculate the shared key using own public and private keys and the public key of the other party */
        uint8_t shared_key[CRYPTO_ECDH_SHARED_KEY_LEN];
        if (!crypto::calc_ecdh_shared_key(client_key.ec_pub_key, client_key.ec_priv_key,
                                          serverPubKeyBuffer, shared_key))
        {
            std::cout << "shared key calculation error." << std::endl;
            return false;
        }

        return 0;

        for(;;) {
            using namespace std; // For strlen.
            std::cout << "Enter message: ";
            char request[max_length];
            std::cin.getline(request, max_length);
            size_t request_length = strlen(request);
            boost::asio::write(s, boost::asio::buffer(request, request_length));

            char reply[max_length];
            size_t reply_length = boost::asio::read(s,
                                                    boost::asio::buffer(reply, request_length));
            std::cout << "Reply is: ";
            std::cout.write(reply, reply_length);
            std::cout << "\n";
            break;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
