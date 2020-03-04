/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ROM_MPU_H_
#define _ROM_MPU_H_

#if defined __cplusplus
extern "C" {
#endif

/*!
 * @addtogroup ROM_API
 * @{
 */

/*! @file */

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <rom_common.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/*! @brief bits for access right */
#define RD_RIGHT (1 << 0)
#define WR_RIGHT (1 << 1)
#define X_RIGHT (1 << 2)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/*! @brief enum MpuRegion_t index of ARM CM4 MPU regions
 *  The MPU can describe up to 8 region rules.
 *  Note that a higher order rule prevails over the lower ones.
 * The boot ROM makes use of upper order rules : 5 .. 7.
 * Rule 0 is a 'background' rule that opens the whole memory plane.
 */
enum MpuRegion_t
{
    MPU_REGION_0, /**< Boot Reserved: background rule */
    MPU_REGION_1, /**< General purpose rule */
    MPU_REGION_2, /**< General purpose rule */
    MPU_REGION_3, /**< General purpose rule */
    MPU_REGION_4, /**< General purpose rule */
    MPU_APP_LAST_REGION = MPU_REGION_4,
    MPU_REGION_5, /**< Boot Reserved */
    MPU_REGION_6, /**< Boot Reserved */
    MPU_REGION_7, /**< Boot Reserved */
    MPU_REGIONS_NB
};

typedef struct __MPU_reg_settings_t
{
    uint32_t rbar;
    uint32_t rasr;
} MPU_reg_settings_t;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*!
 * @brief This function is used to grant access to the pSector region.
 *
 * Note: The pSector region is 'special' because counter intuitively it requires Write
 * access in order to be able to read from it using the flash controller indirect method.
 * The previosuly applied policy.
 * pSector region is protected under rule 7 (highest precedence).
 * The previous rule 7 is saved in RAM before changing it.
 *
 * @param  addr: address of area to grant access to.
 * @param  sz:   size in number of bytes of area.
 * @param  save_rule: save a copy of previous rule
 * @return  -1 if failure, if succesful return the size of the region.
 *
 */
static inline int MPU_pSectorGrantAccessRights(uint32_t addr, size_t sz, MPU_reg_settings_t *save_rule)
{
    int (*p_MPU_pSectorGrantAccessRights)(uint32_t addr, size_t sz, MPU_reg_settings_t * save_rule);
    p_MPU_pSectorGrantAccessRights = (int (*)(uint32_t addr, size_t sz, MPU_reg_settings_t * save_rule))0x030017e5U;

    return p_MPU_pSectorGrantAccessRights(addr, sz, save_rule);
}

/*!
 * @brief This function is used to withdraw access to the pSector region.
 *
 * The pSector region is 'special' because counter intuitively it requires Write
 * access in order to be able to read from it using the flash controller indirect method.
 *
 * @param  save_rule: pointer on RAM MPU_reg_settings_t structure saved by MPU_pSectorGrantAccessRights
 *         used to restore previous settings of region 7 and restrict access to pSector.
 *
 * @return  -1 if failure, if succesful return the size of the region.
 *
 */
static inline int MPU_pSectorWithdrawAccessRights(MPU_reg_settings_t *save_rule)
{
    int (*p_MPU_pSectorWithdrawAccessRights)(MPU_reg_settings_t * save_rule);
    p_MPU_pSectorWithdrawAccessRights = (int (*)(MPU_reg_settings_t * save_rule))0x03001841U;

    return p_MPU_pSectorWithdrawAccessRights(save_rule);
}

/*! @brief MPU_Settings_t structure in RAM to retrieve current MPU configuration
 * see @MPU_GetCurrentSettings
 */
typedef struct
{
    uint32_t ctrl;    /*!< MPU Ctrl register */
    uint32_t rbar[8]; /*!< MPU RBAR array for the 8 rules */
    uint32_t rasr[8]; /*!< MPU RASR array for the 8 rules */
} MPU_Settings_t;

/*!
 * @brief This function is used to read the MPU settings into a RAM structure.
 *
 * @param  settings: pointer of structure to receive the MPU register valkues.

 * @return  none
 *
 */
static inline void MPU_GetCurrentSettings(MPU_Settings_t *settings)
{
    void (*p_MPU_GetCurrentSettings)(MPU_Settings_t * settings);
    p_MPU_GetCurrentSettings = (void (*)(MPU_Settings_t * settings))0x0300178dU;

    p_MPU_GetCurrentSettings(settings);
}

/*!
 * @brief This function is used to set access rights to a region.
 *
 * @param  region_id: 0 .. 7 see @enum MpuRegion_t
 * @param  addr: address of region
 * @param  sz: region size
 * @param  rwx_rights: bit field RD_RIGHT(0) - WR_RIGHT(1) - X_RIGHT(2)
 * @param  save_rule: save a copy of previous rule
 *
 * @return  -1 if failure, if succesful return the size of the region.
 *
 */
static inline int MPU_SetRegionAccessRights(
    enum MpuRegion_t region_id, uint32_t addr, size_t sz, uint8_t rwx_rights, MPU_reg_settings_t *save_rule)
{
    int (*p_MPU_SetRegionAccessRights)(enum MpuRegion_t region_id, uint32_t addr, size_t sz, uint8_t rwx_rights,
                                       MPU_reg_settings_t * save_rule);
    p_MPU_SetRegionAccessRights = (int (*)(enum MpuRegion_t region_id, uint32_t addr, size_t sz, uint8_t rwx_rights,
                                           MPU_reg_settings_t * save_rule))0x03001821U;

    return p_MPU_SetRegionAccessRights(region_id, addr, sz, rwx_rights, save_rule);
}

/*!
 * @brief This function is used to set access rights to a region.
 *
 * Called after previous call to see @MPU_SetRegionAccessRights
 *
 * @param  region_id: 0 .. 7 see @enum MpuRegion_t
 * @param  save_rule: saved copy of previous rule to be restored.
 *         if this parameter is NULL, RBAR and RASR of the given region_id
 *         are cleared.
 *
 * @return  -1 if failure, if succesful return the size of the region.
 *
 */
static inline int MPU_ClearRegionSetting(enum MpuRegion_t region_id, MPU_reg_settings_t *saved_rule)
{
    int (*p_MPU_ClearRegionSetting)(enum MpuRegion_t region_id, MPU_reg_settings_t * saved_rule);
    p_MPU_ClearRegionSetting = (int (*)(enum MpuRegion_t region_id, MPU_reg_settings_t * saved_rule))0x0300184dU;

    return p_MPU_ClearRegionSetting(region_id, saved_rule);
}

/*!
 * @brief This function is used to select the first available rule.
 *
 * Checks if rules have their RASR enable bit set.
 * This for the application to find free riules that were left unused
 * by the ROM code.
 * Implicitly MPU_ClearRegionSetting releases an allocate rule.
 *
 *
 * @param  none
 *
 * @return  -1 if none free, value between 1..4 if succesful.
 *
 */
static inline int MPU_AllocateRegionDesc(void)
{
    int (*p_MPU_AllocateRegionDesc)(void);
    p_MPU_AllocateRegionDesc = (int (*)(void))0x030017c1U;

    return p_MPU_AllocateRegionDesc();
}

#if defined __cplusplus
}
#endif
#endif
