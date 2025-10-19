#include "platform.h"
#include "math.h"

// rr = 2**32
// zz[0] = 2 * rr
// zz[i + 1] = 2 * rr - sqrt(rr * (4 * rr - zz[i]))
// zzrr_scaled[i] = round(zz[i + 1] * rr * 4**i)
static const uint64_t math_zzrr_scaled[] = {
    10805852496753538807U,
    11233398260283373012U,
    11342368475190383932U,
    11369742684536801247U,
    11376594487685006716U,
    11378307954497369421U,
    11378736353457506673U,
    11378843455213691790U,
    11378870230778748829U,
    11378876924677888783U,
    11378878598153166002U,
    11378879016522016071U,
    11378879121114230511U,
    11378879147262284242U,
    11378879153799297682U,
    11378879155433551042U,
    11378879155842114382U,
    11378879155944255217U,
    11378879155969790426U,
    11378879155976174228U,
    11378879155977770179U,
    11378879155978169166U,
    11378879155978268913U,
    11378879155978293850U,
    11378879155978300084U,
    11378879155978301643U,
    11378879155978302032U,
    11378879155978302130U,
    11378879155978302154U
};

uint64_t math_sqrt(uint64_t x, bool round) {
    uint64_t d = (int64_t)1 << 62;
    while (d > x) d >>= 2;

    uint64_t c = 0;
    while (d != 0) {
        if (x >= c + d) {
            x -= c + d;
            c = (c >> 1) + d;
        } else {
            c >>= 1;
        }
        d >>= 2;
    }

    if (round && x > c) ++c;
    return c;
}

uint64_t math_sqrt128(uint64_t high, uint64_t low, bool round) {
    uint64_t d = (int64_t)1 << 62;
    while (d > high) d >>= 2;

    uint64_t high_c = 0;
    while (d != 0) {
        uint64_t high_cd = high_c + d;
        if (high >= high_cd) {
            high -= high_cd;
            high_c = (high_c >> 1) + d;
        } else {
            high_c >>= 1;
        }
        d >>= 2;
    }

    d = (uint64_t)1 << 62;
    uint64_t low_c = 0;
    while (d != 0) {
        uint64_t low_cd = low_c + d;
        if (high > high_c || (high == high_c && low >= low_cd)) {
            high -= high_c + (low < low_cd);
            low -= low_cd;
            low_c = (low_c >> 1) + d;
        } else {
            low_c >>= 1;
        }
        low_c += high_c << 63;
        high_c >>= 1;
        d >>= 2;
    }

    if (round && low > low_c) ++low_c;
    return low_c;
}

static uint64_t math_sincos_add(uint64_t yyrr2, int i) {
    if (yyrr2 == 0) {
        int i2 = i << 1;
        yyrr2 += (math_zzrr_scaled[i] + ((uint64_t)1 << i2)) >> (i2 + 1);
    } else {
        uint64_t ab_sqrt;
        {
            uint64_t a;
            {
                uint64_t high = yyrr2 >> 32;
                uint64_t low = yyrr2 & 0xFFFFFFFF;
                uint64_t highlow = high * low;
                uint64_t temp = highlow + (low * low >> 32);
                uint64_t yyrr2yyrr2_high = (
                    (high * high) +
                    (temp >> 32) +
                    ((highlow + (temp & 0xFFFFFFFF)) >> 32)
                );
                a = yyrr2 - yyrr2yyrr2_high + yyrr2;
            }

            uint64_t b = math_zzrr_scaled[i - 1];
            uint64_t ab_low = (a & 0xFFFFFFFF) * (b & 0xFFFFFFFF);
            uint64_t temp = (a >> 32) * (b & 0xFFFFFFFF) + (ab_low >> 32);
            ab_low = (ab_low & 0xFFFFFFFF) + ((temp & 0xFFFFFFFF) << 32);
            uint64_t ab_high = temp >> 32;
            temp = (b >> 32) * (a & 0xFFFFFFFF) + (ab_low >> 32);
            ab_low = (ab_low & 0xFFFFFFFF) + ((temp & 0xFFFFFFFF) << 32);
            ab_high += (a >> 32) * (b >> 32) + (temp >> 32);
            ab_sqrt = math_sqrt128(ab_high, ab_low, true);
        }

        uint64_t cd_high;
        {
            uint64_t c = -yyrr2; // rrrr - yyrr2
            uint64_t d = math_zzrr_scaled[i];
            uint64_t temp = (
                (c >> 32) * (d & 0xFFFFFFFF) +
                ((c & 0xFFFFFFFF) * (d & 0xFFFFFFFF) >> 32)
            );
            cd_high = (
                ((c >> 32) * (d >> 32)) +
                (temp >> 32) +
                (((d >> 32) * (c & 0xFFFFFFFF) + (temp & 0xFFFFFFFF)) >> 32)
            );
        }

        yyrr2 += (ab_sqrt + (cd_high >> (i + 1)) + ((uint64_t)1 << (i - 1))) >> i;
    }

    return yyrr2;
}

// TODO
int64_t math_atan(uint64_t yyrr2_goal) {
    int64_t best_angle = 0;
    uint64_t best_angle_diff = yyrr2_goal;

    int64_t angle = 0;
    uint64_t yyrr2 = 0;
    for (int i = 0; i < 29; ++i) {
        uint64_t new_yyrr2 = math_sincos_add(yyrr2, i);

        uint64_t diff;
        if (new_yyrr2 > yyrr2_goal) {
            diff = new_yyrr2 - yyrr2_goal;
            angle <<= 1;
        } else {
            diff = yyrr2_goal - new_yyrr2;
            angle = (angle << 1) | 1;
            yyrr2 = new_yyrr2;
        }

        if (diff < best_angle_diff) {
            best_angle = angle | 1;
            best_angle_diff = diff;
        } else best_angle <<= 1;
    }
    return best_angle;
}

void math_sincos(int64_t angle, uint64_t scalescale, bool round, int64_t *out_sin, int64_t *out_cos) {
    int octant = (angle >> 29) & 0x7;
    if (octant & 0x1) angle = -angle;

    uint64_t yyrr2 = 0;
    int64_t angle_bit = 1 << 29;
    for (int i = 0; angle & (angle_bit - 1); ++i) {
        angle_bit >>= 1;
        if (angle & angle_bit) yyrr2 = math_sincos_add(yyrr2, i);
    }

    uint64_t scalescaleyyrr_high, scalescaleyyrr_low;
    if (yyrr2 == 0) {
        if (octant & 1) {
            // yyrr2 = rrrr
            scalescaleyyrr_high = scalescale >> 1;
            scalescaleyyrr_low = scalescale << 63;
        } else {
            // yyrr2 = 0
            scalescaleyyrr_high = 0;
            scalescaleyyrr_low = 0;
        }
    } else {
        uint64_t low = (scalescale & 0xFFFFFFFF) * (yyrr2 & 0xFFFFFFFF);
        uint64_t temp = (scalescale >> 32) * (yyrr2 & 0xFFFFFFFF) + (low >> 32);
        low = (low & 0xFFFFFFFF) + ((temp & 0xFFFFFFFF) << 32);
        uint64_t high = temp >> 32;
        temp = (yyrr2 >> 32) * (scalescale & 0xFFFFFFFF) + (low >> 32);
        low = (low & 0xFFFFFFFF) + ((temp & 0xFFFFFFFF) << 32);
        high += (scalescale >> 32) * (yyrr2 >> 32) + (temp >> 32);

        scalescaleyyrr_high = high >> 1;
        scalescaleyyrr_low = (high << 63) + (low >> 1);
    }

    // scalescalexxrr = scalescalerrrr - scalescaleyyrr
    int64_t scalexr = (int64_t)math_sqrt128(
        scalescale - scalescaleyyrr_high - (scalescaleyyrr_low != 0),
        -scalescaleyyrr_low,
        round
    );
    int64_t scaleyr = (int64_t)math_sqrt128(scalescaleyyrr_high, scalescaleyyrr_low, round);

    switch (octant) {
        case 0: *out_cos =  scalexr; *out_sin =  scaleyr; break;
        case 1: *out_cos =  scaleyr; *out_sin =  scalexr; break;
        case 2: *out_cos = -scaleyr; *out_sin =  scalexr; break;
        case 3: *out_cos = -scalexr; *out_sin =  scaleyr; break;
        case 4: *out_cos = -scalexr; *out_sin = -scaleyr; break;
        case 5: *out_cos = -scaleyr; *out_sin = -scalexr; break;
        case 6: *out_cos =  scaleyr; *out_sin = -scalexr; break;
        case 7: *out_cos =  scalexr; *out_sin = -scaleyr; break;
        default: platform_UNREACHABLE();
    }
}
