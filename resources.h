#ifndef LAB4_RESOURCES_H
#define LAB4_RESOURCES_H

#include "cstdint"
#include <iostream>
#include <chrono>
#include <cstring>

void throw_error_and_halt(const char *msg);

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
void start_message(char *buf, uint32_t &current_size, uint32_t max_size, const char * data);

/// update 'current_position' as it goes through message to get bytes
template<typename T>
T parse_message(char *msg, uint32_t &current_position) {
    T *data = reinterpret_cast<T *>(msg + current_position);
    current_position += sizeof(T);

    return *data;
}

template<typename FT>
std::chrono::duration<int64_t, std::milli>
measure_execution_time(FT func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
}

#endif //LAB4_RESOURCES_H
