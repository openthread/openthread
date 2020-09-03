/*****************************************************************************
 *
 * MODULE:             Persistent Data Manager
 *
 * DESCRIPTION:        Provide management of data which needs to persist over
 *                     cold or warm start
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

#ifndef PDM_H_INCLUDED
#define PDM_H_INCLUDED

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include "EmbeddedTypes.h"

#define PUBLIC


#if defined __cplusplus
extern "C" {
#endif

 /*!
 * @addtogroup PDM library
 * @{
 */
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef SE_HOST_COPROCESSOR
#define PDM_EXTERNAL_FLASH
#endif
#define PDM_NUM_BLOCKS 128
#ifdef PDM_EXTERNAL_FLASH
#define PDM_NAME_SIZE 16
#else
#define PDM_NAME_SIZE 7
#ifndef PDM_USER_SUPPLIED_ID
#define PDM_USER_SUPPLIED_ID /* Defaulting to having this enabled */
#endif
#endif
#define PDM_INVALID_ID ((uint16)(-1))

/* PDM ID range allocation reservations
 * Each PDM_ID_BASE_xxx below is the base value for a block of 256 (0x100) IDs.
 * Within a module the individual IDs used by that module will be an offset from
 * this base.
 *
 * These ID ranges should not be re-used by other modules, even if the modules
 * are not both present in the build.
 *
 * Values should not be changed. Reserve a new range instead of changing an
 * existing range.
 */
#define PDM_ID_BASE_APP_ZB (0x0000) /* 0x0000?0x00ff: ZigBee Application Notes */
#define PDM_ID_BASE_ZPSAPL (0xf000) /* 0xf000?0xf0ff: ZigBee ZPS APL layer */
#define PDM_ID_BASE_ZPSNWK (0xf100) /* 0xf100?0xf1ff: ZigBee ZPS NWK layer */
#define PDM_ID_BASE_RADIO  (0xff00) /* 0xff00?0xffff: Radio driver */

/* PDM ID individual allocation reservations
 * Each PDM_ID_xxx below is an individual ID within one of the reserved ranges.
 * Individual IDs need to be declared here only if they are shared between modules.
 * Please include a descriptive comment to aid accurate identification.
 */
#define PDM_ID_RADIO_SETTINGS (PDM_ID_BASE_RADIO + 0x0000) /* Holds radio KMOD calibration data */

/* Defined because still using heap in PDM builds */
#define PDM_USE_HEAP

#if (defined PDM_USE_HEAP)
/* Empty declaration: buffer not needed */
#define PDM_DECLARE_BUFFER(u16StartSegment, u8NumberOfSegments)
#else
/* Macro to create a static block of RAM to use instead of heap
   NOTE: Must be declared at module scope, not within a function
   NOTE: Value of 12 is based on size of tsPDM_FileSystemRecord */
#define PDM_DECLARE_BUFFER(u16StartSegment, u8NumberOfSegments) \
    PUBLIC uint8 au8PDM_HeapBuffer[(u8NumberOfSegments) * 12] __attribute__((aligned (4)))
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef enum
{
    PDM_E_STATUS_OK,
    PDM_E_STATUS_INVLD_PARAM,
    // NVM based PDM codes
    PDM_E_STATUS_PDM_FULL,
    PDM_E_STATUS_NOT_SAVED,
    PDM_E_STATUS_RECOVERED,
    PDM_E_STATUS_PDM_RECOVERED_NOT_SAVED,
    PDM_E_STATUS_USER_BUFFER_SIZE,
    PDM_E_STATUS_BITMAP_SATURATED_NO_INCREMENT,
    PDM_E_STATUS_BITMAP_SATURATED_OK,
    PDM_E_STATUS_IMAGE_BITMAP_COMPLETE,
    PDM_E_STATUS_IMAGE_BITMAP_INCOMPLETE,
    PDM_E_STATUS_INTERNAL_ERROR
} PDM_teStatus;

typedef enum
{
    PDM_RECOVERY_STATE_NONE = 0,
    PDM_RECOVERY_STATE_NEW, // 1
    PDM_RECOVERY_STATE_RECOVERED, // 2
    PDM_RECOVERY_STATE_RECOVERED_NOT_READ, // 3
    PDM_RECOVERY_STATE_SAVED, // 4
    PDM_RECOVERY_STATE_NOT_SAVED, // 5
    PDM_RECOVERY_STATE_APPENDED, // 6
    // do not move
    PDM_RECOVERY_STATE_NUMBER
} PDM_teRecoveryState;

typedef struct
{
    /* this function gets called after a cold or warm start */
    void (*prInitHwCb)(void);

    /* this function gets called to erase the given sector */
    void (*prEraseCb)(uint8_t u8Sector);

    /* this function gets called to write data to an addresss
     * within a given sector. address zero is the start of the
     * given sector */
    void (*prWriteCb)(uint8_t u8Sector,
#ifdef SE_HOST_COPROCESSOR
                      uint32_t u32Addr,
                      uint32_t u32Len,
#else
                      uint16_t u16Addr,
                      uint16_t u16Len,
#endif
                      uint8_t *pu8Data);

    /* this function gets called to read data from an address
     * within a given sector. address zero is the start of the
     * given sector */
    void (*prReadCb)(uint8_t u8Sector,
#ifdef SE_HOST_COPROCESSOR
                     uint32_t u32Addr,
                     uint32_t u32Len,
#else
                     uint16_t u16Addr,
                     uint16_t u16Len,
#endif
                    uint8_t  *pu8Data);
} PDM_tsHwFncTable;

typedef enum
{
    E_PDM_SYSTEM_EVENT_WEAR_COUNT_TRIGGER_VALUE_REACHED = 0,
    E_PDM_SYSTEM_EVENT_WEAR_COUNT_MAXIMUM_REACHED,
    E_PDM_SYSTEM_EVENT_SAVE_FAILED,
    E_PDM_SYSTEM_EVENT_NOT_ENOUGH_SPACE,
    E_PDM_SYSTEM_EVENT_LARGEST_RECORD_FULL_SAVE_NO_LONGER_POSSIBLE,
    E_PDM_SYSTEM_EVENT_SEGMENT_DATA_CHECKSUM_FAIL,
    E_PDM_SYSTEM_EVENT_SEGMENT_SAVE_OK,
    E_PDM_SYSTEM_EVENT_SEGMENT_DATA_READ_FAIL,
    E_PDM_SYSTEM_EVENT_SEGMENT_DATA_WRITE_FAIL,
    E_PDM_SYSTEM_EVENT_SEGMENT_DATA_ERASE_FAIL,
    E_PDM_SYSTEM_EVENT_SEGMENT_BLANK_CHECK_FAIL,
    E_PDM_SYSTEM_EVENT_SEGMENT_BLANK_DATA_WRITE_FAIL,
    // nerdy event codes
    E_PDM_SYSTEM_EVENT_NVM_SEGMENT_HEADER_REPAIRED,
    E_PDM_SYSTEM_EVENT_NVM_SEGMENT_HEADER_REPAIR_FAILED,
    E_PDM_SYSTEM_EVENT_SYSTEM_INTERNAL_BUFFER_WEAR_COUNT_SWAP,
    E_PDM_SYSTEM_EVENT_SYSTEM_DUPLICATE_FILE_SEGMENT_DETECTED,
    E_PDM_SYSTEM_EVENT_SYSTEM_ERROR,
    // used in test harness
    E_PDM_SYSTEM_EVENT_SEGMENT_PREWRITE,
    E_PDM_SYSTEM_EVENT_SEGMENT_POSTWRITE,
    E_PDM_SYSTEM_EVENT_SEQUENCE_DUPLICATE_DETECTED,
    E_PDM_SYSTEM_EVENT_SEQUENCE_VERIFY_FAIL,
    E_PDM_SYSTEM_EVENT_SMART_SAVE,
    E_PDM_SYSTEM_EVENT_FULL_SAVE
} PDM_eSystemEventCode;

typedef struct
{
    uint32_t u32eventNumber;
    PDM_eSystemEventCode eSystemEventCode;
} PDM_tfpSystemEventCallback;

typedef void (*PDM_tpfvSystemEventCallback)(uint32_t u32eventNumber, PDM_eSystemEventCode eSystemEventCode);

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/*!
 * @brief Initialise the PDM
 *
 * This function is to be called in order to initialise the PDM module
 * call this EVERY cold start and EVERY warm start
 *
 * @param u16StartSegment: Start segment in flash
 * @param u8NumberOfSegments: number of contiguous segments from u16StartSegment
 *                            that belong to the PDM NVM space.
 * @param  fpvPDM_SystemEventCallback: function pointer passed by using layer
 *                                     to be notified of events/errors
 *
 * @retval PDM_E_STATUS_OK if success PDM_E_STATUS_INTERNAL_ERROR otherwise
 */
PUBLIC PDM_teStatus PDM_eInitialise(uint16_t u16StartSegment,
                                    uint8_t u8NumberOfSegments,
                                    PDM_tpfvSystemEventCallback fpvPDM_SystemEventCallback);


/*!
 * @brief Save a PDM record
 *
 * This function saves the specified application data from RAM to the specified record
 * in NVM. The record is identified by means of a 16-bit user-defined value.
 * When a data record is saved to the NVM for the first time, the data is written provided
 * there are enough NVM segments available to hold the data. Upon subsequent save
 * requests, if there has been a change between the RAM-based and NVM-based data
 * buffers then the PDM will attempt to re-save only the segments that have changed
 * (if no data has changed, no save will be performed). This is advantageous due to the
 * restricted size of the NVM and the constraint that old data must be preserved while
 * saving changed data to the NVM.
 * Provided that you have registered a callback function with the PDM (see Section 6.3),
 * the callback mechanism will signal when a save has failed. Upon failure, the callback
 * function will be invoked and pass the event E_PDM_SYSTEM_EVENT_DESCRIPTOR_SAVE_FAILED
 * to the application
 * @param u16IdValue User-defined ID of the record to be saved
 * @param pvDataBuffer Pointer to data buffer to be saved in the record in NVM
 * @param u16Datalength Length of data to be saved, in bytes
 *
 * @retval PDM_E_STATUS_OK (success)
 * @retval PDM_E_STATUS_INVLD_PARAM (specified record ID is invalid)
 * @retval PDM_E_STATUS_NOT_SAVED (save to NVM failed)
 */
PUBLIC PDM_teStatus PDM_eSaveRecordData(uint16_t u16IdValue,
                                        void *pvDataBuffer,
                                        uint16_t u16Datalength);

/*!
 * @brief Save a PDM record next time in idle task
 *
 * Like PDM_eSaveRecordData, except that the record information is queued to
 * saved in the idle task, when PDM_vIdleTask is called. Note that if the
 * internal queue is full, the first record on the queue is saved immediately
 * to make space for this record.
 * @param u16IdValue User-defined ID of the record to be saved
 * @param pvDataBuffer Pointer to data buffer to be saved in the record in NVM
 * @param u16Datalength Length of data to be saved, in bytes
 *
 * @retval PDM_E_STATUS_OK (success)
 */
PUBLIC PDM_teStatus PDM_eSaveRecordDataInIdleTask(uint16_t u16IdValue,
                                                  void *pvDataBuffer,
                                                  uint16_t u16Datalength);

/*!
 * @brief Save queued PDM records
 *
 * Synchronously saves any queued record writes that have been generated by
 * calls to PDM_eSaveRecordDataInIdleTask. To avoid this function taking too
 * much time, the number of records that can be written can be limited by
 * the u8WritesAllowed parameter. Use a value of 255 to effectively avoid
 * this limit.
 * @param u8WritesAllowed Maximum number of records that can be written
 *
 * @retval none
 */
PUBLIC void PDM_vIdleTask(uint8_t u8WritesAllowed);

/*!
 * @brief Purge the pending event available in the queue
 *
 * @retval none
 */
PUBLIC void PDM_vQueuePurge(void);


/*!
* @brief Reads Partial Data from an Existing Record in the File System
 *
 * This function appends data to a record previously written.
 * The record is identified by means of a 16-bit user-defined value.
 *
 * @param  u16IdValue      User-defined ID of the record to be read from
 * @param  u16TableOffset  Data Offset
 * @param  pu8DataBuffer   Data Buffer
 * @param  u16DataLength   Length of input data, in bytes.
 * @param  pu16DataBytesRead    Number Of Data Bytes Read u16IdValue
 *
 * @retval PDM_E_STATUS_OK (success)
 * @retval PDM_E_STATUS_INVLD_PARAM (error in arguments)
 * @retval PDM_E_STATUS_INTERNAL_ERROR
 */
PUBLIC PDM_teStatus PDM_eReadPartialDataFromExistingRecord(uint16_t u16IdValue,
                                                           uint16_t u16TableOffset,
                                                           void *pvDataBuffer,
                                                           uint16_t u16DataBufferLength,
                                                           uint16_t *pu16DataBytesRead);

/*!
 * @brief Delete a PDM record
 *
 * This function reads the specified record of application data from the NVM and stores
 * the read data in the supplied data buffer in RAM. The record is specified using its
 * unique 16-bit identifier.
 * Before calling this function, it may be useful to call PDM_bDoesDataExist() in order
 * to determine whether a record with the specified identifier exists in the EEPROM and,
 * if it does, to obtain its size.
 *
 * @param u16IdValue User-defined ID of the record to be read
 * @param  pvDataBuffer Pointer to the data buffer in RAM where the read data is to
 *                      be stored
 * @param u16DataBufferLength  Length of the data buffer, in bytes
 * @param pu16DataBytesRead Pointer to a location to receive the number of data bytes
 *                          read
 *
 * @retval PDM_E_STATUS_OK if success
 * @retval PDM_E_STATUS_INVLD_PARAM otherwise i.e. specified record ID is invalid
*/
PUBLIC PDM_teStatus PDM_eReadDataFromRecord(uint16_t u16IdValue,
                                            void *pvDataBuffer,
                                            uint16_t u16DataBufferLength,
                                            uint16_t *pu16DataBytesRead);

/*!
 * @brief Delete a PDM record
 *
 * This function deletes the specified record of application data in NVM.
 * @see PDM_eDeleteAllData to alternatively delete all records in NVM
 *
 * @param u16IdValue User-defined ID of the record to be deleted
 *
 * @retval none
 */
PUBLIC void PDM_vDeleteDataRecord(uint16_t u16IdValue);

/*!
 * @brief Delete a PDM record
 *
 * This function deletes all records in NVM, including both application data and stack
 * context data, resulting in an empty PDM file system. The NVM segment Wear Count
 * values are preserved (and incremented) throughout this function call.
 * This function is to be used with extreme care in a Zigbee application context
 *
 * @param none
 *
 * @retval none
 */
PUBLIC void PDM_vDeleteAllDataRecords(void);

/*!
 * @brief Seeks for a certain record by record identifier
 *
 * This function checks whether data associated with thd specified record ID
 * exists in the NVM. If the data record exists, the function returns the data
 * length, in bytes, in a location to which a pointer must be provided.
 *
 * @param u16IdValue User-defined ID of the record to be found
 * @param pu16DataLength Pointer to location to receive length, in bytes,
 *                       of data record (if any) associated with specified record ID
 *
 * @retval true if record was fond false otherwise
 */
PUBLIC bool_t PDM_bDoesDataExist(uint16_t u16IdValue, uint16_t *pu16DataLength);

/*!
 * @brief Tells how many free segments are left in the PDM space
 *
 * This function returns the number of unused segments that remain in the NVM.
 * Note that the returned parameter being a uint8_t the total expected number of
 * segments cannot exceed 255
 *
 * @param none
 *
 * @retval Number of PDM NVM segments free
 */
PUBLIC uint8_t PDM_u8GetSegmentCapacity(void);

/*!
 * @brief Tells how many free segments are occupied in the PDM space
 *
 * This function returns the number of used segments in the NVM
 * @see  PDM_u8GetSegmentCapacity
 *
 * @param none
 *
 * @retval Number of NVM segments used
 */
PUBLIC uint8_t PDM_u8GetSegmentOccupancy(void);

/*!
 * @brief Register a User defined system callback function
 *
 * This function registers a callback function that will be called in error events
 * @see PDM_pfGetSystemCallback
 * @see PDM_eInitialise
 *
 * @param fpvPDM_SystemEventCallback function to replace the one that was set on PDM initialisation
 *
 * @retval none
 */
PUBLIC void PDM_vRegisterSystemCallback(PDM_tpfvSystemEventCallback fpvPDM_SystemEventCallback);

/*!
 * @brief Retrieve the previously defined system callback function
 *
 * This function retrieves the callback function that has been set by PDM_eInitialise
 * or PDM_vRegisterSystemCallback
 * @see PDM_vRegisterSystemCallback
 * @see PDM_eInitialise
 *
 * @param none
 *
 * @retval fpvPDM_SystemEventCallback function used to notify error events
 */
PUBLIC PDM_tpfvSystemEventCallback PDM_pfGetSystemCallback(void);

PUBLIC void PDM_vSetWearCountTriggerLevel(uint32_t u32WearCountTriggerLevel);

PUBLIC PDM_teStatus PDM_eGetSegmentWearCount(uint8_t u8SegmentIndex, uint32_t *pu32WearCount);

PUBLIC PDM_teStatus PDM_eGetDeviceWearCountProfile(uint32_t au32WearCount[], uint8_t u8NumberOfSegments);

PUBLIC void PDM_vSetWearLevelDifference(uint32_t u32WearLevelDifference);

int PDM_Init(void);

#if defined UART_DEBUG
PUBLIC void vPDM_InitialiseDisplayDataInFileSystem(uint16_t *pau16PDMFileIDrecord, uint8_t u8NumberOfPDMSegments);

PUBLIC void vPDM_DisplayDataInFileSystem(void);

PUBLIC int iPDM_DisplayDataWithIdInFileSystem(uint16_t u16IdValue);

PUBLIC void vPDM_DisplayDataInNVM(void);

PUBLIC int iPDM_DisplayNVMsegmentData(uint8_t u8SegmentIndex);

PUBLIC char *psPDM_PrintEventID(PDM_eSystemEventCode eSystemEventCode);

PUBLIC int iPDM_ReadRawNVMsegmentDataToBuffer(uint8_t u8SegmentIndex,
                                              uint8_t *pu8SegmentDataBuffer,
                                              uint16_t *pu16SegmentDataSize);
#endif

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern PUBLIC const uint32_t PDM_g_u32Version;

/*@}*/

#if defined __cplusplus
};
#endif

#endif /*PDM_H_INCLUDED*/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
