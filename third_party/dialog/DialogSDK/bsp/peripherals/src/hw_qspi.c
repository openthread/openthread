/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup QSPI
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file hw_qspi.c
 *
 * @brief Implementation of the QSPI Low Level Driver.
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

#if dg_configUSE_HW_QSPI



#include <stdint.h>
#include <hw_qspi.h>

#define BITS16(base, reg, field, v) \
        ((uint16) (((uint16) (v) << (base ## _ ## reg ## _ ## field ## _Pos)) & \
                (base ## _ ## reg ## _ ## field ## _Msk)))

#define BITS32(base, reg, field, v) \
        ((uint32) (((uint32) (v) << (base ## _ ## reg ## _ ## field ## _Pos)) & \
                (base ## _ ## reg ## _ ## field ## _Msk)))

#define GETBITS16(base, reg, v, field) \
        ((uint16) (((uint16) (v)) & (base ## _ ## reg ## _ ## field ## _Msk)) >> \
                (base ## _ ## reg ## _ ## field ## _Pos))

#define GETBITS32(base, reg, v, field) \
        ((uint32) (((uint32) (v)) & (base ## _ ## reg ## _ ## field ## _Msk)) >> \
                (base ## _ ## reg ## _ ## field ## _Pos))

static const uint8_t dummy_num[] = { 0, 1, 2, 0, 3 };

#if (dg_configFLASH_POWER_DOWN == 1)
__attribute__((section("text_retained")))
#endif
void hw_qspi_set_bus_mode(HW_QSPI_BUS_MODE mode)
{
    switch (mode)
    {
    case HW_QSPI_BUS_MODE_SINGLE:
        QSPIC->QSPIC_CTRLBUS_REG = REG_MSK(QSPIC, QSPIC_CTRLBUS_REG, QSPIC_SET_SINGLE);
        break;

    case HW_QSPI_BUS_MODE_DUAL:
        QSPIC->QSPIC_CTRLBUS_REG = REG_MSK(QSPIC, QSPIC_CTRLBUS_REG, QSPIC_SET_DUAL);
        break;

    case HW_QSPI_BUS_MODE_QUAD:
        QSPIC->QSPIC_CTRLBUS_REG = REG_MSK(QSPIC, QSPIC_CTRLBUS_REG, QSPIC_SET_QUAD);
        hw_qspi_set_io2_output(false);
        hw_qspi_set_io3_output(false);
        break;
    }
}

#if (dg_configFLASH_POWER_DOWN == 1)
__attribute__((section("text_retained")))
#endif
void hw_qspi_set_automode(bool automode)
{
    if (automode)
    {
        const uint32_t burst_cmd_a = QSPIC->QSPIC_BURSTCMDA_REG;
        const uint32_t burst_cmd_b = QSPIC->QSPIC_BURSTCMDB_REG;
        const uint32_t status_cmd = QSPIC->QSPIC_STATUSCMD_REG;
        const uint32_t erase_cmd_b = QSPIC->QSPIC_ERASECMDB_REG;
        const uint32_t burstbrk = QSPIC->QSPIC_BURSTBRK_REG;

        if (GETBITS32(QSPIC, QSPIC_BURSTCMDA_REG, burst_cmd_a, QSPIC_INST_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_BURSTCMDA_REG, burst_cmd_a, QSPIC_ADR_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_BURSTCMDA_REG, burst_cmd_a, QSPIC_DMY_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_BURSTCMDA_REG, burst_cmd_a, QSPIC_EXT_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_BURSTCMDB_REG, burst_cmd_b, QSPIC_DAT_RX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_BURSTCMDB_REG, burst_cmd_b, QSPIC_DAT_RX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_STATUSCMD_REG, status_cmd, QSPIC_RSTAT_RX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_STATUSCMD_REG, status_cmd, QSPIC_RSTAT_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_ERASECMDB_REG, erase_cmd_b, QSPIC_ERS_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_ERASECMDB_REG, erase_cmd_b, QSPIC_WEN_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_ERASECMDB_REG, erase_cmd_b, QSPIC_SUS_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_ERASECMDB_REG, erase_cmd_b, QSPIC_RES_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_ERASECMDB_REG, erase_cmd_b, QSPIC_EAD_TX_MD) == HW_QSPI_BUS_MODE_QUAD ||
            GETBITS32(QSPIC, QSPIC_BURSTBRK_REG, burstbrk, QSPIC_BRK_TX_MD) == HW_QSPI_BUS_MODE_QUAD)
        {
            hw_qspi_set_io2_output(false);
            hw_qspi_set_io3_output(false);
        }
    }

    HW_QSPIC_REG_SETF(CTRLMODE, AUTO_MD, automode);
}

void hw_qspi_set_read_instruction(uint8_t inst, uint8_t send_once, uint8_t dummy_count,
                                  HW_QSPI_BUS_MODE inst_phase,
                                  HW_QSPI_BUS_MODE addr_phase,
                                  HW_QSPI_BUS_MODE dummy_phase,
                                  HW_QSPI_BUS_MODE data_phase)
{
    QSPIC->QSPIC_BURSTCMDA_REG =
        BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_INST, inst) |
        BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_INST_TX_MD, inst_phase) |
        BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_ADR_TX_MD, addr_phase) |
        BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_DMY_TX_MD, dummy_phase);

    QSPIC->QSPIC_BURSTCMDB_REG =
        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DAT_RX_MD, data_phase) |
        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_INST_MD, send_once) |
        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_FORCE, (dummy_count == 3 ? 1 : 0)) |
        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_NUM, dummy_num[dummy_count]);
}

void hw_qspi_set_wrapping_burst_instruction(uint8_t inst, HW_QSPI_WRAP_LEN len,
                                            HW_QSPI_WRAP_SIZE size)
{
    HW_QSPIC_REG_SETF(BURSTCMDA, INST_WB, inst);
    QSPIC->QSPIC_BURSTCMDB_REG =
        (QSPIC->QSPIC_BURSTCMDB_REG &
         ~(REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_WRAP_SIZE) |
           REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_WRAP_LEN))) |
        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_WRAP_SIZE, size) |
        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_WRAP_LEN, len) |
        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_WRAP_MD, 1);
}

void hw_qspi_set_extra_byte(uint8_t extra_byte, HW_QSPI_BUS_MODE bus_mode, uint8_t half_disable_out)
{
    QSPIC->QSPIC_BURSTCMDA_REG =
        (QSPIC->QSPIC_BURSTCMDA_REG &
         ~(REG_MSK(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_EXT_BYTE) |
           REG_MSK(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_EXT_TX_MD))) |
        BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_EXT_BYTE, extra_byte) |
        BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_EXT_TX_MD, bus_mode);

    QSPIC->QSPIC_BURSTCMDB_REG =
        (QSPIC->QSPIC_BURSTCMDB_REG &
         ~(REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_EXT_BYTE_EN) |
           REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_EXT_HF_DS))) |
        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_EXT_BYTE_EN, 1) |
        BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_EXT_HF_DS, half_disable_out ? 1 : 0);
}

void hw_qspi_set_dummy_bytes_count(uint8_t count)
{
    if (count == 3)
    {
        HW_QSPIC_REG_SETF(BURSTCMDB, DMY_FORCE, 1);
    }
    else
    {
        QSPIC->QSPIC_BURSTCMDB_REG =
            (QSPIC->QSPIC_BURSTCMDB_REG &
             ~(REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_FORCE) |
               REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_NUM))) |
            BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_NUM,
                   dummy_num[count]);
    }
}

void hw_qspi_set_read_status_instruction(uint8_t inst, HW_QSPI_BUS_MODE inst_phase,
                                         HW_QSPI_BUS_MODE receive_phase,
                                         uint8_t busy_pos,
                                         uint8_t busy_val,
                                         uint8_t reset_delay,
                                         uint8_t sts_delay)
{
    QSPIC->QSPIC_STATUSCMD_REG =
        BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_BUSY_VAL, busy_val) |
        BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_BUSY_POS , busy_pos) |
        BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_RSTAT_RX_MD, receive_phase) |
        BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_RSTAT_TX_MD, inst_phase) |
        BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_RSTAT_INST, inst) |
        BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_STSDLY_SEL, sts_delay) |
        BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_RESSTS_DLY, reset_delay);
}

void hw_qspi_set_erase_instruction(uint8_t inst, HW_QSPI_BUS_MODE inst_phase,
                                   HW_QSPI_BUS_MODE addr_phase, uint8_t hclk_cycles,
                                   uint8_t cs_hi_cycles)
{
    HW_QSPIC_REG_SETF(ERASECMDA, ERS_INST, inst);
    QSPIC->QSPIC_ERASECMDB_REG =
        (QSPIC->QSPIC_ERASECMDB_REG &
         ~(REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERS_TX_MD) |
           REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_EAD_TX_MD) |
           REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERSRES_HLD) |
           REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERS_CS_HI))) |
        BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERS_TX_MD, inst_phase) |
        BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_EAD_TX_MD, addr_phase) |
        BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERSRES_HLD, hclk_cycles) |
        BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERS_CS_HI, cs_hi_cycles);
}

void hw_qspi_set_write_enable_instruction(uint8_t write_enable, HW_QSPI_BUS_MODE inst_phase)
{
    HW_QSPIC_REG_SETF(ERASECMDA, WEN_INST, write_enable);
    HW_QSPIC_REG_SETF(ERASECMDB, WEN_TX_MD, inst_phase);
}

void hw_qspi_set_suspend_resume_instructions(uint8_t erase_suspend_inst,
                                             HW_QSPI_BUS_MODE suspend_inst_phase,
                                             uint8_t erase_resume_inst,
                                             HW_QSPI_BUS_MODE resume_inst_phase,
                                             uint8_t minimum_delay)
{
    QSPIC->QSPIC_ERASECMDA_REG =
        (QSPIC->QSPIC_ERASECMDA_REG &
         ~(REG_MSK(QSPIC, QSPIC_ERASECMDA_REG, QSPIC_SUS_INST) |
           REG_MSK(QSPIC, QSPIC_ERASECMDA_REG, QSPIC_RES_INST))) |
        BITS32(QSPIC, QSPIC_ERASECMDA_REG, QSPIC_SUS_INST, erase_suspend_inst) |
        BITS32(QSPIC, QSPIC_ERASECMDA_REG, QSPIC_RES_INST, erase_resume_inst);
    QSPIC->QSPIC_ERASECMDB_REG =
        (QSPIC->QSPIC_ERASECMDB_REG &
         ~(REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_SUS_TX_MD) |
           REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_RES_TX_MD) |
           REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_RESSUS_DLY))) |
        BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_SUS_TX_MD, suspend_inst_phase) |
        BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_RES_TX_MD, resume_inst_phase) |
        BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_RESSUS_DLY, minimum_delay);
}

void hw_qspi_erase_block(uint32_t addr)
{
    if (!hw_qspi_get_automode())
    {
        hw_qspi_set_automode(true);
    }

    // Wait for previous erase to end
    while (hw_qspi_get_erase_status() != 0)
    {
    }

    if (hw_qspi_get_address_size() == HW_QSPI_ADDR_SIZE_32)
    {
        addr >>= 12;
    }
    else
    {
        addr >>= 4;
    }

    // Setup erase block page
    HW_QSPIC_REG_SETF(ERASECTRL, ERS_ADDR, addr);
    // Fire erase
    HW_QSPIC_REG_SETF(ERASECTRL, ERASE_EN, 1);
}

void hw_qspi_set_break_sequence(uint16_t sequence, HW_QSPI_BUS_MODE mode,
                                HW_QSPI_BREAK_SEQ_SIZE size, uint8_t dis_out)
{
    QSPIC->QSPIC_BURSTBRK_REG =
        BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_SEC_HF_DS, dis_out) |
        BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_BRK_SZ, size) |
        BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_BRK_TX_MD, mode) |
        BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_BRK_EN, 1) |
        BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_BRK_WRD, sequence);
}

void hw_qspi_set_pads(HW_QSPI_SLEW_RATE rate, HW_QSPI_DRIVE_CURRENT current)
{
    QSPIC->QSPIC_GP_REG =
        BITS16(QSPIC, QSPIC_GP_REG, QSPIC_PADS_SLEW, rate) |
        BITS16(QSPIC, QSPIC_GP_REG, QSPIC_PADS_DRV, current);
}

void hw_qspi_init(const qspi_config *cfg)
{
    hw_qspi_enable_clock();
    hw_qspi_set_automode(false);
    hw_qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);
    hw_qspi_set_io2_output(true);
    hw_qspi_set_io2(1);
    hw_qspi_set_io3_output(true);
    hw_qspi_set_io3(1);

    if (cfg)
    {
        hw_qspi_set_address_size(cfg->address_size);
        hw_qspi_set_clock_mode(cfg->idle_clock);
        hw_qspi_set_read_sampling_edge(cfg->sampling_edge);
    }
}

__RETAINED_CODE void hw_qspi_set_div(HW_QSPI_DIV div)
{
    REG_SETF(CRG_TOP, CLK_AMBA_REG, QSPI_DIV, div);
}

#endif /* dg_configUSE_HW_QSPI */
/**
 * \}
 * \}
 * \}
 */
