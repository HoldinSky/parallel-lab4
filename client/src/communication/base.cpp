#include "base.h"

#include <unistd.h>
#include <sstream>
#include <vector>
#include <thread>

void take_input_for_server(int32_t &matrix_size, int32_t &max_value) {
    std::vector<std::string> tokens;
    while (true) {
        std::string input;
        std::getline(std::cin, input);

        std::stringstream ss(input);

        std::string token;
        while (getline(ss, token, ' ')) {
            if (token.length() > 0)
                tokens.push_back(token);
        }

        if (tokens.size() == 2) {
            try {
                matrix_size = std::stoi(tokens[0]);
                max_value = std::stoi(tokens[1]);
            } catch (std::invalid_argument &err) {
                system("clear");
                std::cout << "Wrong arguments list. Try again\n> ";
                tokens.clear();
                continue;
            }

            break;
        }

        std::cout << "Wrong arguments list. Try again\n> ";
        tokens.clear();
    }
}

namespace clt {
    /// return descriptor of successfully initialized socket
    int32_t create_and_open_socket(uint16_t port, const char *server_addr);

    void user_interface_handling(int32_t socket_fd);
}

void clt::single_client_with_ui() {
    int32_t socket_fd = clt::create_and_open_socket(DEFAULT_PORT, "127.0.0.1");

    user_interface_handling(socket_fd);
    close(socket_fd);

    std::cout << "client :: Finishing the work\n";
}

void clt::user_interface_handling(int32_t socket_fd) {
    uint32_t msg_pos = 0;
    size_t rcode;
    char buffer[BUFFER_SIZE];

    int32_t matrix_size, max_value;

    rcode = recv(socket_fd, buffer, Commands::server_ready_len, 0);
    if (rcode == -1) {
        close(socket_fd);
        print_error_and_halt("client :: Failed to receive confirmation from server\n");
    }

    if (strcmp(buffer, Commands::server_ready) != 0) {
        close(socket_fd);
        print_error_and_halt("client :: Server is not ready. Cannot wait\n");
    }

    std::cout << "Enter 'matrix size' and 'max element's value' separated by space\n> ";

    take_input_for_server(matrix_size, max_value);

    append_to_message(buffer, msg_pos, BUFFER_SIZE, matrix_size);
    append_to_message(buffer, msg_pos, BUFFER_SIZE, max_value);

    auto **matrix = new uint32_t *[matrix_size];

    for (size_t i = 0; i < matrix_size; i++) {
        matrix[i] = new uint32_t[matrix_size];
    }

    rcode = send(socket_fd, buffer, msg_pos, 0);
    if (rcode == -1) {
        close(socket_fd);
        print_error_and_halt("client :: Failed to send task data to the server\n");
    }

//    uint32_t progress = 0;
//    do {
//        if (!safe_send(socket_fd, Commands::get_progress, Commands::get_progress_len, 0)) {
//            close(socket_fd);
//            free_matrix(matrix, matrix_size);
//            return;
//        }
//        recv(socket_fd, &progress, sizeof(progress), 0);
//
//        printf("client :: Task progress: %u%%\n", progress);
//        std::this_thread::sleep_for(std::chrono::milliseconds(500));
//    } while (progress < 100);

    uint64_t row = 0,
            col,
            batch_size,
            aligned_items_number;

    recv(socket_fd, &batch_size, sizeof(batch_size), 0);

    while (row < matrix_size) {
        col = 0;
        while (col < matrix_size) {
            aligned_items_number = std::min(batch_size, matrix_size - col);
            recv(socket_fd, matrix[row] + col, aligned_items_number * sizeof(**matrix), 0);

            col += aligned_items_number;
        }

        row++;
    }

    std::cout << "client :: Task results successfully received. First elements of matrix:\nclient :: ";
    for (size_t i = 0; i < matrix_size; i++) {
        std::cout << matrix[0][i] << ' ';
        if (i >= 9) {
            break;
        }
    }

    std::cout << '\n';

    free_matrix(matrix, matrix_size);
}

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
        print_error_and_halt("client :: Failed to create socket");
    }

    return_code = connect(socket_descriptor, reinterpret_cast<sockaddr *>(&client), sizeof(client));
    if (return_code != 0) {
        close(socket_descriptor);
        print_error_and_halt("client :: Failed to connect");
    }

    return socket_descriptor;
}