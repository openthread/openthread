#include <stddef.h>

extern void foo(void);

void test_1(size_t aArgument)
{
    for(size_t i = 0; i < aArgument; i++) {
        foo();
    }
}

void test_2(size_t aArgument)
{
    for(size_t i = 0; i < aArgument; i++)
    {
        foo();
    }
}

void test_3(size_t aArgument)
{
    for(size_t i = 0; i < aArgument; i++) { foo(); }
}

void test_4(size_t aArgument)
{
    for(size_t i = 0; i < aArgument; i++) foo();
}
