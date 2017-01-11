/**
 ****************************************************************************************
 *
 * @file internal.h
 *
 * @brief Internal header file
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************
 */

#define FTDF_TBD                   0

#define FTDF_FCS_LENGTH            2
#define FTDF_BUFFER_LENGTH         128
#define FTDF_NR_OF_REQ_BUFFERS     16
#define FTDF_NR_OF_RX_BUFFERS      8
#define FTDF_NR_OF_CHANNELS        16
#define FTDF_NR_OF_SCAN_RESULTS    16
#define FTDF_NR_OF_CSL_PEERS       16
#define FTDF_MAX_HEADER_IES        8
#define FTDF_MAX_PAYLOAD_IES       4
#define FTDF_MAX_SUB_IES           8
#define FTDF_MAX_IE_CONTENT        128
#define FTDF_NR_OF_RX_ADDRS        16
#define FTDF_NR_OF_NEIGHBORS       16

#define FTDF_TX_DATA_BUFFER        0
#define FTDF_TX_WAKEUP_BUFFER      1
#define FTDF_TX_ACK_BUFFER         2

#define FTDF_OPT_SECURITY_ENABLED  0x01
#define FTDF_OPT_FRAME_PENDING     0x02
#define FTDF_OPT_SEQ_NR_SUPPRESSED 0x04
#define FTDF_OPT_ACK_REQUESTED     0x08
#define FTDF_OPT_IES_PRESENT       0x10
#define FTDF_OPT_PAN_ID_PRESENT    0x20
#define FTDF_OPT_ENHANCED          0x40

#define FTDF_MSK_RX_CE             0x00000002
#define FTDF_MSK_SYMBOL_TMR_CE     0x00000008
#define FTDF_MSK_TX_CE             0x00000010

// The maximum time in symbols that is takes to process request. After succesful
// processing a request the TX buffer is fully initialized and ready to be sent.
#define FTDF_TSCH_MAX_PROCESS_REQUEST_TIME 35

// The maximum time in symbols needed to search for the next slot for which a
// TX or RX needs to be done
#define FTDF_TSCH_MAX_SCHEDULE_TIME 30

#define FTDF_GET_FIELD_ADDR( fieldName )                ( (volatile uint32_t*) ( IND_F_FTDF_ ## fieldName ) )
#define FTDF_GET_FIELD_ADDR_INDEXED( fieldName, index ) ( (volatile uint32_t*) ( IND_F_FTDF_ ## fieldName + \
                                                                                 (intptr_t) index * \
                                                                                 FTDF_ ## fieldName ## _INTVL ) )
#define FTDF_GET_REG_ADDR( regName )                    ( (volatile uint32_t*) ( IND_R_FTDF_ ## regName ) )
#define FTDF_GET_REG_ADDR_INDEXED( regName, index )     ( (volatile uint32_t*) ( IND_R_FTDF_ ## regName + \
                                                                                 (intptr_t) index * \
                                                                                 FTDF_ ## regName ## _INTVL ) )
#define FTDF_GET_FIELD( fieldName )                     ( ( ( *(volatile uint32_t*) ( IND_F_FTDF_ ## fieldName ) ) & \
                                                            MSK_F_FTDF_ ## fieldName ) >> OFF_F_FTDF_ ## fieldName )
#define FTDF_GET_FIELD_INDEXED( fieldName, \
                                index )                 ( ( ( *FTDF_GET_FIELD_ADDR_INDEXED( fieldName, \
                                                                                            index ) ) \
                                                            & MSK_F_FTDF_ ## fieldName ) >> OFF_F_FTDF_ ## fieldName )

#define FTDF_SET_FIELD( fieldName, value )              do {            \
        uint32_t tmp =  *FTDF_GET_FIELD_ADDR( fieldName ) & ~MSK_F_FTDF_ ## fieldName; \
        *FTDF_GET_FIELD_ADDR( fieldName ) = tmp | \
                                            ( ( ( value ) << OFF_F_FTDF_ ## fieldName ) & MSK_F_FTDF_ ## fieldName ); \
} \
    while ( 0 )

#define FTDF_processRequest FTDF_sndMsg

struct FTDF_Pib
{
    FTDF_ExtAddress         extAddress;
    FTDF_Period             ackWaitDuration;
#ifndef FTDF_LITE
    FTDF_Boolean            associatedPANCoord;
    FTDF_Boolean            associationPermit;
    FTDF_Boolean            autoRequest;
    FTDF_Boolean            battLifeExt;
    FTDF_NumOfBackoffs      battLifeExtPeriods;
    FTDF_Octet              beaconPayload[ FTDF_MAX_BEACON_PAYLOAD_LENGTH ];
    FTDF_Size               beaconPayloadLength;
    FTDF_Order              beaconOrder;
    FTDF_Time               beaconTxTime;
    FTDF_SN                 BSN;
    FTDF_ExtAddress         coordExtAddress;
    FTDF_ShortAddress       coordShortAddress;
    FTDF_SN                 DSN;
    FTDF_Boolean            GTSPermit;
#endif /* !FTDF_LITE */
    FTDF_BEExponent         maxBE;
    FTDF_Size               maxCSMABackoffs;
    FTDF_Period             maxFrameTotalWaitTime;
    FTDF_Size               maxFrameRetries;
    FTDF_BEExponent         minBE;
#ifndef FTDF_LITE
    FTDF_Period             LIFSPeriod;
    FTDF_Period             SIFSPeriod;
#endif /* !FTDF_LITE */
    FTDF_PANId              PANId;
#ifndef FTDF_LITE
    FTDF_Boolean            promiscuousMode; /* Not supported. Use transparent mode instead */
    FTDF_Size               responseWaitTime;
#endif /* !FTDF_LITE */
    FTDF_Boolean            rxOnWhenIdle;
#ifndef FTDF_LITE
    FTDF_Boolean            securityEnabled;
#endif /* !FTDF_LITE */
    FTDF_ShortAddress       shortAddress;
#ifndef FTDF_LITE
    FTDF_Size               superframeOrder;
    FTDF_Period             syncSymbolOffset;
    FTDF_Boolean            timestampSupported;
    FTDF_Period             transactionPersistenceTime;
    FTDF_Period             enhAckWaitDuration;
    FTDF_Boolean            implicitBroadcast;
    FTDF_SimpleAddress      simpleAddress;
    FTDF_Period             disconnectTime;
    FTDF_Priority           joinPriority;
    FTDF_ASN                ASN;
    FTDF_Boolean            noHLBuffers;
    FTDF_SlotframeTable     slotframeTable;
    FTDF_LinkTable          linkTable;
    FTDF_TimeslotTemplate   timeslotTemplate;
    FTDF_HoppingSequenceId  HoppingSequenceId;
    FTDF_ChannelPage        channelPage;
    FTDF_ChannelNumber      numberOfChannels;
    FTDF_Bitmap32           phyConfiguration;
    FTDF_Bitmap8            extendedBitmap;
    FTDF_Length             hoppingSequenceLength;
    FTDF_ChannelNumber      hoppingSequenceList[ FTDF_MAX_HOPPING_SEQUENCE_LENGTH ];
    FTDF_Length             currentHop;
    FTDF_Period             dwellTime;
    FTDF_Period             CSLPeriod;
    FTDF_Period             CSLMaxPeriod;
    FTDF_Bitmap32           CSLChannelMask;
    FTDF_Period             CSLFramePendingWaitT;
    FTDF_Boolean            lowEnergySuperframeSupported;
    FTDF_Boolean            lowEnergySuperframeSyncInterval;
#endif /* !FTDF_LITE */
    FTDF_PerformanceMetrics performanceMetrics;
#ifndef FTDF_LITE
    FTDF_Boolean            useEnhancedBecaon;
    FTDF_IEList             EBIEList;
    FTDF_Boolean            EBFilteringEnabled;
    FTDF_SN                 EBSN;
    FTDF_AutoSA             EBAutoSA;
    FTDF_IEList             EAckIEList;
    FTDF_Boolean            tschEnabled;
    FTDF_Boolean            leEnabled;
#endif /* !FTDF_LITE */
    FTDF_ChannelNumber      currentChannel;
    FTDF_TXPowerTolerance   TXPowerTolerance;
    FTDF_DBm                TXPower;
    FTDF_CCAMode            CCAMode;
#ifndef FTDF_LITE
    FTDF_ChannelPage        currentPage;
    FTDF_Period             maxFrameDuration;
    FTDF_Period             SHRDuration;
    FTDF_KeyTable           keyTable;
    FTDF_DeviceTable        deviceTable;
    FTDF_SecurityLevelTable securityLevelTable;
    FTDF_FrameCounter       frameCounter;
    FTDF_SecurityLevel      mtDataSecurityLevel;
    FTDF_KeyIdMode          mtDataKeyIdMode;
    FTDF_Octet              mtDataKeySource[ 8 ];
    FTDF_KeyIndex           mtDataKeyIndex;
    FTDF_Octet              defaultKeySource[ 8 ];
    FTDF_ExtAddress         PANCoordExtAddress;
    FTDF_ShortAddress       PANCoordShortAddress;
    FTDF_FrameCounterMode   frameCounterMode;
    FTDF_Period             CSLSyncTxMargin;
    FTDF_Period             CSLMaxAgeRemoteInfo;
#endif /* !FTDF_LITE */
    FTDF_TrafficCounters    trafficCounters;
#ifndef FTDF_LITE
    FTDF_Boolean            LECapable;
    FTDF_Boolean            LLCapable;
    FTDF_Boolean            DSMECapable;
    FTDF_Boolean            RFIDCapable;
    FTDF_Boolean            AMCACapable;
#endif /* !FTDF_LITE */
    FTDF_Boolean            metricsCapable;
#ifndef FTDF_LITE
    FTDF_Boolean            rangingSupported;
#endif /* !FTDF_LITE */
    FTDF_Boolean            keepPhyEnabled;
    FTDF_Boolean            metricsEnabled;
#ifndef FTDF_LITE
    FTDF_Boolean            beaconAutoRespond;
    FTDF_Boolean            tschCapable;
    FTDF_Period             tsSyncCorrectThreshold;
#endif /* !FTDF_LITE */
};

typedef uint8_t FTDF_FrameVersion;
#define FTDF_FRAME_VERSION_2003          0
#define FTDF_FRAME_VERSION_2011          1
#define FTDF_FRAME_VERSION_E             2
#define FTDF_FRAME_VERSION_NOT_SUPPORTED 3

typedef struct
{
    FTDF_FrameType      frameType;
    FTDF_FrameVersion   frameVersion;
    FTDF_Bitmap8        options;
    FTDF_SN             SN;
    FTDF_AddressMode    srcAddrMode;
    FTDF_PANId          srcPANId;
    FTDF_Address        srcAddr;
    FTDF_AddressMode    dstAddrMode;
    FTDF_PANId          dstPANId;
    FTDF_Address        dstAddr;
    FTDF_CommandFrameId commandFrameId;
} FTDF_FrameHeader;

typedef struct
{
    FTDF_SecurityLevel    securityLevel;
    FTDF_KeyIdMode        keyIdMode;
    FTDF_KeyIndex         keyIndex;
    FTDF_FrameCounterMode frameCounterMode;
    FTDF_Octet*           keySource;
    FTDF_FrameCounter     frameCounter;
} FTDF_SecurityHeader;

typedef struct
{
    FTDF_Boolean fastA;
    FTDF_Boolean dataR;
} FTDF_AssocAdmin;

typedef uint8_t FTDF_SNSel;
#define FTDF_SN_SEL_DSN  0
#define FTDF_SN_SEL_BSN  1
#define FTDF_SN_SEL_EBSN 2

typedef struct
{
    FTDF_AddressMode addrMode;
    FTDF_Address     addr;
    FTDF_Time        timestamp;
    FTDF_Boolean     dsnValid;
    FTDF_SN          dsn;
    FTDF_Boolean     bsnValid;
    FTDF_SN          bsn;
    FTDF_Boolean     ebsnValid;
    FTDF_SN          ebsn;
} FTDF_RxAddressAdmin;

struct FTDF_Buffer_;

typedef struct
{
    struct FTDF_Buffer_* next;
    struct FTDF_Buffer_* prev;
} FTDF_BufferHeader;

typedef struct FTDF_Buffer_
{
    FTDF_BufferHeader header;
    FTDF_MsgBuffer*   request;
} FTDF_Buffer;

typedef struct
{
    FTDF_BufferHeader head;
    FTDF_BufferHeader tail;
} FTDF_Queue;

typedef struct
{
    FTDF_Address     addr;
    FTDF_AddressMode addrMode;
    FTDF_PANId       PANId;
    FTDF_Queue       queue;
} FTDF_Pending;

typedef struct _FTDF_PendingTL_
{
    struct _FTDF_PendingTL_* next;
    FTDF_Boolean             free;
    FTDF_MsgBuffer*          request;
    FTDF_Time                delta;
    uint8_t                  pendListNr;
    void ( * func )( struct _FTDF_PendingTL_* );
} FTDF_PendingTL;

typedef uint8_t FTDF_RemoteId;
#define FTDF_REMOTE_DATA_REQUEST                 0
#define FTDF_REMOTE_PAN_ID_CONFLICT_NOTIFICATION 1
#define FTDF_REMOTE_BEACON                       2
#define FTDF_REMOTE_KEEP_ALIVE                   3

typedef struct
{
    FTDF_MsgId        msgId;
    FTDF_RemoteId     remoteId;
    FTDF_ShortAddress dstAddr;
} FTDF_RemoteRequest;

#ifndef FTDF_NO_CSL
typedef struct
{
    FTDF_ShortAddress addr;
    FTDF_Time         time;
    FTDF_Period       period;
} FTDF_PeerCslTiming;
#endif /* FTDF_NO_CSL */

typedef uint8_t FTDF_State;
#define FTDF_STATE_UNINITIALIZED 0
#define FTDF_STATE_IDLE          1
#define FTDF_STATE_DATA_REQUEST  2
#define FTDF_STATE_POLL_REQUEST  3
#define FTDF_STATE_SCANNING      4

#ifndef FTDF_NO_TSCH
typedef struct
{
    FTDF_ShortAddress nodeAddr;
    FTDF_Size         nrOfRetries;
    FTDF_Size         nrOfCcaRetries;
    FTDF_BEExponent   BE;
} FTDF_TschRetry;

typedef struct
{
    FTDF_ShortAddress    dstAddr;
    FTDF_KeepAlivePeriod period;
    FTDF_RemoteRequest   msg;
} FTDF_NeighborEntry;
#endif /* FTDF_NO_TSCH */

typedef struct
{
    FTDF_Count fcsErrorCnt;
    FTDF_Count txStdAckCnt;
    FTDF_Count rxStdAckCnt;
} FTDF_LmacCounters;

extern struct FTDF_Pib     FTDF_pib;
extern FTDF_State*         FTDF_state;
extern FTDF_Boolean        FTDF_transparentMode;
extern FTDF_Bitmap32       FTDF_transparentModeOptions;
extern FTDF_Queue          FTDF_reqQueue;
extern FTDF_Queue          FTDF_freeQueue;
extern FTDF_Pending        FTDF_txPendingList[ FTDF_NR_OF_REQ_BUFFERS ];
extern FTDF_PendingTL      FTDF_txPendingTimerList[ FTDF_NR_OF_REQ_BUFFERS ];
extern FTDF_PendingTL*     FTDF_txPendingTimerHead;
extern FTDF_Time           FTDF_txPendingTimerLT;
extern FTDF_Time           FTDF_txPendingTimerTime;
extern FTDF_MsgBuffer*     FTDF_reqCurrent;
extern FTDF_Size           FTDF_nrOfRetries;
extern FTDF_Boolean        FTDF_isPANCoordinator;
extern FTDF_SlotframeEntry FTDF_slotframeTable[ FTDF_MAX_SLOTFRAMES ];
extern FTDF_LinkEntry      FTDF_linkTable[ FTDF_MAX_LINKS ];
#ifndef FTDF_NO_TSCH
extern FTDF_LinkEntry*     FTDF_startLinks[ FTDF_MAX_SLOTFRAMES ];
extern FTDF_LinkEntry*     FTDF_endLinks[ FTDF_MAX_SLOTFRAMES ];
extern FTDF_NeighborEntry  FTDF_neighborTable[ FTDF_NR_OF_NEIGHBORS ];
extern FTDF_Boolean        FTDF_wakeUpEnableTsch;
#endif /* FTDF_NO_TSCH */
#ifndef FTDF_NO_CSL
extern FTDF_Boolean        FTDF_wakeUpEnableLe;
#endif /* FTDF_NO_CSL */
extern FTDF_LmacCounters   FTDF_lmacCounters;

extern FTDF_FrameHeader    FTDF_fh;
extern FTDF_SecurityHeader FTDF_sh;
extern FTDF_AssocAdmin     FTDF_aa;
extern FTDF_Boolean        FTDF_txInProgress;
#ifndef FTDF_NO_CSL
extern FTDF_Time           FTDF_rzTime;
extern FTDF_ShortAddress   FTDF_sendFramePending;
#endif /* FTDF_NO_CSL */
extern FTDF_Time           FTDF_startCslSampleTime;

#ifndef FTDF_NO_TSCH
extern FTDF_LinkEntry*     FTDF_tschSlotLink;
extern FTDF_Time64         FTDF_tschSlotTime;
extern FTDF_ASN            FTDF_tschSlotASN;
#endif /* FTDF_NO_TSCH */

void FTDF_processDataRequest( FTDF_DataRequest* req );
void FTDF_processPurgeRequest( FTDF_PurgeRequest* req );
void FTDF_processAssociateRequest( FTDF_AssociateRequest* req );
void FTDF_processAssociateResponse( FTDF_AssociateResponse* req );
void FTDF_processDisassociateRequest( FTDF_DisassociateRequest* req );
void FTDF_processGetRequest( FTDF_GetRequest* req );
void FTDF_processSetRequest( FTDF_SetRequest* req );
void FTDF_processOrphanResponse( FTDF_OrphanResponse* req );
void FTDF_processResetRequest( FTDF_ResetRequest* req );
void FTDF_processRxEnableRequest( FTDF_RxEnableRequest* req );
void FTDF_processScanRequest( FTDF_ScanRequest* req );
void FTDF_processStartRequest( FTDF_StartRequest* req );
void FTDF_processPollRequest( FTDF_PollRequest* req );
#ifndef FTDF_NO_TSCH
void FTDF_processSetSlotframeRequest( FTDF_SetSlotframeRequest* req );
void FTDF_processSetLinkRequest( FTDF_SetLinkRequest* req );
void FTDF_processTschModeRequest( FTDF_TschModeRequest* req );
void FTDF_processKeepAliveRequest( FTDF_KeepAliveRequest* req );
#endif /* FTDF_NO_TSCH */
void FTDF_processBeaconRequest( FTDF_BeaconRequest* req );
void FTDF_processTransparentRequest( FTDF_TransparentRequest* req );
void FTDF_processRemoteRequest( FTDF_RemoteRequest* req );

void FTDF_processNextRequest( void );
void FTDF_processRxEvent( void );
void FTDF_processTxEvent( void );
void FTDF_processSymbolTimerEvent( void );

void FTDF_sendDataConfirm( FTDF_DataRequest*  dataRequest,
                           FTDF_Status        status,
                           FTDF_Time          timestamp,
                           FTDF_SN            DSN,
                           FTDF_NumOfBackoffs numOfBackoffs,
                           FTDF_IEList*       ackPayload );

void FTDF_sendDataIndication( FTDF_FrameHeader*    frameHeader,
                              FTDF_SecurityHeader* securityHeader,
                              FTDF_IEList*         payloadIEList,
                              FTDF_DataLength      msduLength,
                              FTDF_Octet*          msdu,
                              FTDF_LinkQuality     mpduLinkQuality,
                              FTDF_Time            timestamp );

void FTDF_sendPollConfirm( FTDF_PollRequest* pollRequest, FTDF_Status status );

void FTDF_sendScanConfirm( FTDF_ScanRequest* scanRequest, FTDF_Status status );

void FTDF_sendBeaconNotifyIndication( FTDF_PANDescriptor* PANDescriptor );

void FTDF_sendBeaconConfirm( FTDF_BeaconRequest* beaconRequest, FTDF_Status status );

void FTDF_sendAssociateConfirm( FTDF_AssociateRequest* associateRequest,
                                FTDF_Status            status,
                                FTDF_ShortAddress      assocShortAddr );

void FTDF_sendAssociateDataRequest( void );

void FTDF_sendDisassociateConfirm( FTDF_DisassociateRequest* disReq, FTDF_Status status );

void FTDF_sendSyncLossIndication( FTDF_LossReason lossReason, FTDF_SecurityHeader* securityHeader );

void FTDF_sendPANIdConflictNotification( FTDF_FrameHeader* frameHeader, FTDF_SecurityHeader* securityHeader );

void FTDF_sendCommStatusIndication( FTDF_MsgBuffer*     request,
                                    FTDF_Status         status,
                                    FTDF_PANId          PANId,
                                    FTDF_AddressMode    srcAddrMode,
                                    FTDF_Address        srcAddr,
                                    FTDF_AddressMode    dstAddrMode,
                                    FTDF_Address        dstAddr,
                                    FTDF_SecurityLevel  securityLevel,
                                    FTDF_KeyIdMode      keyIdMode,
                                    FTDF_Octet*         keySource,
                                    FTDF_KeyIndex       keyIndex );

void FTDF_startTimer( FTDF_Period period,
                      void        ( * timerFunc )( void* params ),
                      void*       params );

FTDF_Octet* FTDF_getTxBufPtr( FTDF_Buffer* txBuffer );

void        FTDF_setTxMetaData( FTDF_Buffer*       txBuffer,
                                FTDF_DataLength    frameLength,
                                FTDF_ChannelNumber channel,
                                FTDF_FrameType     frameType,
                                FTDF_Boolean       ackTX,
                                FTDF_SN            SN );

FTDF_Status FTDF_sendFrame( FTDF_ChannelNumber   channel,
                            FTDF_FrameHeader*    frameHeader,
                            FTDF_SecurityHeader* securityHeader,
                            FTDF_Octet*          txPtr,
                            FTDF_DataLength      payloadSize,
                            FTDF_Octet*          payload );

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
FTDF_Status FTDF_sendAckFrame( FTDF_FrameHeader*    frameHeader,
                               FTDF_SecurityHeader* securityHeader,
                               FTDF_Octet*          txPtr );
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

void FTDF_sendTransparentFrame( FTDF_DataLength    frameLength,
                                FTDF_Octet*        frame,
                                FTDF_ChannelNumber channel,
                                FTDF_PTI           pti,
                                FTDF_Boolean       cmsaSuppress );

FTDF_Octet* FTDF_getRxBuffer( void );

void        FTDF_processRxEvent( void );

FTDF_Octet* FTDF_addFrameHeader( FTDF_Octet*       txPtr,
                                 FTDF_FrameHeader* frameHeader,
                                 FTDF_DataLength   msduLength );

FTDF_Octet* FTDF_getFrameHeader( FTDF_Octet*       rxPtr,
                                 FTDF_FrameHeader* frameHeader );

FTDF_DataLength FTDF_getMicLength( FTDF_SecurityLevel securityLevel );

FTDF_Octet*     FTDF_getSecurityHeader( FTDF_Octet*          rxPtr,
                                        uint8_t              frameVersion,
                                        FTDF_SecurityHeader* securityHeader );

FTDF_Octet* FTDF_addSecurityHeader( FTDF_Octet*          txPtr,
                                    FTDF_SecurityHeader* securityHeader );

FTDF_Status FTDF_secureFrame( FTDF_Octet*          bufPtr,
                              FTDF_Octet*          privPtr,
                              FTDF_FrameHeader*    frameHeader,
                              FTDF_SecurityHeader* securityHeader );

FTDF_Status FTDF_unsecureFrame( FTDF_Octet*          bufPtr,
                                FTDF_Octet*          privPtr,
                                FTDF_FrameHeader*    frameHeader,
                                FTDF_SecurityHeader* securityHeader );

FTDF_KeyDescriptor* FTDF_lookupKey( FTDF_AddressMode devAddrMode,
                                    FTDF_PANId       devPANId,
                                    FTDF_Address     devAddr,
                                    FTDF_FrameType   frameType,
                                    FTDF_KeyIdMode   keyIdMode,
                                    FTDF_KeyIndex    keyIndex,
                                    FTDF_Octet*      keySource );

FTDF_DeviceDescriptor* FTDF_lookupDevice( FTDF_Size                    nrOfDeviceDescriptorHandles,
                                          FTDF_DeviceDescriptorHandle* deviceDescriptorHandles,
                                          FTDF_AddressMode             devAddrMode,
                                          FTDF_PANId                   devPANId,
                                          FTDF_Address                 devAddr );

FTDF_SecurityLevelDescriptor* FTDF_getSecurityLevelDescr( FTDF_FrameType      frameType,
                                                          FTDF_CommandFrameId commandFrameId );

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
FTDF_Octet* FTDF_addIes( FTDF_Octet*  txPtr,
                         FTDF_IEList* headerIEList,
                         FTDF_IEList* payloadIEList,
                         FTDF_Boolean withTerminationIE );

FTDF_Octet* FTDF_getIes( FTDF_Octet*   rxPtr,
                         FTDF_Octet*   frameEndPtr,
                         FTDF_IEList** headerIEList,
                         FTDF_IEList** payloadIEList );
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

#ifndef FTDF_NO_CSL
void FTDF_setPeerCslTiming( FTDF_IEList* headerIEList,
                            FTDF_Time    timeStamp );
void FTDF_setCslSampleTime( void );
void FTDF_getWakeupParams( FTDF_ShortAddress dstAddr,
                           FTDF_Time*        wakeupStartTime,
                           FTDF_Period*      wakeupPeriod );
#endif /* FTDF_NO_CSL */

void            FTDF_getExtAddress( void );
void            FTDF_setExtAddress( void );
void            FTDF_getAckWaitDuration( void );
void            FTDF_getEnhAckWaitDuration( void );
void            FTDF_setEnhAckWaitDuration( void );
void            FTDF_getImplicitBroadcast( void );
void            FTDF_setImplicitBroadcast( void );
void            FTDF_setShortAddress( void );
void            FTDF_setSimpleAddress( void );
void            FTDF_getRxOnWhenIdle( void );
void            FTDF_setRxOnWhenIdle( void );
void            FTDF_getPANId( void );
void            FTDF_setPANId( void );
void            FTDF_getCurrentChannel( void );
void            FTDF_setCurrentChannel( void );
void            FTDF_getMaxFrameTotalWaitTime( void );
void            FTDF_setMaxFrameTotalWaitTime( void );
#ifndef FTDF_NO_CSL
void            FTDF_setLeEnabled( void );
void            FTDF_getCslFramePendingWaitT( void );
void            FTDF_setCslFramePendingWaitT( void );
#endif /* FTDF_NO_CSL */
void            FTDF_getLmacPmData( void );
void            FTDF_getLmacTrafficCounters( void );
void            FTDF_setMaxCSMABackoffs( void );
void            FTDF_setMaxBE( void );
void            FTDF_setMinBE( void );
void            FTDF_getKeepPhyEnabled( void );
void            FTDF_setKeepPhyEnabled( void );
#ifndef FTDF_NO_TSCH
void            FTDF_setTimeslotTemplate( void );
#endif /* FTDF_NO_TSCH */

void            FTDF_initLmac( void );
void            FTDF_initQueues( void );
void            FTDF_initQueue( FTDF_Queue* queue );
void            FTDF_queueBufferHead( FTDF_Buffer* buffer, FTDF_Queue* queue );
FTDF_Status     FTDF_queueReqHead( FTDF_MsgBuffer* request, FTDF_Queue* queue );
FTDF_Buffer*    FTDF_dequeueBufferTail( FTDF_Queue* queue );
FTDF_MsgBuffer* FTDF_dequeueReqTail( FTDF_Queue* queue );
FTDF_MsgBuffer* FTDF_dequeueByHandle( FTDF_Handle handle, FTDF_Queue* queue );
FTDF_Buffer*    FTDF_dequeueByBuffer( FTDF_Buffer* buffer, FTDF_Queue* queue );
FTDF_Boolean    FTDF_isQueueEmpty( FTDF_Queue* queue );
void            FTDF_addTxPendingTimer( FTDF_MsgBuffer* request, uint8_t pendListNr, FTDF_Time delta, void ( * func )(
                                            FTDF_PendingTL* ) );
void            FTDF_removeTxPendingTimer( FTDF_MsgBuffer* request );
void            FTDF_restoreTxPendingTimer( void );
FTDF_Boolean    FTDF_getTxPendingTimerHead( FTDF_Time* time );
void            FTDF_sendTransactionExpired( FTDF_PendingTL* ptr );
#ifndef FTDF_NO_TSCH
void            FTDF_processKeepAliveTimerExp( FTDF_PendingTL* ptr );
void            FTDF_resetKeepAliveTimer( FTDF_ShortAddress dstAddr );
#endif /* FTDF_NO_TSCH */

void            FTDF_processCommandFrame( FTDF_Octet*          rxBuffer,
                                          FTDF_FrameHeader*    frameHeader,
                                          FTDF_SecurityHeader* securityHeader,
                                          FTDF_IEList*         payloadIEList );

void               FTDF_scanReady( FTDF_ScanRequest* );
void               FTDF_addPANdescriptor( FTDF_PANDescriptor* );
void               FTDF_sendBeaconRequest( FTDF_ChannelNumber channel );
void               FTDF_sendBeaconRequestIndication( FTDF_FrameHeader* frameHeader,
                                                     FTDF_IEList*      payloadIEList );
void               FTDF_sendOrphanNotification( FTDF_ChannelNumber channel );

#ifndef FTDF_NO_TSCH
void               FTDF_setTschEnabled( void );
FTDF_Status        FTDF_scheduleTsch( FTDF_MsgBuffer* request );
void               FTDF_tschProcessRequest( void );
FTDF_Size          FTDF_getTschSyncSubIe( void );
FTDF_Octet*        FTDF_addTschSyncSubIe( FTDF_Octet* txPtr );
FTDF_ShortAddress  FTDF_getRequestAddress( FTDF_MsgBuffer* request );
FTDF_MsgBuffer*    FTDF_tschGetPending( FTDF_MsgBuffer* request );
FTDF_Octet*        FTDF_addCorrTimeIE( FTDF_Octet* txPtr, FTDF_Time rxTimestamp );
void               FTDF_correctSlotTime( FTDF_Time rxTimestamp );
void               FTDF_correctSlotTimeFromAck( FTDF_IEList* headerIEList );
void               FTDF_initTschRetries( void );
FTDF_TschRetry*    FTDF_getTschRetry( FTDF_ShortAddress nodeAddr );
FTDF_NumOfBackoffs FTDF_getNumOfBackoffs( FTDF_BEExponent BE );
void               FTDF_initBackoff( void );
FTDF_SN            FTDF_processTschSN( FTDF_MsgBuffer* msg, FTDF_SN sn, uint8_t* priv );
#endif /* FTDF_NO_TSCH */

FTDF_Time64        FTDF_getCurTime64( void );
void               FTDF_initCurTime64( void );
