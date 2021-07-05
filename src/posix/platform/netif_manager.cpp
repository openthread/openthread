#include "posix/platform/netif_manager.hpp"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "lib/platform/exit_code.h"
#include "posix/platform/ip6_utils.hpp"
#include "posix/platform/platform-posix.h"

namespace ot {
namespace Posix {

void NetifManager::UpdateLink(const char *aNetifName, bool aUp)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, aNetifName, sizeof(ifr.ifr_name) - 1);

    VerifyOrDie(ioctl(mFd, SIOCGIFFLAGS, &ifr) == 0, OT_EXIT_ERROR_ERRNO);
    bool ifState = ((ifr.ifr_flags & IFF_UP) == IFF_UP);

    otLogNotePlat("changing interface %s state to %s%s.", aNetifName, aUp ? "up" : "down",
                  (aUp == ifState) ? " (already done, ignoring)" : "");
    VerifyOrExit(ifState != aUp);

    ifr.ifr_flags = aUp ? (ifr.ifr_flags | IFF_UP) : (ifr.ifr_flags & ~IFF_UP);
    VerifyOrExit(ioctl(mFd, SIOCSIFFLAGS, &ifr) == 0, otLogWarnPlat("Failed to update link"));

exit:
    return;
}

int NetifManager::GetFlags(const char *aNetifName)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, aNetifName, sizeof(ifr.ifr_name) - 1);
    VerifyOrDie(ioctl(mFd, SIOCGIFFLAGS, &ifr) == 0, OT_EXIT_ERROR_ERRNO);

    return ifr.ifr_flags;
}

bool NetifManager::IsUp(const char *aNetifName)
{
    return ((GetFlags(aNetifName) & IFF_UP) == IFF_UP);
}

bool NetifManager::IsRunning(const char *aNetifName)
{
    return ((GetFlags(aNetifName) & IFF_RUNNING) == IFF_RUNNING);
}

void NetifManager::SetMtu(const char *aNetifName, int aMtu)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, aNetifName, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_mtu = aMtu;
    VerifyOrDie(ioctl(mFd, SIOCSIFMTU, static_cast<void *>(&ifr)) == 0, OT_EXIT_ERROR_ERRNO);
}

void NetifManager::UpdateMulticast(unsigned int aNetifIndex, const otIp6Address &aAddress, bool aIsAdded)
{
    struct ipv6_mreq mreq;

    memcpy(&mreq.ipv6mr_multiaddr, &aAddress, sizeof(mreq.ipv6mr_multiaddr));
    mreq.ipv6mr_interface = aNetifIndex;

    int err = setsockopt(mFd, IPPROTO_IPV6, (aIsAdded ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP), &mreq, sizeof(mreq));
#if defined(__APPLE__) || defined(__FreeBSD__)
    if ((err != 0) && (errno == EINVAL) && (IN6_IS_ADDR_MC_LINKLOCAL(&mreq.ipv6mr_multiaddr)))
    {
        // FIX ME
        // on mac OS (and FreeBSD), the first time we run (but not subsequently), we get a failure on this
        // particular join. do we need to bring up the interface at least once prior to joining? we need to figure
        // out why so we can get rid of this workaround

        otLogWarnPlat("ignoring %s failure (EINVAL) for MC LINKLOCAL address (%s)",
                      aIsAdded ? "IPV6_JOIN_GROUP" : "IPV6_LEAVE_GROUP",
                      Ip6Utils::Ip6AddressString(mreq.ipv6mr_multiaddr.s6_addr).AsCString());
        err = 0;
    }
#endif

    VerifyOrExit(err == 0, otLogWarnPlat("%s failure (%d)", aIsAdded ? "IPV6_JOIN_GROUP" : "IPV6_LEAVE_GROUP", errno));

exit:
    return;
}

bool NetifManager::HasAddress(unsigned int aNetifIndex, const otIp6Address &aAddress)
{
    bool            ret     = false;
    struct ifaddrs *ifAddrs = nullptr;

    VerifyOrDie(getifaddrs(&ifAddrs) != -1, OT_EXIT_ERROR_ERRNO);

    for (struct ifaddrs *addr = ifAddrs; addr != nullptr; addr = addr->ifa_next)
    {
        struct sockaddr_in6 *ip6Addr;

        if (if_nametoindex(addr->ifa_name) != aNetifIndex || addr->ifa_addr->sa_family != AF_INET6)
        {
            continue;
        }

        ip6Addr = reinterpret_cast<sockaddr_in6 *>(addr->ifa_addr);
        if (memcmp(&ip6Addr->sin6_addr, &aAddress, sizeof(aAddress)) == 0)
        {
            ExitNow(ret = true);
        }
    }

exit:
    freeifaddrs(ifAddrs);
    return ret;
}

void NetifManager::Init(void)
{
    VerifyOrExit(mFd == -1);
    mFd = SocketWithCloseExec(AF_INET6, SOCK_DGRAM, IPPROTO_IP, kSocketNonBlock);
    VerifyOrDie(mFd >= 0, OT_EXIT_ERROR_ERRNO);

exit:
    return;
}

void NetifManager::Deinit(void)
{
    if (mFd != -1)
    {
        close(mFd);
        mFd = -1;
    }
}

NetifManager &NetifManager::Get(void)
{
    static NetifManager instance;

    return instance;
}

} // namespace Posix
} // namespace ot
