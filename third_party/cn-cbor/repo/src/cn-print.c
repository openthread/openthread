#ifndef CN_PRINT_C
#define CN_PRINT_C
#define CN_INCLUDE_DUMPER
#ifdef CN_INCLUDE_DUMPER
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif
#ifdef EMACS_INDENTATION_HELPER
} /* Duh. */
#endif

#include <stdio.h>
#ifdef MSV_CRT
#include <winsock2.h>
#else
#define _snprintf snprintf
#endif
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "cn-cbor/cn-cbor.h"
#include "cbor.h"

typedef struct _write_state
{
	char * rgbOutput;
	ssize_t ib;
	size_t cbLeft;
	uint8_t * rgFlags;
	const char * szIndentWith;
	const char * szEndOfLine;
} cn_write_state;

typedef void(*cn_visit_func)(const cn_cbor *cb, int depth, void *context);
extern void _visit(const cn_cbor *cb,
	cn_visit_func visitor,
	cn_visit_func breaker,
	void *context);

const char RgchHex[] = { '0', '1', '2', '3', '4', '5', '6', '7',
'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

bool _isWritable(cn_write_state * ws, size_t cb)
{
	if (ws->rgbOutput == NULL) return true;
	if ((ws->ib < 0) || (ws->ib + cb > ws->cbLeft)) {
		ws->ib = -1;
		return false;
	}
	return true;
}

void write_data(cn_write_state * ws, const char * sz, size_t cb)
{
	if (_isWritable(ws, cb)) {
		if (ws->rgbOutput != NULL) memcpy(ws->rgbOutput + ws->ib, sz, cb);
		ws->ib += cb;
	}
}

void _doIndent(cn_write_state * ws, int depth)
{
	int i;
	char * sz = ws->rgbOutput + ws->ib;
	size_t cbIndentWith = strlen(ws->szIndentWith);
	int cbIndent = depth * cbIndentWith;


	if (ws->rgbOutput == NULL) {
		ws->ib += cbIndent;
		return;
	}

	if (_isWritable(ws, cbIndent)) {
		for (i = 0; i < depth; i++) {
			memcpy(sz, ws->szIndentWith, cbIndentWith);
			sz += cbIndentWith;
		}
	}

	ws->ib += cbIndent;

	return;
}

void _print_encoder(const cn_cbor * cb, int depth, void * context)
{
	int i;
	char rgchT[256];
	int cch;
	cn_write_state * ws = (cn_write_state *)context;
	uint8_t flags = ws->rgFlags[depth];

	if (flags & 1) {
		write_data(ws, ", ", 2);
		ws->rgFlags[depth] &= 0xfe;

		if (ws->szIndentWith) {
			write_data(ws, ws->szEndOfLine, strlen(ws->szEndOfLine));
			_doIndent(ws, depth);
		}
	}

	if (flags & 2) {
		write_data(ws, ": ", 2);
		ws->rgFlags[depth] &= 0xfd;
	}

	switch (cb->type) {
	case CN_CBOR_BYTES_CHUNKED:
	case CN_CBOR_TEXT_CHUNKED:
	  break;

	case CN_CBOR_ARRAY:
		write_data(ws, "[", 1);
		ws->rgFlags[depth] |= 4;

		if (ws->szIndentWith) {
			write_data(ws, ws->szEndOfLine, strlen(ws->szEndOfLine));
			_doIndent(ws, depth + 1);
		}
		break;

	case CN_CBOR_MAP:
		write_data(ws, "{", 1);
		ws->rgFlags[depth] |= 8;

		if (ws->szIndentWith) {
			write_data(ws, ws->szEndOfLine, strlen(ws->szEndOfLine));
			_doIndent(ws, depth + 1);
		}
		break;

	case CN_CBOR_TAG:
	case CN_CBOR_UINT:
	case CN_CBOR_SIMPLE:
	  cch = _snprintf(rgchT, sizeof(rgchT), "%u", (unsigned int) cb->v.uint);
		write_data(ws, rgchT, cch);
		break;

	case CN_CBOR_FALSE:
		write_data(ws, "false", 5);
		break;

	case CN_CBOR_TRUE:
		write_data(ws, "true", 4);
		break;

	case CN_CBOR_NULL:
		write_data(ws, "null", 4);
		break;

	case CN_CBOR_UNDEF:
		write_data(ws, "undef", 5);
		break;

	case CN_CBOR_INT:
	  cch = _snprintf(rgchT, sizeof(rgchT), "%d", (unsigned int) cb->v.sint);
		write_data(ws, rgchT, cch);
		break;

	case CN_CBOR_DOUBLE:
		cch = _snprintf(rgchT, sizeof(rgchT), "%f", cb->v.dbl);
		write_data(ws, rgchT, cch);
		break;

	case CN_CBOR_INVALID:
		write_data(ws, "invalid", 7);
		break;

	case CN_CBOR_TEXT:
		write_data(ws, "\"", 1);
		write_data(ws, cb->v.str, cb->length);
		write_data(ws, "\"", 1);
		break;

	case CN_CBOR_BYTES:
		write_data(ws, "h'", 2);
		for (i = 0; i < cb->length; i++) {
			write_data(ws, &RgchHex[(cb->v.str[i] / 16) & 0xf], 1);
			write_data(ws, &RgchHex[cb->v.str[i] & 0xf], 1);
		}
		write_data(ws, "\'", 1);
		break;
	}

	if (depth > 0) {
		if (ws->rgFlags[depth - 1] & 4) ws->rgFlags[depth] |= 1;
		else if (ws->rgFlags[depth - 1] & 8) {
			if (flags & 2) ws->rgFlags[depth] |= 1;
			else ws->rgFlags[depth] |= 2;
		}
	}
}

void _print_breaker(const cn_cbor * cb, int depth, void * context)
{
	cn_write_state * ws = (cn_write_state *)context;

	switch (cb->type) {
	case CN_CBOR_ARRAY:
		if (ws->szIndentWith) {
			write_data(ws, ws->szEndOfLine, strlen(ws->szEndOfLine));
			_doIndent(ws, depth);
		}

		write_data(ws, "]", 1);
		ws->rgFlags[depth + 1] = 0;
		break;

	case CN_CBOR_MAP:
		if (ws->szIndentWith) {
			write_data(ws, ws->szEndOfLine, strlen(ws->szEndOfLine));
			_doIndent(ws, depth);
		}

		write_data(ws, "}", 1);
		ws->rgFlags[depth + 1] = 0;
		break;

	default:
	  break;
	}
}

ssize_t cn_cbor_printer_write(char * rgbBuffer, size_t cbBuffer, const cn_cbor * cb, const char * szIndentWith, const char * szEndOfLine)
{
	uint8_t flags[128] = { 0 };
	char rgchZero[1] = { 0 };

	cn_write_state ws = { rgbBuffer, 0, cbBuffer, flags, szIndentWith, szEndOfLine };
	_visit(cb, _print_encoder, _print_breaker, &ws);
	write_data(&ws, rgchZero, 1);

	return ws.ib;
}

#ifdef EMACS_INDENTATION_HELPER
{ /* Duh. */
#endif
#ifdef _cplusplus
}	/* extern "C" */
#endif

#endif // CN_INCLUDE_DUMPER
#endif // CN_PRINT_C

