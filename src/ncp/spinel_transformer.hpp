/**
 * NCP Spinel Transformer allows to transform spinel frames sent between Application Processor (AP) and Network Co-Processor (NCP).
 */

namespace SpinelTransformer {

/**
 * Transforms spinel frames before sending to AP/NCP.
 *
 * This method transforms outbound frames in both directions, i.e. from AP to NCP and from NCP to AP.
 * NOTE: \p aOutputLength might be different than \p aInputLength, so make sure to allocate enough memory.
 *
 * @param[in] aInput Pointer to buffer containing spinel frame.
 * @param[in] aInputLength Length of spinel frame.
 * @param[out] aOutput Pointer to allocated buffer for transformed spinel frame.
 * @param[in,out] aOutputLength Length of allocated buffer for transformed spinel frame.
 * In return, it will contain the actual length of transformed frame.
 * @return \c true on success, \c false otherwise.
 */
bool TransformOutbound(const unsigned char *aInput, size_t aInputLength, unsigned char *aOutput, size_t *aOutputLength);

/**
 * Restores spinel frames by transforming data received from AP/NCP.
 *
 * This method transforms inbound frames in both directions, i.e. from AP to NCP and from NCP to AP.
 * @param[in] aInput Pointer to buffer containing received data.
 * @param[in] aInputLength Length of received data.
 * @param[out] aOutput Pointer to allocated buffer for restored spinel frame.
 * @param[in,out] aOutputLength Length of allocated buffer for restored spinel frame.
 * In return, it will contain the actual length of restored frame.
 * @return \c true on success, \c false otherwise.
 */
bool TransformInbound(const unsigned char *aInput, size_t aInputLength, unsigned char *aOutput, size_t *aOutputLength);

} // namespace SpinelTransformer
