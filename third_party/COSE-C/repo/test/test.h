#ifndef _countof

#define _countof(x) (sizeof(x)/sizeof(x[0]))
#endif

#ifdef USE_CBOR_CONTEXT
cn_cbor_context * context;
#define CBOR_CONTEXT_PARAM , context
#define CBOR_CONTEXT_PARAM_COMMA context ,
#else
#define CBOR_CONTEXT_PARAM
#define CBOR_CONTEXT_PARAM_COMMA
#endif

//  encrypt.c

int ValidateEnveloped(const cn_cbor * pControl);
int EncryptMessage();
int BuildEnvelopedMessage(const cn_cbor * pControl);
int ValidateEncrypt(const cn_cbor * pControl);
int BuildEncryptMessage(const cn_cbor * pControl);
void Enveloped_Corners();
void Encrypt_Corners();
void Recipient_Corners();


//  sign.c

int ValidateSigned(const cn_cbor * pControl);
int SignMessage();
int BuildSignedMessage(const cn_cbor * pControl);
int ValidateSign0(const cn_cbor * pControl);
int BuildSign0Message(const cn_cbor * pControl);
void Sign_Corners();
void Sign0_Corners(const cn_cbor * pControl);


// mac_testc

int ValidateMAC(const cn_cbor * pControl);
int MacMessage();
int BuildMacMessage(const cn_cbor * pControl);
int ValidateMac0(const cn_cbor * pControl);
int BuildMac0Message(const cn_cbor * pControl);
void MAC_Corners();
void MAC0_Corners();

#ifdef USE_CBOR_CONTEXT
//  context.c
extern cn_cbor_context * CreateContext(unsigned int iFailPoint);
void FreeContext(cn_cbor_context* pContext);
#endif


//  test.c
enum {
	Attributes_MAC_protected=1,
	Attributes_MAC0_protected,
	Attributes_Recipient_protected,
	Attributes_Enveloped_protected,
	Attributes_Encrypt_protected,
	Attributes_Sign_protected,
	Attributes_Signer_protected,
	Attributes_Sign0_protected,
} whichSet;

extern int CFails;

int MapAlgorithmName(const cn_cbor * p);
byte * GetCBOREncoding(const cn_cbor * pControl, int * pcbEncoded);
//bool SetAttributes(HCOSE hHandle, const cn_cbor * pAttributes, int which, bool fPublicKey);
cn_cbor * BuildKey(const cn_cbor * pKeyIn, bool fPublicKey);
byte * FromHex(const char * rgch, int cch);
bool SetSendingAttributes(HCOSE hMsg, const cn_cbor * pIn, int base);
bool SetReceivingAttributes(HCOSE hMsg, const cn_cbor * pIn, int base);
int IsAlgorithmSupported(const cn_cbor * alg);

//
//  Internal macros to make testing easier
//

#define CHECK_RETURN(functionCall, errorReturn, onFailure) \
{	\
	if (!functionCall) onFailure; \
}

#define CHECK_FAILURE(functionCall, errorReturn, onFailure)       \
    { \
        bool bReturn = functionCall;  \
        if (!bReturn) { \
            if (cose_error.err != errorReturn) onFailure; \
        } else if (errorReturn != COSE_ERR_NONE) onFailure; \
    }

#define CHECK_FAILURE_PTR(functionCall, errorReturn, onFailure)       \
    { \
        void * bReturn = functionCall;  \
        if (bReturn == NULL) { \
            if (cose_error.err != errorReturn) onFailure; \
        } else if (errorReturn != COSE_ERR_NONE) onFailure; \
    }
