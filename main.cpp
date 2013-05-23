#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>

#include <Winsock.h>
#include <WinSock2.h>
#include <inttypes.h>

#include "Socket.h"
#include "message.h"
#include "helper.h"
#include "crypto.h"

using namespace std;

#define IV_KEY_DEFAULT  "0x00000000"
#define CLIENT_KEY      "0x12345678"
#define SERVER_KEY      "0x87654321"

/*
static uint32_t endianness = 0xdeadbeef;
enum endianness { BIG, LITTLE };

#define ENDIANNESS ( *(const char *)&endianness == 0xef ? LITTLE \
                   : *(const char *)&endianness == 0xde ? BIG \
                   : assert(0))

#define LE_BYTE_ENDIAN
*/

static string readArg( char* argv )
{
    if( argv ) {
        int length = strlen( argv ) + 1;
        string argstr;
        argstr.append( argv );
        return argstr;
    }
    return "";
}

u_char* authenticateToAS( const string& as_name,
                          u_int as_port,
                          const string& as_key,
                          const string& client_id,
                          const string& server_id )
{
    // ap_name = 85.23.168.83 ap_port = 6220 file = file.dat client_id = ALICE server_id = BOB
    DES_cblock deskey;
    DES_cblock iv_ret;

    Helper::hexstr_to_key( IV_KEY_DEFAULT, &iv_ret);

    u_char* as_auth_resp_decoded = NULL;

    MSG_KRB_AS_REQ auth_as_req;
    auth_as_req.type = MSG_TYPE_KRB_AS_REQ;

    memset(&(auth_as_req.client_id), 0, SIZE_ID);
    memset(&(auth_as_req.server_id), 0, SIZE_ID);
    strcpy( auth_as_req.client_id, client_id.c_str() );
    strcpy( auth_as_req.server_id, server_id.c_str() );
    auth_as_req.timestamp = time(0);

    // Send REQ to AS server using socket and wait for response
    SocketClient s(as_name.c_str(), as_port);
    s.sendRequest( (char *) &auth_as_req, sizeof( MSG_KRB_AS_REQ ) );
    u_char *server_resp = (u_char*)s.receiveResponse(sizeof(MSG_KRB_AS_RESP));
    s.close();

    // Decrypt response from AS server
    MSG_KRB_AS_RESP* as_auth_resp  = (MSG_KRB_AS_RESP*)server_resp;

    cout << "AS resp type:" << as_auth_resp->type << endl;
    if( as_auth_resp->type == MSG_TYPE_KRB_AS_ERR )  throw "Error to request received.";
    if( as_auth_resp->type != MSG_TYPE_KRB_AS_RESP )  throw "Incorrect response";

    Helper::hexstr_to_key( CLIENT_KEY, &deskey);

    // Decrypt credential info (krb_credential_t) from MSG_KRB_AS_RESP
    as_auth_resp_decoded = Crypto::cbc_crypto_oper( as_auth_resp->cred,
                                            SIZE_CRED,
                                            &deskey,
                                            &iv_ret,
                                            DES_DECRYPT);

    delete server_resp;
    cout << "End of AS authentication" << endl;

    return as_auth_resp_decoded;
}

u_char* authenticateToAP( SocketClient& s,
                          u_char* as_auth_resp,
                          const string& client_id,
                          const string& server_id )
{
    krb_credential_t* krb_credential_resp = (krb_credential_t*)as_auth_resp;

    cout << "AP server-id:" << krb_credential_resp->server_id << endl;

    // Communication to AP Server
    MSG_KRB_AP_REQ auth_ap_req;
    auth_ap_req.type = MSG_TYPE_KRB_AP_REQ;

    // Use ticket (krb_ticket_t) in krb_credential_t
    memcpy(auth_ap_req.ticket, krb_credential_resp->ticket, SIZE_TICKET);

    // Fill auth in MSG_KRB_AP_REQ
    krb_auth_t* auth_ap_req_auth = new krb_auth_t;
    memset( auth_ap_req_auth->client_id, 0, SIZE_ID);
    strcpy( auth_ap_req_auth->client_id, client_id.c_str() );
    auth_ap_req_auth->client_ipadd = Helper::readLocalIPAddr();
    auth_ap_req_auth->timestamp = time(0);

    // Encode auth
    DES_cblock iv_ret;
    Helper::hexstr_to_key( IV_KEY_DEFAULT, &iv_ret);
    u_char* session_key = new u_char [sizeof(DES_cblock)];

    memcpy( session_key, krb_credential_resp->session_key, sizeof( DES_cblock ) );
    cout << "authenticateToAP, krb_credential_resp->session_key:" << endl;
    Helper::printHexValue( session_key, sizeof( DES_cblock ) );

    u_char* auth_ap_req_auth_encoded = Crypto::cbc_crypto_oper( (u_char*)auth_ap_req_auth,
                                                        SIZE_AUTH,
                                                        (DES_cblock*)session_key,
                                                        &iv_ret,
                                                        DES_ENCRYPT );
    memcpy(auth_ap_req.auth, auth_ap_req_auth_encoded, SIZE_AUTH);

    delete as_auth_resp;
    delete auth_ap_req_auth;
    delete auth_ap_req_auth_encoded;

    cout << "authenticateToAP, sending MSG_KRB_AP_REQ" << endl;

    // Send REQ to AP server using socket and wait for response
    s.sendRequest( (char *) &auth_ap_req, sizeof(MSG_KRB_AP_REQ) );
    u_char* server_resp = (u_char*)s.receiveResponse( sizeof(MSG_KRB_AP_RESP) );

    // Parse response from AP server
    MSG_KRB_AP_RESP* ap_auth_resp = (MSG_KRB_AP_RESP*)server_resp;

    cout << "AP resp type:" << ap_auth_resp->type << endl;
    if( ap_auth_resp->type == MSG_TYPE_KRB_AP_ERR )  throw "Error to request received.";
    if( ap_auth_resp->type != MSG_TYPE_KRB_AP_RESP )  throw "Incorrect response.";

    // Decrypt MSG_KRB_AP_RESP
#if 0
    u_char* ap_auth_resp_nonce_decoded = Crypto::cbc_crypto_oper( ap_auth_resp->nonce,
                                                          SIZE_NONCE,
                                                          (DES_cblock*)session_key,
                                                          &iv_ret,
                                                          DES_DECRYPT);

    // Parse nonce from final response
    uint8_t* nonce = reinterpret_cast<uint8_t*>(ap_auth_resp_nonce_decoded);
    delete ap_auth_resp_nonce_decoded;
#endif
    delete server_resp;

    cout << "End of AP authentication" << endl;

    //cout << "authenticateToAP returns :" << std::hex << session_key;

    return session_key;
}

void receiveFileFromAP( SocketClient& s,
                   u_char* session_key,
                   const string& file )
{
    cout << "Starting filereceive" << endl;
    u_int err(0);
    bool terminated( false );

    MSG_KRB_PRIVATE* ap_private_msg = new MSG_KRB_PRIVATE;
    ap_private_msg->type = MSG_TYPE_KRB_PRIVATE;
    ap_private_msg->length = 8;
    memset(&(ap_private_msg->data), 0, SIZE_PRV);

    MSG_AP_DATA_REQ* ap_data_req = new MSG_AP_DATA_REQ;
    ap_data_req->type = MSG_TYPE_AP_DATA_REQ;

    // Use IV-value in real CBC manner
    DES_cblock cbc_iv;
    Helper::hexstr_to_key( IV_KEY_DEFAULT, &cbc_iv);

    u_char* ap_data_req_encoded = Crypto::cbc_crypto_oper( (u_char*)ap_data_req,
                                                   (u_long)ap_private_msg->length,
                                                   (DES_cblock*)session_key,
                                                   &cbc_iv,
                                                   DES_ENCRYPT,
                                                   true );

    cout << "receiveFileFromAP, using session_key:" << endl;
    Helper::printHexValue( session_key, sizeof( DES_cblock ) );

    memcpy(ap_private_msg->data, ap_data_req_encoded, ap_private_msg->length);

    // Send MSG to AP server using socket
    try {
        s.sendRequest( (char*)ap_private_msg, sizeof(MSG_KRB_PRIVATE) );
    }
    catch (...) {
        cerr << "Send error" << endl;
        err = -1;
        terminated = true;
    }
    // File to be downloaded is file.dat
    ofstream datafile;

    if( !terminated )
    {
        datafile.open (file.c_str());
    }

    try {
        while( !terminated )
        {
            // Wait response to the request
            u_char *server_resp = (u_char*)s.receiveResponse( sizeof(MSG_KRB_PRIVATE) );
            // Parse response from AP server
            MSG_KRB_PRIVATE* ap_private_resp = (MSG_KRB_PRIVATE*)server_resp;
            cout << "AP resp type:" << ap_private_resp->type << endl;

            // Decrypt data from MSG_KRB_PRIVATE
            u_char* ap_private_data_decoded = Crypto::cbc_crypto_oper(
                                                    ap_private_resp->data,
                                                    ap_private_resp->length,
                                                    (DES_cblock*)session_key,
                                                    &cbc_iv,
                                                    DES_DECRYPT,
                                                    true );

            uint16_t type = *(uint16_t*)ap_private_data_decoded;
            cout << "AP resp data type:" << type << endl;

            if( type == MSG_TYPE_AP_DATA_PLD ) {

                MSG_AP_DATA_PLD* ap_data_pld_resp =
                                        (MSG_AP_DATA_PLD*)ap_private_data_decoded;

                cout << "AP PLD resp length:" << ap_data_pld_resp->type << endl;
                cout << "AP PLD resp sequence:" << ap_data_pld_resp->type << endl;

                datafile << ap_data_pld_resp->type;
            }
            else if( type == MSG_TYPE_AP_TERMINATE ){

                MSG_AP_TERMINATE* ap_data_pld_resp =
                                        (MSG_AP_TERMINATE*)ap_private_data_decoded;


                terminated = true;
            }
            else{
                err = -1;
                cout << "Invalid response from AP server" << endl;
            }
            delete ap_private_data_decoded;
            delete server_resp;
            if( err != 0){
                break;
            }
        }
    }
    catch (...) {
        cerr << "Receive error." << endl;
        err = -1;
    }

    datafile.close();
    delete ap_data_req;
    delete ap_private_msg;

    if( err != 0)   throw "Transmission failed";
}

int main(int argc, char* argv[])
{
    cout << endl << "Secure Client Server" << endl;

    // need 8 args
    if( argc < 8 ) {
        if( argc > 0 ) // Do not display error if arg count is 0
        {
            cout << "Invalid number of arguments." << endl;
        }
        cout << "Usage:" << endl;
        cout << "./client <as_name> <as_port> <as_key> <ap_name> <ap_port>\
<file> <client_id> <server_id>" << endl;
        cout << "Example" << endl;
        cout << "./client 85.23.168.83 6110 0xFE59 85.23.168.83 6220\
image.jpg ALICE BOB" << endl;
        return -1;
    }
    int err(0);
    u_int ap_port_int, as_port_int;

    string as_name = readArg( argv[1] );
    string as_port = readArg( argv[2] );
    as_port_int = atoi ( as_port.c_str() );

    string as_key = readArg( argv[3] );
    string ap_name = readArg( argv[4] );
    string ap_port = readArg( argv[5] );
    ap_port_int = atoi ( ap_port.c_str() );
    string file = readArg( argv[6] );
    string client_id = readArg( argv[7] );
    string server_id = readArg( argv[8] );

    try {
        u_char* as_resp = authenticateToAS( as_name,
                                            as_port_int,
                                            as_key,
                                            client_id,
                                            server_id );


        SocketClient s(ap_name, ap_port_int);   // Used in both authenticateToAP
        // and receiveFileFromAP
        u_char* session_key = authenticateToAP( s,
                                                as_resp,
                                                client_id,
                                                server_id );

        receiveFileFromAP( s,
                           session_key,
                           file);

        s.close();
        delete session_key;
    }
    catch (const char* s) {
        err = -1;
        cerr << s << endl;
    }
    catch (string &s) {
        err = -1;
        cerr << s << endl;
    }
    catch (...) {
        err = -1;
        cerr << "unhandled exception" << endl;
    }

    return err;
}
