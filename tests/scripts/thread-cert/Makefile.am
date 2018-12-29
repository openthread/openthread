#
#  Copyright (c) 2016-2017, The OpenThread Authors.
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
    Cert_5_2_01_REEDAttach.py                                        \
    Cert_5_2_03_LeaderReject2Hops.py                                 \
    Cert_5_2_04_REEDUpgrade.py                                       \
    Cert_5_2_05_AddressQuery.py                                      \
    Cert_5_2_06_RouterDowngrade.py                                   \
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
    Cert_5_3_09_AddressQuery.py                                      \
    Cert_5_3_10_AddressQuery.py                                      \
    Cert_5_3_11_AddressQueryTimeoutIntervals.py                      \
    Cert_5_5_01_LeaderReboot.py                                      \
    Cert_5_5_02_LeaderReboot.py                                      \
    Cert_5_5_03_SplitMergeChildren.py                                \
    Cert_5_5_04_SplitMergeRouters.py                                 \
    Cert_5_5_05_SplitMergeREED.py                                    \
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
    Cert_6_1_02_REEDAttach_MED.py                                    \
    Cert_6_1_02_REEDAttach_SED.py                                    \
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
    Cert_8_1_01_Commissioning.py                                     \
    Cert_8_1_02_Commissioning.py                                     \
    Cert_8_2_01_JoinerRouter.py                                      \
    Cert_8_2_02_JoinerRouter.py                                      \
    Cert_9_2_04_ActiveDataset.py                                     \
    Cert_9_2_07_DelayTimer.py                                        \
    Cert_9_2_08_PersistentDatasets.py                                \
    Cert_9_2_09_PendingPartition.py                                  \
    Cert_9_2_10_PendingPartition.py                                  \
    Cert_9_2_11_MasterKey.py                                         \
    Cert_9_2_12_Announce.py                                          \
    Cert_9_2_13_EnergyScan.py                                        \
    Cert_9_2_14_PanIdQuery.py                                        \
    Cert_9_2_15_PendingPartition.py                                  \
    Cert_9_2_16_ActivePendingPartition.py                            \
    Cert_9_2_17_Orphan.py                                            \
    Cert_9_2_18_RollBackActiveTimestamp.py                           \
    coap.py                                                          \
    command.py                                                       \
    common.py                                                        \
    config.py                                                        \
    ipv6.py                                                          \
    lowpan.py                                                        \
    mac802154.py                                                     \
    message.py                                                       \
    mle.py                                                           \
    net_crypto.py                                                    \
    network_data.py                                                  \
    network_layer.py                                                 \
    node.py                                                          \
    node_cli.py                                                      \
    pcap.py                                                          \
    simulator.py                                                     \
    sniffer.py                                                       \
    sniffer_transport.py                                             \
    test_coap.py                                                     \
    test_coaps.py                                                    \
    test_common.py                                                   \
    test_crypto.py                                                   \
    test_diag.py                                                     \
    test_ipv6.py                                                     \
    test_lowpan.py                                                   \
    test_mac802154.py                                                \
    test_mle.py                                                      \
    test_service.py                                                  \
    test_network_data.py                                             \
    test_network_layer.py                                            \
    $(NULL)

check_PROGRAMS                                                     = \
    $(NULL)

check_SCRIPTS                                                      = \
    test_coap.py                                                     \
    test_coaps.py                                                    \
    test_common.py                                                   \
    test_crypto.py                                                   \
    test_ipv6.py                                                     \
    test_lowpan.py                                                   \
    test_mac802154.py                                                \
    test_mle.py                                                      \
    test_service.py                                                  \
    test_network_data.py                                             \
    test_network_layer.py                                            \
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
    Cert_5_2_01_REEDAttach.py                                        \
    Cert_5_2_05_AddressQuery.py                                      \
    Cert_5_2_06_RouterDowngrade.py                                   \
    Cert_5_2_07_REEDSynchronization.py                               \
    Cert_5_2_04_REEDUpgrade.py                                       \
    Cert_5_3_01_LinkLocal.py                                         \
    Cert_5_3_02_RealmLocal.py                                        \
    Cert_5_3_03_AddressQuery.py                                      \
    Cert_5_3_04_AddressMapCache.py                                   \
    Cert_5_3_05_RoutingLinkQuality.py                                \
    Cert_5_3_06_RouterIdMask.py                                      \
    Cert_5_3_06b_RouterIdMask.py                                     \
    Cert_5_3_07_DuplicateAddress.py                                  \
    Cert_5_3_08_ChildAddressSet.py                                   \
    Cert_5_3_09_AddressQuery.py                                      \
    Cert_5_3_10_AddressQuery.py                                      \
    Cert_5_3_11_AddressQueryTimeoutIntervals.py                      \
    Cert_5_5_01_LeaderReboot.py                                      \
    Cert_5_5_02_LeaderReboot.py                                      \
    Cert_5_5_03_SplitMergeChildren.py                                \
    Cert_5_5_04_SplitMergeRouters.py                                 \
    Cert_5_5_05_SplitMergeREED.py                                    \
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
    Cert_6_1_02_REEDAttach_MED.py                                    \
    Cert_6_1_02_REEDAttach_SED.py                                    \
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
    Cert_5_2_03_LeaderReject2Hops.py                                 \
    Cert_7_1_01_BorderRouterAsLeader.py                              \
    Cert_7_1_02_BorderRouterAsRouter.py                              \
    Cert_7_1_03_BorderRouterAsLeader.py                              \
    Cert_7_1_04_BorderRouterAsRouter.py                              \
    Cert_7_1_05_BorderRouterAsRouter.py                              \
    Cert_8_1_01_Commissioning.py                                     \
    Cert_8_1_02_Commissioning.py                                     \
    Cert_8_2_01_JoinerRouter.py                                      \
    Cert_8_2_02_JoinerRouter.py                                      \
    Cert_9_2_04_ActiveDataset.py                                     \
    Cert_9_2_07_DelayTimer.py                                        \
    Cert_9_2_08_PersistentDatasets.py                                \
    Cert_9_2_09_PendingPartition.py                                  \
    Cert_9_2_10_PendingPartition.py                                  \
    Cert_9_2_11_MasterKey.py                                         \
    Cert_9_2_12_Announce.py                                          \
    Cert_9_2_13_EnergyScan.py                                        \
    Cert_9_2_14_PanIdQuery.py                                        \
    Cert_9_2_15_PendingPartition.py                                  \
    Cert_9_2_16_ActivePendingPartition.py                            \
    Cert_9_2_17_Orphan.py                                            \
    Cert_9_2_18_RollBackActiveTimestamp.py                           \
    $(NULL)

if OPENTHREAD_ENABLE_DIAG
check_SCRIPTS                                                     += \
    test_diag.py                                                     \
    $(NULL)
endif

TESTS_ENVIRONMENT                                                  = \
    export                                                           \
    top_builddir='$(top_builddir)'                                   \
    VERBOSE=1;                                                       \
    $(NULL)

TESTS                                                              = \
    $(check_PROGRAMS)                                                \
    $(check_SCRIPTS)                                                 \
    $(NULL)

XFAIL_NCP_TESTS                                                    = \
    test_coaps.py                                                    \
    test_diag.py                                                     \
    test_service.py                                                  \
    Cert_5_3_10_AddressQuery.py                                      \
    Cert_8_1_01_Commissioning.py                                     \
    Cert_8_1_02_Commissioning.py                                     \
    Cert_8_2_01_JoinerRouter.py                                      \
    Cert_8_2_02_JoinerRouter.py                                      \
    Cert_9_2_04_ActiveDataset.py                                     \
    Cert_9_2_07_DelayTimer.py                                        \
    Cert_9_2_08_PersistentDatasets.py                                \
    Cert_9_2_09_PendingPartition.py                                  \
    Cert_9_2_10_PendingPartition.py                                  \
    Cert_9_2_11_MasterKey.py                                         \
    Cert_9_2_12_Announce.py                                          \
    Cert_9_2_13_EnergyScan.py                                        \
    Cert_9_2_14_PanIdQuery.py                                        \
    Cert_9_2_15_PendingPartition.py                                  \
    Cert_9_2_16_ActivePendingPartition.py                            \
    Cert_9_2_17_Orphan.py                                            \
    Cert_9_2_18_RollBackActiveTimestamp.py                           \
    $(NULL)

XFAIL_TESTS = $(if $(filter $(NODE_TYPE),ncp-sim),$(XFAIL_NCP_TESTS))

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
