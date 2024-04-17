#include "base.h"

#include <thread>
#include <unistd.h>
#include <cstring>
#include <unordered_set>
#include <vector>

#define DEFAULT_PORT 2772
#define BUFFER_SIZE 32768

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

class SocketHandler {
public:

    SocketHandler() = default;

    ~SocketHandler() {
        for (auto &client: connected_clients) {
            close(client.socket_fd);
        }
    }

private:
    void set_receive_timeout(int32_t s_fd, uint32_t secs);

    void remove_receive_timeout(int32_t s_fd);

public:

    std::thread accept_connection(const int32_t &socket_handler);

    void handle_request(std::thread *self, accepted_client client);

private:
    struct timeval tv{};

    std::unordered_set<accepted_client> connected_clients{};
};

int32_t srv::create_and_open_socket(uint16_t port) {
    sockaddr_in server{};

    int32_t socket_descriptor;
    int64_t return_code;

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor < 0) {
        close(socket_descriptor);
        throw_error_and_halt("server :: Failed to create socket");
    }

    return_code = bind(socket_descriptor, reinterpret_cast<sockaddr *>(&server), sizeof(server));
    if (return_code != 0) {
        close(socket_descriptor);
        throw_error_and_halt("server :: Failed to bind socket");
    }

    listen(socket_descriptor, 5);

    return socket_descriptor;
}

int32_t srv::routine() {
    int32_t main_loop_socket_d = create_and_open_socket(DEFAULT_PORT);

    std::vector<std::thread> thread_handlers{};

    SocketHandler s_handler{};

    std::cout << "server :: Ready to receive connections\n";

    while (true) {
        auto thread = s_handler.accept_connection(main_loop_socket_d);

        std::cout << "server :: Successfully accepted connection\n";

        thread_handlers.push_back(std::move(thread));
    }

    for (auto &t: thread_handlers) {
        t.join();
    }

    close(main_loop_socket_d);
}

std::thread SocketHandler::accept_connection(const int32_t &socket_handler) {
    accepted_client accepted{};

    uint32_t client_len = sizeof(accepted.address);
    accepted.socket_fd = accept(socket_handler, reinterpret_cast<sockaddr *>(&accepted.address), &client_len);

    if (accepted.socket_fd == -1) {
        close(socket_handler);
        throw_error_and_halt("server :: Failed to accept socket connection");
    }

    this->connected_clients.insert(accepted);

    std::thread t(&SocketHandler::handle_request, this, &t, accepted);
    return t;
}

void SocketHandler::handle_request(std::thread *self, accepted_client client) {
    uint32_t msg_pos = 0;
    size_t rcode;
    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, sizeof buffer);

    start_message(buffer, msg_pos, BUFFER_SIZE, "ready");
    send(client.socket_fd, buffer, msg_pos + 1, 0);

    set_receive_timeout(client.socket_fd, 10);
    rcode = recv(client.socket_fd, buffer, sizeof(uint32_t), 0);

    if (rcode != 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::cout << "server :: Failed to receive message during 10 seconds\n";
            close(client.socket_fd);
            return;
        }
    }

    remove_receive_timeout(client.socket_fd);


}

void SocketHandler::set_receive_timeout(int32_t s_fd, uint32_t secs) {
    tv.tv_sec = secs;
    tv.tv_usec = 0;

    setsockopt(s_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
}

void SocketHandler::remove_receive_timeout(int32_t s_fd) {
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    setsockopt(s_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
}