#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>

inline void _swap_int(int& a, int& b) {
    int tmp = a;
    a = b;
    b = tmp;
}


inline void _swap_ptr(void *a, void *b) {
    void *tmp = a;
    a = b;
    b = tmp;
}

#endif // HELPERS_H