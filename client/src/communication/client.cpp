#include "client.h"

#include <unistd.h>
#include <sstream>
#include <vector>
#include <thread>

void take_input_for_server(int32_t &matrix_size, int32_t &max_value) {
    std::cout << "Enter 'matrix size' and 'max element's value' separated by space\n> ";

    std::vector<std::string> tokens;
    std::string input;
    while (true) {
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
                input.clear();
                continue;
            }

            break;
        }

        std::cout << "Wrong arguments list. Try again\n> ";
        tokens.clear();
        input.clear();
    }
}

namespace clt {
    /// return descriptor of successfully initialized socket
    int32_t create_and_open_socket(uint16_t port, const char *server_addr);

    void user_interface_handling(int32_t socket_fd);
}

void clt::single_client_with_ui() {
    int32_t socket_fd = clt::create_and_open_socket(DEFAULT_PORT, SERVER_ADDR);

    user_interface_handling(socket_fd);
    close(socket_fd);

    std::cout << "client :: Finishing the work\n";
}

void clt::user_interface_handling(int32_t socket_fd) {
    uint32_t msg_pos = 0;
    size_t rcode;
    char buffer[BUFFER_SIZE];
    uint32_t bytes_to_receive = 0;

    int32_t matrix_size, max_value;

    set_timeout(socket_fd, SO_RCVTIMEO, 30);
    rcode = recv(socket_fd, buffer, Commands::ready_receive_data_len, 0);
    if (rcode == -1) {
        close(socket_fd);
        print_error_and_halt("client :: Failed to receive confirmation from server\n");
    }
    remove_timeout(socket_fd, SO_RCVTIMEO);

    if (strcmp(buffer, Commands::ready_receive_data) != 0) {
        close(socket_fd);
        print_error_and_halt("client :: Server is not ready\n");
    }

    while (strcmp(buffer, Commands::data_received) != 0) {
        take_input_for_server(matrix_size, max_value);

        start_message(buffer, msg_pos, BUFFER_SIZE, matrix_size);
        append_to_message(buffer, msg_pos, BUFFER_SIZE, max_value);

        if (!safe_send(socket_fd, buffer, msg_pos, 0)) {
            return;
        }

        recv(socket_fd, &bytes_to_receive, sizeof bytes_to_receive, 0);
        recv(socket_fd, buffer, bytes_to_receive, 0);

        if (strcmp(buffer, Commands::emergency_exit) == 0) {
            std::cout << "client :: Emergency exit initiated by server\n";
            close(socket_fd);
            return;
        }
        if (strcmp(buffer, Commands::data_received) != 0) {
            printf("%s\n", buffer);
        }
    }

    std::cout << "client :: Server is ready to start\n";

    safe_send(socket_fd, Commands::start_task, Commands::start_task_len, 0);

    recv(socket_fd, &bytes_to_receive, sizeof bytes_to_receive, 0);
    recv(socket_fd, buffer, bytes_to_receive, 0);
    std::cout << buffer << std::endl;

    // request progress
    // get progress

    uint32_t progress = 0;
    while (strcmp(reinterpret_cast<char *>(&progress), Commands::result_ready) != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        safe_send(socket_fd, Commands::get_progress, Commands::get_progress_len, 0);

        recv(socket_fd, &progress, sizeof progress, 0);
        printf("client :: Task progress: %u%%\n", std::min(progress, 100u));
    }

    recv(socket_fd, &bytes_to_receive, sizeof bytes_to_receive, 0);
    recv(socket_fd, buffer, bytes_to_receive, 0);
    std::cout << buffer << std::endl;

    // send request for results
    if (!safe_send(socket_fd, Commands::get_results, Commands::get_results_len, 0)) {
        return;
    }

    uint64_t row = 0,
            col,
            batch_size,
            aligned_items_number;


    auto **matrix = new uint32_t *[matrix_size];

    for (size_t i = 0; i < matrix_size; i++) {
        matrix[i] = new uint32_t[matrix_size];
    }

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