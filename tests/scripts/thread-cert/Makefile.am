#
#  Copyright (c) 2016, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

LOG_DRIVER=$(abs_top_srcdir)/third_party/openthread-test-driver/test-driver

EXTRA_DIST                                                         = \
    Cert_5_1_01_RouterAttach.py                                      \
    Cert_5_1_02_ChildAddressTimeout.py                               \
    Cert_5_1_03_RouterAddressReallocation.py                         \
    Cert_5_1_04_RouterAddressReallocation.py                         \
    Cert_5_1_05_RouterAddressTimeout.py                              \
    Cert_5_1_06_RemoveRouterId.py                                    \
    Cert_5_1_07_MaxChildCount.py                                     \
    Cert_5_1_08_RouterAttachConnectivity.py                          \
    Cert_5_1_09_REEDAttachConnectivity.py                            \
    Cert_5_1_10_RouterAttachLinkQuality.py                           \
    Cert_5_1_11_REEDAttachLinkQuality.py                             \
    Cert_5_1_12_NewRouterNeighborSync.py                             \
    Cert_5_1_13_RouterReset.py                                       \
    Cert_5_2_01_BecomeActiveRouter.py                                \
    Cert_5_2_02_LeaderReject1Hop.py                                  \
    Cert_5_2_03_LeaderReject2Hops.py                                 \
    Cert_5_2_04_REEDUpgrade.py                                       \
    Cert_5_2_05_AddressQuery.py                                      \
    Cert_5_2_07_REEDSynchronization.py                               \
    Cert_5_3_01_LinkLocal.py                                         \
    Cert_5_3_02_RealmLocal.py                                        \
    Cert_5_3_03_AddressQuery.py                                      \
    Cert_5_3_04_AddressMapCache.py                                   \
    Cert_5_3_05_RoutingLinkQuality.py                                \
    Cert_5_3_06_RouterIdMask.py                                      \
    Cert_5_3_06b_RouterIdMask.py                                     \
    Cert_5_3_07_DuplicateAddress.py                                  \
    Cert_5_3_08_ChildAddressSet.py                                   \
    Cert_5_3_10_AddressQuery.py                                      \
    Cert_5_5_01_LeaderReset.py                                       \
    Cert_5_5_02_LeaderReboot.py                                      \
    Cert_5_5_03_SplitMergeChildren.py                                \
    Cert_5_5_04_SplitMergeRouters.py                                 \
    Cert_5_5_05_SplitMergeREED.py                                    \
    Cert_5_5_06_SplitWeight.py                                       \
    Cert_5_5_07_SplitMergeThreeWay.py                                \
    Cert_5_5_08_SplitRoutersLostLeader.py                            \
    Cert_5_6_01_NetworkDataRegisterBeforeAttachLeader.py             \
    Cert_5_6_02_NetworkDataRegisterBeforeAttachRouter.py             \
    Cert_5_6_03_NetworkDataRegisterAfterAttachLeader.py              \
    Cert_5_6_04_NetworkDataRegisterAfterAttachRouter.py              \
    Cert_5_6_05_NetworkDataRegisterAfterAttachRouter.py              \
    Cert_5_6_06_NetworkDataExpiration.py                             \
    Cert_5_6_07_NetworkDataRequestREED.py                            \
    Cert_5_6_08_ContextManagement.py                                 \
    Cert_5_6_09_NetworkDataForwarding.py                             \
    Cert_5_8_01_KeySynchronization.py                                \
    Cert_5_8_02_KeyIncrement.py                                      \
    Cert_5_8_03_KeyIncrementRollOver.py                              \
    Cert_6_1_01_RouterAttach.py                                      \
    Cert_6_1_02_REEDAttach.py                                        \
    Cert_6_1_03_RouterAttachConnectivity.py                          \
    Cert_6_1_04_REEDAttachConnectivity.py                            \
    Cert_6_1_05_RouterAttachLinkQuality.py                           \
    Cert_6_1_06_REEDAttachLinkQuality.py                             \
    Cert_6_1_07_EDSynchronization.py                                 \
    Cert_6_2_01_NewPartition.py                                      \
    Cert_6_2_02_NewPartition.py                                      \
    Cert_6_3_01_OrphanReattach.py                                    \
    Cert_6_3_02_NetworkDataUpdate.py                                 \
    Cert_6_4_01_LinkLocal.py                                         \
    Cert_6_4_02_RealmLocal.py                                        \
    Cert_6_5_01_ChildResetSynchronize.py                             \
    Cert_6_5_02_ChildResetReattach.py                                \
    Cert_6_6_01_KeyIncrement.py                                      \
    Cert_6_6_02_KeyIncrementRollOver.py                              \
    Cert_7_1_01_BorderRouterAsLeader.py                              \
    Cert_7_1_02_BorderRouterAsRouter.py                              \
    Cert_7_1_03_BorderRouterAsLeader.py                              \
    Cert_7_1_04_BorderRouterAsRouter.py                              \
    Cert_7_1_05_BorderRouterAsRouter.py                              \
    Cert_9_2_14_PanIdQuery.py                                        \
    node.py                                                          \
    $(NULL)

if OPENTHREAD_BUILD_TESTS

# First, list all essential program and script tests that MUST be run.

check_PROGRAMS =

check_SCRIPTS                                                      = \
    $(NULL)

if OPENTHREAD_TESTS_SUBSET1

check_SCRIPTS                                                     += \
    Cert_5_1_01_RouterAttach.py                                      \
    Cert_5_1_02_ChildAddressTimeout.py                               \
    Cert_5_1_03_RouterAddressReallocation.py                         \
    Cert_5_1_04_RouterAddressReallocation.py                         \
    Cert_5_1_05_RouterAddressTimeout.py                              \
    Cert_5_1_06_RemoveRouterId.py                                    \
    Cert_5_1_07_MaxChildCount.py                                     \
    Cert_5_1_08_RouterAttachConnectivity.py                          \
    Cert_5_1_09_REEDAttachConnectivity.py                            \
    $(NULL)

endif # OPENTHREAD_TESTS_SUBSET1

if OPENTHREAD_TESTS_SUBSET2

check_SCRIPTS                                                     += \
    Cert_5_1_10_RouterAttachLinkQuality.py                           \
    Cert_5_1_11_REEDAttachLinkQuality.py                             \
    Cert_5_1_12_NewRouterNeighborSync.py                             \
    Cert_5_1_13_RouterReset.py                                       \
    Cert_5_2_01_BecomeActiveRouter.py                                \
    Cert_5_2_02_LeaderReject1Hop.py                                  \
    Cert_5_2_03_LeaderReject2Hops.py                                 \
    Cert_5_2_04_REEDUpgrade.py                                       \
    Cert_5_2_05_AddressQuery.py                                      \
    Cert_5_2_07_REEDSynchronization.py                               \
    Cert_5_3_01_LinkLocal.py                                         \
    Cert_5_3_02_RealmLocal.py                                        \
    Cert_5_3_03_AddressQuery.py                                      \
    Cert_5_3_04_AddressMapCache.py                                   \
    Cert_5_3_05_RoutingLinkQuality.py                                \
    $(NULL)

endif # OPENTHREAD_TESTS_SUBSET2

if OPENTHREAD_TESTS_SUBSET3

check_SCRIPTS                                                     += \
    Cert_5_3_06_RouterIdMask.py                                      \
    Cert_5_3_06b_RouterIdMask.py                                     \
    Cert_5_3_07_DuplicateAddress.py                                  \
    Cert_5_3_08_ChildAddressSet.py                                   \
    Cert_5_3_10_AddressQuery.py                                      \
    Cert_5_5_01_LeaderReset.py                                       \
    Cert_5_5_02_LeaderReboot.py                                      \
    $(NULL)

endif # OPENTHREAD_TESTS_SUBSET3

if OPENTHREAD_TESTS_SUBSET4

check_SCRIPTS                                                     += \
    Cert_5_5_03_SplitMergeChildren.py                                \
    Cert_5_5_04_SplitMergeRouters.py                                 \
    Cert_5_5_05_SplitMergeREED.py                                    \
    Cert_5_5_06_SplitWeight.py                                       \
    Cert_5_5_07_SplitMergeThreeWay.py                                \
    Cert_5_5_08_SplitRoutersLostLeader.py                            \
    Cert_5_6_01_NetworkDataRegisterBeforeAttachLeader.py             \
    Cert_5_6_02_NetworkDataRegisterBeforeAttachRouter.py             \
    $(NULL)

endif # OPENTHREAD_TESTS_SUBSET4

if OPENTHREAD_TESTS_SUBSET5

check_SCRIPTS                                                     += \
    Cert_5_6_03_NetworkDataRegisterAfterAttachLeader.py              \
    Cert_5_6_04_NetworkDataRegisterAfterAttachRouter.py              \
    Cert_5_6_05_NetworkDataRegisterAfterAttachRouter.py              \
    Cert_5_6_06_NetworkDataExpiration.py                             \
    Cert_5_6_07_NetworkDataRequestREED.py                            \
    Cert_5_6_08_ContextManagement.py                                 \
    Cert_5_6_09_NetworkDataForwarding.py                             \
    Cert_5_8_01_KeySynchronization.py                                \
    Cert_5_8_02_KeyIncrement.py                                      \
    Cert_5_8_03_KeyIncrementRollOver.py                              \
    Cert_6_1_01_RouterAttach.py                                      \
    Cert_6_1_02_REEDAttach.py                                        \
    Cert_6_1_03_RouterAttachConnectivity.py                          \
    Cert_6_1_04_REEDAttachConnectivity.py                            \
    Cert_6_1_05_RouterAttachLinkQuality.py                           \
    Cert_6_1_06_REEDAttachLinkQuality.py                             \
    Cert_6_1_07_EDSynchronization.py                                 \
    Cert_6_2_01_NewPartition.py                                      \
    Cert_6_2_02_NewPartition.py                                      \
    Cert_6_3_01_OrphanReattach.py                                    \
    Cert_6_3_02_NetworkDataUpdate.py                                 \
    Cert_6_4_01_LinkLocal.py                                         \
    Cert_6_4_02_RealmLocal.py                                        \
    Cert_6_5_01_ChildResetSynchronize.py                             \
    Cert_6_5_02_ChildResetReattach.py                                \
    Cert_6_6_01_KeyIncrement.py                                      \
    Cert_6_6_02_KeyIncrementRollOver.py                              \
    Cert_7_1_01_BorderRouterAsLeader.py                              \
    Cert_7_1_02_BorderRouterAsRouter.py                              \
    Cert_7_1_03_BorderRouterAsLeader.py                              \
    Cert_7_1_04_BorderRouterAsRouter.py                              \
    Cert_7_1_05_BorderRouterAsRouter.py                              \
    Cert_9_2_14_PanIdQuery.py                                        \
    $(NULL)

endif # OPENTHREAD_TESTS_SUBSET5

TESTS_ENVIRONMENT                                                  = \
    export                                                           \
    top_builddir='$(top_builddir)'                                   \
    VERBOSE=1;                                                       \
    $(NULL)

TESTS                                                              = \
    $(check_PROGRAMS)                                                \
    $(check_SCRIPTS)                                                 \
    $(NULL)

XFAIL_NCP_TESTS                                                        = \
        Cert_5_3_07_DuplicateAddress.py                                  \
        Cert_5_6_01_NetworkDataRegisterBeforeAttachLeader.py             \
        Cert_5_6_02_NetworkDataRegisterBeforeAttachRouter.py             \
        Cert_5_6_03_NetworkDataRegisterAfterAttachLeader.py              \
        Cert_5_6_04_NetworkDataRegisterAfterAttachRouter.py              \
        Cert_5_6_05_NetworkDataRegisterAfterAttachRouter.py              \
        Cert_5_6_06_NetworkDataExpiration.py                             \
        Cert_5_6_07_NetworkDataRequestREED.py                            \
        Cert_5_6_08_ContextManagement.py                                 \
        Cert_6_3_02_NetworkDataUpdate.py                                 \
        Cert_7_1_01_BorderRouterAsLeader.py                              \
        Cert_7_1_02_BorderRouterAsRouter.py                              \
        Cert_7_1_03_BorderRouterAsLeader.py                              \
        Cert_7_1_04_BorderRouterAsRouter.py                              \
        Cert_7_1_05_BorderRouterAsRouter.py                              \
        Cert_9_2_14_PanIdQuery.py                                        \
        $(NULL)

XFAIL_TESTS = $(if $(filter $(NODE_TYPE),ncp-sim),$(XFAIL_NCP_TESTS))

endif # OPENTHREAD_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
