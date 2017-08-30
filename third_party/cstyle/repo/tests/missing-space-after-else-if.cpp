extern void foo(void);
extern void bar(void);

void test_1(int aArgument)
{
    if(aArgument % 2 != 0) foo(); else if(aArgument == 0) bar();
}

void test_2(int aArgument)
{
    if(aArgument % 2 != 0) { foo(); } else if(aArgument == 0) { bar(); }
}

void test_3(int aArgument)
{
    if(aArgument % 2 != 0) {
        foo();
    } else if(aArgument == 0) {
        bar();
    }
}

void test_4(int aArgument)
{
    if(aArgument % 2 != 0)
    {
        foo();
    }
    else if(aArgument == 0)
    {
        bar();
    }
}
