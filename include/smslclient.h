// Client interface.
// 7/5/2019, LiWentan.

#ifndef _SKCLIENT_H_
#define _SKCLIENT_H_

#include "smslserver.h"

using namespace std;

namespace sk_cs {

class SKClient {
public:
    /**
     * Specify the ip hand port to use this interface.
     */
    SKClient(const string& ip, short port);
    
    /**
     * SET.
     */
    int set(const string& key, const string& val);
     
    /**
     * GET.
     */
    int get(const string& key, string& val);
    
    /**
     * DEL.
     */
    int del(const string& key);

private:
    int _call_server(const Msg& request, Msg& response);
    sockaddr_in _srv_addr;
    int _socket;
};
    
} // End namespace sk_cs.

#endif // End ifndef _SKCLIENT_H_.