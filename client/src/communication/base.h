#ifndef LAB4_CLIENT_BASE_H
#define LAB4_CLIENT_BASE_H

#include "../../../resources.h"

#include <sys/socket.h>
#include <arpa/inet.h>

namespace clt {
    /// return descriptor of successfully initialized socket
    int32_t create_and_open_socket(uint16_t port, const char *server_addr);
}

#endif //LAB4_CLIENT_BASE_H
