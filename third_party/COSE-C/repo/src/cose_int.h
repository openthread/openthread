#ifndef __COSE_INT_H__
#define __COSE_INT_H__
#include <assert.h>

// These definitions are here because they aren't required for the public
// interface, and they were quite confusing in cn-cbor.h

#ifdef USE_COUNTER_SIGNATURES
struct _COSE_COUNTER_SIGN;
typedef struct _COSE_COUNTER_SIGN COSE_CounterSign;
#endif

#define UNUSED(x) ((void) (x))

typedef struct _COSE {
	COSE_INIT_FLAGS m_flags;		//  Not sure what goes here yet
	int m_ownMsg;		//  Do I own the pointer @ m_cbor?
	int m_ownUnprotectedMap; //  Do I own the pointer @ m_unportectedMap?
	int m_msgType;		//  What message type is this?
	int m_refCount;			//  Allocator Reference Counting.
	cn_cbor * m_cbor;
	cn_cbor * m_cborRoot;
	cn_cbor * m_protectedMap;
	cn_cbor * m_unprotectMap;
	cn_cbor * m_dontSendMap;
	const byte * m_pbExternal;
	size_t m_cbExternal;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context m_allocContext;
#endif
	struct _COSE * m_handleList;
#ifdef USE_COUNTER_SIGNATURES
	COSE_CounterSign * m_counterSigners;
#endif
} COSE;

struct _SignerInfo;
typedef struct _SignerInfo COSE_SignerInfo;

typedef struct {
	COSE m_message;	    // The message object
	COSE_SignerInfo * m_signerFirst;
} COSE_SignMessage;

typedef struct {
	COSE m_message;	    // The message object
} COSE_Sign0Message;

struct _SignerInfo {
	COSE m_message;
	const cn_cbor * m_pkey;
	COSE_SignerInfo * m_signerNext;
};

struct _RecipientInfo;
typedef struct _RecipientInfo COSE_RecipientInfo;

#if 0
typedef struct {
	COSE m_message;		// The message object
	const byte * pbContent;
	size_t cbContent;
} COSE_Encrypt;
#endif 

typedef struct {
	COSE m_message;		// The message object
	const byte * pbContent;
	size_t cbContent;
	COSE_RecipientInfo * m_recipientFirst;
} COSE_Enveloped;

typedef COSE_Enveloped COSE_Encrypt;

struct _RecipientInfo {
	COSE_Enveloped m_encrypt;
	COSE_RecipientInfo * m_recipientNext;
	const cn_cbor * m_pkey;
	const cn_cbor * m_pkeyStatic;
};

typedef struct {
	COSE m_message;			// The message object
	COSE_RecipientInfo * m_recipientFirst;
} COSE_MacMessage;

#if 0
typedef struct {
	COSE m_message;			// The message object
} COSE_Mac0Message;
#endif
typedef COSE_MacMessage COSE_Mac0Message;

#ifdef USE_COUNTER_SIGNATURES
typedef struct _COSE_COUNTER_SIGN {
	COSE_SignerInfo m_signer;
	COSE_CounterSign * m_next;
} COSE_CounterSign;
#endif

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
*  Allocate space required
*
* @param[in]	ctx  The allocation context, or NULL for normal calloc.
* @param[in]	count	Number of items to allocate
* @param[in]	size	Size of item to allocate
* @return				A pointer to the object needed
*/
#define COSE_CALLOC(count, size, ctx) ((((ctx)) && ((ctx)->calloc_func)) ? \
	((ctx)->calloc_func(count, size, (ctx)->context)) : \
	calloc(count, size))

/**
* Free a
* @param  free_func [description]
* @return           [description]
*/
#define COSE_FREE(ptr, ctx) ((((ctx) && (ctx)->free_func)) ? \
    ((ctx)->free_func((ptr), (ctx)->context)) : \
    free((ptr)))

#define CBOR_CONTEXT_PARAM , context
#define CBOR_CONTEXT_PARAM_COMMA context ,
//#define CN_CALLOC_CONTEXT() CN_CALLOC(context)
#define CN_CBOR_FREE(p, context) cn_cbor_free(p, context)

#else

#define CBOR_CONTEXT_PARAM
#define CBOR_CONTEXT_PARAM_COMMA
#define CN_CALLOC_CONTEXT() CN_CALLOC
#define COSE_CALLOC(count, size, ctx) calloc(count, size)
#define CN_CBOR_FREE(p, context) cn_cbor_free(p)



#define COSE_FREE(ptr, ctx) free(ptr)

#endif // USE_CBOR_CONTEXT

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(p) ((void)&(p))
#endif

extern cose_error _MapFromCBOR(cn_cbor_errback err);

/*
 *  Set of routines for handle checking
 */

extern void _COSE_InsertInList(COSE ** rootNode, COSE * newMsg);
extern bool _COSE_IsInList(COSE * rootNode, COSE * thisMsg);
extern void _COSE_RemoveFromList(COSE ** rootNode, COSE * thisMsg);

extern bool IsValidEncryptHandle(HCOSE_ENCRYPT h);
extern bool IsValidEnvelopedHandle(HCOSE_ENVELOPED h);
extern bool IsValidRecipientHandle(HCOSE_RECIPIENT h);
extern bool IsValidSignerHandle(HCOSE_SIGNER h);
extern bool IsValidCounterSignHandle(HCOSE_COUNTERSIGN h);

extern bool _COSE_Init(COSE_INIT_FLAGS flags, COSE * pcose, int msgType, CBOR_CONTEXT_COMMA cose_errback * errp);
extern bool _COSE_Init_From_Object(COSE* pobj, cn_cbor * pcbor, CBOR_CONTEXT_COMMA cose_errback * perror);
extern void _COSE_Release(COSE * pcose);

extern cn_cbor * _COSE_map_get_string(COSE * cose, const char * key, int flags, cose_errback * errp);
extern cn_cbor * _COSE_map_get_int(COSE * cose, int key, int flags, cose_errback * errp);
extern bool _COSE_map_put(COSE * cose, int key, cn_cbor * value, int flags, cose_errback * errp);

bool _COSE_SetExternal(COSE * hcose, const byte * pbExternalData, size_t cbExternalData, cose_errback * perr);


extern HCOSE_ENVELOPED _COSE_Enveloped_Init_From_Object(cn_cbor *, COSE_Enveloped * pIn, CBOR_CONTEXT_COMMA cose_errback * errp);
extern void _COSE_Enveloped_Release(COSE_Enveloped * p);
extern bool _COSE_Enveloped_decrypt(COSE_Enveloped * pcose, COSE_RecipientInfo * pRecip, const byte *pbKeyIn, size_t cbKeyIn, const char * szContext, cose_errback * perr);
extern bool _COSE_Enveloped_encrypt(COSE_Enveloped * pcose, const byte * pbKeyIn, size_t cbKeyIn, const char * szContext, cose_errback * perr);
extern bool _COSE_Enveloped_SetContent(COSE_Enveloped * cose, const byte * rgbContent, size_t cbContent, cose_errback * errp);

extern HCOSE_ENCRYPT _COSE_Encrypt_Init_From_Object(cn_cbor *, COSE_Encrypt * pIn, CBOR_CONTEXT_COMMA cose_errback * errp);
extern void _COSE_Encrypt_Release(COSE_Encrypt * p);
extern bool _COSE_Encrypt_SetContent(COSE_Encrypt * cose, const byte * rgbContent, size_t cbContent, cose_errback * errp);
extern bool _COSE_Encrypt_Build_AAD(COSE * pMessage, byte ** ppbAAD, size_t * pcbAAD, const char * szContext, cose_errback * perr);


extern COSE_RecipientInfo * _COSE_Recipient_Init_From_Object(cn_cbor *, CBOR_CONTEXT_COMMA cose_errback * errp);
extern void _COSE_Recipient_Free(COSE_RecipientInfo *);
extern bool _COSE_Recipient_decrypt(COSE_RecipientInfo * pRecip, COSE_RecipientInfo * pRecipUse, int algIn, size_t cbitKey, byte * pbKey, cose_errback * errp);
extern bool _COSE_Recipient_encrypt(COSE_RecipientInfo * pRecipient, const byte * pbContent, size_t cbContent, cose_errback * perr);
extern byte * _COSE_RecipientInfo_generateKey(COSE_RecipientInfo * pRecipient, int algIn, size_t cbitKeySize, cose_errback * perr);


//  Signed items
extern HCOSE_SIGN _COSE_Sign_Init_From_Object(cn_cbor *, COSE_SignMessage * pIn, CBOR_CONTEXT_COMMA cose_errback * errp);
extern void _COSE_Sign_Release(COSE_SignMessage * p);

//  Signer items

extern bool _COSE_SignerInfo_Init(COSE_INIT_FLAGS flags, COSE_SignerInfo * pcose, int msgType, CBOR_CONTEXT_COMMA cose_errback * errp);
extern bool _COSE_Signer_sign(COSE_SignerInfo * pSigner, const cn_cbor * pcborBody, const cn_cbor * pcborProtected, cose_errback * perr);
extern COSE_SignerInfo * _COSE_SignerInfo_Init_From_Object(cn_cbor * cbor, COSE_SignerInfo * pIn, CBOR_CONTEXT_COMMA cose_errback * perr);
extern bool _COSE_SignerInfo_Free(COSE_SignerInfo * pSigner);
extern bool _COSE_Signer_validate(COSE_SignMessage * pSign, COSE_SignerInfo * pSigner, const cn_cbor * pbContent, const cn_cbor * pbProtected, cose_errback * perr);


// Sign0 items
extern HCOSE_SIGN0 _COSE_Sign0_Init_From_Object(cn_cbor * cbor, COSE_Sign0Message * pIn, CBOR_CONTEXT_COMMA cose_errback * perr);
extern void _COSE_Sign0_Release(COSE_Sign0Message * p);

//  Mac-ed items
extern HCOSE_MAC _COSE_Mac_Init_From_Object(cn_cbor *, COSE_MacMessage * pIn, CBOR_CONTEXT_COMMA cose_errback * errp);
extern bool _COSE_Mac_Release(COSE_MacMessage * p);
extern bool _COSE_Mac_Build_AAD(COSE * pCose, const char * szContext, byte ** ppbAuthData, size_t * pcbAuthData, CBOR_CONTEXT_COMMA cose_errback * perr);
extern bool _COSE_Mac_compute(COSE_MacMessage * pcose, const byte * pbKeyIn, size_t cbKeyIn, const char * szContext, cose_errback * perr);
extern bool _COSE_Mac_validate(COSE_MacMessage * pcose, COSE_RecipientInfo * pRecip, const byte * pbKeyIn, size_t cbKeyIn, const char * szContext, cose_errback * perr);

//  MAC0 Items
extern HCOSE_MAC0 _COSE_Mac0_Init_From_Object(cn_cbor *, COSE_Mac0Message * pIn, CBOR_CONTEXT_COMMA cose_errback * errp);
extern bool _COSE_Mac0_Release(COSE_Mac0Message * p);

//  Counter Sign Items
extern HCOSE_COUNTERSIGN _COSE_CounterSign_get(COSE * pMessage, int iSigner, cose_errback * perr);
extern bool _COSE_CounterSign_add(COSE * pMessage, HCOSE_COUNTERSIGN hSigner, cose_errback * perr);
extern bool _COSE_CountSign_create(COSE * pMessage, cn_cbor * pcnBody, CBOR_CONTEXT_COMMA cose_errback * perr);

//
//  Debugging Items

//#define DO_ASSERT assert(false);
#define DO_ASSERT
#define CHECK_CONDITION(condition, error) { if (!(condition)) { DO_ASSERT; if (perr != NULL) {perr->err = error;} goto errorReturn;}}
#define FAIL_CONDITION(error) { DO_ASSERT; if (perr != NULL) {perr->err = error;} goto errorReturn;}
#define CHECK_CONDITION_CBOR(condition, error) { if (!(condition)) { DO_ASSERT; if (perr != NULL) {perr->err = _MapFromCBOR(error);} goto errorReturn;}}

extern cn_cbor * _COSE_encode_protected(COSE * pMessage, cose_errback * perr);


//// Defines on positions

#define INDEX_PROTECTED 0
#define INDEX_UNPROTECTED 1
#define INDEX_BODY 2
#define INDEX_SIGNERS 3
#define INDEX_RECIPIENTS 3
#define INDEX_MAC_TAG 3
#define INDEX_MAC_RECIPIENTS 4
#define INDEX_SIGNATURE 2

//// Defines on message types

#define COSE_Header_Protected 99
#define COSE_Header_Unprotected 98
#define COSE_Header_Type 97
#define COSE_Header_Ciphertext 96
#define COSE_Header_Recipients 95
#define COSE_Header_Signature 94
#define COSE_Header_Signers 93


bool _COSE_array_replace(COSE * pMessage, cn_cbor * cb_value, int index, CBOR_CONTEXT_COMMA cn_cbor_errback * errp);
cn_cbor * _COSE_arrayget_int(COSE * pMessage, int index);

///  NEW CBOR FUNCTIONS

bool cn_cbor_array_replace(cn_cbor * cb_array, cn_cbor * cb_value, int index, CBOR_CONTEXT_COMMA cn_cbor_errback *errp);
cn_cbor * cn_cbor_bool_create(int boolValue, CBOR_CONTEXT_COMMA cn_cbor_errback * errp);

extern size_t cn_cbor_encode_size(cn_cbor * object);

enum {
	COSE_Int_Alg_AES_CBC_MAC_256_64 = -22
};

// KEY
void eckey_release(eckey_t * eckey);
bool eckey_from_cbor(eckey_t * eckey, const cn_cbor * pKey, cose_errback * perr);

#define COSE_CounterSign_object 1000
#endif // __COSE_INT_H__
