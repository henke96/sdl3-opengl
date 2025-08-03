// macro helpers
#define x_STR(X) #X
#define x_XSTR(X) x_STR(X)
#define x_ARRAY_LEN(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))
#define x_ARRAY_END(ARRAY) &ARRAY[sizeof(ARRAY)]
#define x_STR_LEN(STR) (x_ARRAY_LEN(STR) - 1)
#define x_STR_COMMA_LEN(STR) (STR), x_STR_LEN(STR)
#define x_MAX(A, B) ((A > B) ? (A) : (B))
