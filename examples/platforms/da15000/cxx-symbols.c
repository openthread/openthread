/**
 * Stubs out exception handling and general C++ bloat
 * for suitability for embedded environments.
 */

#include <stdint.h>

__extension__ typedef int __guard __attribute__((mode (__DI__)));
extern int __cxa_guard_acquire(__guard *g);
extern void __cxa_guard_release (__guard *g);
extern void __cxa_guard_abort (__guard *g);
extern void __cxa_pure_virtual(void);

int __cxa_guard_acquire(__guard *g) {
    return !*(char *)(g);
}
void __cxa_guard_release (__guard *g) {
    *(char *)g = 1;
}
void __cxa_guard_abort (__guard *g) {
    (void) g;
}
void __cxa_pure_virtual(void) {
    while(1);
}

void *__dso_handle;
