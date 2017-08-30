#include <stdbool.h>
#include <stdint.h>

// Positive tests: these should generate a violation

// Arrays

// Missing space around opening brace

static const uint8_t sOctets_1[] = {0x01, 0x02 };
static const uint8_t sOctets_2[] ={0x01, 0x02 };
static const uint8_t sOctets_3[] ={ 0x01, 0x02 };
static const uint8_t sOctets_4[] =	{0x01, 0x02	};
static const uint8_t sOctets_5[] ={0x01, 0x02	};
static const uint8_t sOctets_6[] ={	0x01, 0x02	};

// Missing space around closing brace

static const uint8_t sOctets_7[] = { 0x01, 0x02};
static const uint8_t sOctets_8[] =	{	0x01, 0x02};

// Classes

class class_1 {};

class class_2{ };

class class_3{
};

// Structures

struct struct_1 {};

struct struct_2{ };

struct struct_3{
};

// Enumerations

enum enum_1 {};

enum enum_2{ };

enum enum_3{
};

// Namespaces

namespace namespace_1{};

namespace namespace_1{
};

namespace namespace_1{namespace namespace_2{};};

namespace namespace_1{

namespace namespace_2{};
};

// Functions / Methods

static int test_func_1(void){return 0; }
static int test_func_2(void){ return 0;}
static inline bool test_func_3(void){
    return false; }
static inline bool test_func_4(void) {
    return false;}
static inline void test_func_5(void)
{int a;
    return;
}

// Nested Initializers

struct test_struct_5 {
    int m_a;
    bool m_b;
};

struct test_struct_6 {
    struct test_struct_5 m_a;
    bool m_b;
};

static const struct test_struct_6 sTestStruct6[] =
    {{{ 0, true }, true }, {{ 1, false}, true }};

// Control Structures

static void test_control_structures(void)
{
    int a = 0;
    int i = 0;

    if (true){a = a + 1;}

    if (false){
        a = a - 1;
    }

    if (a == 0) {
        a = a + 1;
    }else{
        a = a - 1;}

    if (a == 1) {
        a = a * 2;
    }else if (a == 2){
        a = a / 2;
    }

    while (a > 0){
        a -= 1;}

    do{ a += 1; }while (a < 0);

    for (i = 0; i < a; i++){
        a--;}

    switch (a){case 0: a += 1;}

    switch (a){

    case 2: a *= 2;

    }

    switch (a){

    case 1: a -= 1;}
}

// Negative tests: these should not generate a violation

// None for now
