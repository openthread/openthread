#ifndef CN_ENCODER_C
#define CN_ENCODER_C

#ifdef  __cplusplus
extern "C" {
#endif
#ifdef EMACS_INDENTATION_HELPER
} /* Duh. */
#endif

#ifdef _MSC_VER
#include <WinSock2.h>
#define inline _inline
#else
//#include <arpa/inet.h>
#endif
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif
#include <stdbool.h>
#include <assert.h>

#include "dll-export.h"
#include "cn-cbor/cn-cbor.h"
#include "cbor.h"

#define hton8p(p) (*(uint8_t*)(p))
#define hton16p(p) (htons(*(uint16_t*)(p)))
#define hton32p(p) (htonl(*(uint32_t*)(p)))
static uint64_t hton64p(const uint8_t *p) {
  /* TODO: does this work on both BE and LE systems? */
  uint64_t ret = hton32p(p);
  ret <<= 32;
  ret |= hton32p(p+4);
  return ret;
}

typedef struct _write_state
{
  uint8_t *buf;
  ssize_t offset;
  ssize_t size;
} cn_write_state;

#define ensure_writable(sz) if ((ws->buf != NULL)  && ((ws->offset<0) || (ws->offset + (sz) > ws->size))) { \
  ws->offset = -1; \
  return; \
}

#define write_byte_and_data(b, data, sz) \
if(ws->buf) { \
  ws->buf[ws->offset++] = (b); \
  memcpy(ws->buf+ws->offset, (data), (sz)); \
} else { \
  ws->offset++; \
} \
ws->offset += sz;

#define write_byte(b) \
if(ws->buf) { \
  ws->buf[ws->offset++] = (b); \
} else { \
  ws->offset++; \
}

#define write_byte_ensured(b) \
ensure_writable(1); \
write_byte(b);

static uint8_t _xlate[] = {
  IB_FALSE,    /* CN_CBOR_FALSE */
  IB_TRUE,     /* CN_CBOR_TRUE */
  IB_NIL,      /* CN_CBOR_NULL */
  IB_UNDEF,    /* CN_CBOR_UNDEF */
  IB_UNSIGNED, /* CN_CBOR_UINT */
  IB_NEGATIVE, /* CN_CBOR_INT */
  IB_BYTES,    /* CN_CBOR_BYTES */
  IB_TEXT,     /* CN_CBOR_TEXT */
  IB_BYTES,    /* CN_CBOR_BYTES_CHUNKED */
  IB_TEXT,     /* CN_CBOR_TEXT_CHUNKED */
  IB_ARRAY,    /* CN_CBOR_ARRAY */
  IB_MAP,      /* CN_CBOR_MAP */
  IB_TAG,      /* CN_CBOR_TAG */
  IB_PRIM,     /* CN_CBOR_SIMPLE */
  0xFF,        /* CN_CBOR_DOUBLE */
  0xFF         /* CN_CBOR_INVALID */
};

static inline bool is_indefinite(const cn_cbor *cb)
{
  return (cb->flags & CN_CBOR_FL_INDEF) != 0;
}

static void _write_positive(cn_write_state *ws, cn_cbor_type typ, uint64_t val) {
  uint8_t ib;

  assert((size_t)typ < sizeof(_xlate));

  ib = _xlate[typ];
  if (ib == 0xFF) {
    ws->offset = -1;
    return;
  }

  if (val < 24) {
    ensure_writable(1);
    write_byte(ib | (uint8_t) val);
  } else if (val < 256) {
    ensure_writable(2);
    write_byte(ib | 24);
    write_byte((uint8_t)val);
  } else if (val < 65536) {
    uint16_t be16 = (uint16_t)val;
    ensure_writable(3);
    be16 = hton16p(&be16);
    write_byte_and_data(ib | 25, (const void*)&be16, 2);
  } else if (val < 0x100000000L) {
    uint32_t be32 = (uint32_t)val;
    ensure_writable(5);
    be32 = hton32p(&be32);
    write_byte_and_data(ib | 26, (const void*)&be32, 4);
  } else {
    uint64_t be64;
    ensure_writable(9);
    be64 = hton64p((const uint8_t*)&val);
    write_byte_and_data(ib | 27, (const void*)&be64, 8);
  }
}

#ifndef CBOR_NO_FLOAT
static void _write_double(cn_write_state *ws, double val)
{
  float float_val = val;
  if (float_val == val) {                /* 32 bits is enough and we aren't NaN */
    uint32_t be32;
    uint16_t be16, u16;
    union {
      float f;
      uint32_t u;
    } u32;
    u32.f = float_val;
    if ((u32.u & 0x1FFF) == 0) { /* worth trying half */
      int s16 = (u32.u >> 16) & 0x8000;
      int exp = (u32.u >> 23) & 0xff;
      int mant = u32.u & 0x7fffff;
      if (exp == 0 && mant == 0)
        ;              /* 0.0, -0.0 */
      else if (exp >= 113 && exp <= 142) /* normalized */
        s16 += ((exp - 112) << 10) + (mant >> 13);
      else if (exp >= 103 && exp < 113) { /* denorm, exp16 = 0 */
        if (mant & ((1 << (126 - exp)) - 1))
          goto float32;         /* loss of precision */
        s16 += ((mant + 0x800000) >> (126 - exp));
      } else if (exp == 255 && mant == 0) { /* Inf */
        s16 += 0x7c00;
      } else
        goto float32;           /* loss of range */

      ensure_writable(3);
      u16 = s16;
      be16 = hton16p((const uint8_t*)&u16);

      write_byte_and_data(IB_PRIM | 25, (const void*)&be16, 2);
      return;
    }
  float32:
    ensure_writable(5);
    be32 = hton32p((const uint8_t*)&u32.u);

    write_byte_and_data(IB_PRIM | 26, (const void*)&be32, 4);

  } else if (val != val) {      /* NaN -- we always write a half NaN*/
    ensure_writable(3);
    write_byte_and_data(IB_PRIM | 25, (const void*)"\x7e\x00", 2);
  } else {
    uint64_t be64;
    /* Copy the same problematic implementation from the decoder. */
    union {
      double d;
      uint64_t u;
    } u64;

    u64.d = val;

    ensure_writable(9);
    be64 = hton64p((const uint8_t*)&u64.u);

    write_byte_and_data(IB_PRIM | 27, (const void*)&be64, 8);

  }
}
#endif /* CBOR_NO_FLOAT */

// TODO: make public?
typedef void (*cn_visit_func)(const cn_cbor *cb, int depth, void *context);
void _visit(const cn_cbor *cb,
                   cn_visit_func visitor,
                   cn_visit_func breaker,
                   void *context)
{
    const cn_cbor *p = cb;
    int depth = 0;
    while (p)
    {
visit:
      visitor(p, depth, context);
      if (p->first_child) {
        p = p->first_child;
        depth++;
      } else{
        // Empty indefinite
#ifdef CN_INCLUDE_DUMPER
          breaker(p, depth, context);
#else
        if (is_indefinite(p)) {
          breaker(p, depth, context);
        }
#endif
        if (p->next) {
          p = p->next;
        } else {
          while (p->parent) {
            depth--;
#ifdef CN_INCLUDE_DUMPER
            breaker(p->parent, depth, context);
#else
            if (is_indefinite(p->parent)) {
              breaker(p->parent, depth, context);
            }
#endif
            if (p->parent->next) {
              p = p->parent->next;
              goto visit;
            }
            p = p->parent;
          }
          return;
        }
      }
    }
}

#define CHECK(st) (st); \
if (ws->offset < 0) { return; }

void _encoder_visitor(const cn_cbor *cb, int depth, void *context)
{
  cn_write_state *ws = context;
  UNUSED_PARAM(depth);

  switch (cb->type) {
  case CN_CBOR_ARRAY:
    if (is_indefinite(cb)) {
      write_byte_ensured(IB_ARRAY | AI_INDEF);
    } else {
      CHECK(_write_positive(ws, CN_CBOR_ARRAY, cb->length));
    }
    break;
  case CN_CBOR_MAP:
    if (is_indefinite(cb)) {
      write_byte_ensured(IB_MAP | AI_INDEF);
    } else {
      CHECK(_write_positive(ws, CN_CBOR_MAP, cb->length/2));
    }
    break;
  case CN_CBOR_BYTES_CHUNKED:
  case CN_CBOR_TEXT_CHUNKED:
    write_byte_ensured(_xlate[cb->type] | AI_INDEF);
    break;

  case CN_CBOR_TEXT:
  case CN_CBOR_BYTES:
    CHECK(_write_positive(ws, cb->type, cb->length));
    ensure_writable(cb->length);
    if (ws->buf) {
      memcpy(ws->buf+ws->offset, cb->v.str, cb->length);
    }
    ws->offset += cb->length;
    break;

  case CN_CBOR_FALSE:
  case CN_CBOR_TRUE:
  case CN_CBOR_NULL:
  case CN_CBOR_UNDEF:
    write_byte_ensured(_xlate[cb->type]);
    break;

  case CN_CBOR_TAG:
  case CN_CBOR_UINT:
  case CN_CBOR_SIMPLE:
    CHECK(_write_positive(ws, cb->type, cb->v.uint));
    break;

  case CN_CBOR_INT:
    assert(cb->v.sint < 0);
    CHECK(_write_positive(ws, CN_CBOR_INT, ~(cb->v.sint)));
    break;

  case CN_CBOR_DOUBLE:
#ifndef CBOR_NO_FLOAT
    CHECK(_write_double(ws, cb->v.dbl));
#endif /* CBOR_NO_FLOAT */
    break;

  case CN_CBOR_INVALID:
    ws->offset = -1;
    break;
  }
}

void _encoder_breaker(const cn_cbor *cb, int depth, void *context)
{
  cn_write_state *ws = context;
  UNUSED_PARAM(cb);
  UNUSED_PARAM(depth);
#ifdef CN_INCLUDE_DUMPER
  if (is_indefinite(cb)) {
#endif
      write_byte_ensured(IB_BREAK);
#ifdef CN_INCLUDE_DUMPER
  }
#endif
}

ssize_t cn_cbor_encoder_write(uint8_t *buf,
			      size_t buf_offset,
			      size_t buf_size,
			      const cn_cbor *cb)
{
  cn_write_state ws = { buf, buf_offset, buf_size };
  if (!ws.buf && ws.size <= 0) { ws.size = (ssize_t)(((size_t)-1) / 2); }
  _visit(cb, _encoder_visitor, _encoder_breaker, &ws);
  if (ws.offset < 0) { return -1; }
  return ws.offset - buf_offset;
}

#ifdef  __cplusplus
}
#endif

#endif  /* CN_CBOR_C */
