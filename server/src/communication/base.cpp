#include "base.h"
#include "socket_handler.h"

#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>

namespace srv {
    int32_t create_and_open_socket(uint16_t port);
}

int32_t srv::routine() {
    int32_t main_loop_socket_d = create_and_open_socket(DEFAULT_PORT);

    std::vector<std::thread> client_thread_handlers{};

    SocketHandler s_handler{};

    std::cout << "server :: Ready to receive connections\n";

    bool stop_flag = false;
    std::thread terminal_thread([&]() {
        std::string terminal_input;
        std::getline(std::cin, terminal_input);

        if (strcmp(terminal_input.c_str(), Commands::stop_server) == 0) {
            stop_flag = true;
        }

        terminal_input.clear();
    });

    while (!stop_flag) {
        auto thread = s_handler.accept_connection(main_loop_socket_d);

        client_thread_handlers.push_back(std::move(thread));
    }

    for (auto &t: client_thread_handlers) {
        t.join();
    }

    close(main_loop_socket_d);
    return 0;
}

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
        print_error_and_halt("server :: Failed to create socket");
    }

    return_code = bind(socket_descriptor, reinterpret_cast<sockaddr *>(&server), sizeof(server));
    if (return_code == -1) {
        close(socket_descriptor);
        print_error_and_halt("server :: Failed to bind socket");
    }

    listen(socket_descriptor, 5);

    return socket_descriptor;
}
