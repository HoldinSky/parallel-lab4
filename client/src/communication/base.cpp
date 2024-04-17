#include "base.h"
#include "../../../resources.h"

#include <unistd.h>

int32_t clt::create_and_open_socket(uint16_t port, const char *server_addr) {
    sockaddr_in client{};

    int32_t socket_descriptor;
    int64_t return_code;

    client.sin_family = AF_INET;
    client.sin_port = htons(port);
    client.sin_addr.s_addr = inet_addr(server_addr);

    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor < 0) {
        close(socket_descriptor);
        throw_error_and_halt("client :: Failed to create socket");
    }

    return_code = connect(socket_descriptor, reinterpret_cast<sockaddr *>(&client), sizeof(client));
    if (return_code != 0) {
        close(socket_descriptor);
        throw_error_and_halt("client :: Failed to connect");
    }

    return socket_descriptor;
}