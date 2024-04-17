#ifndef SERVER_BASE_H
#define SERVER_BASE_H

#include "../../../resources.h"

#include <sys/socket.h>
#include <arpa/inet.h>

namespace srv {
    /// return descriptor of successfully initialized socket
    int32_t create_and_open_socket(uint16_t port);

    int32_t routine();
}

#endif //SERVER_BASE_H
