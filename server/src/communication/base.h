#ifndef LAB4_SERVER_BASE_H
#define LAB4_SERVER_BASE_H

#include "../../../resources.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 2772
#define BUFFER_SIZE 32768

namespace srv {
    struct Commands {
        const char* const get_progress = "smp";
        const uint32_t get_progress_len = strlen(get_progress) + 1;

        const char* const stop_server = "stop";
    };

    int32_t routine();
}

struct accepted_client {
    int32_t socket_fd;
    sockaddr_in address;

    bool operator==(const accepted_client &other) const {
        return socket_fd == other.socket_fd &&
               address.sin_addr.s_addr == other.address.sin_addr.s_addr &&
               address.sin_port == other.address.sin_port &&
               address.sin_family == other.address.sin_family;
    };

    std::size_t operator()(const accepted_client &client) const noexcept {
        return std::hash<int32_t>()(client.socket_fd) ^ std::hash<uint32_t>()(client.address.sin_addr.s_addr);
    }
};

template<>
struct std::hash<accepted_client> {
    std::size_t operator()(const accepted_client &client) const noexcept {
        return client(client);
    }
};

#endif //LAB4_SERVER_BASE_H
