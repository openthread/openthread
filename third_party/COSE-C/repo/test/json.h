#pragma once

extern const cn_cbor * ParseJson(const char * fileName);

extern unsigned char *base64_decode(const char *data,	size_t input_length,	size_t *output_length);
