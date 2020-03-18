/****************************************************************************
 *
 * MODULE:             Micro MAC
 *
 * DESCRIPTION:
 * Low-level functions for MAC/BBC control
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
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "jendefs.h"
#include "BbcAndPhyRegs.h"
#include "MicroSpecific.h"
#include "MMAC.h"
#if (defined JENNIC_CHIP_JN5169) || (defined JENNIC_CHIP_FAMILY_JN517x)
#include "JPT.h"
#endif
#if defined (JENNIC_CHIP_FAMILY_JN518x)
#include "radio_jn518x.h"
#include "rom_psector.h"
extern PUBLIC void vMMAC_SetRadioStandard(uint8 u8NewChannel);
#endif

#if !(defined JENNIC_CHIP_FAMILY_JN516x) && !(defined JENNIC_CHIP_FAMILY_JN517x)
/* Address obtained from linker */
extern uint32 __mac_buffer_base;
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
#define INDEX_ADDR(PAGE,WORD) (0x01001000 + ((PAGE) << 8) + ((WORD) << 4))
#define LOOKUP_PAGE           (4)
#define LOOKUP_START_WORD     (3)
#define LOOKUP_END_WORD       (7)
#define PHY_BASE_ADDR         (REG_SYS_BASE + (PHY_OFFSET << 2))

/* Customer MAC address at page 5, word 7 (16-byte words, 16 words/page) */
#define MAC_ADDR_CUSTOMER 0x01001570
/* Default MAC address at page 5, word 8 (16-byte words, 16 words/page) */
#define MAC_ADDR_DEFAULT 0x01001580
#else
#define MAC_ADDR_DEFAULT 0x0009fc70
#endif

#if (defined JENNIC_CHIP_FAMILY_JN518x)
/* This workaround requires interrupts to be enabled, or for the application's
   interrupt handler to call vMMAC_CheckRxStarted() when the REG_BBC_INT_T0
   interrupt fires */
#if !(defined RFP_MODEM_WORKAROUND)
#define RFP_MODEM_WORKAROUND
#endif

/* Interrupt priorities. For JN518x this is not centrally defined so defaults
 * are given here; they can be altered by the application as required after
 * MMAC has been initialised. The BBC priority should always be lower than the
 * RFP_TMU priority, which is set in the radio driver, and the MODEM priority
 * should be higher than 3 so it is still active during critical sections */
#define IRQ_PRIORITY_BBC       (5U)
#define IRQ_PRIORITY_MODEM     (2U)

/* The ZB_MODEM event register bits are not defined in the header file, so
 * define the ones that we need here */
#define ZB_MODEM_ZB_EVENTS_EOP (1U << 10U)
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
#if (defined JENNIC_CHIP_FAMILY_JN516x) && !(defined JENNIC_CHIP_JN5169)
/* Copied from MiniMac.h. Only used internally here so not exposed in public
   API, which makes it tricky to manage. Hence a copy rather than a
   reference */
typedef enum PACK
{
    E_MODULE_STD   = 0,
    E_MODULE_HPM05 = 1,
    E_MODULE_HPM06 = 2
} teModuleType;
#endif

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PUBLIC void vMMAC_IntHandlerBbc(void);
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
PRIVATE uint8 u8GetPapValue(int8 i8TxPower);
#ifdef INCLUDE_ECB_API
PRIVATE void vAesCmdWaitBusy(void);
#endif
#endif
#if !(defined JENNIC_CHIP_FAMILY_JN516x) && !(defined JENNIC_CHIP_FAMILY_JN517x)
PRIVATE uint8 u8RssiToED(uint32 u32Rssi);
#endif

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
#if (defined JENNIC_CHIP_FAMILY_JN516x) && !(defined JENNIC_CHIP_JN5169)
PUBLIC teModuleType eMMacModuleType;
#endif

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE void (*prIntHandler)(uint32);
PRIVATE void (*prPhyIntHandler)(uint32);
PRIVATE uint8 u8SctlMask;

#if (defined JENNIC_CHIP_JN5169) || (defined JENNIC_CHIP_FAMILY_JN517x)
/* It fortuitously happens that the default for these is 0, because the
   default power setting for the radio is 8 (actually 8.5) dBm; see the
   table in u8GetPapValue */
PRIVATE uint8 u8PowerAdj = 0;
PRIVATE uint8 u8Atten3db = 0;
#endif

/* High-power settings, configured by the higher layer. On JN5168 there were
   fixed settings for these based on the module type, but later chips are
   more flexible. Default to 0: u8MMAC_HpmCcaThreshold value of 0 is used to
   indicate that value has not been set. Max power values are used as-is:
   defaults are higher than normal range anyway */
#if !(defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_JN5169)
PRIVATE uint8 u8MMAC_HpmCcaThreshold = 0;
#endif
#if (defined JENNIC_CHIP_JN5169) || (defined JENNIC_CHIP_FAMILY_JN517x)
PRIVATE int8  i8MMAC_HpmTxMaxPower = 127;
PRIVATE int8  i8MMAC_HpmTxMaxPowerCh26 = 127;
#endif

#ifdef RFP_MODEM_WORKAROUND
PRIVATE bool_t bWorkaroundTimerRunning;
#endif

/****************************************************************************/
/***        Public Functions                                              ***/
/****************************************************************************/
/* Initialisation */
PUBLIC void vMMAC_Enable(void)
{
#if defined (JENNIC_CHIP_FAMILY_JN518x)
	vRadio_Jn518x_RadioInit(RADIO_MODE_STD_USE_INITCAL);

	/* Leave TXTO at default, as we use it to detect a lock-up case in the
	   MiniMac. Note that, if we wanted to disable it, writing 0 to the
	   register to clear the enable bits causes it to fire anyway; we need to
	   ensure that the counter value in bits 6:0 remains non-0 as well */
	//JN518X_ZBMAC->TXTO = 1;

    /* RFT1778: AGC blocking due to bad CRC. Requires modem EOP interrupt, so
     * we always enable that here */
    JN518X_ZBMODEM->ZB_EVENTS_CLEAR = ZB_MODEM_ZB_EVENTS_EOP;
    JN518X_ZBMODEM->ZB_EVENTS_ENABLE |= ZB_MODEM_ZB_EVENTS_EOP;

    NVIC_EnableIRQ(ZIGBEE_MODEM_IRQn);
    NVIC_SetPriority(ZIGBEE_MODEM_IRQn, IRQ_PRIORITY_MODEM);
#else

    /* Enable protocol domain: stack won't work without it */
    vREG_SysWrite(REG_SYS_PWR_CTRL,
                  u32REG_SysRead(REG_SYS_PWR_CTRL) | REG_SYSCTRL_PWRCTRL_PPDC_MASK);

    /* Ensure protocol domain is running */
    while ((u32REG_SysRead(REG_SYS_STAT)
            & REG_SYSCTRL_STAT_PROPS_MASK) == 0);

#endif

    /* Clear out interrupt registers */
    vREG_BbcWrite(REG_BBC_ISR, 0xffffffff);

    /* Enable TX and RX interrupts within BBC: allows them to wake CPU from
       doze, but not enabled enough to generate an interrupt (see
       vMMAC_EnableInterrupts for that) */
    vREG_BbcWrite(REG_BBC_IER, REG_BBC_INT_TX_MASK
                               | REG_BBC_INT_RX_MASK
                               | REG_BBC_INT_RX_H_MASK);

#if !(defined JENNIC_CHIP_FAMILY_JN516x) && !(defined JENNIC_CHIP_FAMILY_JN517x)
    /* For JN518x, set buffer offset address */
 #if (defined ROM_BUILD_FOR_ZB)
    /* For ROM build, use fixed offset into base of RAM bank 0 */
    vREG_BbcWrite(DMA_ADDR_OFFSET, 0x04000000);
 #else
    /* Obtain value from linker file */
    vREG_BbcWrite(DMA_ADDR_OFFSET, (uint32)&__mac_buffer_base);
 #endif
#endif
}

PUBLIC void vMMAC_Disable(void)
{
	/* Check that block is powered before trying to disable it, as otherwise
	 * function would crash */
	if (TRUE == bMMAC_PowerStatus())
	{
#if defined (JENNIC_CHIP_FAMILY_JN518x)
		NVIC_DisableIRQ(ZIGBEE_MODEM_IRQn);
		NVIC_DisableIRQ(ZIGBEE_MAC_IRQn);
		vMMAC_RadioToOffAndWait();
		vRadio_Jn518x_RadioDeInit();
#else
		vMMAC_RadioToOffAndWait();
		vAHI_ProtocolPower(FALSE);
#endif
	}
}

PUBLIC void vMMAC_EnableInterrupts(void (*prHandler)(uint32 u32Mask))
{
    /* Store user handler */
    prIntHandler = prHandler;

#if (defined JENNIC_CHIP_FAMILY_JN516x)
    /* Set up BBC interrupt handler. JN517x sets this at compile time */
    isr_handlers[MICRO_ISR_NUM_BBC] = vMMAC_IntHandlerBbc;
#endif

    /* Enable BBC interrupt in PIC/NVIC */
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
    MICRO_SET_PIC_ENABLE(MICRO_ISR_MASK_BBC);
#else
    /* Enable interrupt; priority level can be changed by application, but it
     * has to be lower than RFP_TMU */
    NVIC_EnableIRQ(ZIGBEE_MAC_IRQn);
    NVIC_SetPriority(ZIGBEE_MAC_IRQn, IRQ_PRIORITY_BBC);
#endif

    /* Enable interrupts in CPU */
    MICRO_ENABLE_INTERRUPTS();
}

PUBLIC void vMMAC_RegisterPhyIntHandler(void (*prHandler)(uint32 u32Mask))
{
    /* Store user handler */
    prPhyIntHandler = prHandler;
}

PUBLIC void vMMAC_SetChannel(uint8 u8Channel)
{
    /* Basic channel set function now just calls the main function, to reduce
     * ongoing support effort of maintaining both */
    vMMAC_SetChannelAndPower(u8Channel, i8MMAC_GetTxPowerLevel());
}

PUBLIC void vMMAC_SetChannelAndPower(uint8 u8Channel, int i8TxPower)
{
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
    uint32 u32RegData;
    uint32 u32PapValue;
#endif
#ifndef MAC_KEEP_STAY_ON_SET
    uint32 u32RxCtlData;
#endif
    MICRO_INT_STORAGE;

    /* Disable interrupts */
    MICRO_INT_ENABLE_ONLY(0);

#ifndef MAC_KEEP_STAY_ON_SET
    /* Read current RX control setting in case we want to re-enable it later */
    u32RxCtlData = u32REG_BbcRead(REG_BBC_RXCTL);

    /* Turn radio off and wait for it to be off. If we were sending or
       receiving, this might result in an interrupt to be processed */
    vMMAC_RadioToOffAndWait();
#endif

#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
    /* Change channel */
    vREG_PhyWrite(REG_PHY_CHAN, u8Channel);

    /* For M06 on channel 26, must turn down power to stay within
       standards. In other cases, set power to requested level. Call to set
       power level also stores u8PowerAdj and u8Atten3db if appropriate */

#if (defined JENNIC_CHIP_FAMILY_JN516x) && !(defined JENNIC_CHIP_JN5169)

    if (   (E_MODULE_HPM06 == eMMacModuleType)
        && (26             == u8Channel)
        && (0              <= i8TxPower)
       )
    {
        u32PapValue = u8GetPapValue(-9);
    }
    else

#else /* JN5169, JN517x */

    /* Modify TX power level down to limits (if applied) for use with
       high-power modules. Two limits:
         i8MMAC_HpmTxMaxPower for channels 11 to 25
         i8MMAC_HpmTxMaxPowerCh26 for channel 26
     */
    if (26 == u8Channel)
    {
        if (i8TxPower > i8MMAC_HpmTxMaxPowerCh26)
        {
            i8TxPower = i8MMAC_HpmTxMaxPowerCh26;
        }
    }
    else
    {
        if (i8TxPower > i8MMAC_HpmTxMaxPower)
        {
            i8TxPower = i8MMAC_HpmTxMaxPower;
        }
    }

#endif
    /* Common to JN61x and JN517x. Note that for JN516x this is the 'else'
       part of an if, so we need the braces */
    {
        u32PapValue = u8GetPapValue(i8TxPower);
    }

    u32RegData = u32REG_PhyRead(REG_PHY_PA_CTRL);
    u32RegData &= ~REG_PHY_PA_CTRL_PAP_MASK;
    u32RegData |= u32PapValue << REG_PHY_PA_CTRL_PAP_BIT;
    vREG_PhyWrite(REG_PHY_PA_CTRL, u32RegData);

#else /* JN518x */

    vRadio_SetChannelAndPower(u8Channel, i8TxPower);

#endif

#if (defined JENNIC_CHIP_JN5169) || (defined JENNIC_CHIP_FAMILY_JN517x)
    /* Change BMOD setting and apply other settings */
    vJPT_TxPowerAdjust(u8PowerAdj, u8Atten3db, u8Channel);
#endif

#ifndef MAC_KEEP_STAY_ON_SET
    /* Check for pending RX interrupts: if not, return RX to previous state */
    if (0 == (u32REG_BbcRead(REG_BBC_MISR)
              & (REG_BBC_INT_TX_MASK | REG_BBC_INT_RX_MASK)
             )
       )
    {
        vMMAC_RxCtlUpdate(u32RxCtlData);
    }
#endif

    /* Restore interrupts */
    MICRO_INT_RESTORE_STATE();
}

/* Miscellaneous */
PUBLIC uint32 u32MMAC_GetTime(void)
{
    return u32REG_BbcRead(REG_BBC_SCFRC);
}

PUBLIC void vMMAC_RadioOff(void)
{
    vMMAC_RxCtlUpdate(0);
}

PUBLIC void vMMAC_RadioToOffAndWait(void)
{
    uint32 u32State;

    /* Turn radio off and wait for it to be off. If we were sending or
       receiving, this might result in an interrupt to be processed */
    vMMAC_RadioOff();

    do
    {
        u32State = u32REG_BbcRead(REG_BBC_SM_STATE) & REG_BBC_SM_STATE_SUP_MASK;
        u32State |= u32MMAC_GetPhyState();
    } while (u32State != 0);
}

PUBLIC void vMMAC_SetCutOffTimer(uint32 u32CutOffTime, bool_t bEnable)
{
    /* Using temporary variable reduces code size, thanks to compiler needing
       a hint to not save static variable mid-process */
    uint8 u8TempVal = u8SctlMask;

    vREG_BbcWrite(REG_BBC_SCESL, u32CutOffTime);

    u8TempVal |= REG_BBC_SCTL_CE_MASK;
    if (FALSE == bEnable)
    {
        u8TempVal ^= REG_BBC_SCTL_CE_MASK;
    }

    u8SctlMask = u8TempVal;
}

PUBLIC void vMMAC_SynchroniseBackoffClock(bool_t bEnable)
{
    /* Using temporary variable reduces code size, thanks to compiler needing
       a hint to not save static variable mid-process */
    uint8 u8TempVal = u8SctlMask;

    u8TempVal |= REG_BBC_SCTL_SNAP_MASK;
    if (FALSE == bEnable)
    {
        u8TempVal ^= REG_BBC_SCTL_SNAP_MASK;
    }

    u8SctlMask = u8TempVal;
}

PUBLIC uint8 u8MMAC_EnergyDetect(uint32 u32DurationSymbols)
{
    /* Energy detect is performed synchronously */

#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
    /* Reset energy detect accumulator */
	uint8  u8AccumulatedEnergy = 0;
	uint8  u8SampleEnergy;
    uint32 u32EndTime;

    /* Turn on PHY in RX mode */
    vREG_PhyWrite(REG_PHY_MCTRL, REG_PHY_MCTRL_MPHYON_MASK
                                 | REG_PHY_MCTRL_MIOM_MASK);

    /* Wait for correct state */
    while ((u32REG_PhyRead(REG_PHY_STAT) & REG_PHY_STAT_STATE_MASK)
           != REG_PHY_STAT_STATE_RX);

    /* Use SCFRC to time directly */
    u32EndTime = u32REG_BbcRead(REG_BBC_SCFRC) + u32DurationSymbols;

    while (((int32)(u32EndTime - u32REG_BbcRead(REG_BBC_SCFRC))) > 0)
    {
        /* Clear event status */
        vREG_PhyWrite(REG_PHY_IS, REG_PHY_INT_ED_MASK);

        /* Start energy detect */
        vREG_PhyWrite(REG_PHY_MCTRL, REG_PHY_MCTRL_MPHYON_MASK
                                     | REG_PHY_MCTRL_MIOM_MASK
                                     | REG_PHY_MCTRL_MEDT_MASK);

        /* Wait for completion */
        while (0 == (u32REG_PhyRead(REG_PHY_IS) & REG_PHY_INT_ED_MASK));

        /* Read value */
        u8SampleEnergy = u8MMAC_GetRxLqi(NULL);

        if (u8SampleEnergy > u8AccumulatedEnergy)
        {
            u8AccumulatedEnergy = u8SampleEnergy;
        }
    }

    /* Clear event status */
    vREG_PhyWrite(REG_PHY_IS, REG_PHY_INT_ED_MASK);

    /* Turn off PHY */
    vREG_PhyWrite(REG_PHY_MCTRL, 0);

    return u8AccumulatedEnergy;
#else
    /* JN518x */
    return (uint8)i16Radio_Jn518x_GetRSSI(u32DurationSymbols, FALSE, NULL);
#endif
}

/* Receive */
PUBLIC void vMMAC_SetRxAddress(uint32 u32PanId, uint16 u16Short, tsExtAddr *psMacAddr)
{
    vREG_BbcWrite(REG_BBC_RXMPID, u32PanId);
    vREG_BbcWrite(REG_BBC_RXMSAD, u16Short);
    vREG_BbcWrite(REG_BBC_RXMEADL, psMacAddr->u32L);
    vREG_BbcWrite(REG_BBC_RXMEADH, psMacAddr->u32H);
}

PUBLIC void vMMAC_SetRxPanId(uint32 u32PanId)
{
    vREG_BbcWrite(REG_BBC_RXMPID, u32PanId);
}

PUBLIC void vMMAC_SetRxShortAddr(uint16 u16Short)
{
    vREG_BbcWrite(REG_BBC_RXMSAD, u16Short);
}

PUBLIC void vMMAC_SetRxExtendedAddr(tsExtAddr *psMacAddr)
{
    vREG_BbcWrite(REG_BBC_RXMEADL, psMacAddr->u32L);
    vREG_BbcWrite(REG_BBC_RXMEADH, psMacAddr->u32H);
}

PUBLIC void vMMAC_SetRxStartTime(uint32 u32Time)
{
    vREG_BbcWrite(REG_BBC_RXETST, u32Time);
}

PUBLIC void vMMAC_StartMacReceive(tsMacFrame *psFrame, teRxOption eOptions)
{
    uint32 u32RxOptions = ((uint32)eOptions) & 0xff;
    uint32 u32RxConfig = (((uint32)eOptions) >> 8) & 0xff;

    /* Disable TX, just in case */
    vREG_BbcWrite(REG_BBC_TXCTL, 0x0);

    /* Ensure MAC mode is enabled, with any pre-configured settings */
    vREG_BbcWrite(REG_BBC_SCTL, u8SctlMask);

    /* Set RX buffer pointer */
    vREG_BbcWrite(REG_BBC_RXBUFAD, (uint32)psFrame);

    /* Start RX */
    vREG_BbcWrite(REG_BBC_RXPROM, u32RxConfig);
    vMMAC_RxCtlUpdate(u32RxOptions);
}

PUBLIC void vMMAC_StartPhyReceive(tsPhyFrame *psFrame, teRxOption eOptions)
{
    uint32 u32RxOptions = ((uint32)eOptions) & 0xff;
    uint32 u32RxConfig = (((uint32)eOptions) >> 8) & 0xff;

    /* Disable TX, just in case */
    vREG_BbcWrite(REG_BBC_TXCTL, 0x0);

    /* Ensure PHY mode is enabled, with any pre-configured settings */
    vREG_BbcWrite(REG_BBC_SCTL, 0x20 | u8SctlMask);

    /* Set RX buffer pointer */
    vREG_BbcWrite(REG_BBC_RXBUFAD, (uint32)psFrame);

    /* Start RX */
    vREG_BbcWrite(REG_BBC_RXPROM, u32RxConfig);
    vMMAC_RxCtlUpdate(u32RxOptions);
}

PUBLIC bool_t bMMAC_RxDetected(void)
{
    if (u32REG_BbcRead(REG_BBC_RXSTAT) & REG_BBC_RXSTAT_INPKT_MASK)
    {
        return TRUE;
    }

    return FALSE;
}

PUBLIC uint32 u32MMAC_GetRxErrors(void)
{
    return u32REG_BbcRead(REG_BBC_RXSTAT);
}

PUBLIC uint32 u32MMAC_GetRxTime(void)
{
    return u32REG_BbcRead(REG_BBC_RXTSTP);
}

PUBLIC uint8 u8MMAC_GetRxLqi(uint8 *pu8Msq)
{
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
	uint32 u32MStat = u32REG_PhyRead(REG_PHY_MSTAT0);
    uint8  u8Ed;

    if (pu8Msq)
    {
        *pu8Msq = (u32MStat & REG_PHY_MSTAT_SQI_MASK) >> REG_PHY_MSTAT_SQI_BIT;
    }

    u8Ed = (u32MStat & REG_PHY_MSTAT_ED_MASK) >> REG_PHY_MSTAT_ED_BIT;

 #if (defined JENNIC_CHIP_JN5169) || (defined JENNIC_CHIP_FAMILY_JN517x)
    /* Modify reported ED value to match JN5168 */
    if (u8Ed > 8)
    {
        u8Ed -= 8;
    }
    else
    {
        u8Ed = 0;
    }
 #endif

#else
    if (pu8Msq)
    {
    	/* CJGTODO: Not yet implemented */
        *pu8Msq = 0;
    }

    /* Chip returns dBm value in 10 bits (signed, to 2 binary places). We want
     * to convert this to 0-255 range, where 0 is about -100dBm and 255 is
     * about 10dBm */
    uint32 u32Rssi;
    uint8  u8Ed;

    u32Rssi = JN518X_RFPMODEM->rx_datapath.mf_rssi_dbm_packet.val;

    /* Following assumes 10 bits starting at bit 0, so we check for this at
     * build time */
 #if (EXTAPB_REGFILE_RX_DP_MF_RSSI_DBM_PACKET_MF_RSSI_DBM_MASK != 0x3ff)
  #error Confirm bit fields for mf_rssi_dbm_packet mf_rssi_dbm field
 #endif

    /* Call generic RSSI-to-ED converter */
    u8Ed = u8RssiToED(u32Rssi);
#endif

    return u8Ed;
}

/* Transmit */
PUBLIC void vMMAC_SetTxParameters(uint8 u8Attempts, uint8 u8MinBE,
                                  uint8 u8MaxBE, uint8 u8MaxBackoffs)
{
    vREG_BbcWrite(REG_BBC_TXRETRY, u8Attempts);
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
    vREG_BbcWrite(REG_BBC_TXMBEBT,
                  (  (((uint32)u8MinBE)       << REG_BBC_TXMBEBT_MINBE_BIT)
                   | (((uint32)u8MaxBackoffs) << REG_BBC_TXMBEBT_MAXBO_BIT)
                   | (((uint32)u8MaxBE)       << REG_BBC_TXMBEBT_MAXBE_BIT)
                  )
                 );
#else
    vREG_BbcReadModWrite32(REG_BBC_TXMBEBT,
                           (  REG_BBC_TXMBEBT_MINBE_MASK
                            | REG_BBC_TXMBEBT_MAXBO_MASK
                            | REG_BBC_TXMBEBT_MAXBE_MASK
                           ),
                           (
                              (((uint32)u8MinBE)       << REG_BBC_TXMBEBT_MINBE_BIT)
                            | (((uint32)u8MaxBackoffs) << REG_BBC_TXMBEBT_MAXBO_BIT)
                            | (((uint32)u8MaxBE)       << REG_BBC_TXMBEBT_MAXBE_BIT)
                           )
                          );
#endif
}

PUBLIC void vMMAC_SetTxStartTime(uint32 u32Time)
{
    vREG_BbcWrite(REG_BBC_TXTSTP, u32Time);
}

PUBLIC void vMMAC_SetCcaMode(teCcaMode eCcaMode)
{
    uint32 u32Val;

    /* Store value directly into register */
    u32Val = u32REG_PhyRead(REG_PHY_MCCA);
    u32Val &= ~REG_PHY_MCCA_CCAM_MASK;
    u32Val |= ((uint32)eCcaMode) << REG_PHY_MCCA_CCAM_BIT;
    vREG_PhyWrite(REG_PHY_MCCA, u32Val);
}

PUBLIC void vMMAC_StartMacTransmit(tsMacFrame *psFrame, teTxOption eOptions)
{
    /* Disable RX and reset CSMA context, just in case */
    vMMAC_RadioToOffAndWait();
    vREG_BbcWrite(REG_BBC_TXCSMAC, 0x0);

    /* Ensure MAC mode is enabled, with any pre-configured settings */
    vREG_BbcWrite(REG_BBC_SCTL, u8SctlMask);

    /* Set TX buffer pointer */
    vREG_BbcWrite(REG_BBC_TXBUFAD, (uint32)psFrame);

    /* Start TX */
    vREG_BbcWrite(REG_BBC_TXCTL, (uint32)eOptions);
}

PUBLIC void vMMAC_StartPhyTransmit(tsPhyFrame *psFrame, teTxOption eOptions)
{
    /* Disable RX and reset CSMA context, just in case */
    vMMAC_RadioToOffAndWait();
    vREG_BbcWrite(REG_BBC_TXCSMAC, 0x0);

    /* Ensure PHY mode is enabled, with any pre-configured settings */
    vREG_BbcWrite(REG_BBC_SCTL, 0x20 | u8SctlMask);

    /* Set TX buffer pointer */
    vREG_BbcWrite(REG_BBC_TXBUFAD, (uint32)psFrame);

    /* Start TX */
    vREG_BbcWrite(REG_BBC_TXCTL, (uint32)eOptions);
}

PUBLIC uint32 u32MMAC_GetTxErrors(void)
{
    return u32REG_BbcRead(REG_BBC_TXSTAT);
}

PUBLIC void vMMAC_GetMacAddress(tsExtAddr *psMacAddr)
{
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
    /* Get MAC address from index sector. First check for MAC in customer
       area */
    psMacAddr->u32L = *(uint32 *)(MAC_ADDR_CUSTOMER + 4);
    psMacAddr->u32H = *(uint32 *)(MAC_ADDR_CUSTOMER);

    /* If customer MAC is blank, use default instead. Index sector is all
       1s if blank */
    if (   (psMacAddr->u32L == 0xffffffff)
        && (psMacAddr->u32H == 0xffffffff)
       )
    {
        psMacAddr->u32L = *(uint32 *)(MAC_ADDR_DEFAULT + 4);
        psMacAddr->u32H = *(uint32 *)(MAC_ADDR_DEFAULT);
    }
#else
    uint64_t u64MacAddr;

    /* For JN518x: check pFlash for customer MAC address. API function
     * returns 0 on failure, also MAC address of 0 means that it has not been
     * written */
    u64MacAddr = psector_ReadIeee802_15_4_MacId1();

    if (0 != u64MacAddr)
    {
    	/* Valid, so use it */
    	psMacAddr->u32L = U64_LOWER_U32(u64MacAddr);
    	psMacAddr->u32H = U64_UPPER_U32(u64MacAddr);
    }
    else
    {
		/* Get the factory MAC address from the N-2 page */
		psMacAddr->u32L = *(volatile uint32 *)(MAC_ADDR_DEFAULT);
		psMacAddr->u32H = *(volatile uint32 *)(MAC_ADDR_DEFAULT + 4);
    }
#endif
}

PUBLIC uint32 u32MMAC_PollInterruptSource(uint32 u32Mask)
{
    uint32 u32Isr;

    /* Read pending interrupt sources and apply mask */
    u32Isr = u32REG_BbcRead(REG_BBC_ISR) & u32Mask;

    /* Clear them */
    vREG_BbcWrite(REG_BBC_ISR, u32Isr);

    /* Return them */
    return u32Isr;
}

PUBLIC uint32 u32MMAC_PollInterruptSourceUntilFired(uint32 u32Mask)
{
    uint32 u32Isr;

    /* Read pending interrupt sources until masked value gives non-zero
       result (assumes interrupts are not enabled, otherwise the sources
       will be cleared automatically when generated) */
    do
    {
        u32Isr = u32REG_BbcRead(REG_BBC_ISR) & u32Mask;
    } while (u32Isr == 0);

    /* Clear them */
    vREG_BbcWrite(REG_BBC_ISR, u32Isr);

    /* Return them */
    return u32Isr;
}

PUBLIC void vMMAC_ConfigureInterruptSources(uint32 u32Mask)
{
    /* Enable TX and RX interrupts within BBC: allows them to wake CPU from
       doze, but not enabled enough to generate an interrupt (see
       vMMAC_EnableInterrupts for that) */
#if defined RFP_MODEM_WORKAROUND
    u32Mask |= REG_BBC_INT_T0_MASK;
#endif
    vREG_BbcWrite(REG_BBC_IER, u32Mask);
}

#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
PUBLIC void vUpdateRegisters(void)
{
    uint8  *pu8LookupAddr = (uint8 *)INDEX_ADDR(LOOKUP_PAGE, LOOKUP_START_WORD);
    uint8  *pu8EndAddr = (uint8 *)INDEX_ADDR(LOOKUP_PAGE, LOOKUP_END_WORD + 1);
    uint32  u32RegValue;
    uint32  u32RegAddr;
    uint32  u32NewValue;
    uint8   u8BitShift;

    do
    {
        /* Extract next address (bits 15:8 of AVP) */
        u32RegAddr = pu8LookupAddr[1];
        if (u32RegAddr & (1 << 7))
        {
            /* Reached end of list */
            pu8LookupAddr = pu8EndAddr;
        }
        else
        {
            /* Work out how many bits to shift the new byte value by */
            u8BitShift = (u32RegAddr & 0x3) * 8;

            /* Determine address (word aligned) and extract value (bits 7:0
               of AVP) */
            u32RegAddr = PHY_BASE_ADDR + (u32RegAddr & 0x7c);
            u32NewValue = pu8LookupAddr[0];

            /* Read current value of register */
            u32RegValue = *(volatile uint32 *)u32RegAddr;

            /* Mask out then insert new value based on byte offset within
               word */
            u32RegValue |= (0xffUL << u8BitShift);
            u32RegValue ^= (0xffUL << u8BitShift);
            u32RegValue |= (u32NewValue << u8BitShift);

            /* Write value into register */
            *(volatile uint32 *)u32RegAddr = u32RegValue;

            /* Move to next entry in list */
            pu8LookupAddr += 2;
        }
    } while (pu8LookupAddr < pu8EndAddr);
}
#endif

/* Return the current state of the PHY, by reading the TMU global_state
   register. The bits are as follows:
   - global_state[0] = 1 when the global power up of the analog modules is
                         ongoing (assertion G1/G2 signals)
   - global_state[1] = 1 when the global analog modules are powered up, but
                         no actual Rx/Tx is ongoing yet (in this state eg the
                         calibration routines can be run)
   - global_state[2] = 1 when digital TX part is active
   - global_state[3] = 1 when digital RX part is active
   - global_state[4] = 1 short transition state when TX/RX is finished
   - global_state[5] = 1 when the global power down of the analog modules is
                         ongoing(de-assertion G1/G2 signals)
   - global_state[6] = 1 when the power up of the TX analog modules is ongoing
                         (assertion TX1/TX2/TX3 signals)
   - global_state[7] = 1 when the power down of the TX analog modules is
                         ongoing (de-assertion TX1/TX2/TX3 signals)
   - global_state[8] = 1 when the power up of the RX analog modules is ongoing
                         (assertion RX1/RX2/TX3 signals)
   - global_state[9] = 1 when the power down of the RX analog modules is
                         ongoing (de-assertion RX1/RX2/RX3 signals)

   Note the bits in the range global_state[5:0] are mutually exclusive. Only 1
   bit at a time can be asserted. When none of these bits is active, the RFP
   is completely idle.
*/
PUBLIC uint32 u32MMAC_GetPhyState(void)
{
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
    return u32REG_PhyRead(REG_PHY_STAT) & REG_PHY_STAT_STATE_MASK;
#else
    uint32 u32Store;
    uint32 u32State;

    MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Store);

    JN518X_RFPMODEM->test.version_set_snap.fields.version = 1;
    do
    {
        volatile int i;
        /* Short delay */
        for (i = 0; i < 10; i++);
    } while (JN518X_RFPMODEM->test.status.fields.snack == 0);

    JN518X_RFPMODEM->test.reset_snap.val = 1;

    u32State = JN518X_RFPMODEM->tmu.global_status.fields.global_state;

    MICRO_RESTORE_INTERRUPTS(u32Store);

    return u32State;
#endif
}

#if defined RFP_MODEM_WORKAROUND
PUBLIC void vMMAC_CheckRxStarted(void)
{
    int iLoopCount;
    uint32 u32RfpGlobalStatus;
    uint32 u32PreCountOrig;

    /* As the interrupt bit is checked after the TX and RX interrupt bits,
       there is a slim possibility that it fired at the same time that some
       other activity completed, that activity changed what the radio is now
       expected to be doing and hence this timeout is no longer appropriate.
       However, we can live with this: a false positive resulting in an
       unnecessary recovery is not the end of the world. Also, we do have a
       flag that we can check to cope with the case where the RX  has now been
       stopped */
    if (bWorkaroundTimerRunning)
    {
        bWorkaroundTimerRunning = FALSE;

        /* If we are also transmitting (such as for RX in CCA) then we don't
           try any recovery as the radio state may be transitioning and so is
           harder to check reliably. In this case, a lock-up will be recovered
           from by the TX lock-up recovery code */
        if (0 == u32REG_BbcRead(REG_BBC_TXCTL))
        {
            /* Not in TX so check radio status, which should now be at RX
               state */
            u32RfpGlobalStatus = u32MMAC_GetPhyState();
		    if (0 == (u32RfpGlobalStatus & 8))
		    {
		        /* Not in RX, so perform recovery procedure.
		           Store pre-count value, then replace with maximum value.
		           This should cause RFP state to move out of stuck state
		           waiting for PLL */
                u32PreCountOrig = JN518X_RFPMODEM->tmu.comparator_pre_g2.fields.comparator_pre;
                JN518X_RFPMODEM->tmu.comparator_pre_g2.fields.comparator_pre = 0x7FF;

                /* Wait for change to take effect */
                iLoopCount = 0;
                do
                {
                	iLoopCount++;
                    u32RfpGlobalStatus = u32MMAC_GetPhyState();
                } while ((iLoopCount < 1000) && (0 == (u32RfpGlobalStatus & 8)));

        		/* Restore pre-count value */
        		JN518X_RFPMODEM->tmu.comparator_pre_g2.fields.comparator_pre = u32PreCountOrig;
		    }
    	}
    }
}

PUBLIC void vMMAC_AbortRadio(void)
{
    /* Recovery code is from Steve Bate, 24th August 2017 */
    int iLoopCount;
    uint32 u32RfpGlobalStatus;
    uint32 u32PreCountOrig;

    /* Store pre-count value, then replace with maximum value. This should
       cause RFP state to move out of stuck state waiting for PLL */
    u32PreCountOrig = JN518X_RFPMODEM->tmu.comparator_pre_g2.fields.comparator_pre;
    JN518X_RFPMODEM->tmu.comparator_pre_g2.fields.comparator_pre = 0x7FF;

    /* RFP will then move to RX, so we can send an abort, and it will then
       move to TX, so we can send another abort. The RFP global state goes
       to 0 for one cycle during this, so we must be robust to that. This
       loop deals with all of these factors */
    iLoopCount = 2;
    while (iLoopCount > 0)
    {
        u32RfpGlobalStatus = u32MMAC_GetPhyState();

        /* If status is 'RX' or 'TX', trigger an abort */
	    if ((u32RfpGlobalStatus & 0x4) || (u32RfpGlobalStatus & 0x08))
	    {
	        /* Trigger abort; register does not need clearing afterwards */
	        JN518X_RFPMODEM->tmu.triggers.val = 0x4;
	    }

        /* If status is 0, count down: need this to happen twice before we
           can exit */
	    if (u32RfpGlobalStatus == 0x0)
	    {
	    	iLoopCount--;
	    }
	}

	/* Restore pre-count value */
	JN518X_RFPMODEM->tmu.comparator_pre_g2.fields.comparator_pre = u32PreCountOrig;
}
#endif

PUBLIC void vMMAC_RxCtlUpdate(uint32 u32NewValue)
{
    /* Write to the register */
    vREG_BbcWrite(REG_BBC_RXCTL, u32NewValue);

#if defined RFP_MODEM_WORKAROUND
    /* Guarantee interrupt protection around this area, as we stop, start and
       clear interrupts */
    uint32 u32Store;

    MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Store);

    if (u32NewValue)
    {
        /* Start timer: 192us should be more than sufficient */
        bWorkaroundTimerRunning = TRUE;
        vREG_BbcWrite(REG_BBC_SCTR0, 12);
    }
    else
    {
        /* Clear timer */
        bWorkaroundTimerRunning = FALSE;
        vREG_BbcWrite(REG_BBC_SCTR0, 0);
    }

    /* Clear ISR, in case it has already fired */
    vREG_BbcWrite(REG_BBC_ISR, REG_BBC_INT_T0_MASK);

    MICRO_RESTORE_INTERRUPTS(u32Store);
#endif
}

PUBLIC void vMMAC_SetHighPowerOptions(void)
{
    uint32 u32RegData;
#if !(defined JENNIC_CHIP_FAMILY_JN516x) && !(defined JENNIC_CHIP_FAMILY_JN517x)
    uint32 u32Thresh;
#endif

#if (defined JENNIC_CHIP_FAMILY_JN516x) && !(defined JENNIC_CHIP_JN5169)

    /* Module settings                  STD   M05   M06 */
    uint8 const au8CcaThresholds[3] = {  57,   68,   96};
    uint8 const au8PaAtten[3]       = {   0,    0,    1};
    uint8 const au8TxOffset[3]      = {0x7f, 0x7f, 0x7c};

    /* Set CCA threshold based on module type */
    u32RegData = u32REG_PhyRead(REG_PHY_MCCA);
    u32RegData &= 0xfffff00fUL;
    u32RegData |= ((uint32)(au8CcaThresholds[(uint8)eMMacModuleType])) << 4;
    vREG_PhyWrite(REG_PHY_MCCA, u32RegData);

    /* Set PA attenuation based on module type */
    u32RegData = u32REG_PhyRead(REG_PHY_PA_CTRL);
    u32RegData &= 0xffffff8fUL;
    u32RegData |= ((uint32)(au8PaAtten[(uint8)eMMacModuleType])) << 4;
    vREG_PhyWrite(REG_PHY_PA_CTRL, u32RegData);

    /* Change IDLE to TX offset based on module type */
    vREG_PhyWrite(REG_PHY_VCO_TXO, au8TxOffset[(uint8)eMMacModuleType]);

#else

    /* Just copy stored values, if set */
    if (u8MMAC_HpmCcaThreshold)
    {
        /* Set CCA threshold based on module type */
        u32RegData = u32REG_PhyRead(REG_PHY_MCCA);
        u32RegData &= ~REG_PHY_MCCA_CCA_ED_THR_MASK;
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
        u32RegData |= (((uint32)(u8MMAC_HpmCcaThreshold)) << REG_PHY_MCCA_CCA_ED_THR_BIT);
#else
        /* Threshold is now 10 bits wide and a signed dBm value, with a
           resolution down to 0.25 dB, as with power level setting. Thus the
           requested 8-bit value, which is in the same form as an ED/LQI value
           (0 to 255), must be converted by the inverse of the RSSI->ED
           calculation, so:
               <reg val> = <API val> / 0.57954545 - 400

           Note that value is on the same scale as the RSSI and linear part of
           the ED value, so 0x2d9 equates to -70dBm (ish).

           We do the divide by 0.57954545 (multiply by 1.72549021) as multiply
           by 14474461 then divide by 8388608; 8388608 is (1 << 23), chosen
           for fast division */
        u32Thresh = (uint32)u8MMAC_HpmCcaThreshold * 14474461UL;
        u32Thresh >>= 23;
        u32Thresh = (uint32)((int32)u32Thresh - 400L);

        /* Shift, mask and OR value back into register */
        u32Thresh <<= REG_PHY_MCCA_CCA_ED_THR_BIT;
        u32Thresh &= REG_PHY_MCCA_CCA_ED_THR_MASK;
        u32RegData |= u32Thresh;
#endif
        vREG_PhyWrite(REG_PHY_MCCA, u32RegData);
    }

#endif
}

#if !(defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_JN5169)
/* Sets high-power module (HPM) settings for CCA threshold and maximum TX
   power, with a separate setting for channel 26. To support new FCC
   compliance, have to turn down the power on channel 26 even on standard
   modules so the same function is used for that, too. */
PUBLIC void vMMAC_SetComplianceLimits(int8  i8TxMaxPower,
                                      int8  i8TxMaxPowerCh26,
                                      uint8 u8CcaThreshold)
{
    /* Store values for use later */
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
    i8MMAC_HpmTxMaxPower     = i8TxMaxPower;
    i8MMAC_HpmTxMaxPowerCh26 = i8TxMaxPowerCh26;
#else /* JN518x */
    vRadio_SetComplianceLimits(i8TxMaxPower, i8TxMaxPowerCh26);
#endif
    u8MMAC_HpmCcaThreshold   = u8CcaThreshold;
}
#endif

PUBLIC int8 i8MMAC_GetTxPowerLevel(void)
{
#if ((defined JENNIC_CHIP_FAMILY_JN514x) || (defined JENNIC_CHIP_FAMILY_JN516x)) && !(defined JENNIC_CHIP_JN5169)

    int8 const ai8TxPower[] = {-32, -20, -9, 0};
    uint8  u8PapValue;
    uint32 u32RegData;

    u32RegData = u32REG_PhyRead(REG_PHY_PA_CTRL);
    u8PapValue = (u32RegData & REG_PHY_PA_CTRL_PAP_MASK) >> REG_PHY_PA_CTRL_PAP_BIT;

    return ai8TxPower[u8PapValue];

#elif (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)

    /* For integer operation, all values are x10 */
    int16 const ai16TxPower[] = {-322, -294, -183, -72, 37, 85};
    int8  const ai8TxInc[] = {0, 8, 12, 16};
    int8  const ai8TxAtten[] = {0, -25};
    int16 i16Total;
    uint8  u8PapValue;
    uint32 u32RegData;

    u32RegData = u32REG_PhyRead(REG_PHY_PA_CTRL);
    u8PapValue = (u32RegData & REG_PHY_PA_CTRL_PAP_MASK) >> REG_PHY_PA_CTRL_PAP_BIT;

    i16Total = ai16TxPower[u8PapValue] + ai8TxInc[u8PowerAdj] + ai8TxAtten[u8Atten3db];

    /* Convert result to correct value by dividing by 10, with rounding
       (negatives round up, so -0.5 becomes 0) */
    return (int8)((i16Total + 5) / 10);

#else /* JN518x onwards */
    return i8Radio_GetTxPowerLevel_dBm();
#endif
}

#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
#ifdef INCLUDE_ECB_API
/* Security */
PUBLIC void MMAC_EcbWriteKey(uint32 au32KeyData[4])
{
    /* Wait until idle */
    vAesCmdWaitBusy();

    /* Write the new key */
    vREG_AesWrite(REG_AES_DATA_IN + 0, au32KeyData[0]);
    vREG_AesWrite(REG_AES_DATA_IN + 1, au32KeyData[1]);
    vREG_AesWrite(REG_AES_DATA_IN + 2, au32KeyData[2]);
    vREG_AesWrite(REG_AES_DATA_IN + 3, au32KeyData[3]);

    /* Issue command */
    vREG_AesWrite(REG_AES_ACL_CMD, REG_AES_ACL_CMD_SET_KEY);
}

/****************************************************************************
 *
 * NAME:       vMMAC_EcbEncodeStripe
 *
 * DESCRIPTION:
 * Encodes a 128-bit data stripe using the AES Coprocessor. The input buffers MUST
 * be multiples of 128-bits. The function, upon return indicates how many stripes
 * in the input buffer have been decoded. In cases where the software loses context
 * on the AES Coprocessor, this allows the the process to continue where it left off
 * once it manages to regain context.
 *
 * PARAMETERS:      Name                RW  Usage
 *                  pau8inputData       R   pointer to user allocated input data buffer
 *                  pau8outputData      W   pointer to user allocated output data buffer
 *
 ****************************************************************************/
PUBLIC void vMMAC_EcbEncodeStripe(
        uint32 au32InputData[4],
        uint32 au32OutputData[4])
{
    /* Wait */
    vAesCmdWaitBusy();

    /* Pass in data */
    vREG_AesWrite(REG_AES_DATA_IN + 0, au32InputData[0]);
    vREG_AesWrite(REG_AES_DATA_IN + 1, au32InputData[1]);
    vREG_AesWrite(REG_AES_DATA_IN + 2, au32InputData[2]);
    vREG_AesWrite(REG_AES_DATA_IN + 3, au32InputData[3]);

    /* Issue command */
    vREG_AesWrite(REG_AES_ACL_CMD, REG_AES_ACL_CMD_GO);

    /* Blocking wait for encode to complete */
    vAesCmdWaitBusy();

    /* copy data into the user supplied buffer */
    au32OutputData[0] = u32REG_AesRead(REG_AES_DATA_OUT + 0);
    au32OutputData[1] = u32REG_AesRead(REG_AES_DATA_OUT + 1);
    au32OutputData[2] = u32REG_AesRead(REG_AES_DATA_OUT + 2);
    au32OutputData[3] = u32REG_AesRead(REG_AES_DATA_OUT + 3);
}
#endif
#endif

PUBLIC void vMMAC_IntHandlerBbc(void)
{
    uint32 u32Isr;

    /* Read enabled interrupts */
    u32Isr = u32REG_BbcRead(REG_BBC_MISR);

    /* Clear them */
    vREG_BbcWrite(REG_BBC_ISR, u32Isr);

#if defined RFP_MODEM_WORKAROUND
    if (u32Isr & REG_BBC_INT_T0_MASK)
    {
        vMMAC_CheckRxStarted();
        u32Isr &= ~REG_BBC_INT_T0_MASK;
    }
#endif

    /* Pass result to callback, if registered */
    if (prIntHandler)
    {
        prIntHandler(u32Isr);
    }
}

#if (defined JENNIC_CHIP_FAMILY_JN518x)
/* RFT1778: Bad CRC causes AGC to lock. Handle this after the ZBMODEM EOP
 * interrupt fires, with code in the radio driver */
PUBLIC void vMMAC_IntHandlerPhy(void)
{
    uint32 u32EventStatus;

    /* Read enabled interrupts */
    u32EventStatus = JN518X_ZBMODEM->ZB_EVENTS_STATUS & JN518X_ZBMODEM->ZB_EVENTS_ENABLE;

    /* Clear them */
    JN518X_ZBMODEM->ZB_EVENTS_CLEAR = u32EventStatus;

    /* If EOP has fired, call patch  for RFT1788 */
    if (0 != (u32EventStatus & ZB_MODEM_ZB_EVENTS_EOP))
    {
        /* Function returns a value even though the coding standard suggests
         * that it doesn't; we don't care so cast to void anyway */
        (void)vRadio_Jn518x_RFT1778_bad_crc();
    }

    /* Pass result to callback, if registered */
    if (prPhyIntHandler)
    {
        prPhyIntHandler(u32EventStatus);
    }
}
#endif

/****************************************************************************/
/***        Private Functions                                             ***/
/****************************************************************************/
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
PRIVATE uint8 u8GetPapValue(int8 i8TxPower)
{
#if ((defined JENNIC_CHIP_FAMILY_JN514x) || (defined JENNIC_CHIP_FAMILY_JN516x)) && !(defined JENNIC_CHIP_JN5169)
    uint8 u8TxPower;

    /* Obtain value to store in PAP register.
       Possible PAP values are  meaning (dBm)
                  3              0
                  2             -9
                  1             -20
                  0             -32
    */

    /* If value is positive we truncate it to 0dBm by setting the value to 64.
       Hence we end up with values from 64 down to 32 to indicate 0 to -32 */
    if (i8TxPower >= 0)
    {
        u8TxPower = 64;
    }
    else
    {
        u8TxPower = (uint8)((int8)64 + i8TxPower);
    }

    /* Now map the requested TX power to the available values.
       Doing the sums:
         u8TxPower  meaning (dBm)  gives u8PapValue so (dBm)
             64         0                    3           0
             63-52    -1 to -12              2          -9
             51-40   -13 to -24              1         -20
             39-32   -25 to -32              0         -32

       This is actually outside tolerance for -1, -2, -13 and -25 dBm!
       Subtracting 26 or 25 instead of 28 gives the best fit and within tolerance
     */
    return (uint8)(((uint32)(u8TxPower - 28)) / (uint32)12);
#else
    /* For JN5169 onwards, we have 6 power levels from -32 to +10 dBm.
          Possible PAP | meaning
           values are  |  (dBm)
          -------------+---------
                5      |   8.5
                4      |   3.7
                3      |  -7.2
                2      | -18.3
                1      | -29.4
                0      | -32.2

       There are also two adjustments that can be made:
         increments of 0, +0.8dB, +1.2dB and +1.6dB
         attenuation of 2.5dB

       To map all of this from the requested dBm level we're going to use a
       table */
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"
    static struct __attribute__ ((__packed__))
    {
        unsigned int iPapValue      : 3;
        unsigned int iTXPowerAdjust : 2;
        unsigned int iAtten3db      : 1;
        unsigned int iPad           : 2;
    } const asPowerSettings[] = {
                            {5, 3, 0}, /* +10dBm */
                            {5, 1, 0},
                            {5, 0, 0},
                            {5, 2, 1},
                            {5, 0, 1},
                            {4, 2, 0},
                            {4, 0, 0},
                            {4, 3, 1},
                            {4, 2, 1}, /*   2dBm (actually 2.4dBm) */
                            {4, 1, 1}, /*   1dBm (actually 2.0dBm) */
                            {4, 0, 1}, /*   0dBm (actually 1.2dBm) */
                            {4, 0, 1},
                            {4, 0, 1},
                            {3, 3, 0},
                            {3, 3, 0},
                            {3, 3, 0},
                            {3, 3, 0},
                            {3, 0, 0},
                            {3, 3, 1},
                            {3, 2, 1},
                            {3, 0, 1}, /* -10dBm */
                            {3, 0, 1},
                            {3, 0, 1},
                            {3, 0, 1},
                            {2, 3, 0},
                            {2, 3, 0},
                            {2, 3, 0},
                            {2, 2, 0},
                            {2, 0, 0},
                            {2, 3, 1},
                            {2, 2, 1}, /* -20dBm */
                            {2, 1, 1},
                            {2, 0, 1},
                            {2, 0, 1},
                            {2, 0, 1},
                            {1, 3, 0},
                            {1, 3, 0},
                            {1, 3, 0},
                            {1, 2, 0},
                            {1, 0, 0},
                            {1, 0, 0}, /* -30dBm */
                            {1, 3, 1},
                            {0, 0, 0}};
#pragma GCC diagnostic pop

    uint8 u8Row;

    /* Element 0 of array relates to +10dBm, element 1 is +9dBm, etc., all
       the way down to -32dBm */
    if (i8TxPower > 10)
    {
        i8TxPower = 10;
    }
    if (i8TxPower < -32)
    {
        i8TxPower = -32;
    }

    u8Row = (uint8)((int8)10 - i8TxPower);

#if 0 /* Debug build option */
    u8MMAC_TxPowerSetting = u8Row;
#endif

    /* Store settings for use by vJPT_TxPowerAdjust */
    u8PowerAdj = (uint8)asPowerSettings[u8Row].iTXPowerAdjust;
    u8Atten3db = (uint8)asPowerSettings[u8Row].iAtten3db;

    return (uint8)asPowerSettings[u8Row].iPapValue;
#endif
}

#ifdef INCLUDE_ECB_API
/****************************************************************************
 *
 * NAME:       vAesCmdWaitBusy
 *
 * DESCRIPTION:
 * waits until the cmd is complete. Blocking function, polls the wishbone
 * until not busy.
 *
 ****************************************************************************/
PRIVATE void vAesCmdWaitBusy(void)
{
    uint32 u32CmdDataReturn;

    do
    {
        u32CmdDataReturn = u32REG_AesRead(REG_AES_ACL_CMD);
    } while ((u32CmdDataReturn & REG_AES_ACL_CMD_DONE_MASK) == 0);
}
#endif
#endif

#if !(defined JENNIC_CHIP_FAMILY_JN516x) && !(defined JENNIC_CHIP_FAMILY_JN517x)
PRIVATE uint8 u8RssiToED(uint32 u32Rssi)
{
    /* ED value for received frames and for manual energy scan now seems
     * consistent so we are using the same conversion for both. It is
     * sufficiently linear up to +10dBm down to about -75dBm (manual) or
     * across entire range (received frame) so we shall scale the input range
     * from -100dBm up to +10dBm into the output range 0 to 255, truncating at
     * each end. Note that the input value is in dBm, signed, with 2
     * additional fractional bits of precision */

    int32 i32Rssi;

    /* Input was a 10-bit signed value. Sign extend by a combination of shifts
     * and casts */
	i32Rssi = ((int32)(u32Rssi << 22)) >> 22;

    /* Original code was copied into radio driver, so use that version */
    return u8Radio_Jn518x_GetEDfromRSSI(i32Rssi);
}
#endif


PUBLIC bool_t bMMAC_PowerStatus(void)
{
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
    if (u32REG_SysRead(REG_SYS_STAT) & 8)
#else
    if (U_SYSCON->AHBCLKCTRL[1] & SYSCON_AHBCLKCTRL1_ZIGBEE_MASK)
#endif
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************/
/***        End of file                                                   ***/
/****************************************************************************/
