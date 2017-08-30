#include <stdint.h>

// Positive tests: these should generate a violation

// No space on either side of binary operators

void test_1(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a=1;
    b=1;
    c=1;

    // Binary arithmetic operators
    
    c = (a+b);
    c = (a*b);
    c = (a-b);
    c = (a/b);
    c = (a%b);

    // Binary bitwise operators

    c = (a&b);
    c = (a|b);
    c = (a^b);

    // Binary shift (or stream) operators

    c = (a<<b);
    c = (a>>b);

    // Binary arithmetic, bitwise, and shift with assignment operators

    c+=a;
    c*=a;
    c-=a;
    c/=a;
    c%=a;
    c&=a;
    c|=a;
    c^=a;
    c<<=a;
    c>>=a;

    // Binary logical operators

    result = (a&&b);
    result = (a||b);

    // Binary comparison operators

    result = (a==b);
    result = (a!=b);
    result = (a<=b);
    result = (a>=b);
    result = (a<b);
    result = (a>b);
}

// Single space on the left side of binary operators

void test_2_1(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a =1;
    b =1;
    c =1;

    // Binary arithmetic operators
    
    c = (a +b);
    c = (a *b);
    c = (a -b);
    c = (a /b);
    c = (a %b);

    // Binary bitwise operators

    c = (a &b);
    c = (a |b);
    c = (a ^b);

    // Binary shift (or stream) operators

    c = (a <<b);
    c = (a >>b);

    // Binary arithmetic, bitwise, and shift with assignment operators

    c +=a;
    c *=a;
    c -=a;
    c /=a;
    c %=a;
    c &=a;
    c |=a;
    c ^=a;
    c <<=a;
    c >>=a;

    // Binary logical operators

    result = (a &&b);
    result = (a ||b);

    // Binary comparison operators

    result = (a ==b);
    result = (a !=b);
    result = (a <=b);
    result = (a >=b);
    result = (a <b);
    result = (a >b);
}

// Multiple spaces on the left side of binary operators

void test_2_2(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a  =1;
    b  =1;
    c  =1;

    // Binary arithmetic operators
    
    c = (a  +b);
    c = (a  *b);
    c = (a  -b);
    c = (a  /b);
    c = (a  %b);

    // Binary bitwise operators

    c = (a  &b);
    c = (a  |b);
    c = (a  ^b);

    // Binary shift (or stream) operators

    c = (a  <<b);
    c = (a  >>b);

    // Binary arithmetic, bitwise, and shift with assignment operators

    c  +=a;
    c  *=a;
    c  -=a;
    c  /=a;
    c  %=a;
    c  &=a;
    c  |=a;
    c  ^=a;
    c  <<=a;
    c  >>=a;

    // Binary logical operators

    result = (a  &&b);
    result = (a  ||b);

    // Binary comparison operators

    result = (a  ==b);
    result = (a  !=b);
    result = (a  <=b);
    result = (a  >=b);
    result = (a  <b);
    result = (a  >b);
}

// Single space on the right side of binary operators

void test_3_1(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a= 1;
    b= 1;
    c= 1;

    // Binary arithmetic operators
    
    c = (a+ b);
    c = (a* b);
    c = (a- b);
    c = (a/ b);
    c = (a% b);

    // Binary bitwise operators

    c = (a& b);
    c = (a| b);
    c = (a^ b);

    // Binary shift (or stream) operators

    c = (a<< b);
    c = (a>> b);

    // Binary arithmetic, bitwise, and shift with assignment operators

    c+= a;
    c*= a;
    c-= a;
    c/= a;
    c%= a;
    c&= a;
    c|= a;
    c^= a;
    c<<= a;
    c>>= a;

    // Binary logical operators

    result = (a&& b);
    result = (a|| b);

    // Binary comparison operators

    result = (a== b);
    result = (a!= b);
    result = (a<= b);
    result = (a>= b);
    result = (a< b);
    result = (a> b);
}

// Multiple spaces on the right side of binary operators

void test_3_2(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a=  1;
    b=  1;
    c=  1;

    // Binary arithmetic operators
    
    c = (a+  b);
    c = (a*  b);
    c = (a-  b);
    c = (a/  b);
    c = (a%  b);

    // Binary bitwise operators

    c = (a&  b);
    c = (a|  b);
    c = (a^  b);

    // Binary shift (or stream) operators

    c = (a<<  b);
    c = (a>>  b);

    // Binary arithmetic, bitwise, and shift with assignment operators

    c+=  a;
    c*=  a;
    c-=  a;
    c/=  a;
    c%=  a;
    c&=  a;
    c|=  a;
    c^=  a;
    c<<=  a;
    c>>=  a;

    // Binary logical operators

    result = (a&&  b);
    result = (a||  b);

    // Binary comparison operators

    result = (a==  b);
    result = (a!=  b);
    result = (a<=  b);
    result = (a>=  b);
    result = (a<  b);
    result = (a>  b);
}

// Single tab on the left side of binary operators

void test_4_1(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a	=1;
    b	=1;
    c	=1;

    // Binary arithmetic operators
    
    c = (a	+b);
    c = (a	*b);
    c = (a	-b);
    c = (a	/b);
    c = (a	%b);

    // Binary bitwise operators

    c = (a	&b);
    c = (a	|b);
    c = (a	^b);

    // Binary shift (or stream) operators

    c = (a	<<b);
    c = (a	>>b);

    // Binary arithmetic, bitwise, and shift with assignment operators

    c	+=a;
    c	*=a;
    c	-=a;
    c	/=a;
    c	%=a;
    c	&=a;
    c	|=a;
    c	^=a;
    c	<<=a;
    c	>>=a;

    // Binary logical operators

    result = (a	&&b);
    result = (a	||b);

    // Binary comparison operators

    result = (a	==b);
    result = (a	!=b);
    result = (a	<=b);
    result = (a	>=b);
    result = (a	<b);
    result = (a	>b);
}

// Multiple tabs on the left side of binary operators

void test_4_2(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a		=1;
    b		=1;
    c		=1;

    // Binary arithmetic operators
    
    c = (a		+b);
    c = (a		*b);
    c = (a		-b);
    c = (a		/b);
    c = (a		%b);

    // Binary bitwise operators

    c = (a		&b);
    c = (a		|b);
    c = (a		^b);

    // Binary shift (or stream) operators

    c = (a		<<b);
    c = (a		>>b);

    // Binary arithmetic, bitwise, and shift with assignment operators

    c		+=a;
    c		*=a;
    c		-=a;
    c		/=a;
    c		%=a;
    c		&=a;
    c		|=a;
    c		^=a;
    c		<<=a;
    c		>>=a;

    // Binary logical operators

    result = (a		&&b);
    result = (a		||b);

    // Binary comparison operators

    result = (a		==b);
    result = (a		!=b);
    result = (a		<=b);
    result = (a		>=b);
    result = (a		<b);
    result = (a		>b);
}

// Single tab on the right side of binary operators

void test_5_1(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a=	1;
    b=	1;
    c=	1;

    // Binary arithmetic operators
    
    c = (a+	b);
    c = (a*	b);
    c = (a-	b);
    c = (a/	b);
    c = (a%	b);

    // Binary bitwise operators

    c = (a&	b);
    c = (a|	b);
    c = (a^	b);

    // Binary shift (or stream) operators

    c = (a<<	b);
    c = (a>>	b);

    // Binary arithmetic, bitwise, and shift with assignment operators

    c+=	a;
    c*=	a;
    c-=	a;
    c/=	a;
    c%=	a;
    c&=	a;
    c|=	a;
    c^=	a;
    c<<=	a;
    c>>=	a;

    // Binary logical operators

    result = (a&&	b);
    result = (a||	b);

    // Binary comparison operators

    result = (a==	b);
    result = (a!=	b);
    result = (a<=	b);
    result = (a>=	b);
    result = (a<	b);
    result = (a>	b);
}

// Multiple tabs on the right side of binary operators

void test_5_2(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a=		1;
    b=		1;
    c=		1;

    // Binary arithmetic operators
    
    c = (a+		b);
    c = (a*		b);
    c = (a-		b);
    c = (a/		b);
    c = (a%		b);

    // Binary bitwise operators

    c = (a&		b);
    c = (a|		b);
    c = (a^		b);

    // Binary shift (or stream) operators

    c = (a<<		b);
    c = (a>>		b);

    // Binary arithmetic, bitwise, and shift with assignment operators

    c+=		a;
    c*=		a;
    c-=		a;
    c/=		a;
    c%=		a;
    c&=		a;
    c|=		a;
    c^=		a;
    c<<=		a;
    c>>=		a;

    // Binary logical operators

    result = (a&&		b);
    result = (a||		b);

    // Binary comparison operators

    result = (a==		b);
    result = (a!=		b);
    result = (a<=		b);
    result = (a>=		b);
    result = (a<		b);
    result = (a>		b);
}

// Parenthesized left and right arguments

void test_6(void)
{
    int32_t   a, b, c;
    bool      result;
    int32_t * p;

    // Things that should be caught

    // Binary assignment operator

    a=(1);
    b=(1);
    c=(1);

    // Binary arithmetic operators
    
    c = ((a)+(b));
    c = ((a)*(b));
    c = ((a)-(b));
    c = ((a)/(b));
    c = ((a)%(b));

    // Binary bitwise operators

    c = ((a)&(b));
    c = ((a)|(b));
    c = ((a)^(b));

    // Binary shift (or stream) operators

    c = ((a)<<(b));
    c = ((a)>>(b));

    // Binary arithmetic, bitwise, and shift with assignment operators

    c+=(a);
    c*=(a);
    c-=(a);
    c/=(a);
    c%=(a);
    c&=(a);
    c|=(a);
    c^=(a);
    c<<=(a);
    c>>=(a);

    // Binary logical operators

    result = ((a)&&(b));
    result = ((a)||(b));

    // Binary comparison operators

    result = ((a)==(b));
    result = ((a)!=(b));
    result = ((a)<=(b));
    result = ((a)>=(b));
    result = ((a)<(b));
    result = ((a)>(b));
}

// Positive tests: default parameters

extern int test_7_1(int a=5);
extern int test_7_2(int a = 5+2);
extern int test_7_3(int a = 5-2);
extern int test_7_4(int a = 5*2);
extern int test_7_5(int a = 5/2);
extern int test_7_6(int a = 5%2);
extern int test_7_7(int a = 5&2);
extern int test_7_8(int a = 5|2);
extern int test_7_9(int a = 5^2);
extern int test_7_10(bool a = 5&&2);
extern int test_7_11(bool a = 5||2);
extern int test_7_12(int a = 5|2);
extern int test_7_13(int a = 5^2);
extern int test_7_14(int a = 5<<2);
extern int test_7_15(int a = 5>>2);
extern int test_7_16(bool a = 5==2);
extern int test_7_17(bool a = 5!=2);
extern int test_7_18(bool a = 5<2);
extern int test_7_19(bool a = 5>2);
extern int test_7_20(bool a = 5>=2);
extern int test_7_21(bool a = 5<=2);

// *STYLE-OFF*

// Negative tests: Things that shouldn't be caught

struct pointer_wrapper
{
    uint32_t *m_a;
    const uint32_t *m_b;
    volatile uint32_t *m_c;
    const volatile uint32_t *m_d;
    volatile const uint32_t *m_e;
};

struct reference_wrapper
{
    uint32_t &m_f;
    const uint32_t &m_g;
    volatile uint32_t &m_h;
    const volatile uint32_t &m_i;
    volatile const uint32_t &m_j;
};

struct pointer_reference_wrapper
{
    uint32_t * &m_f;
    const uint32_t * &m_g;
    volatile uint32_t * &m_h;
    const volatile uint32_t * &m_i;
    volatile const uint32_t * &m_j;
};

class test_class
{
public:
    test_class(int &a);
    test_class(void *a);
    ~test_class(void);
};

static int test_8(int a);

static int test_8(int a)
{
    return -a;
}

static int test_8_1(int a)
{
    return  -a;
}

static int test_8_2(int a)
{
    return-a;
}

static int test_8_3(int a)
{
    return(-a);
}

static int test_8_4(int a)
{
    return (-a);
}

static int g_a;

static void * test_9(void);

static void * test_9(void)
{
    return &g_a;
}

static void * test_9_1(void)
{
    return  &g_a;
}

static void * test_9_2(void)
{
    return&g_a;
}

static void * test_9_3(void)
{
    return(&g_a);
}

static void * test_9_4(void)
{
    return (&g_a);
}

static int test_10(int *a);

static int test_10(int *a)
{
    return *a;
}

static int test_10_1(int *a)
{
    return  *a;
}

static int test_10_2(int *a)
{
    return*a;
}

static int test_10_3(int *a)
{
    return(*a);
}

static int test_10_4(int *a)
{
    return (*a);
}

void test_11(void)
{
    int32_t                   a, b, c;
    int32_t                  &r = c;
    bool                      result;
    int32_t                  *p, **pp;
    struct pointer_wrapper    s, *sp;

    a = b = 1;

    // Things that shouldn't be caught;

    /* A tricky comment with a hyphenated-word */

    // Another tricky comment with a hyphenated-word

    typedef void (*functor)(void);   // Function typedef
    typedef int& int_ref;            // Reference type typedef
    typedef char foo_t;              // Abstract type typedef

    extern void foo(int32_t&a);
    extern void bar(int32_t&);
    extern int32_t foo(const foo_t *&a);
    extern int32_t bar(const foo_t*&a);
    extern void bar(int32_t *p);
    extern void foo(const foo_t * const *a);
    extern void bar(foo_t const * const *a);
    extern void foo(const foo_t * *a);
    extern void bar(foo_t const * *a);
    extern void foo(foo_t * *a);
    extern void foo(foo_t * const *a);
    extern void foo(volatile foo_t * volatile *a);
    extern void bar(foo_t volatile * volatile *a);
    extern void foo(volatile foo_t * *a);
    extern void bar(foo_t volatile * *a);
    extern void foo(foo_t * *a);
    extern void foo(foo_t * volatile *a);
    extern void foo(const volatile foo_t * const volatile *a);
    extern void bar(foo_t const volatile * const volatile *a);
    extern void foo(const volatile foo_t * *a);
    extern void bar(foo_t const volatile * *a);
    extern void foo(foo_t * *a);
    extern void foo(foo_t * const volatile *a);
    extern const foo_t *proto_1(const foo_t * const *a);
    extern const foo_t *proto_2(foo_t const * const *a);
    extern const foo_t *proto_1(const foo_t * *a);
    extern const foo_t *proto_2(foo_t const * *a);
    extern const foo_t *proto_1(foo_t * *a);
    extern const foo_t *proto_1(foo_t * const *a);
    extern const foo_t *proto_1(volatile foo_t * volatile *a);
    extern const foo_t *proto_2(foo_t volatile * volatile *a);
    extern const foo_t *proto_1(volatile foo_t * *a);
    extern const foo_t *proto_2(foo_t volatile * *a);
    extern const foo_t *proto_1(foo_t * *a);
    extern const foo_t *proto_1(foo_t * volatile *a);
    extern const foo_t *proto_1(const volatile foo_t * const volatile *a);
    extern const foo_t *proto_2(foo_t const volatile * const volatile *a);
    extern const foo_t *proto_1(const volatile foo_t * *a);
    extern const foo_t *proto_2(foo_t const volatile * *a);
    extern const foo_t *proto_1(foo_t * *a);
    extern const foo_t *proto_1(foo_t * const volatile *a);
    extern volatile foo_t *proto_3(const foo_t * const *a);
    extern volatile foo_t *proto_4(foo_t const * const *a);
    extern volatile foo_t *proto_3(const foo_t * *a);
    extern volatile foo_t *proto_4(foo_t const * *a);
    extern volatile foo_t *proto_3(foo_t * *a);
    extern volatile foo_t *proto_3(foo_t * const *a);
    extern volatile foo_t *proto_3(volatile foo_t * volatile *a);
    extern volatile foo_t *proto_4(foo_t volatile * volatile *a);
    extern volatile foo_t *proto_3(volatile foo_t * *a);
    extern volatile foo_t *proto_4(foo_t volatile * *a);
    extern volatile foo_t *proto_3(foo_t * *a);
    extern volatile foo_t *proto_3(foo_t * volatile *a);
    extern volatile foo_t *proto_3(const volatile foo_t * const volatile *a);
    extern volatile foo_t *proto_4(foo_t const volatile * const volatile *a);
    extern volatile foo_t *proto_3(const volatile foo_t * *a);
    extern volatile foo_t *proto_4(foo_t const volatile * *a);
    extern volatile foo_t *proto_3(foo_t * *a);
    extern volatile foo_t *proto_3(foo_t * const volatile *a);
    extern const volatile foo_t *proto_5(const foo_t * const *a);
    extern const volatile foo_t *proto_6(foo_t const * const *a);
    extern const volatile foo_t *proto_5(const foo_t * *a);
    extern const volatile foo_t *proto_6(foo_t const * *a);
    extern const volatile foo_t *proto_5(foo_t * *a);
    extern const volatile foo_t *proto_5(foo_t * const *a);
    extern const volatile foo_t *proto_5(volatile foo_t * volatile *a);
    extern const volatile foo_t *proto_6(foo_t volatile * volatile *a);
    extern const volatile foo_t *proto_5(volatile foo_t * *a);
    extern const volatile foo_t *proto_6(foo_t volatile * *a);
    extern const volatile foo_t *proto_5(foo_t * *a);
    extern const volatile foo_t *proto_5(foo_t * volatile *a);
    extern const volatile foo_t *proto_5(const volatile foo_t * const volatile *a);
    extern const volatile foo_t *proto_6(foo_t const volatile * const volatile *a);
    extern const volatile foo_t *proto_5(const volatile foo_t * *a);
    extern const volatile foo_t *proto_6(foo_t const volatile * *a);
    extern const volatile foo_t *proto_5(foo_t * *a);
    extern const volatile foo_t *proto_5(foo_t * const volatile *a);
    extern const foo_t * const *proto_7(const foo_t * const *a);
    extern const foo_t * const *proto_8(foo_t const * const *a);
    extern const foo_t * const *proto_7(const foo_t * *a);
    extern const foo_t * const *proto_8(foo_t const * *a);
    extern const foo_t * const *proto_7(foo_t * *a);
    extern const foo_t * const *proto_7(foo_t * const *a);
    extern const foo_t * const *proto_7(volatile foo_t * volatile *a);
    extern const foo_t * const *proto_8(foo_t volatile * volatile *a);
    extern const foo_t * const *proto_7(volatile foo_t * *a);
    extern const foo_t * const *proto_8(foo_t volatile * *a);
    extern const foo_t * const *proto_7(foo_t * *a);
    extern const foo_t * const *proto_7(foo_t * volatile *a);
    extern const foo_t * const *proto_7(const volatile foo_t * const volatile *a);
    extern const foo_t * const *proto_8(foo_t const volatile * const volatile *a);
    extern const foo_t * const *proto_7(const volatile foo_t * *a);
    extern const foo_t * const *proto_8(foo_t const volatile * *a);
    extern const foo_t * const *proto_7(foo_t * *a);
    extern const foo_t * const *proto_7(foo_t * const volatile *a);
    extern void proto_9(foo_t a[6]);

    p = &a;
    sp = &s;
    sp->m_a = (uint32_t*)p;
    sp->m_a = (uint32_t*)&b;

    result = (*p == a);
    c = *p;
    c = p[-a];
    c = -a;

    test_7_1(-1);

    result = (const_cast<const int32_t*>(p) == const_cast<const int32_t*>(p));

    result = (static_cast<uint32_t>(a) == static_cast<uint32_t>(b));

    result = (*reinterpret_cast<uint32_t*>(p) == static_cast<uint32_t>(b));

    a++;
    ++a;
    --a;
    a--;

    bar(&a);

    extern int operator+(test_class a, test_class b);
    extern int operator-(test_class a, test_class b);
    extern int operator*(test_class a, test_class b);
    extern int operator/(test_class a, test_class b);
    extern int operator%(test_class a, test_class b);
    extern int operator^(test_class a, test_class b);
    extern int operator&(test_class a, test_class b);
    extern int operator|(test_class a, test_class b);

    extern int operator&&(test_class a, test_class b);
    extern int operator||(test_class a, test_class b);

    extern int operator<(test_class a, test_class b);
    extern int operator>(test_class a, test_class b);
    extern int operator==(test_class a, test_class b);
    extern int operator!=(test_class a, test_class b);
    extern int operator<=(test_class a, test_class b);
    extern int operator>=(test_class a, test_class b);

    extern int operator+=(test_class a, test_class b);
    extern int operator-=(test_class a, test_class b);
    extern int operator*=(test_class a, test_class b);
    extern int operator/=(test_class a, test_class b);
    extern int operator%=(test_class a, test_class b);
    extern int operator^=(test_class a, test_class b);
    extern int operator&=(test_class a, test_class b);
    extern int operator|=(test_class a, test_class b);
    extern int operator<<(test_class a, test_class b);
    extern int operator>>(test_class a, test_class b);
    extern int operator>>=(test_class a, test_class b);
    extern int operator<<=(test_class a, test_class b);

    class test_class
    {
        int operator=(test_class a);
    };
}





