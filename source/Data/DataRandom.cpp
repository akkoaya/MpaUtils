#include "MpaUtils/Data/DataRandom.h"

#include <random>
#include <stdexcept>

namespace mpa::data {

long long RandomInt(long long min_inclusive, long long max_inclusive) {
    if (min_inclusive > max_inclusive) {
        throw std::invalid_argument("RandomInt: min must be <= max");
    }
    thread_local std::mt19937_64 generator{std::random_device{}()};
    std::uniform_int_distribution<long long> distribution(min_inclusive, max_inclusive);
    return distribution(generator);
}

}  // namespace mpa::data

