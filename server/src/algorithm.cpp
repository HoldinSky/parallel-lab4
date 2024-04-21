#include "algorithm.h"

#include <random>
#include <thread>

static std::random_device seed;

void Algorithm::fill_matrix(
        uint32_t **matrix,
        uint32_t matrix_size,
        uint32_t max_value
) {
    std::default_random_engine generator(seed());
    std::uniform_int_distribution<int> distribution(0, static_cast<int32_t>(max_value));

    auto random_value = [&] { return distribution(generator); };

    uint32_t progress_tracker = 0;
    const size_t one_hundreds = matrix_size / 100;

    for (size_t i = 0; i < matrix_size; i++, progress_tracker++) {
        if (progress_tracker > one_hundreds) {
            {
                lock_guard _(this->progress_mutex);
                set_percent_done(i / one_hundreds);
                progress_tracker = 0;
            }
        }

        uint32_t column_sum = 0;
        for (size_t j = 0; j < matrix_size; j++) {
            if (i == j) continue;

            matrix[i][j] = random_value();
            column_sum += matrix[i][j];
        }

        matrix[i][i] = column_sum;
    }
}


int Algorithm::run(uint32_t **matrix, uint32_t matrix_size, uint32_t max_value) {
    percent_done = 0;

    fill_matrix(matrix, matrix_size, max_value);

    return 0;
}
