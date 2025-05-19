#include "utility/random_util.hpp"

using namespace bindings;

std::random_device RandomUtil::dev;
std::mt19937 RandomUtil::rng(RandomUtil::dev());
std::uniform_int_distribution<std::mt19937::result_type> RandomUtil::dist(0, std::numeric_limits<uint32_t>::max());
