/**
 * Allows to encrypt spinel frames sent between Application Processor (AP) and Network Co-Processor (NCP).
 */

namespace SpinelEncrypter {

/**
 * Encrypts spinel frames before sending to AP/NCP.
 *
 * This method encrypts outbound frames in both directions, i.e. from AP to NCP and from NCP to AP.
 *
 * @param[in,out] aFrameBuf Pointer to buffer containing the frame, also where the encrypted frame will be placed.
 * @param[in] aFrameSize Max number of bytes in frame buffer (max length of spinel frame + additional data for encryption).
 * @param[in,out] aFrameLength Pointer to store frame length, on input value is set to frame length,
 * on output changed to show the frame length after encryption.
 * @return \c true on success, \c false otherwise.
 */
bool EncryptOutbound(unsigned char *aFrameBuf, size_t aFrameSize, size_t *aFrameLength);

/**
 * Decrypts spinel frames received from AP/NCP.
 *
 * This method decrypts inbound frames in both directions, i.e. from AP to NCP and from NCP to AP.
 *
 * @param[in,out] aFrameBuf Pointer to buffer containing encrypted frame, also where the decrypted frame will be placed.
 * @param[in] aFrameSize Max number of bytes in frame buffer (max length of spinel frame + additional data for encryption).
 * @param[in,out] aFrameLength Pointer to store frame length, on input value is set to encrypted frame length,
 * on output changed to show the frame length after decryption.
 * @return \c true on success, \c false otherwise.
 */
bool DecryptInbound(unsigned char *aFrameBuf, size_t aFrameSize, size_t *aFrameLength);

} // namespace SpinelEncrypter
