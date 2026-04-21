#pragma once

#include <cstdint>
#include <vector>

int64_t sum_array_basic(const std::vector<int32_t>& v);
int64_t sum_array_neon(const std::vector<int32_t>& v);

bool neon_enabled_at_compile_time();
