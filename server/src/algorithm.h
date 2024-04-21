#ifndef LAB4_ALGORITHM_H
#define LAB4_ALGORITHM_H

#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

using lock_guard = std::unique_lock<std::mutex>;

class Algorithm {
private:
    std::atomic<unsigned int> percent_done{0};
    std::mutex progress_mutex{};

private:
    void set_percent_done(unsigned int new_percentage) {
        this->percent_done = new_percentage;
        if (this->percent_done >= 99) this->percent_done = 100;
    }

    void fill_matrix(uint32_t **matrix, uint32_t matrix_size, uint32_t max_value);

public:
    Algorithm() = default;

    int run(uint32_t **matrix, uint32_t matrix_size, uint32_t max_value);

    uint32_t get_percentage() {
        lock_guard _(this->progress_mutex);
        return this->percent_done;
    }
};

#endif //LAB4_ALGORITHM_H
