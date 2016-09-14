
#include <SDKDDKVer.h>
#include "CppUnitTest.h"
#include "test_util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#pragma region Test Declarations

// test_aes.cpp
void TestMacBeaconFrame();
void TestMacDataFrame();
void TestMacCommandFrame();

#pragma endregion

utAssertTrue s_AssertTrue;

namespace UnitTests
{		
	TEST_CLASS(UnitTest1)
	{
	public:

        UnitTest1() { s_AssertTrue = AssertTrue; }

        // Helper for openthread test code to call
        static void AssertTrue(bool condition, const wchar_t* message)
        {
            Assert::IsTrue(condition, message);
        }

        // test_aes.cpp
        TEST_METHOD(TestMacBeaconFrame) { ::TestMacBeaconFrame(); }
        TEST_METHOD(TestMacDataFrame) { ::TestMacDataFrame(); }
        TEST_METHOD(TestMacCommandFrame) { ::TestMacCommandFrame(); }
	};
}