extern void foo(void);

void test_1(int aArgument)
{
    if(aArgument % 2 != 0) foo();
}

void test_2(int aArgument)
{
    if(aArgument % 2 != 0) { foo(); }
}

void test_3(int aArgument)
{
    if(aArgument % 2 != 0) {
        foo();
    }
}

void test_4(int aArgument)
{
    if(aArgument % 2 != 0)
    {
        foo();
    }
}
