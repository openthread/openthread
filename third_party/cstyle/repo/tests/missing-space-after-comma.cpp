#include <inttypes.h>
#include <stdint.h>

// Comments should not generate violations

// This comment,a single line comment,is missing a space after its commas.

// Single-quoted commas should not generate a violation

static char comma_char_1 = ',';

// Double-quoted commas should not generate a violation

static const char * const comma_string_1 = ",";
static const char * const comma_string_2 = "tag,value";
static const char * const comma_string_3 = "\"tag\",\"value\"";
static const char * const comma_string_4 = "'tag','value'";
static const char * const comma_string_5 = "size=\"17,11\"";

// Single-quoted escaped single quotes with commas should not generate a violation

static wchar_t charsets[] =
    {
        L'\'', L'<', L'>', L'?', L',', L'.',  L'/',  L'`', L'Ä', L'Ö', L'Ü', L'Ç', L'Ñ', L'É', L'À', L'Å', L'Æ', L'~'
    };

/*
 * This comment,a multi- line comment,
 * is missing a space after its commas.
 * However, only the one that is not at the
 * end of the line should be flagged.
 */

static int array_1[] = {0,1,2,3,4,5};	// These should generate a violation
static int array_2[] = {0,		// End of line, these should not 
                        1,		// generate a violation.
                        2,
                        3,
                        4,
                        5};

extern void foo(void);

void test_1(int m, int n)
{
    for (int i = 0,j = 0; i < m && j < n; i++,j++) {
        foo();
    }
}

class test_2
{
public:
    test_2(void) : m_a(0),m_b(0) { return; }
    test_2(int a, int b) :
        m_a(a),
        m_b(b)
    {
        return;
    }

private:
    int m_a;
    int m_b;
};
