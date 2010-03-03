#ifndef _IPHLPAPI_H
#define _IPHLPAPI_H

#include <iprtrmib.h>
#include <ipexport.h>
#include <iptypes.h>
#include <netioapi.h>
#ifdef __cplusplus
extern "C" {
#endif
DWORD WINAPI AddIPAddress(IPAddr,IPMask,DWORD,PULONG,PULONG);
DWORD WINAPI CreateIpForwardEntry(PMIB_IPFORWARDROW);
DWORD WINAPI CreateIpNetEntry(PMIB_IPNETROW);
DWORD WINAPI CreateProxyArpEntry(DWORD,DWORD,DWORD);
DWORD WINAPI DeleteIPAddress(ULONG);
DWORD WINAPI DeleteIpForwardEntry(PMIB_IPFORWARDROW);
DWORD WINAPI DeleteIpNetEntry(PMIB_IPNETROW);
DWORD WINAPI DeleteProxyArpEntry(DWORD,DWORD,DWORD);
DWORD WINAPI EnableRouter(HANDLE*,OVERLAPPED*);
DWORD WINAPI FlushIpNetTable(DWORD);
DWORD WINAPI GetAdapterIndex(LPWSTR,PULONG);
DWORD WINAPI GetAdaptersInfo(PIP_ADAPTER_INFO,PULONG);
DWORD WINAPI GetBestInterface(IPAddr,PDWORD);
DWORD WINAPI GetBestRoute(DWORD,DWORD,PMIB_IPFORWARDROW);
DWORD WINAPI GetExtendedTcpTable(PVOID,PDWORD,BOOL,ULONG,TCP_TABLE_CLASS,ULONG);
DWORD WINAPI GetFriendlyIfIndex(DWORD);
DWORD WINAPI GetIcmpStatistics(PMIB_ICMP);
DWORD WINAPI GetIfEntry(PMIB_IFROW);
DWORD WINAPI GetIfTable(PMIB_IFTABLE,PULONG,BOOL);
DWORD WINAPI GetInterfaceInfo(PIP_INTERFACE_INFO,PULONG);
DWORD WINAPI GetIpAddrTable(PMIB_IPADDRTABLE,PULONG,BOOL);
DWORD WINAPI GetIpForwardTable(PMIB_IPFORWARDTABLE,PULONG,BOOL);
DWORD WINAPI GetIpNetTable(PMIB_IPNETTABLE,PULONG,BOOL);
DWORD WINAPI GetIpStatistics(PMIB_IPSTATS);
DWORD WINAPI GetIpStatisticsEx(PMIB_IPSTATS,DWORD);
DWORD WINAPI GetNetworkParams(PFIXED_INFO,PULONG);
DWORD WINAPI GetNumberOfInterfaces(PDWORD);
DWORD WINAPI GetPerAdapterInfo(ULONG,PIP_PER_ADAPTER_INFO, PULONG);
BOOL WINAPI GetRTTAndHopCount(IPAddr,PULONG,ULONG,PULONG);
DWORD WINAPI GetTcpStatistics(PMIB_TCPSTATS);
DWORD WINAPI GetTcpTable(PMIB_TCPTABLE,PDWORD,BOOL);
DWORD WINAPI GetUniDirectionalAdapterInfo(PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS,PULONG);
DWORD WINAPI GetUdpStatistics(PMIB_UDPSTATS);
DWORD WINAPI GetUdpTable(PMIB_UDPTABLE,PDWORD,BOOL);
DWORD WINAPI IpReleaseAddress(PIP_ADAPTER_INDEX_MAP);
DWORD WINAPI IpRenewAddress(PIP_ADAPTER_INDEX_MAP);
DWORD WINAPI NotifyAddrChange(PHANDLE,LPOVERLAPPED);
DWORD WINAPI NotifyRouteChange(PHANDLE,LPOVERLAPPED);
DWORD WINAPI SendARP(IPAddr,IPAddr,PULONG,PULONG);
DWORD WINAPI SetIfEntry(PMIB_IFROW);
DWORD WINAPI SetIpForwardEntry(PMIB_IPFORWARDROW);
DWORD WINAPI SetIpNetEntry(PMIB_IPNETROW);
DWORD WINAPI SetIpStatistics(PMIB_IPSTATS);
DWORD WINAPI SetIpTTL(UINT);
DWORD WINAPI SetTcpEntry(PMIB_TCPROW);
DWORD WINAPI UnenableRouter(OVERLAPPED*, LPDWORD);
#ifdef __cplusplus
}
#endif
#endif /* _IPHLPAPI_H */
