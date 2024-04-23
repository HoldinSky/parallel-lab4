#include "server.h"
#include "../algorithm.h"

#include <poll.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>
#include <unordered_map>

namespace srv {
    int32_t create_and_open_socket(uint16_t port);
}

int32_t srv::routine() {
    int32_t main_loop_socket_d = create_and_open_socket(DEFAULT_PORT);

    std::unordered_map<accepted_client, std::thread> client_thread_handlers{};

    std::cout << "server :: Ready to receive connections\n";

    bool stop_flag = false;

    std::thread terminal_thread([&]() {
        std::string terminal_input;
        std::getline(std::cin, terminal_input);

        if (strcmp(terminal_input.c_str(), Commands::stop_server) == 0) {
            stop_flag = true;
            return;
        }

        terminal_input.clear();
    });

    std::thread _([&]() {
        while(!stop_flag) {
            std::this_thread::sleep_for(std::chrono::minutes(1));
            for (auto &t : client_thread_handlers) {
                if (t.second.joinable()) {
                    t.second.join();
                    client_thread_handlers.erase(t.first);
                }
            }
        }
    });

    struct pollfd fds[1];
    fds[0].fd = main_loop_socket_d;
    fds[0].events = POLLIN;

    int rcode;
    while (!stop_flag) {
        rcode = poll(fds, 1, 1000); // 1000 means wait 1 second

        if (rcode == -1) {
            std::cerr << "server :: Error in poll\n";
            break;
        }

        if (fds[0].revents & POLLIN) {
            auto connection = srv::accept_connection(main_loop_socket_d);

            client_thread_handlers.insert(std::move(connection));
        }
    }

    for (auto &t: client_thread_handlers) {
        if (t.second.joinable()) {
            t.second.join();
        }
    }

    terminal_thread.join();

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

accepted_client internal_accept(const int32_t &socket_handler) {
    accepted_client accepted{};

    uint32_t client_len = sizeof(accepted.address);
    accepted.socket_fd = accept(socket_handler, reinterpret_cast<sockaddr *>(&accepted.address), &client_len);

    if (accepted.socket_fd == -1) {
        close(socket_handler);
        print_error_and_halt("server :: Failed to accept socket connection\n");
    }

    return accepted;
}

std::pair<accepted_client, std::thread> srv::accept_connection(const int32_t &socket_handler) {
    auto accepted = internal_accept(socket_handler);

    std::cout << "server :: Successfully accepted connection\n";

    std::thread t(&srv::handle_request, accepted);
    return std::make_pair(accepted, std::move(t));
}

int32_t srv::handle_request(accepted_client client) {
    uint32_t msg_pos = 0;
    size_t rcode;
    char buffer[BUFFER_SIZE];

    uint64_t matrix_size;
    int32_t attempts = 3, max_value;

    start_message(buffer, ++msg_pos, BUFFER_SIZE, Commands::ready_receive_data);
    if (!safe_send(client.socket_fd, buffer, msg_pos, 0)) {
        return -1;
    }

    // accept task input in 300 seconds timeout
    set_timeout(client.socket_fd, SO_RCVTIMEO, 300);
    while (attempts--) {
        std::memset(buffer, 0, sizeof buffer);
        msg_pos = 0;

        rcode = recv(client.socket_fd, buffer, sizeof(int32_t) * 2, 0);
        if (rcode == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cout << "server :: Failed to receive message during 30 seconds\n";
                safe_send(client.socket_fd, &Commands::emergency_exit_len, sizeof Commands::emergency_exit_len, 0);
                safe_send(client.socket_fd, Commands::emergency_exit, Commands::emergency_exit_len, 0);

                close(client.socket_fd);

                return -1;
            }
        }

        matrix_size = static_cast<uint64_t>(parse_message<int32_t>(buffer, msg_pos));
        max_value = parse_message<int32_t>(buffer, msg_pos);

        if (matrix_size < 2) {
            sprintf(buffer, "Invalid argument: matrix size cannot be less than 2. (%d more attempts)", attempts);
            uint32_t len = strlen(buffer) + 1;

            if (!safe_send(client.socket_fd, &len, sizeof len, 0) ||
                !safe_send(client.socket_fd, buffer, len, 0)) {
                return -1;
            }

            continue;
        }
        if (max_value < 1) {
            sprintf(buffer, "Invalid argument: max element value cannot be less than 1. (%d more attempts)", attempts);
            uint32_t len = strlen(buffer) + 1;

            if (!safe_send(client.socket_fd, &len, sizeof len, 0) ||
                !safe_send(client.socket_fd, buffer, len, 0)) {
                return -1;
            }

            continue;
        }

        safe_send(client.socket_fd, &Commands::data_received_len, sizeof Commands::data_received_len, 0);
        safe_send(client.socket_fd, Commands::data_received, Commands::data_received_len, 0);
        break;
    }
    remove_timeout(client.socket_fd, SO_RCVTIMEO);

    if (attempts < 0) {
        // send exit message
        safe_send(client.socket_fd, &Commands::emergency_exit_len, sizeof Commands::emergency_exit_len, 0);
        safe_send(client.socket_fd, Commands::emergency_exit, Commands::emergency_exit_len, 0);
        return -1;
    }

    auto **matrix = new uint32_t *[matrix_size];

    for (size_t i = 0; i < matrix_size; i++) {
        matrix[i] = new uint32_t[matrix_size];
    }

    Algorithm algorithm{};

    // receive start task request

    set_timeout(client.socket_fd, SO_RCVTIMEO, 60);
    rcode = recv(client.socket_fd, buffer, Commands::start_task_len, 0);
    remove_timeout(client.socket_fd, SO_RCVTIMEO);

    if (rcode == -1 || strcmp(buffer, Commands::start_task) != 0) {
        close(client.socket_fd);
        print_error_and_halt("server :: Start task timeout is up\n");
    }

    std::thread task([&]() { algorithm.run(matrix, matrix_size, max_value); });

    start_message(buffer, msg_pos, BUFFER_SIZE, "server :: Task is started");

    if (!safe_send(client.socket_fd, &++msg_pos, sizeof msg_pos, 0) ||
        !safe_send(client.socket_fd, buffer, msg_pos, 0)) {
        return -1;
    }

    // get request for progress
    // send progress back

    set_timeout(client.socket_fd, SO_RCVTIMEO, 1);
    while (true) {
        rcode = recv(client.socket_fd, buffer, Commands::get_progress_len, 0);

        if (rcode == -1) {
            continue;
        }

        if (strcmp(buffer, Commands::get_progress) == 0) {
            auto progress = algorithm.get_percentage();

            if (progress >= 100) {
                safe_send(client.socket_fd, Commands::result_ready, Commands::result_ready_len, 0);
                break;
            } else {
                safe_send(client.socket_fd, &progress, sizeof progress, 0);
            }
        } else {
            uint32_t mock = 0;
            safe_send(client.socket_fd, &mock, sizeof mock, 0);
        }
    }
    remove_timeout(client.socket_fd, SO_RCVTIMEO);

    task.join();

    start_message(buffer, msg_pos, BUFFER_SIZE, "server :: Task successfully finished");
    safe_send(client.socket_fd, &++msg_pos, sizeof msg_pos, 0);
    safe_send(client.socket_fd, buffer, msg_pos, 0);

    // get request for results
    // send results

    recv(client.socket_fd, buffer, Commands::get_results_len, 0);
    if (strcmp(buffer, Commands::get_results) != 0) {
        close(client.socket_fd);
        return -1;
    }

    uint64_t col = 0, row, aligned_items_number;
    uint64_t items_in_batch = BUFFER_SIZE / sizeof(**matrix);

    safe_send(client.socket_fd, &items_in_batch, sizeof items_in_batch, 0);

    set_timeout(client.socket_fd, SO_SNDTIMEO, 60);
    while (col < matrix_size) {
        row = 0;
        while (row < matrix_size) {
            aligned_items_number = std::min(items_in_batch, matrix_size - row);
            if (!safe_send(client.socket_fd, matrix[col] + row, aligned_items_number * sizeof(**matrix), 0)) {
                free_matrix(matrix, matrix_size);
                return 0;
            }

            row += aligned_items_number;
        }

        col++;
    }
    remove_timeout(client.socket_fd, SO_SNDTIMEO);

    std::cout << "server :: First elements of matrix:\nserver :: ";
    for (size_t i = 0; i < matrix_size; i++) {
        std::cout << matrix[0][i] << ' ';
        if (i >= 9) {
            break;
        }
    }

    free_matrix(matrix, matrix_size);

    std::cout << "\nserver :: Results successfully sent to client\n"
                 "server :: Request completed successfully\n";

    close(client.socket_fd);

    return 0;
}