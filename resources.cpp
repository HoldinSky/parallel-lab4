#include "resources.h"

#include <sys/socket.h>

void print_error(const char* const msg) {
    fprintf(stderr, "%s", msg);
}

void print_error_and_halt(const char *const msg) {
    print_error(msg);
    exit(-1);
}

void populate_message_with_bytes(
        char *buf,
        const uint32_t max_size,
        uint32_t &current_size,
        size_t data_size,
        char *ptr_to_data
) {
    while (data_size--) {
        buf[current_size++] = *ptr_to_data++;
        if (current_size == max_size - 1) break;
    }

    buf[current_size] = '\0';
}


/// 'current_size' becomes the size of 'data' excluding '\0'
void append_to_message(char *buf, uint32_t &current_size, const uint32_t max_size, const char *const data) {
    auto ptr = const_cast<char *>(data);
    size_t data_size = strlen(data);

    populate_message_with_bytes(buf, max_size, current_size, data_size, ptr);
}

/// rewrites passed message 'data' at the beginning of 'buf'.
/// 'current_size' becomes the size of 'data' excluding '\0'
void start_message(char* buf, uint32_t &current_size, const uint32_t max_size, const char* const data) {
    current_size = 0;
    append_to_message(buf, current_size, max_size, data);
}

bool safe_send(int32_t socket_fd, const void *buf, size_t n, int flags) {
    int error = 0;
    socklen_t len = sizeof(error);
    int retval = getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len);

    char err_buf[100];
    if (retval != 0) {
        sprintf(err_buf, "server :: Error getting socket error code: %s\n", strerror(retval));
        print_error(err_buf);
        return false;
    }

    if (error != 0) {
        sprintf(err_buf, "server :: Socket error: %s\n", strerror(error));
        print_error(err_buf);
        return false;
    }

    send(socket_fd, buf, n, flags);

    return true;
}

void free_matrix(uint32_t **matrix, uint32_t size) {
    for (size_t i = 0; i < size; i++) {
        delete[] matrix[i];
    }

    delete[] matrix;
}