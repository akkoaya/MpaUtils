#pragma once

namespace mpa::data {

// Inclusive on both ends. Throws std::invalid_argument if min > max.
// Backed by a thread_local std::mt19937_64 seeded from std::random_device on first use,
// so it is safe to call concurrently from multiple pipeline threads.
long long RandomInt(long long min_inclusive, long long max_inclusive);

}  // namespace mpa::data
