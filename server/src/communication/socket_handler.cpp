#include "socket_handler.h"

std::thread SocketHandler::accept_connection(const int32_t &socket_handler) {
    accepted_client accepted{};

    uint32_t client_len = sizeof(accepted.address);
    accepted.socket_fd = accept(socket_handler, reinterpret_cast<sockaddr *>(&accepted.address), &client_len);

    if (accepted.socket_fd == -1) {
        close(socket_handler);
        throw_error_and_halt("server :: Failed to accept socket connection");
    }

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

        start_message(buffer, msg_pos, BUFFER_SIZE, "ready");
        send(client.socket_fd, buffer, msg_pos + 1, 0);

        if (attempts == 2) {
            this->set_timeout(client.socket_fd, SO_RCVTIMEO, 10);
        }
        rcode = recv(client.socket_fd, buffer, sizeof(int32_t) * 2, 0);

        if (rcode != 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cout << "server :: Failed to receive message during 10 seconds\n";
                close(client.socket_fd);
                return -1;
            }
        }

        if (attempts == 2) {
            remove_timeout(client.socket_fd, SO_RCVTIMEO);
        }

        msg_pos = 0;
        matrix_size = parse_message<uint64_t>(buffer, msg_pos);
        max_value = parse_message<int32_t>(buffer, msg_pos);

        if (matrix_size < 2) {
            sprintf(buffer, "Invalid argument: matrix size cannot be less than 2. (%d more attempts)", attempts);
            send(client.socket_fd, buffer, strlen(buffer), 0);
            continue;
        }
        if (max_value < 1) {
            sprintf(buffer, "Invalid argument: max element value cannot be less than 1. (%d more attempts)", attempts);
            send(client.socket_fd, buffer, strlen(buffer), 0);
            continue;
        }

        break;
    }

    if (attempts < 0) {
        return -1;
    }

    std::unique_ptr<std::unique_ptr<uint32_t[]>[]> matrix =
            std::make_unique<std::unique_ptr<uint32_t[]>[]>(matrix_size);

    for (size_t i = 0; i < matrix_size; i++) {
        matrix[i] = std::make_unique<uint32_t[]>(matrix_size);
    }

    Algorithm algo{};
    int32_t algo_rcode = 1;
    std::thread alg_thread([&]() { algo_rcode = algo.run(matrix, matrix_size, max_value); });

    this->set_timeout(client.socket_fd, SO_RCVTIMEO, 0, 500000);
    while (algo_rcode) {
        rcode = recv(client.socket_fd, buffer, this->commands.get_progress_len, 0);

        if (!rcode && strcmp(buffer, this->commands.get_progress) == 0) {
            auto progress = algo.get_percentage();
            start_message(buffer, msg_pos, BUFFER_SIZE, progress);

            send(client.socket_fd, buffer, sizeof(uint32_t), 0);
        }
    }
    this->remove_timeout(client.socket_fd, SO_RCVTIMEO);

    alg_thread.join();

    uint64_t bytes_sent = 0;
    uint64_t aligned_bytes_number = BUFFER_SIZE - BUFFER_SIZE % sizeof(matrix[0][0]);
    uint64_t bytes_to_send = sizeof(matrix[0][0]) * matrix_size * matrix_size;

    this->set_timeout(client.socket_fd, SO_SNDTIMEO, 60);
    rcode = send(client.socket_fd, reinterpret_cast<char *>(&bytes_to_send), sizeof(bytes_to_send), 0);

    if (rcode == 0) {
        while (bytes_sent < bytes_to_send) {
            send(client.socket_fd, reinterpret_cast<char *>(matrix.get()) + bytes_sent,
                 std::min(aligned_bytes_number, bytes_to_send - bytes_sent), 0);

            bytes_to_send += aligned_bytes_number;
        }
    }

    this->remove_timeout(client.socket_fd, SO_SNDTIMEO);

    close(client.socket_fd);
    this->connected_clients.erase(client);

    return 0;
}

void SocketHandler::set_timeout(int32_t s_fd, int32_t timeout_type, uint32_t secs, uint32_t usec) {
    tv.tv_sec = secs;
    tv.tv_usec = usec;

    setsockopt(s_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
}

void SocketHandler::remove_timeout(int32_t s_fd, int32_t timeout_type) {
    this->tv.tv_sec = 0;
    this->tv.tv_usec = 0;

    setsockopt(s_fd, SOL_SOCKET, timeout_type, (const char *) &tv, sizeof tv);
}
