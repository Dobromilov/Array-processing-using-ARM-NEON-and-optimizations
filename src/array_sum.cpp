#include "array_sum.h"

int64_t sum_array_basic(const std::vector<int32_t>& v) {
    int64_t sum = 0;

    for (int32_t num : v) {
        if (num > 0) {
            sum += num;
        } else if (num < 0) {
            sum += -(int64_t)num;
        }
    }

    return sum;
}

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>

int64_t sum_array_neon(const std::vector<int32_t>& v) {
    size_t i = 0;
    int32x4_t acc = vdupq_n_s32(0);
    int32x4_t zero = vdupq_n_s32(0);

    for (; i + 3 < v.size(); i += 4) {
        int32x4_t vec = vld1q_s32(&v[i]);

        uint32x4_t mask_pos = vcgtq_s32(vec, zero);
        uint32x4_t mask_neg = vcltq_s32(vec, zero);

        int32x4_t sign = vshrq_n_s32(vec, 31);
        int32x4_t abs_val = veorq_s32(vec, sign);
        abs_val = vsubq_s32(abs_val, sign);

        int32x4_t pos_part = vandq_s32(vec, vreinterpretq_s32_u32(mask_pos));
        int32x4_t neg_part = vandq_s32(abs_val, vreinterpretq_s32_u32(mask_neg));

        int32x4_t contrib = vorrq_s32(pos_part, neg_part);

        acc = vaddq_s32(acc, contrib);
    }

    int64_t sum = 0;

    alignas(16) int32_t temp[4];
    vst1q_s32(temp, acc);

    sum += temp[0];
    sum += temp[1];
    sum += temp[2];
    sum += temp[3];

    for (; i < v.size(); ++i) {
        int32_t num = v[i];
        if (num > 0) {
            sum += num;
        } else if (num < 0) {
            sum += -(int64_t)num;
        }
    }

    return sum;
}

bool neon_enabled_at_compile_time() {
    return true;
}

#else

int64_t sum_array_neon(const std::vector<int32_t>& v) {
    // Fallback for non-ARM environments where NEON intrinsics are unavailable.
    return sum_array_basic(v);
}

bool neon_enabled_at_compile_time() {
    return false;
}

#endif
