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

#define WPP_NAME "settings.tmh"

#include "settings.hpp"

#include <openthread/platform/settings.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "meshcop/dataset.hpp"
#include "thread/mle.hpp"

namespace ot {

#if (OPENTHREAD_CONFIG_LOG_UTIL != 0)
#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO)

void SettingsBase::LogNetworkInfo(const char *aAction, const NetworkInfo &aNetworkInfo) const
{
    otLogInfoCore("Non-volatile: %s NetworkInfo {rloc:0x%04x, extaddr:%s, role:%s, mode:0x%02x, keyseq:0x%x, ...",
                  aAction, aNetworkInfo.mRloc16, aNetworkInfo.mExtAddress.ToString().AsCString(),
                  Mle::Mle::RoleToString(static_cast<otDeviceRole>(aNetworkInfo.mRole)), aNetworkInfo.mDeviceMode,
                  aNetworkInfo.mKeySequence);

    otLogInfoCore("Non-volatile: ... pid:0x%x, mlecntr:0x%x, maccntr:0x%x, mliid:%02x%02x%02x%02x%02x%02x%02x%02x}",
                  aNetworkInfo.mPreviousPartitionId, aNetworkInfo.mMleFrameCounter, aNetworkInfo.mMacFrameCounter,
                  aNetworkInfo.mMlIid[0], aNetworkInfo.mMlIid[1], aNetworkInfo.mMlIid[2], aNetworkInfo.mMlIid[3],
                  aNetworkInfo.mMlIid[4], aNetworkInfo.mMlIid[5], aNetworkInfo.mMlIid[6], aNetworkInfo.mMlIid[7]);
}

void SettingsBase::LogParentInfo(const char *aAction, const ParentInfo &aParentInfo) const
{
    otLogInfoCore("Non-volatile: %s ParentInfo {extaddr:%s}", aAction, aParentInfo.mExtAddress.ToString().AsCString());
}

void SettingsBase::LogChildInfo(const char *aAction, const ChildInfo &aChildInfo) const
{
    otLogInfoCore("Non-volatile: %s ChildInfo {rloc:0x%04x, extaddr:%s, timeout:%u, mode:0x%02x}", aAction,
                  aChildInfo.mRloc16, aChildInfo.mExtAddress.ToString().AsCString(), aChildInfo.mTimeout,
                  aChildInfo.mMode);
}

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

void Settings::Init(void)
{
    otPlatSettingsInit(&GetInstance());
}

void Settings::Wipe(void)
{
    otPlatSettingsWipe(&GetInstance());
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
    otError  error = OT_ERROR_NONE;
    uint16_t length;

    SuccessOrExit(error = Read(aIsActive ? kKeyActiveDataset : kKeyPendingDataset, aDataset.GetBytes(),
                               MeshCoP::Dataset::kMaxSize, length));
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
    otError error;

    SuccessOrExit(error = ReadFixedSize(kKeyNetworkInfo, &aNetworkInfo, sizeof(NetworkInfo)));
    LogNetworkInfo("Read", aNetworkInfo);

exit:
    return error;
}

otError Settings::SaveNetworkInfo(const NetworkInfo &aNetworkInfo)
{
    otError error;

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
    otError error;

    SuccessOrExit(error = ReadFixedSize(kKeyParentInfo, &aParentInfo, sizeof(ParentInfo)));
    LogParentInfo("Read", aParentInfo);

exit:
    return error;
}

otError Settings::SaveParentInfo(const ParentInfo &aParentInfo)
{
    otError error;

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

otError Settings::DeleteChildInfo(void)
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
    Reset();
}

void Settings::ChildInfoIterator::Reset(void)
{
    mIndex  = 0;
    mIsDone = false;
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
    SuccessOrExit(error = otPlatSettingsDelete(&GetInstance(), kKeyChildInfo, mIndex));
    LogChildInfo("Removed", mChildInfo);

exit:
    LogFailure(error, "removing ChildInfo entry", true);
    return error;
}

void Settings::ChildInfoIterator::Read(void)
{
    uint16_t size = sizeof(ChildInfo);
    otError  error;

    SuccessOrExit(error = otPlatSettingsGet(&GetInstance(), kKeyChildInfo, mIndex,
                                            reinterpret_cast<uint8_t *>(&mChildInfo), &size));
    VerifyOrExit(size >= sizeof(ChildInfo), error = OT_ERROR_NOT_FOUND);
    LogChildInfo("Read", mChildInfo);

exit:
    mIsDone = (error != OT_ERROR_NONE);
}

otError Settings::ReadFixedSize(Key aKey, void *aBuffer, uint16_t aExpectedSize) const
{
    uint16_t size = aExpectedSize;
    otError  error;

    SuccessOrExit(error = otPlatSettingsGet(&GetInstance(), aKey, 0, reinterpret_cast<uint8_t *>(aBuffer), &size));
    VerifyOrExit(size >= aExpectedSize, error = OT_ERROR_NOT_FOUND);

exit:
    return error;
}

otError Settings::Read(Key aKey, void *aBuffer, uint16_t aMaxBufferSize, uint16_t &aReadSize) const
{
    uint16_t size = aMaxBufferSize;
    otError  error;

    SuccessOrExit(error = otPlatSettingsGet(&GetInstance(), aKey, 0, reinterpret_cast<uint8_t *>(aBuffer), &size));
    aReadSize = (size <= aMaxBufferSize) ? size : aMaxBufferSize;

exit:
    return error;
}

otError Settings::Save(Key aKey, const void *aBuffer, uint16_t aSize)
{
    return otPlatSettingsSet(&GetInstance(), aKey, reinterpret_cast<const uint8_t *>(aBuffer), aSize);
}

otError Settings::Add(Key aKey, const void *aBuffer, uint16_t aSize)
{
    return otPlatSettingsAdd(&GetInstance(), aKey, reinterpret_cast<const uint8_t *>(aBuffer), aSize);
}

otError Settings::Delete(Key aKey)
{
    return otPlatSettingsDelete(&GetInstance(), aKey, -1);
}

} // namespace ot
