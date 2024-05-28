#ifndef INCLUDES_H
#define INCLUDES_H

#ifdef DEBUG
#define debug_print(X, ...) \
    printf("[DEBUG] %s:%d: ", __FILE__, __LINE__); \
    printf(X __VA_OPT__(,) __VA_ARGS__); \
    printf("\n"); \

#define error_print(X, ...) \
    printf("[ERROR] %s:%d: ", __FILE__, __LINE__); \
    printf(X __VA_OPT__(,) __VA_ARGS__); \
    printf("\n"); \

#else
#define debug_print(X, ...) 
#define error_print(X, ...)
#endif 

#define defer(X) \
    result = X; \
    goto defer; \

#endif // INCLUDES_H