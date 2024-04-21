#include "socket_handler.h"
#include "../algorithm.h"

#include <unistd.h>

SocketHandler::~SocketHandler() {
    for (auto &client: connected_clients) {
        close(client.socket_fd);
    }
}

std::thread SocketHandler::accept_connection(const int32_t &socket_handler) {
    accepted_client accepted{};

    uint32_t client_len = sizeof(accepted.address);
    accepted.socket_fd = accept(socket_handler, reinterpret_cast<sockaddr *>(&accepted.address), &client_len);

    if (accepted.socket_fd == -1) {
        close(socket_handler);
        print_error_and_halt("server :: Failed to accept socket connection\n");
    }

    std::cout << "server :: Successfully accepted connection\n";

    this->connected_clients.insert(accepted);

    std::thread t(&SocketHandler::handle_request, this, accepted);
    return t;
}

int32_t SocketHandler::handle_request(accepted_client client) {
    uint32_t msg_pos = 0;
    size_t rcode;
    char buffer[BUFFER_SIZE];

    uint64_t matrix_size;
    int32_t attempts = 3, max_value;
    while (attempts--) {
        std::memset(buffer, 0, sizeof buffer);

        start_message(buffer, msg_pos, BUFFER_SIZE, Commands::server_ready);
        if (!safe_send(client.socket_fd, buffer, Commands::server_ready_len, 0)) {
            close(client.socket_fd);
            this->connected_clients.erase(client);

            return -1;
        }

        if (attempts == 2) {
            this->set_timeout(client.socket_fd, SO_RCVTIMEO, 30);
        }
        rcode = recv(client.socket_fd, buffer, sizeof(int32_t) * 2, 0);

        if (rcode == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cout << "server :: Failed to receive message during 30 seconds\n";
                close(client.socket_fd);
                return -1;
            }
        }

        if (attempts == 2) {
            remove_timeout(client.socket_fd, SO_RCVTIMEO);
        }

        msg_pos = 0;
        matrix_size = static_cast<uint64_t>(parse_message<int32_t>(buffer, msg_pos));
        max_value = parse_message<int32_t>(buffer, msg_pos);

        if (matrix_size < 2) {
            sprintf(buffer, "Invalid argument: matrix size cannot be less than 2. (%d more attempts)\n", attempts);

            if (!safe_send(client.socket_fd, buffer, strlen(buffer), 0)) {
                close(client.socket_fd);
                this->connected_clients.erase(client);

                return -1;
            }

            continue;
        }
        if (max_value < 1) {
            sprintf(buffer, "Invalid argument: max element value cannot be less than 1. (%d more attempts)\n",
                    attempts);

            if (!safe_send(client.socket_fd, buffer, strlen(buffer), 0)) {
                close(client.socket_fd);
                this->connected_clients.erase(client);

                return -1;
            }

            continue;
        }

        break;
    }

    if (attempts < 0) {
        return -1;
    }

    auto **matrix = new uint32_t *[matrix_size];

    for (size_t i = 0; i < matrix_size; i++) {
        matrix[i] = new uint32_t[matrix_size];
    }

    Algorithm algorithm{};
    int32_t algorithm_rcode = 1;

    std::thread progress_thread([&]() {
        this->set_timeout(client.socket_fd, SO_RCVTIMEO, 0, 500000);
        while (algorithm_rcode) {
            rcode = recv(client.socket_fd, buffer, Commands::get_progress_len, 0);

            if (rcode > 0 && strcmp(buffer, Commands::get_progress) == 0) {
                auto progress = algorithm.get_percentage();
                start_message(buffer, msg_pos, BUFFER_SIZE, progress);

                if (!safe_send(client.socket_fd, buffer, sizeof(uint32_t), 0)) {
                    close(client.socket_fd);
                    this->connected_clients.erase(client);

                    free_matrix(matrix, matrix_size);
                    return -1;
                }
            }
        }
        this->remove_timeout(client.socket_fd, SO_RCVTIMEO);
    });

    algorithm_rcode = algorithm.run(matrix, matrix_size, max_value);
    progress_thread.join();

    uint64_t items_in_batch = BUFFER_SIZE / sizeof(**matrix);

    if (!safe_send(client.socket_fd, &items_in_batch, sizeof(items_in_batch), 0)) {
        close(client.socket_fd);
        this->connected_clients.erase(client);

        free_matrix(matrix, matrix_size);
        return -1;
    }

    uint64_t row = 0, col, aligned_items_number;

    this->set_timeout(client.socket_fd, SO_SNDTIMEO, 60);
    while (row < matrix_size) {
        col = 0;
        while (col < matrix_size) {
            aligned_items_number = std::min(items_in_batch, matrix_size - col);
            if (!safe_send(client.socket_fd, matrix[row] + col, aligned_items_number * sizeof(**matrix), 0)) {
                close(client.socket_fd);
                this->connected_clients.erase(client);

                free_matrix(matrix, matrix_size);
                return -1;
            }

            col += aligned_items_number;
        }

        row++;
    }
    this->remove_timeout(client.socket_fd, SO_SNDTIMEO);

    std::cout << "server :: First elements of matrix:\nserver :: ";
    for (size_t i = 0; i < matrix_size; i++) {
        std::cout << matrix[0][i] << ' ';
        if (i >= 9) {
            break;
        }
    }

    std::cout << "\nserver :: Request completed successfully\n";

    close(client.socket_fd);
    this->connected_clients.erase(client);

    free_matrix(matrix, matrix_size);
    return 0;
}

void SocketHandler::set_timeout(int32_t s_fd, int32_t timeout_type, uint32_t secs, uint32_t usec) {
    tv.tv_sec = secs;
    tv.tv_usec = usec;

    setsockopt(s_fd, SOL_SOCKET, timeout_type, (const char *) &tv, sizeof tv);
}

void SocketHandler::remove_timeout(int32_t s_fd, int32_t timeout_type) {
    this->tv.tv_sec = 0;
    this->tv.tv_usec = 0;

    setsockopt(s_fd, SOL_SOCKET, timeout_type, (const char *) &tv, sizeof tv);
}
