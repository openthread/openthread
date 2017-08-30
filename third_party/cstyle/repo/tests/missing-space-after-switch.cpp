extern void describe(const char *aDescription);

void test_1(int aArgument)
{
    switch(aArgument) {

    case 0:
        describe("The value is zero");
        break;

    case 1:
        describe("The value is odd");
        break;
        
    case 2:
        describe("The value is even");
        break;

    }
}

void test_2(int aArgument)
{
    switch(aArgument)
    {

    case 0:
        describe("The value is zero");
        break;

    case 1:
        describe("The value is odd");
        break;
        
    case 2:
        describe("The value is even");
        break;

    }
}

void test_3(int aArgument)
{
    switch(aArgument) { case 0: describe("The value is zero"); break;
                        case 1: describe("The value is odd");  break;
                        case 2: describe("The value is even"); break; }
}
