// Client interface.
// 7/5/2019, LiWentan.

#include "../include/smslclient.h"

namespace sk_cs {

SKClient::SKClient(const string& ip, short port) {
    memset(&_srv_addr, 0, sizeof(sockaddr_in));
    _srv_addr.sin_family = AF_INET;
    _srv_addr.sin_port = htons(port);
    _srv_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}
    
int SKClient::set(const string& key, const string& val) {
    Msg request, response;
    request.status = MSG_SET;
    size_t key_size = key.size() >= 1024 ? 1023 : key.size();
    size_t val_size = val.size() >= 1024 ? 1023 : val.size();
    memcpy(request.key.data, key.c_str(), key_size);
    memcpy(request.val.data, val.c_str(), val_size);
    request.key.data[key_size] = '\0';
    request.val.data[val_size] = '\0';
    request.key.size = key_size + 1;
    request.val.size = val_size + 1;
    int ret = _call_server(request, response);
    if (ret < 0) {
        return -1;
    }
    return response.status;
}

int SKClient::get(const string& key, string& val) {
    Msg request, response;
    request.status = MSG_GET;
    if (key.size() >= 1024) {
        // Unsupported key.
        return -1;
    }
    memcpy(request.key.data, key.c_str(), key.size());
    request.key.data[key.size()] = '\0';
    request.key.size = key.size() + 1;
    int ret = _call_server(request, response);
    if (ret < 0) {
        return -1;
    }
    val = response.val.data;
    return response.status;
}

int SKClient::del(const string& key) {
    Msg request, response;
    request.status = MSG_GET;
    if (key.size() >= 1024) {
        // Unsupported key.
        return -1;
    }
    memcpy(request.key.data, key.c_str(), key.size());
    request.key.data[key.size()] = '\0';
    request.key.size = key.size() + 1;
    int ret = _call_server(request, response);
    if (ret < 0) {
        return -1;
    }
    return response.status;
}

int SKClient::_call_server(const Msg& request, Msg& response) {
    _socket = socket(PF_INET, SOCK_STREAM, 0);
    if (_socket < 0) {
        toscreen << "Initialize the client socket failed.\n";
        return -1;
    }
    int ret = connect(_socket, (sockaddr*)&_srv_addr, sizeof(sockaddr));
    if (ret < 0) {
        toscreen << "Connect to server failed.\n";
        return -1;
    }
    write(_socket, &request, sizeof(Msg));
    read(_socket, &response, sizeof(Msg));
    close(_socket);
    return 0;
}

} // End namespace sk_cs.
