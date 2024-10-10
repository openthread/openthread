#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cn-cbor/cn-cbor.h>

#include "json.h"

#ifdef USE_CBOR_CONTEXT
extern cn_cbor_context * context;
#define CBOR_CONTEXT_PARAM , context
#define CBOR_CONTEXT_PARAM_COMMA context ,
#else
#define CBOR_CONTEXT_PARAM
#define CBOR_CONTEXT_PARAM_COMMA
#endif


const cn_cbor * ParseString(char * rgch, int ib, int cch)
{
	char ch;
	int ib2;
	cn_cbor * node = NULL;
	cn_cbor * parent = NULL;
	cn_cbor * root = NULL;

	for (; ib < cch; ib++) {
		node = NULL;
		ch = rgch[ib];
		switch (ch) {
		case '{':
			node = cn_cbor_map_create(CBOR_CONTEXT_PARAM_COMMA NULL);
			break;

		case '[':
			node = cn_cbor_array_create(CBOR_CONTEXT_PARAM_COMMA NULL);
			break;

		case '}':
		case ']':
			if (parent == NULL) {
				fprintf(stderr, "Parse failure @ '%s'\n", &rgch[ib]);
				return NULL;
			}
			parent = parent->parent;
			break;

		case ' ':
		case '\r':
		case '\n':
		case':':
		case ',':
			break;

		case '"':
			for (ib2 = ib + 1; ib2 < cch; ib2++) if (rgch[ib2] == '"') break;
			rgch[ib2] = 0;
			node = cn_cbor_string_create(&rgch[ib+1], CBOR_CONTEXT_PARAM_COMMA NULL);
			// rgch[ib2] = '"';
			ib = ib2;
			break;

		case't':
			if (strncmp(&rgch[ib], "true", 4) != 0) goto error;
			node = cn_cbor_data_create(NULL, 0, CBOR_CONTEXT_PARAM_COMMA NULL);
			node->type = CN_CBOR_TRUE;
			ib += 3;
			break;

		case'f':
			if (strncmp(&rgch[ib], "false", 5) != 0) goto error;
			node = cn_cbor_data_create(NULL, 0, CBOR_CONTEXT_PARAM_COMMA NULL);
			node->type = CN_CBOR_FALSE;
			ib += 4;
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case'9':
		case'-':
			node = cn_cbor_int_create(atol(&rgch[ib]), CBOR_CONTEXT_PARAM_COMMA NULL);
			if (rgch[ib] == '-') ib++;
			while (isdigit(rgch[ib])) ib++;
			ib--;
			break;

		default:
			error:
			fprintf(stderr, "Parse failure @ '%s'\n", &rgch[ib]);
			return NULL;
		}

		if ((node != NULL) && (parent != NULL)) {
			node->parent = parent;
			if (parent->last_child != NULL) {
				parent->last_child->next = node;
				parent->last_child = node;
			}
			else {
				parent->first_child = node;
			}
			parent->last_child = node;
			parent->length++;

			if ((node->type == CN_CBOR_MAP) || (node->type == CN_CBOR_ARRAY)) {
				parent = node;
			}
		}
		if (parent == NULL) {
			parent = node;
			if (root == NULL) root = node;
		}
	}

	return root;
}

const cn_cbor * ParseJson(const char * fileName)
{
	int     cch;
	char *	rgch;
    FILE * fp = fopen(fileName, "r");

	if (fp == NULL) {
		fprintf(stderr, "Cannot open file '%s'\n", fileName);
		
		return NULL;
	}

	rgch = malloc(8 * 1024);
	cch = (int) fread(rgch, 1, 8*1024, fp);
	fclose(fp);

	return ParseString(rgch, 0, cch);
}


extern void build_decoding_table();


static char encoding_table[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
'w', 'x', 'y', 'z', '0', '1', '2', '3',
'4', '5', '6', '7', '8', '9', '-', '_' };
static char *decoding_table = NULL;
static int mod_table[] = { 0, 2, 1 };


char *base64_encode(const unsigned char *data,
	size_t input_length,
	size_t *output_length) {

	*output_length = 4 * ((input_length + 2) / 3);

	char *encoded_data = malloc(*output_length);
	if (encoded_data == NULL) return NULL;

	for (size_t i = 0, j = 0; i < input_length;) {

		uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for (int i = 0; i < mod_table[input_length % 3]; i++)
		encoded_data[*output_length - 1 - i] = '=';

	return encoded_data;
}


unsigned char *base64_decode(const char *data,
	size_t input_length,
	size_t *output_length) {
	char * p = NULL;

	if (decoding_table == NULL) build_decoding_table();

	if (input_length % 4 != 0) {
		int c = 4 - (input_length % 4);
		p = malloc(input_length + c);
		memcpy(p, data, input_length);
		memcpy(p + input_length, "====", c);
		input_length += c;
		data = p;
	}

	*output_length = input_length / 4 * 3;
	if (data[input_length - 1] == '=') (*output_length)--;
	if (data[input_length - 2] == '=') (*output_length)--;

	unsigned char *decoded_data = malloc(*output_length);
	if (decoded_data == NULL) {
		if (p != NULL) free(p);
		return NULL;
	}

	for (unsigned int i = 0, j = 0; i < input_length;) {

		uint32_t sextet_a = data[i] == '=' ? 0 & i++ : (unsigned)decoding_table[(int) data[i++]];
		uint32_t sextet_b = data[i] == '=' ? 0 & i++ : (unsigned)decoding_table[(int) data[i++]];
		uint32_t sextet_c = data[i] == '=' ? 0 & i++ : (unsigned)decoding_table[(int) data[i++]];
		uint32_t sextet_d = data[i] == '=' ? 0 & i++ : (unsigned)decoding_table[(int) data[i++]];

		uint32_t triple = (sextet_a << 3 * 6)
			+ (sextet_b << 2 * 6)
			+ (sextet_c << 1 * 6)
			+ (sextet_d << 0 * 6);

		if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
		if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
		if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
	}

	free(p);
	return decoded_data;
}


void build_decoding_table() {

	decoding_table = malloc(256);

	for (int i = 0; i < 64; i++)
		decoding_table[(int) encoding_table[i]] = (char) i;
}


void base64_cleanup() {
	free(decoding_table);
}
