#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/des.h>
#include <sys/types.h>
#include <gmon.h>

using namespace std;

class Crypto {

public:
    static u_char* cbc_crypto_oper( u_char* input,
                             u_long length,
                             DES_cblock* key,
                             DES_cblock* iv_ret,
                             u_int oper,
                             bool modify_iv = false);
};


#endif // CRYPTO_H
