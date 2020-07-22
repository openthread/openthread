/*
 *  Copyright (c) 2018, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes definitions for non-volatile storage of settings.
 */

#include "settings.hpp"

#include <openthread/platform/settings.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "meshcop/dataset.hpp"
#include "thread/mle.hpp"

namespace ot {

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_UTIL != 0)
#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO)

void SettingsBase::LogNetworkInfo(const char *aAction, const NetworkInfo &aNetworkInfo) const
{
    otLogInfoCore(
        "Non-volatile: %s NetworkInfo {rloc:0x%04x, extaddr:%s, role:%s, mode:0x%02x, version:%hu, keyseq:0x%x, ...",
        aAction, aNetworkInfo.GetRloc16(), aNetworkInfo.GetExtAddress().ToString().AsCString(),
        Mle::Mle::RoleToString(static_cast<Mle::DeviceRole>(aNetworkInfo.GetRole())), aNetworkInfo.GetDeviceMode(),
        aNetworkInfo.GetVersion(), aNetworkInfo.GetKeySequence());

    otLogInfoCore("Non-volatile: ... pid:0x%x, mlecntr:0x%x, maccntr:0x%x, mliid:%s}",
                  aNetworkInfo.GetPreviousPartitionId(), aNetworkInfo.GetMleFrameCounter(),
                  aNetworkInfo.GetMacFrameCounter(), aNetworkInfo.GetMeshLocalIid().ToString().AsCString());
}

void SettingsBase::LogParentInfo(const char *aAction, const ParentInfo &aParentInfo) const
{
    otLogInfoCore("Non-volatile: %s ParentInfo {extaddr:%s, version:%hu}", aAction,
                  aParentInfo.GetExtAddress().ToString().AsCString(), aParentInfo.GetVersion());
}

void SettingsBase::LogChildInfo(const char *aAction, const ChildInfo &aChildInfo) const
{
    otLogInfoCore("Non-volatile: %s ChildInfo {rloc:0x%04x, extaddr:%s, timeout:%u, mode:0x%02x, version:%hu}", aAction,
                  aChildInfo.GetRloc16(), aChildInfo.GetExtAddress().ToString().AsCString(), aChildInfo.GetTimeout(),
                  aChildInfo.GetMode(), aChildInfo.GetVersion());
}

#if OPENTHREAD_CONFIG_DUA_ENABLE
void SettingsBase::LogDadInfo(const char *aAction, const DadInfo &aDadInfo) const
{
    otLogInfoCore("Non-volatile: %s DadInfo {DadCounter:%2d}", aAction, aDadInfo.GetDadCounter());
}
#endif

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO)

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN)

void SettingsBase::LogFailure(otError error, const char *aText, bool aIsDelete) const
{
    if ((error != OT_ERROR_NONE) && (!aIsDelete || (error != OT_ERROR_NOT_FOUND)))
    {
        otLogWarnCore("Non-volatile: Error %s %s", otThreadErrorToString(error), aText);
    }
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN)
#endif // #if (OPENTHREAD_CONFIG_LOG_UTIL != 0)

// LCOV_EXCL_STOP

#if !OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE

SettingsDriver::SettingsDriver(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

void SettingsDriver::Init(void)
{
    otPlatSettingsInit(&GetInstance());
}

void SettingsDriver::Deinit(void)
{
    otPlatSettingsDeinit(&GetInstance());
}

otError SettingsDriver::Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return otPlatSettingsAdd(&GetInstance(), aKey, aValue, aValueLength);
}

otError SettingsDriver::Delete(uint16_t aKey, int aIndex)
{
    return otPlatSettingsDelete(&GetInstance(), aKey, aIndex);
}

otError SettingsDriver::Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength) const
{
    return otPlatSettingsGet(&GetInstance(), aKey, aIndex, aValue, aValueLength);
}

otError SettingsDriver::Set(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return otPlatSettingsSet(&GetInstance(), aKey, aValue, aValueLength);
}

void SettingsDriver::Wipe(void)
{
    otPlatSettingsWipe(&GetInstance());
}

#else // !OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE

SettingsDriver::SettingsDriver(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mFlash(aInstance)
{
}

void SettingsDriver::Init(void)
{
    mFlash.Init();
}

void SettingsDriver::Deinit(void)
{
}

otError SettingsDriver::Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return mFlash.Add(aKey, aValue, aValueLength);
}

otError SettingsDriver::Delete(uint16_t aKey, int aIndex)
{
    return mFlash.Delete(aKey, aIndex);
}

otError SettingsDriver::Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength) const
{
    return mFlash.Get(aKey, aIndex, aValue, aValueLength);
}

otError SettingsDriver::Set(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return mFlash.Set(aKey, aValue, aValueLength);
}

void SettingsDriver::Wipe(void)
{
    mFlash.Wipe();
}

#endif // !OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE

void Settings::Init(void)
{
    Get<SettingsDriver>().Init();
}

void Settings::Deinit(void)
{
    Get<SettingsDriver>().Deinit();
}

void Settings::Wipe(void)
{
    Get<SettingsDriver>().Wipe();
    otLogInfoCore("Non-volatile: Wiped all info");
}

otError Settings::SaveOperationalDataset(bool aIsActive, const MeshCoP::Dataset &aDataset)
{
    otError error = Save(aIsActive ? kKeyActiveDataset : kKeyPendingDataset, aDataset.GetBytes(), aDataset.GetSize());

    LogFailure(error, "saving OperationalDataset", false);
    return error;
}

otError Settings::ReadOperationalDataset(bool aIsActive, MeshCoP::Dataset &aDataset) const
{
    otError  error  = OT_ERROR_NONE;
    uint16_t length = MeshCoP::Dataset::kMaxSize;

    SuccessOrExit(error = Read(aIsActive ? kKeyActiveDataset : kKeyPendingDataset, aDataset.GetBytes(), length));
    VerifyOrExit(length <= MeshCoP::Dataset::kMaxSize, error = OT_ERROR_NOT_FOUND);

    aDataset.SetSize(length);

exit:
    return error;
}

otError Settings::DeleteOperationalDataset(bool aIsActive)
{
    otError error = Delete(aIsActive ? kKeyActiveDataset : kKeyPendingDataset);

    LogFailure(error, "deleting OperationalDataset", true);

    return error;
}

otError Settings::ReadNetworkInfo(NetworkInfo &aNetworkInfo) const
{
    otError  error;
    uint16_t length = sizeof(NetworkInfo);

    aNetworkInfo.Init();
    SuccessOrExit(error = Read(kKeyNetworkInfo, &aNetworkInfo, length));
    LogNetworkInfo("Read", aNetworkInfo);

exit:
    return error;
}

otError Settings::SaveNetworkInfo(const NetworkInfo &aNetworkInfo)
{
    otError     error = OT_ERROR_NONE;
    NetworkInfo prevNetworkInfo;
    uint16_t    length = sizeof(prevNetworkInfo);

    if ((Read(kKeyNetworkInfo, &prevNetworkInfo, length) == OT_ERROR_NONE) && (length == sizeof(NetworkInfo)) &&
        (prevNetworkInfo == aNetworkInfo))
    {
        LogNetworkInfo("Re-saved", aNetworkInfo);
        ExitNow();
    }

    SuccessOrExit(error = Save(kKeyNetworkInfo, &aNetworkInfo, sizeof(NetworkInfo)));
    LogNetworkInfo("Saved", aNetworkInfo);

exit:
    LogFailure(error, "saving NetworkInfo", false);
    return error;
}

otError Settings::DeleteNetworkInfo(void)
{
    otError error;

    SuccessOrExit(error = Delete(kKeyNetworkInfo));
    otLogInfoCore("Non-volatile: Deleted NetworkInfo");

exit:
    LogFailure(error, "deleting NetworkInfo", true);
    return error;
}

otError Settings::ReadParentInfo(ParentInfo &aParentInfo) const
{
    otError  error;
    uint16_t length = sizeof(ParentInfo);

    aParentInfo.Init();
    SuccessOrExit(error = Read(kKeyParentInfo, &aParentInfo, length));
    LogParentInfo("Read", aParentInfo);

exit:
    return error;
}

otError Settings::SaveParentInfo(const ParentInfo &aParentInfo)
{
    otError    error = OT_ERROR_NONE;
    ParentInfo prevParentInfo;
    uint16_t   length = sizeof(ParentInfo);

    if ((Read(kKeyParentInfo, &prevParentInfo, length) == OT_ERROR_NONE) && (length == sizeof(ParentInfo)) &&
        (prevParentInfo == aParentInfo))
    {
        LogParentInfo("Re-saved", aParentInfo);
        ExitNow();
    }

    SuccessOrExit(error = Save(kKeyParentInfo, &aParentInfo, sizeof(ParentInfo)));
    LogParentInfo("Saved", aParentInfo);

exit:
    LogFailure(error, "saving ParentInfo", false);
    return error;
}

otError Settings::DeleteParentInfo(void)
{
    otError error;

    SuccessOrExit(error = Delete(kKeyParentInfo));
    otLogInfoCore("Non-volatile: Deleted ParentInfo");

exit:
    LogFailure(error, "deleting ParentInfo", true);
    return error;
}

otError Settings::AddChildInfo(const ChildInfo &aChildInfo)
{
    otError error;

    SuccessOrExit(error = Add(kKeyChildInfo, &aChildInfo, sizeof(aChildInfo)));
    LogChildInfo("Added", aChildInfo);

exit:
    LogFailure(error, "adding ChildInfo", false);
    return error;
}

otError Settings::DeleteAllChildInfo(void)
{
    otError error;

    SuccessOrExit(error = Delete(kKeyChildInfo));
    otLogInfoCore("Non-volatile: Deleted all ChildInfo");

exit:
    LogFailure(error, "deleting all ChildInfo", true);
    return error;
}

Settings::ChildInfoIterator::ChildInfoIterator(Instance &aInstance)
    : SettingsBase(aInstance)
    , mIndex(0)
    , mIsDone(false)
{
    Read();
}

void Settings::ChildInfoIterator::Advance(void)
{
    if (!mIsDone)
    {
        mIndex++;
        Read();
    }
}

otError Settings::ChildInfoIterator::Delete(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mIsDone, error = OT_ERROR_INVALID_STATE);
    SuccessOrExit(error = Get<SettingsDriver>().Delete(kKeyChildInfo, mIndex));
    LogChildInfo("Removed", mChildInfo);

exit:
    LogFailure(error, "removing ChildInfo entry", true);
    return error;
}

void Settings::ChildInfoIterator::Read(void)
{
    uint16_t length = sizeof(ChildInfo);
    otError  error;

    mChildInfo.Init();
    SuccessOrExit(
        error = Get<SettingsDriver>().Get(kKeyChildInfo, mIndex, reinterpret_cast<uint8_t *>(&mChildInfo), &length));
    LogChildInfo("Read", mChildInfo);

exit:
    mIsDone = (error != OT_ERROR_NONE);
}

#if OPENTHREAD_CONFIG_DUA_ENABLE
otError Settings::ReadDadInfo(DadInfo &aDadInfo) const
{
    otError  error;
    uint16_t length = sizeof(DadInfo);

    aDadInfo.Init();
    SuccessOrExit(error = Read(kKeyDadInfo, &aDadInfo, length));
    LogDadInfo("Read", aDadInfo);

exit:
    return error;
}

otError Settings::SaveDadInfo(const DadInfo &aDadInfo)
{
    otError  error = OT_ERROR_NONE;
    DadInfo  prevDadInfo;
    uint16_t length = sizeof(DadInfo);

    if ((Read(kKeyDadInfo, &prevDadInfo, length) == OT_ERROR_NONE) && (length == sizeof(DadInfo)) &&
        (prevDadInfo == aDadInfo))
    {
        LogDadInfo("Re-saved", aDadInfo);
        ExitNow();
    }

    SuccessOrExit(error = Save(kKeyDadInfo, &aDadInfo, sizeof(DadInfo)));
    LogDadInfo("Saved", aDadInfo);

exit:
    LogFailure(error, "saving DadInfo", false);
    return error;
}

otError Settings::DeleteDadInfo(void)
{
    otError error;

    SuccessOrExit(error = Delete(kKeyDadInfo));
    otLogInfoCore("Non-volatile: Deleted DadInfo");

exit:
    LogFailure(error, "deleting DadInfo", true);
    return error;
}
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

otError Settings::Read(Key aKey, void *aBuffer, uint16_t &aSize) const
{
    return Get<SettingsDriver>().Get(aKey, 0, reinterpret_cast<uint8_t *>(aBuffer), &aSize);
}

otError Settings::Save(Key aKey, const void *aValue, uint16_t aSize)
{
    return Get<SettingsDriver>().Set(aKey, reinterpret_cast<const uint8_t *>(aValue), aSize);
}

otError Settings::Add(Key aKey, const void *aValue, uint16_t aSize)
{
    return Get<SettingsDriver>().Add(aKey, reinterpret_cast<const uint8_t *>(aValue), aSize);
}

otError Settings::Delete(Key aKey)
{
    return Get<SettingsDriver>().Delete(aKey, -1);
}

} // namespace ot
