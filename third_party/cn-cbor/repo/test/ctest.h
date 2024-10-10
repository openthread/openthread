/* Copyright 2011,2012 Bas van den Berg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CTEST_H
#define CTEST_H

#ifdef _MSC_VER
#define snprintf _snprintf
#pragma section(".ctest")

#include <io.h>
#define isatty _isatty
#endif

#ifndef UNUSED_PARAM
  /**
   * \def UNUSED_PARAM(p);
   *
   * A macro for quelling compiler warnings about unused variables.
   */
#  define UNUSED_PARAM(p) ((void)&(p))
#endif /* UNUSED_PARM */

typedef void (*SetupFunc)(void*);
typedef void (*TearDownFunc)(void*);

struct ctest {
    const char* ssname;  // suite name
    const char* ttname;  // test name
    void (*run)();
    int skip;

    void* data;
    SetupFunc setup;
    TearDownFunc teardown;

    unsigned int magic;
};

#define __FNAME(sname, tname) __ctest_##sname##_##tname##_run
#define __TNAME(sname, tname) __ctest_##sname##_##tname

#define __CTEST_MAGIC (0xdeadbeef)
#ifdef __APPLE__
#define __Test_Section __attribute__ ((unused,section ("__DATA, .ctest")))
#define MS__Test_Section
#else
#ifdef _MSC_VER
#define __Test_Section
#define MS__Test_Section __declspec(allocate(".ctest"))
#else
#define __Test_Section __attribute__ ((unused,section (".ctest")))
#define MS__Test_Section
#endif
#endif

#define __CTEST_STRUCT(sname, tname, _skip, __data, __setup, __teardown) \
	MS__Test_Section \
    struct ctest __TNAME(sname, tname) __Test_Section = { \
        .ssname=#sname, \
        .ttname=#tname, \
        .run = __FNAME(sname, tname), \
        .skip = _skip, \
        .data = __data, \
        .setup = (SetupFunc)__setup,					\
        .teardown = (TearDownFunc)__teardown,				\
        .magic = __CTEST_MAGIC };

#define CTEST_DATA(sname) struct sname##_data

#define CTEST_SETUP(sname) \
    void __attribute__ ((weak)) sname##_setup(struct sname##_data* data)

#define CTEST_TEARDOWN(sname) \
    void __attribute__ ((weak)) sname##_teardown(struct sname##_data* data)

#define __CTEST_INTERNAL(sname, tname, _skip) \
    void __FNAME(sname, tname)(); \
    __CTEST_STRUCT(sname, tname, _skip, NULL, NULL, NULL) \
    void __FNAME(sname, tname)()

#ifdef __APPLE__
#define SETUP_FNAME(sname) NULL
#define TEARDOWN_FNAME(sname) NULL
#else
#define SETUP_FNAME(sname) sname##_setup
#define TEARDOWN_FNAME(sname) sname##_teardown
#endif

#define __CTEST2_INTERNAL(sname, tname, _skip) \
    static struct sname##_data  __ctest_##sname##_data; \
    CTEST_SETUP(sname); \
    CTEST_TEARDOWN(sname); \
    void __FNAME(sname, tname)(struct sname##_data* data); \
    __CTEST_STRUCT(sname, tname, _skip, &__ctest_##sname##_data, SETUP_FNAME(sname), TEARDOWN_FNAME(sname)) \
    void __FNAME(sname, tname)(struct sname##_data* data)


void CTEST_LOG(char *fmt, ...);
void CTEST_ERR(char *fmt, ...);  // doesn't return

#define CTEST(sname, tname) __CTEST_INTERNAL(sname, tname, 0)
#define CTEST_SKIP(sname, tname) __CTEST_INTERNAL(sname, tname, 1)

#define CTEST2(sname, tname) __CTEST2_INTERNAL(sname, tname, 0)
#define CTEST2_SKIP(sname, tname) __CTEST2_INTERNAL(sname, tname, 1)


void assert_str(const char* exp, const char* real, const char* caller, int line);
#define ASSERT_STR(exp, real) assert_str(exp, real, __FILE__, __LINE__)

void assert_data(const unsigned char* exp, int expsize,
                 const unsigned char* real, int realsize,
                 const char* caller, int line);
#define ASSERT_DATA(exp, expsize, real, realsize) \
    assert_data(exp, expsize, real, realsize, __FILE__, __LINE__)

void assert_equal(long exp, long real, const char* caller, int line);
#define ASSERT_EQUAL(exp, real) assert_equal(exp, real, __FILE__, __LINE__)

void assert_not_equal(long exp, long real, const char* caller, int line);
#define ASSERT_NOT_EQUAL(exp, real) assert_not_equal(exp, real, __FILE__, __LINE__)

void assert_null(void* real, const char* caller, int line);
#define ASSERT_NULL(real) assert_null((void*)real, __FILE__, __LINE__)

void assert_not_null(const void* real, const char* caller, int line);
#define ASSERT_NOT_NULL(real) assert_not_null(real, __FILE__, __LINE__)

void assert_true(int real, const char* caller, int line);
#define ASSERT_TRUE(real) assert_true(real, __FILE__, __LINE__)

void assert_false(int real, const char* caller, int line);
#define ASSERT_FALSE(real) assert_false(real, __FILE__, __LINE__)

void assert_fail(const char* caller, int line);
#define ASSERT_FAIL() assert_fail(__FILE__, __LINE__)

#ifdef CTEST_MAIN

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <inttypes.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdint.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <dlfcn.h>
#endif

//#define COLOR_OK

static size_t ctest_errorsize;
static char* ctest_errormsg;
#define MSG_SIZE 4096
static char ctest_errorbuffer[MSG_SIZE];
static jmp_buf ctest_err;
static int color_output = 1;
static const char* suite_name;

typedef int (*filter_func)(struct ctest*);

#define ANSI_BLACK    "\033[0;30m"
#define ANSI_RED      "\033[0;31m"
#define ANSI_GREEN    "\033[0;32m"
#define ANSI_YELLOW   "\033[0;33m"
#define ANSI_BLUE     "\033[0;34m"
#define ANSI_MAGENTA  "\033[0;35m"
#define ANSI_CYAN     "\033[0;36m"
#define ANSI_GREY     "\033[0;37m"
#define ANSI_DARKGREY "\033[01;30m"
#define ANSI_BRED     "\033[01;31m"
#define ANSI_BGREEN   "\033[01;32m"
#define ANSI_BYELLOW  "\033[01;33m"
#define ANSI_BBLUE    "\033[01;34m"
#define ANSI_BMAGENTA "\033[01;35m"
#define ANSI_BCYAN    "\033[01;36m"
#define ANSI_WHITE    "\033[01;37m"
#define ANSI_NORMAL   "\033[0m"

static CTEST(suite, test) { }

static void msg_start(const char* color, const char* title) {
    int size;
    if (color_output) {
        size = snprintf(ctest_errormsg, ctest_errorsize, "%s", color);
        ctest_errorsize -= size;
        ctest_errormsg += size;
    }
    size = snprintf(ctest_errormsg, ctest_errorsize, "  %s: ", title);
    ctest_errorsize -= size;
    ctest_errormsg += size;
}

static void msg_end() {
    int size;
    if (color_output) {
        size = snprintf(ctest_errormsg, ctest_errorsize, ANSI_NORMAL);
        ctest_errorsize -= size;
        ctest_errormsg += size;
    }
    size = snprintf(ctest_errormsg, ctest_errorsize, "\n");
    ctest_errorsize -= size;
    ctest_errormsg += size;
}

void CTEST_LOG(char *fmt, ...)
{
    va_list argp;
    msg_start(ANSI_BLUE, "LOG");

    va_start(argp, fmt);
    int size = vsnprintf(ctest_errormsg, ctest_errorsize, fmt, argp);
    ctest_errorsize -= size;
    ctest_errormsg += size;
    va_end(argp);

    msg_end();
}

void CTEST_ERR(char *fmt, ...)
{
    va_list argp;
    msg_start(ANSI_YELLOW, "ERR");

    va_start(argp, fmt);
    int size = vsnprintf(ctest_errormsg, ctest_errorsize, fmt, argp);
    ctest_errorsize -= size;
    ctest_errormsg += size;
    va_end(argp);

    msg_end();
    longjmp(ctest_err, 1);
}

void assert_str(const char* exp, const char*  real, const char* caller, int line) {
    if ((exp == NULL && real != NULL) ||
        (exp != NULL && real == NULL) ||
        (exp && real && strcmp(exp, real) != 0)) {
        CTEST_ERR("%s:%d  expected '%s', got '%s'", caller, line, exp, real);
    }
}

void assert_data(const unsigned char* exp, int expsize,
                 const unsigned char* real, int realsize,
                 const char* caller, int line) {
    int i;
    if (expsize != realsize) {
        CTEST_ERR("%s:%d  expected %d bytes, got %d", caller, line, expsize, realsize);
    }
    for (i=0; i<expsize; i++) {
        if (exp[i] != real[i]) {
            CTEST_ERR("%s:%d expected 0x%02x at offset %d got 0x%02x",
                caller, line, exp[i], i, real[i]);
        }
    }
}

void assert_equal(long exp, long real, const char* caller, int line) {
    if (exp != real) {
        CTEST_ERR("%s:%d  expected %ld, got %ld", caller, line, exp, real);
    }
}

void assert_not_equal(long exp, long real, const char* caller, int line) {
    if ((exp) == (real)) {
        CTEST_ERR("%s:%d  should not be %ld", caller, line, real);
    }
}

void assert_null(void* real, const char* caller, int line) {
    if ((real) != NULL) {
        CTEST_ERR("%s:%d  should be NULL", caller, line);
    }
}

void assert_not_null(const void* real, const char* caller, int line) {
    if (real == NULL) {
        CTEST_ERR("%s:%d  should not be NULL", caller, line);
    }
}

void assert_true(int real, const char* caller, int line) {
    if ((real) == 0) {
        CTEST_ERR("%s:%d  should be true", caller, line);
    }
}

void assert_false(int real, const char* caller, int line) {
    if ((real) != 0) {
        CTEST_ERR("%s:%d  should be false", caller, line);
    }
}

void assert_fail(const char* caller, int line) {
    CTEST_ERR("%s:%d  shouldn't come here", caller, line);
}


static int suite_all(struct ctest* t) {
    UNUSED_PARAM(t);
    return 1;
}

static int suite_filter(struct ctest* t) {
    return strncmp(suite_name, t->ssname, strlen(suite_name)) == 0;
}

#ifdef _MSC_VER
int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}
#endif

static uint64_t getCurrentTime() {
    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t now64 = now.tv_sec;
    now64 *= 1000000;
    now64 += (now.tv_usec);
    return now64;
}

static void color_print(const char* color, const char* text) {
    if (color_output)
        printf("%s%s"ANSI_NORMAL"\n", color, text);
    else
        printf("%s\n", text);
}

#ifdef __APPLE__
static void *find_symbol(struct ctest *test, const char *fname)
{
    size_t len = strlen(test->ssname) + 1 + strlen(fname);
    char *symbol_name = (char *) malloc(len + 1);
    memset(symbol_name, 0, len + 1);
    snprintf(symbol_name, len + 1, "%s_%s", test->ssname, fname);

    //fprintf(stderr, ">>>> dlsym: loading %s\n", symbol_name);
    void *symbol = dlsym(RTLD_DEFAULT, symbol_name);
    if (!symbol) {
        //fprintf(stderr, ">>>> ERROR: %s\n", dlerror());
    }
    // returns NULL on error

    free(symbol_name);
    return symbol;
}
#endif

#ifdef CTEST_SEGFAULT
#include <signal.h>
static void sighandler(int signum)
{
    char msg[128];
    sprintf(msg, "[SIGNAL %d: %s]", signum, sys_siglist[signum]);
    color_print(ANSI_BRED, msg);
    fflush(stdout);

    /* "Unregister" the signal handler and send the signal back to the process
     * so it can terminate as expected */
    signal(signum, SIG_DFL);
    kill(getpid(), signum);
}
#endif

int ctest_main(int argc, const char *argv[])
{
    static int total = 0;
    static int num_ok = 0;
    static int num_fail = 0;
    static int num_skip = 0;
    static int index = 1;
    static filter_func filter = suite_all;

#ifdef CTEST_SEGFAULT
    signal(SIGSEGV, sighandler);
#endif

    if (argc == 2) {
        suite_name = argv[1];
        filter = suite_filter;
    }

    color_output = isatty(1);

    uint64_t t1 = getCurrentTime();

    struct ctest* ctest_begin = &__TNAME(suite, test);
    struct ctest* ctest_end = &__TNAME(suite, test);
    // find begin and end of section by comparing magics
    while (1) {
        struct ctest* t = ctest_begin-1;
        if (t->magic != __CTEST_MAGIC) break;
        ctest_begin--;
    }
    while (1) {
        struct ctest* t = ctest_end+1;
        if (t->magic != __CTEST_MAGIC) break;
        ctest_end++;
    }
    ctest_end++;    // end after last one

    static struct ctest* test;
    for (test = ctest_begin; test != ctest_end; test++) {
        if (test == &__ctest_suite_test) continue;
        if (filter(test)) total++;
    }

    for (test = ctest_begin; test != ctest_end; test++) {
        if (test == &__ctest_suite_test) continue;
        if (filter(test)) {
            ctest_errorbuffer[0] = 0;
            ctest_errorsize = MSG_SIZE-1;
            ctest_errormsg = ctest_errorbuffer;
            printf("TEST %d/%d %s:%s ", index, total, test->ssname, test->ttname);
            fflush(stdout);
            if (test->skip) {
                color_print(ANSI_BYELLOW, "[SKIPPED]");
                num_skip++;
            } else {
                int result = setjmp(ctest_err);
                if (result == 0) {
#ifdef __APPLE__
                    if (!test->setup) {
                        test->setup = (SetupFunc)find_symbol(test, "setup");
                    }
                    if (!test->teardown) {
                        test->teardown = (SetupFunc)find_symbol(test, "teardown");
                    }
#endif

                    if (test->setup) test->setup(test->data);
                    if (test->data)
                      test->run(test->data);
                    else
                      test->run();
                    if (test->teardown) test->teardown(test->data);
                    // if we got here it's ok
#ifdef COLOR_OK
                    color_print(ANSI_BGREEN, "[OK]");
#else
                    printf("[OK]\n");
#endif
                    num_ok++;
                } else {
                    color_print(ANSI_BRED, "[FAIL]");
                    num_fail++;
                }
                if (ctest_errorsize != MSG_SIZE-1) printf("%s", ctest_errorbuffer);
            }
            index++;
        }
    }
    uint64_t t2 = getCurrentTime();

    const char* color = (num_fail) ? ANSI_BRED : ANSI_GREEN;
    char results[80];
    sprintf(results, "RESULTS: %d tests (%d ok, %d failed, %d skipped) ran in %"PRIu64" ms", total, num_ok, num_fail, num_skip, (t2 - t1)/1000);
    color_print(color, results);
    return num_fail;
}

#endif

#endif
