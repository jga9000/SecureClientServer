#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <gmon.h>
#include <Winsock.h>
#include <WinSock2.h>
#include <inttypes.h>
#include <iostream>
#include "helper.h"

using namespace std;

#define HEXSTR_PREFIX "0x"

static int xtod(char c)
{
	if (c>='0' && c<='9') return c-'0';
	if (c>='A' && c<='F') return c-'A'+10;
	if (c>='a' && c<='f') return c-'a'+10;
	return -1;
};

void Helper::hexstr_to_key(const char *str, DES_cblock *key)
{
	int i = strlen(str), j = strlen(HEXSTR_PREFIX);
	int k = sizeof(DES_cblock), l = i-j;

	if ((l > 0) && (l <= (2*k)) &&
		(strncmp(str, HEXSTR_PREFIX, j) == 0))
	{
		memset(key, '\0', k);

		for (--i, j = 0; j < l; --i, ++j)
		{
			if ((k = xtod(str[i])) == -1)
				throw "Invalid hexstr_to_key input";

			(*key)[j>>1] |= (k << 4*(j%2));
		}
		return;
	}
    throw "Invalid hexstr_to_key input";
}

void Helper::printHexValue(u_char *str, u_int len)
{
    cout << "0x";
	for(int i(0); i < len; i++)
	{
        u_int v_int = (u_int)str[i];
        cout << std::hex << v_int;
	}
	cout << endl;
}

struct in_addr Helper::readLocalIPAddr()
{
    char ac[80];
    struct in_addr addr;

    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
        cerr << "Error " << WSAGetLastError() <<
                " when getting local host name." << endl;
        throw "Cannot get host name";
    }
    struct hostent *phe = gethostbyname(ac);

    if (phe == 0) {
        throw "Bad host lookup";
    }
    int addr_count( 0 );
    for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
        addr_count++;
    }
    cout << "Local IP addresses:" << addr_count << endl;
    cout << "Using address at 0" << endl;

    memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr));
    cout << "Address: " << inet_ntoa(addr) << endl;

    return addr;
}
