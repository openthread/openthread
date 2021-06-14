/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <test/helpers.h>
#include <test/macros.h>
#include <string.h>

#if defined(MBEDTLS_CHECK_PARAMS)
#include <setjmp.h>
#endif

/*----------------------------------------------------------------------------*/
/* Static global variables */

#if defined(MBEDTLS_CHECK_PARAMS)
typedef struct
{
    uint8_t expected_call;
    uint8_t expected_call_happened;

    jmp_buf state;

    mbedtls_test_param_failed_location_record_t location_record;
}
param_failed_ctx_t;
static param_failed_ctx_t param_failed_ctx;
#endif

#if defined(MBEDTLS_PLATFORM_C)
static mbedtls_platform_context platform_ctx;
#endif

/*----------------------------------------------------------------------------*/
/* Helper Functions */

int mbedtls_test_platform_setup( void )
{
    int ret = 0;
#if defined(MBEDTLS_PLATFORM_C)
    ret = mbedtls_platform_setup( &platform_ctx );
#endif /* MBEDTLS_PLATFORM_C */
    return( ret );
}

void mbedtls_test_platform_teardown( void )
{
#if defined(MBEDTLS_PLATFORM_C)
    mbedtls_platform_teardown( &platform_ctx );
#endif /* MBEDTLS_PLATFORM_C */
}

static int ascii2uc(const char c, unsigned char *uc)
{
    if( ( c >= '0' ) && ( c <= '9' ) )
        *uc = c - '0';
    else if( ( c >= 'a' ) && ( c <= 'f' ) )
        *uc = c - 'a' + 10;
    else if( ( c >= 'A' ) && ( c <= 'F' ) )
        *uc = c - 'A' + 10;
    else
        return( -1 );

    return( 0 );
}

int mbedtls_test_unhexify( unsigned char *obuf,
                           size_t obufmax,
                           const char *ibuf,
                           size_t *len )
{
    unsigned char uc, uc2;

    *len = strlen( ibuf );

    /* Must be even number of bytes. */
    if ( ( *len ) & 1 )
        return( -1 );
    *len /= 2;

    if ( (*len) > obufmax )
        return( -1 );

    while( *ibuf != 0 )
    {
        if ( ascii2uc( *(ibuf++), &uc ) != 0 )
            return( -1 );

        if ( ascii2uc( *(ibuf++), &uc2 ) != 0 )
            return( -1 );

        *(obuf++) = ( uc << 4 ) | uc2;
    }

    return( 0 );
}

void mbedtls_test_hexify( unsigned char *obuf,
                          const unsigned char *ibuf,
                          int len )
{
    unsigned char l, h;

    while( len != 0 )
    {
        h = *ibuf / 16;
        l = *ibuf % 16;

        if( h < 10 )
            *obuf++ = '0' + h;
        else
            *obuf++ = 'a' + h - 10;

        if( l < 10 )
            *obuf++ = '0' + l;
        else
            *obuf++ = 'a' + l - 10;

        ++ibuf;
        len--;
    }
}

unsigned char *mbedtls_test_zero_alloc( size_t len )
{
    void *p;
    size_t actual_len = ( len != 0 ) ? len : 1;

    p = mbedtls_calloc( 1, actual_len );
    TEST_HELPER_ASSERT( p != NULL );

    memset( p, 0x00, actual_len );

    return( p );
}

unsigned char *mbedtls_test_unhexify_alloc( const char *ibuf, size_t *olen )
{
    unsigned char *obuf;
    size_t len;

    *olen = strlen( ibuf ) / 2;

    if( *olen == 0 )
        return( mbedtls_test_zero_alloc( *olen ) );

    obuf = mbedtls_calloc( 1, *olen );
    TEST_HELPER_ASSERT( obuf != NULL );
    TEST_HELPER_ASSERT( mbedtls_test_unhexify( obuf, *olen, ibuf, &len ) == 0 );

    return( obuf );
}

int mbedtls_test_hexcmp( uint8_t * a, uint8_t * b,
                         uint32_t a_len, uint32_t b_len )
{
    int ret = 0;
    uint32_t i = 0;

    if( a_len != b_len )
        return( -1 );

    for( i = 0; i < a_len; i++ )
    {
        if( a[i] != b[i] )
        {
            ret = -1;
            break;
        }
    }
    return ret;
}

#if defined(MBEDTLS_CHECK_PARAMS)
void mbedtls_test_param_failed_get_location_record(
         mbedtls_test_param_failed_location_record_t *location_record )
{
    *location_record = param_failed_ctx.location_record;
}

void mbedtls_test_param_failed_expect_call( void )
{
    param_failed_ctx.expected_call_happened = 0;
    param_failed_ctx.expected_call = 1;
}

int mbedtls_test_param_failed_check_expected_call( void )
{
    param_failed_ctx.expected_call = 0;

    if( param_failed_ctx.expected_call_happened != 0 )
        return( 0 );

    return( -1 );
}

void* mbedtls_test_param_failed_get_state_buf( void )
{
    return &param_failed_ctx.state;
}

void mbedtls_test_param_failed_reset_state( void )
{
    memset( param_failed_ctx.state, 0, sizeof( param_failed_ctx.state ) );
}

void mbedtls_param_failed( const char *failure_condition,
                           const char *file,
                           int line )
{
    /* Record the location of the failure */
    param_failed_ctx.location_record.failure_condition = failure_condition;
    param_failed_ctx.location_record.file = file;
    param_failed_ctx.location_record.line = line;

    /* If we are testing the callback function...  */
    if( param_failed_ctx.expected_call != 0 )
    {
        param_failed_ctx.expected_call = 0;
        param_failed_ctx.expected_call_happened = 1;
    }
    else
    {
        /* ...else try a long jump. If the execution state has not been set-up
         * or reset then the long jump buffer is all zero's and the call will
         * with high probability fault, emphasizing there is something to look
         * at.
         */

        longjmp( param_failed_ctx.state, 1 );
    }
}
#endif /* MBEDTLS_CHECK_PARAMS */
