#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <gmon.h>
#include "crypto.h"

using namespace std;

u_char* Crypto::cbc_crypto_oper( u_char* input,
                                u_long length,
                                DES_cblock* key,
                                DES_cblock* iv_ret,
                                u_int oper,
                                bool modify_iv )
{
    if( length < 8 )    length = 8;
    DES_key_schedule schedule;

    DES_set_odd_parity(key);
    DES_set_key_unchecked(key, &schedule);
    DES_set_key(key, &schedule);
    DES_key_sched(key, &schedule);

    u_char* output = new u_char[ length ];
    memset(output, 0, length);

    /* void DES_ncbc_encrypt(const u_char *input, u_char *output,
            long length, DES_key_schedule *schedule, DES_cblock *ivec,
            int enc);
            */
    if(!modify_iv)
    {
        DES_cbc_encrypt(input, output, length,
                        &schedule, iv_ret, oper );
    }
    else{   // use ncbc
        DES_ncbc_encrypt(input, output, length,
                        &schedule, iv_ret, oper );
    }

    return output;
}
