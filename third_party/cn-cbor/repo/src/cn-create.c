#ifndef CN_CREATE_C
#define CN_CREATE_C

#ifdef  __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>

#include "dll-export.h"
#include "cn-cbor/cn-cbor.h"
#include "cbor.h"

#define INIT_CB(v) \
  if (errp) {errp->err = CN_CBOR_NO_ERROR;} \
  (v) = CN_CALLOC_CONTEXT(); \
  if (!(v)) { if (errp) {errp->err = CN_CBOR_ERR_OUT_OF_MEMORY;} return NULL; }

MYLIB_EXPORT
cn_cbor* cn_cbor_map_create(CBOR_CONTEXT_COMMA cn_cbor_errback *errp)
{
  cn_cbor* ret;
  INIT_CB(ret);

  ret->type = CN_CBOR_MAP;
  ret->flags |= CN_CBOR_FL_COUNT;

  return ret;
}

MYLIB_EXPORT
cn_cbor* cn_cbor_data_create(const uint8_t* data, int len
                             CBOR_CONTEXT,
                             cn_cbor_errback *errp)
{
  cn_cbor* ret;
  INIT_CB(ret);

  ret->type = CN_CBOR_BYTES;
  ret->length = len;
  ret->v.str = (const char*) data; // TODO: add v.ustr to the union?

  return ret;
}

MYLIB_EXPORT
cn_cbor* cn_cbor_string_create(const char* data
                               CBOR_CONTEXT,
                               cn_cbor_errback *errp)
{
  cn_cbor* ret;
  INIT_CB(ret);

  ret->type = CN_CBOR_TEXT;
  ret->length = strlen(data);
  ret->v.str = data;

  return ret;
}

MYLIB_EXPORT
cn_cbor* cn_cbor_int_create(int64_t value
                            CBOR_CONTEXT,
                            cn_cbor_errback *errp)
{
  cn_cbor* ret;
  INIT_CB(ret);

  if (value<0) {
    ret->type = CN_CBOR_INT;
    ret->v.sint = value;
  } else {
    ret->type = CN_CBOR_UINT;
    ret->v.uint = value;
  }

  return ret;
}

static bool _append_kv(cn_cbor *cb_map, cn_cbor *key, cn_cbor *val)
{
  //Connect key and value and insert them into the map.
  key->parent = cb_map;
  key->next = val;
  val->parent = cb_map;
  val->next = NULL;

  if(cb_map->last_child) {
    cb_map->last_child->next = key;
  } else {
    cb_map->first_child = key;
  }
  cb_map->last_child = val;
  cb_map->length += 2;
  return true;
}

MYLIB_EXPORT
bool cn_cbor_map_put(cn_cbor* cb_map,
                     cn_cbor *cb_key, cn_cbor *cb_value,
                     cn_cbor_errback *errp)
{
  //Make sure input is a map. Otherwise
  if(!cb_map || !cb_key || !cb_value || cb_map->type != CN_CBOR_MAP)
  {
    if (errp) {errp->err = CN_CBOR_ERR_INVALID_PARAMETER;}
    return false;
  }

  return _append_kv(cb_map, cb_key, cb_value);
}

MYLIB_EXPORT
bool cn_cbor_mapput_int(cn_cbor* cb_map,
                        int64_t key, cn_cbor* cb_value
                        CBOR_CONTEXT,
                        cn_cbor_errback *errp)
{
  cn_cbor* cb_key;

  //Make sure input is a map. Otherwise
  if(!cb_map || !cb_value || cb_map->type != CN_CBOR_MAP)
  {
    if (errp) {errp->err = CN_CBOR_ERR_INVALID_PARAMETER;}
    return false;
  }

  cb_key = cn_cbor_int_create(key CBOR_CONTEXT_PARAM, errp);
  if (!cb_key) { return false; }
  return _append_kv(cb_map, cb_key, cb_value);
}

MYLIB_EXPORT
bool cn_cbor_mapput_string(cn_cbor* cb_map,
                           const char* key, cn_cbor* cb_value
                           CBOR_CONTEXT,
                           cn_cbor_errback *errp)
{
  cn_cbor* cb_key;

  //Make sure input is a map. Otherwise
  if(!cb_map || !cb_value || cb_map->type != CN_CBOR_MAP)
  {
    if (errp) {errp->err = CN_CBOR_ERR_INVALID_PARAMETER;}
    return false;
  }

  cb_key = cn_cbor_string_create(key CBOR_CONTEXT_PARAM,  errp);
  if (!cb_key) { return false; }
  return _append_kv(cb_map, cb_key, cb_value);
}

MYLIB_EXPORT
cn_cbor* cn_cbor_array_create(CBOR_CONTEXT_COMMA cn_cbor_errback *errp)
{
  cn_cbor* ret;
  INIT_CB(ret);

  ret->type = CN_CBOR_ARRAY;
  ret->flags |= CN_CBOR_FL_COUNT;

  return ret;
}

MYLIB_EXPORT
bool cn_cbor_array_append(cn_cbor* cb_array,
                          cn_cbor* cb_value,
                          cn_cbor_errback *errp)
{
  //Make sure input is an array.
  if(!cb_array || !cb_value || cb_array->type != CN_CBOR_ARRAY)
  {
    if (errp) {errp->err = CN_CBOR_ERR_INVALID_PARAMETER;}
    return false;
  }

  cb_value->parent = cb_array;
  cb_value->next = NULL;
  if(cb_array->last_child) {
    cb_array->last_child->next = cb_value;
  } else {
    cb_array->first_child = cb_value;
  }
  cb_array->last_child = cb_value;
  cb_array->length++;
  return true;
}

#ifdef  __cplusplus
}
#endif

#endif  /* CN_CBOR_C */
