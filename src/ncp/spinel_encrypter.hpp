/**
 * Allows to encrypt spinel frames sent between Application Processor (AP) and Network Co-Processor (NCP).
 */

namespace SpinelEncrypter {

/**
 * Encrypts spinel frames before sending to AP/NCP.
 *
 * This method encrypts outbound frames in both directions, i.e. from AP to NCP and from NCP to AP.
 * NOTE: \p aOutputLength might be different than \p aInputLength, so make sure to allocate enough memory.
 *
 * @param[in] aInput Pointer to buffer containing spinel frame.
 * @param[in] aInputLength Length of spinel frame.
 * @param[out] aOutput Pointer to allocated buffer for encrypted spinel frame.
 * @param[in,out] aOutputLength Length of allocated buffer for encrypted spinel frame.
 * In return, it will contain the actual length of encrypted frame.
 * @return \c true on success, \c false otherwise.
 */
bool EncryptOutbound(const unsigned char *aInput, size_t aInputLength, unsigned char *aOutput, size_t *aOutputLength);

/**
 * Decrypts spinel frames by decrypting data received from AP/NCP.
 *
 * This method decrypts inbound frames in both directions, i.e. from AP to NCP and from NCP to AP.
 * @param[in] aInput Pointer to buffer containing received encrypted data.
 * @param[in] aInputLength Length of received encrypted data.
 * @param[out] aOutput Pointer to allocated buffer for decrypted spinel frame.
 * @param[in,out] aOutputLength Length of allocated buffer for decrypted spinel frame.
 * In return, it will contain the actual length of decrypted frame.
 * @return \c true on success, \c false otherwise.
 */
bool DecryptInbound(const unsigned char *aInput, size_t aInputLength, unsigned char *aOutput, size_t *aOutputLength);

} // namespace SpinelEncrypter
