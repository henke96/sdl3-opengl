#define util_TO_UPPER(CH) (((unsigned int)(CH) - 97 /* a */ <= 25 /* z - a */) ? (CH) - 32 /* a - A */ : (CH))

#define util_INT8_MAX_CHARS 4
#define util_UINT8_MAX_CHARS 3
#define util_INT16_MAX_CHARS 6
#define util_UINT16_MAX_CHARS 5
#define util_INT32_MAX_CHARS 11
#define util_UINT32_MAX_CHARS 10
#define util_INT64_MAX_CHARS 20
#define util_UINT64_MAX_CHARS 20

ptrdiff_t util_cstr_len(const char *str);

int util_cstr_cmp(const char *left, const char *right);

// `buffer_end` points 1 past where last digit is written.
// Returns pointer to first character of result.
char *util_int_to_str(char *buffer_end, int64_t number);

// Returns number of characters in the parsed number (max `max_chars`), 0 if parsing failed, or -1 on overflow.
// The result is stored in `*number` if successful.
ptrdiff_t util_str_to_int(const void *buffer, ptrdiff_t max_chars, int64_t *number);

// java: java.util.zip.CRC32
int32_t util_crc32(uint8_t *data, ptrdiff_t data_length);
