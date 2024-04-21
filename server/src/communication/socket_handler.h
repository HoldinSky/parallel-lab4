#ifndef LAB4_SERVER_SOCKET_HANDLER_H
#define LAB4_SERVER_SOCKET_HANDLER_H

#include "base.h"

#include <thread>
#include <unordered_set>

class SocketHandler {
public:

    SocketHandler() = default;

    ~SocketHandler();

private:
    void set_timeout(int32_t s_fd, int32_t timeout_type, uint32_t secs, uint32_t usec = 0);
    void remove_timeout(int32_t s_fd, int32_t timeout_type);

public:

    std::thread accept_connection(const int32_t &socket_handler);

    int32_t handle_request(accepted_client client);

private:
    struct timeval tv{};

    std::unordered_set<accepted_client> connected_clients{};
};

#endif //LAB4_SERVER_SOCKET_HANDLER_H
