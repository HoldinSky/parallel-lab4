#include "resources.h"

void throw_error_and_halt(const char *const msg) {
    std::cerr << msg << std::endl;
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
