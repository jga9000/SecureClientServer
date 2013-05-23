#ifndef HELPER_H
#define HELPER_H

#include <openssl/des.h>


class Helper {

public:
    /**
     * Converts a character string representing a hex string
     * (e.g. "0xFF6E") into a 8-byte DES key array. The lowest
     * significant byte is written to the first element in
     * the DES key array. The string MUST be null-terminated.
     *
     * @param str String representation of a hex string.
     * @param key The DES key array to write the bytes to.
     * @return Throws error if input is invalid.
     */
    static void hexstr_to_key(const char *str, DES_cblock *key);

    static struct in_addr readLocalIPAddr();

    static void printHexValue(u_char *str, u_int len);

};


#endif // HELPER_H
