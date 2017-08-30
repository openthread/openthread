extern void foo(void);

void test_1(void)
{
    do { foo(); } while(true);
}

void test_2(void)
{
    do {
        foo();
    } while(true);
}

void test_3(void)
{
    do
    {
        foo();
    }
    while(true);
}

void test_4(void)
{
    do foo(); while(true);
}

void test_5(void)
{
    while(true) { foo(); }
}

void test_6(void)
{
    while(true) {
        foo();
    }
}

void test_7(void)
{
    while(true)
    {
        foo();
    }
}

void test_8(void)
{
    while(true) foo();
}
