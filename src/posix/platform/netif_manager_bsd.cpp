#include "posix/platform/netif_manager.hpp"

#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
#include <netinet/in.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <net/if_var.h>
#endif // defined(__APPLE__) || defined(__FreeBSD__)
#include <net/route.h>
#include <netinet6/in6_var.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
// the prf_ra structure is defined inside another structure (in6_prflags), and C++
//   treats that as out of scope if another structure tries to use it -- this (slightly gross)
//   workaround makes us dependent on our definition remaining in sync (at least the size of it),
//   so we add a compile-time check that will fail if the SDK ever changes
//
// our definition of the struct:
struct prf_ra
{
    u_char onlink : 1;
    u_char autonomous : 1;
    u_char reserved : 6;
} prf_ra;
// compile time check to make sure they're the same size:
static_assert(sizeof(struct prf_ra) == sizeof(in6_prflags::prf_ra),
              "prf_ra and in6_prflags.prf_ra are not the same size!");
#endif
#include <net/if_dl.h>    // struct sockaddr_dl
#include <netinet6/nd6.h> // ND6_INFINITE_LIFETIME

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <net/if_tun.h>
#endif // defined(__NetBSD__) || defined(__FreeBSD__)

#include "posix/platform/ip6_utils.hpp"

namespace ot {
namespace Posix {

void NetifManager::Destroy(const char *aNetifName)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, aNetifName, sizeof(ifr.ifr_name));
    VerifyOrDie(ioctl(mFd, SIOCIFDESTROY, &ifr) == 0, OT_EXIT_ERROR_ERRNO);
}

void NetifManager::UpdateUnicast(unsigned int aNetifIndex, const otIp6AddressInfo &aAddressInfo, bool aIsAdded)
{
    int                 rval;
    struct in6_aliasreq ifr6;

    memset(&ifr6, 0, sizeof(ifr6));
    VerifyOrDie(if_indextoname(aNetifIndex, ifr6.ifra_name) != nullptr, OT_EXIT_ERROR_ERRNO);
    ifr6.ifra_addr.sin6_family = AF_INET6;
    ifr6.ifra_addr.sin6_len    = sizeof(ifr6.ifra_addr);
    memcpy(&ifr6.ifra_addr.sin6_addr, aAddressInfo.mAddress, sizeof(struct in6_addr));
    ifr6.ifra_prefixmask.sin6_family = AF_INET6;
    ifr6.ifra_prefixmask.sin6_len    = sizeof(ifr6.ifra_prefixmask);
    Ip6Utils::InitNetmaskWithPrefixLength(ifr6.ifra_prefixmask.sin6_addr, aAddressInfo.mPrefixLength);
    ifr6.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
    ifr6.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;

#if defined(__APPLE__)
    ifr6.ifra_lifetime.ia6t_expire    = ND6_INFINITE_LIFETIME;
    ifr6.ifra_lifetime.ia6t_preferred = ND6_INFINITE_LIFETIME;
#endif

    rval = ioctl(mFd, aIsAdded ? SIOCAIFADDR_IN6 : SIOCDIFADDR_IN6, &ifr6);
    if (rval == 0)
    {
        otLogInfoPlat("%s %s/%u", (aIsAdded ? "Added" : "Removed"),
                      Ip6Utils::Ip6AddressString(aAddressInfo.mAddress).AsCString(), aAddressInfo.mPrefixLength);
    }
    else if (errno != EALREADY)
    {
        otLogWarnPlat("Failed to %s %s/%u: %s", (aIsAdded ? "add" : "remove"),
                      Ip6Utils::Ip6AddressString(aAddressInfo.mAddress).AsCString(), aAddressInfo.mPrefixLength,
                      strerror(errno));
    }
}

} // namespace Posix
} // namespace ot

#endif // defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
