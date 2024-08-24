#ifndef CBOR_PROTOCOL_H__
#define CBOR_PROTOCOL_H__

#if BYTE_ORDER_BIG_ENDIAN
/* The host byte order is the same as network byte order,
   so these functions are all just identity.  */
#define ntohl(x)    (x)
#define ntohs(x)    (x)
#define htonl(x)    (x)
#define htons(x)    (x)
#else
#define swap16(x) (((x & 0xff00) >> 8) | ((x & 0x00ff) << 8))
#define swap32(x) (((x & 0xff000000) >> 24) | ((x & 0x00ff0000) >> 8) | ((x & 0x0000ff00) << 8) | ((x & 0x000000ff) << 24))
#define ntohl(x)  swap32(x)
#define ntohs(x)  swap16(x)
#define htonl(x)  swap32(x)
#define htons(x)  swap16(x)
#endif

#ifdef _MSC_VER
#define inline _inline
#endif

/* The 8 major types */
#define MT_UNSIGNED 0
#define MT_NEGATIVE 1
#define MT_BYTES    2
#define MT_TEXT     3
#define MT_ARRAY    4
#define MT_MAP      5
#define MT_TAG      6
#define MT_PRIM     7

/* The initial bytes resulting from those */
#define IB_UNSIGNED (MT_UNSIGNED << 5)
#define IB_NEGATIVE (MT_NEGATIVE << 5)
#define IB_BYTES    (MT_BYTES << 5)
#define IB_TEXT     (MT_TEXT << 5)
#define IB_ARRAY    (MT_ARRAY << 5)
#define IB_MAP      (MT_MAP << 5)
#define IB_TAG      (MT_TAG << 5)
#define IB_PRIM     (MT_PRIM << 5)

#define IB_NEGFLAG (IB_NEGATIVE - IB_UNSIGNED)
#define IB_NEGFLAG_AS_BIT(ib) ((ib) >> 5)
#define IB_TEXTFLAG (IB_TEXT - IB_BYTES)

#define IB_AI(ib) ((ib) & 0x1F)
#define IB_MT(ib) ((ib) >> 5)

/* Tag numbers handled by this implementation */
#define TAG_TIME_EPOCH 1
#define TAG_BIGNUM     2
#define TAG_BIGNUM_NEG 3
#define TAG_URI        32
#define TAG_RE         35

/* Initial bytes of those tag numbers */
#define IB_TIME_EPOCH (IB_TAG | TAG_TIME_EPOCH)
#define IB_BIGNUM     (IB_TAG | TAG_BIGNUM)
#define IB_BIGNUM_NEG (IB_TAG | TAG_BIGNUM_NEG)
/* TAG_URI and TAG_RE are non-immediate tags */

/* Simple values handled by this implementation */
#define VAL_FALSE 20
#define VAL_TRUE  21
#define VAL_NIL   22
#define VAL_UNDEF 23

/* Initial bytes of those simple values */
#define IB_FALSE (IB_PRIM | VAL_FALSE)
#define IB_TRUE  (IB_PRIM | VAL_TRUE)
#define IB_NIL   (IB_PRIM | VAL_NIL)
#define IB_UNDEF (IB_PRIM | VAL_UNDEF)

/* AI values with more data in head */
#define AI_1 24
#define AI_2 25
#define AI_4 26
#define AI_8 27
#define AI_INDEF 31
#define IB_BREAK (IB_PRIM | AI_INDEF)
/* For  */
#define IB_UNUSED (IB_TAG | AI_INDEF)

/* Floating point initial bytes */
#define IB_FLOAT2 (IB_PRIM | AI_2)
#define IB_FLOAT4 (IB_PRIM | AI_4)
#define IB_FLOAT8 (IB_PRIM | AI_8)

// These definitions are here because they aren't required for the public
// interface, and they were quite confusing in cn-cbor.h

#ifdef USE_CBOR_CONTEXT
/**
 * Allocate enough space for 1 `cn_cbor` structure.
 *
 * @param[in]  ctx  The allocation context, or NULL for calloc.
 * @return          A pointer to a `cn_cbor` or NULL on failure
 */
#define CN_CALLOC(ctx) ((ctx) && (ctx)->calloc_func) ? \
    (ctx)->calloc_func(1, sizeof(cn_cbor), (ctx)->context) : \
    calloc(1, sizeof(cn_cbor));

/**
 * Free a
 * @param  free_func [description]
 * @return           [description]
 */
#define CN_FREE(ptr, ctx) ((ctx) && (ctx)->free_func) ? \
    (ctx)->free_func((ptr), (ctx)->context) : \
    free((ptr));

#define CBOR_CONTEXT_PARAM , context
#define CN_CALLOC_CONTEXT() CN_CALLOC(context)
#define CN_CBOR_FREE_CONTEXT(p) CN_FREE(p, context)

#else

#define CBOR_CONTEXT_PARAM
#define CN_CALLOC_CONTEXT() CN_CALLOC
#define CN_CBOR_FREE_CONTEXT(p) CN_FREE(p)

#ifndef CN_CALLOC
#define CN_CALLOC calloc(1, sizeof(cn_cbor))
#endif

#ifndef CN_FREE
#define CN_FREE free
#endif

#endif // USE_CBOR_CONTEXT

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(p) ((void)&(p))
#endif

#endif // CBOR_PROTOCOL_H__
