#include "platform.h"
#include "util.h"

ptrdiff_t util_cstr_len(const char *str) {
    const char *c = str;
    for (; *c != '\0'; ++c);
    return c - str;
}

int util_cstr_cmp(const char *left, const char *right) {
    for (;;) {
        int diff = *left - *right;
        if (diff != 0 || *left == '\0') return diff;
        ++left;
        ++right;
    }
}

char *util_int_to_str(char *buffer_end, int64_t number) {
    uint64_t n = number < 0 ? -((uint64_t)number) : (uint64_t)number;
    do {
        *--buffer_end = (char)('0' + n % 10);
        n /= 10;
    } while (n != 0);

    if (number < 0) *--buffer_end = '-';
    return buffer_end;
}

ptrdiff_t util_str_to_int(const void *buffer, ptrdiff_t max_chars, int64_t *number) {
    if (max_chars <= 0) return 0;

    uint64_t negative = ((const char *)buffer)[0] == '-';

    uint64_t max_number = (uint64_t)INT64_MAX + negative;
    uint64_t result = 0;
    ptrdiff_t i = (ptrdiff_t)negative;
    for (; i < max_chars; ++i) {
        uint64_t digit_value = (uint64_t)((const uint8_t *)buffer)[i] - '0';
        if (digit_value > 9) break;

        if (result > (max_number - digit_value) / 10) return -1;
        result = result * 10 + digit_value;
    }

    if (i > (ptrdiff_t)negative) {
        *number = (int64_t)(result * (1 - (negative << 1)));
        return i;
    }
    return 0;
}

int32_t util_crc32(uint8_t *data, ptrdiff_t data_length) {
    uint32_t crc = 0xFFFFFFFF;
    for (ptrdiff_t i = 0; i < data_length; ++i) {
        crc = crc ^ (uint32_t)data[i];
        for (int j = 0; j < 8; ++j) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return (int32_t)~crc;
}
