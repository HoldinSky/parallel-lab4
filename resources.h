#ifndef LAB4_RESOURCES_H
#define LAB4_RESOURCES_H

#include "cstdint"
#include <iostream>
#include <chrono>
#include <cstring>

#define DEFAULT_PORT 2772
#define BUFFER_SIZE 32768

constexpr uint32_t str_length(const char *const str) {
    uint32_t len = 0;

    for (; str[len] != '\0'; len++);

    return len + 1;
}

struct Commands {
    static constexpr const char *const get_progress = "smp";    // send me progress
    static constexpr uint32_t get_progress_len = str_length(get_progress);

    static constexpr const char *const stop_server = "stop";
    static constexpr uint32_t stop_server_len = str_length(stop_server);

    static constexpr const char *const server_ready = "ready";
    static constexpr uint32_t server_ready_len = str_length(server_ready);
};

void print_error(const char* msg);

void print_error_and_halt(const char *msg);

bool safe_send(int32_t socket_fd, const void *buf, size_t n, int flags);

void populate_message_with_bytes(
        char *buf,
        uint32_t max_size,
        uint32_t &current_size,
        size_t data_size,
        char *ptr_to_data
);

/// updates 'current_size' variable in accordance to count of bytes added to 'buf'.
/// 'current_size' increases by the size of 'data'
template<typename T>
void append_to_message(char *buf, uint32_t &current_size, const uint32_t max_size, T data) {
    auto ptr = reinterpret_cast<char *>(&data);
    size_t data_size = sizeof(data);

    populate_message_with_bytes(buf, max_size, current_size, data_size, ptr);
}

/// rewrites passed 'data' at the beginning of 'buf'.
/// 'current_size' becomes the size of 'data'
template<typename T>
void start_message(char *buf, uint32_t &current_size, const uint32_t max_size, T data) {
    current_size = 0;
    append_to_message(buf, current_size, max_size, data);
}

/// 'current_size' becomes the size of 'data' excluding '\0'
void append_to_message(char *buf, uint32_t &current_size, uint32_t max_size, const char *data);

/// rewrites passed message 'data' at the beginning of 'buf'.
/// 'current_size' becomes the size of 'data' excluding '\0'
void start_message(char *buf, uint32_t &current_size, uint32_t max_size, const char *data);

/// update 'current_position' as it goes through message to get bytes
template<typename T>
T parse_message(char *msg, uint32_t &current_position) {
    T *data = reinterpret_cast<T *>(msg + current_position);
    current_position += sizeof(T);

    return *data;
}

void free_matrix(uint32_t **matrix, uint32_t size);

#endif //LAB4_RESOURCES_H
