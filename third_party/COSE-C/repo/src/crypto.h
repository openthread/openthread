/**
* Perform an AES-CCM Decryption operation
*
* @param[in]   COSE_Enveloped Pointer to COSE Encryption context object
* @param[in]   int          Size of the Tag value to be create
* @param[in]   int          Size of the Message Length field
* @param[in]   byte *       Pointer to authenticated data structure
* @param[in]   int          Size of authenticated data structure
* @return                   Did the function succeed?
*/
bool AES_CCM_Decrypt(COSE_Enveloped * pcose, int TSize, int LSize, const byte * pbKey, size_t cbitKey, const byte * pbCrypto, size_t cbCrypto, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr);
bool AES_GCM_Decrypt(COSE_Enveloped * pcose, const byte * pbKey, size_t cbKey, const byte * pbCrypto, size_t cbCrypto, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr);
bool AES_KW_Decrypt(COSE_Enveloped * pcose, const byte * pbKeyIn, size_t cbitKey, const byte * pbCipherText, size_t cbCipherText, byte * pbKeyOut, int * pcbKeyOut, cose_errback * perr);

/**
* Perform an AES-CCM Encryption operation
*
* @param[in]   COSE_Enveloped Pointer to COSE Encryption context object
* @param[in]   int          Size of the Tag value to be create
* @param[in]   int          Size of the Message Length field
* @param[in]   byte *       Pointer to authenticated data structure
* @param[in]   int          Size of authenticated data structure
* @return                   Did the function succeed?
*/
bool AES_CCM_Encrypt(COSE_Enveloped * pcose, int TSize, int LSize, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr);
bool AES_GCM_Encrypt(COSE_Enveloped * pcose, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr);
bool AES_KW_Encrypt(COSE_RecipientInfo * pcose, const byte * pbKeyIn, int cbitKey, const byte *  pbContent, int  cbContent, cose_errback * perr);


extern bool AES_CMAC_Validate(COSE_MacMessage * pcose, int KeySize, int TagSize, const byte * pbKey, int cbitKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr);

extern bool AES_CBC_MAC_Create(COSE_MacMessage * pcose, int TagSize, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr);
extern bool AES_CBC_MAC_Validate(COSE_MacMessage * pcose, int TagSize, const byte * pbKey, size_t cbitKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr);

/**
* Perform an HMAC Creation operation
*
* @param[in]	COSE_Enveloped	Pointer to COSE Encryption context object
* @param[in]	int				Hash function to be used
* @param[in]	int				Size of Tag value to be created
* @param[in]	byte *			Pointer to authenticated data structure
* @param[in]	int				Size of authenticated data structure
* @param[in]	cose_errback *	Error return location
* @return						Did the function succeed?
*/
bool HMAC_Create(COSE_MacMessage * pcose, int HSize, int TSize, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr);
bool HMAC_Validate(COSE_MacMessage * pcose, int HSize, int TSize, const byte * pbKey, size_t cbitKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr);

bool HKDF_Extract(COSE * pcose, const byte * pbKey, size_t cbKey, size_t cbitDigest, byte * rgbDigest, size_t * pcbDigest, cose_errback * perr);
bool HKDF_Expand(COSE * pcose, size_t cbitDigest, const byte * pbPRK, size_t cbPRK, const byte * pbInfo, size_t cbInfo, byte * pbOutput, size_t cbOutput, cose_errback * perr);

bool HKDF_AES_Expand(COSE * pcose, size_t cbitKey, const byte * pbPRK, size_t cbPRK, const byte * pbInfo, size_t cbInfo, byte * pbOutput, size_t cbOutput, cose_errback * perr);

/**
* Perform a signature operation
*
* @param[in]	COSE_SignerInfo Pointer to COSE SignerInfo context object
* @param[in]	byte *			Pointer to text to be signed
* @param[in]	size_t			size of text to be signed
* @param[in]	cose_errback *	Error return location
* @return						Did the function succeed?
*/
bool ECDSA_Sign(COSE * pSigner, int index, const eckey_t * eckey, int cbitDigest, const byte * rgbToSign, size_t cbToSign, cose_errback * perr);
bool ECDSA_Verify(COSE * pSigner, int index, const eckey_t * eckey, int cbitsDigest, const byte * rgbToSign, size_t cbToSign, cose_errback * perr);

bool ECDH_ComputeSecret(COSE * pReciient, cn_cbor ** ppKeyMe, const cn_cbor * pKeyYou, byte ** ppbSecret, size_t * pcbSecret, CBOR_CONTEXT_COMMA cose_errback *perr);

/**
*  Generate random bytes in a buffer
*
* @param[in]   byte *      Pointer to buffer to be filled
* @param[in]   size_t      Size of buffer to be filled
* @return                  none
*/
void rand_bytes(byte * pb, size_t cb);
