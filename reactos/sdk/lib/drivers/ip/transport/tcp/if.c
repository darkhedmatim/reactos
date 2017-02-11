
#include "precomp.h"

#include "lwip/pbuf.h"
#include "lwip/netifapi.h"
#include "lwip/ip.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"

err_t
TCPSendDataCallback(struct netif *netif, struct pbuf *p, struct ip_addr *dest)
{
    NDIS_STATUS NdisStatus;
    PNEIGHBOR_CACHE_ENTRY NCE;
    IP_PACKET Packet;
    IP_ADDRESS RemoteAddress, LocalAddress;
    PIPv4_HEADER Header;

    /* The caller frees the pbuf struct */

    if (((*(u8_t*)p->payload) & 0xF0) == 0x40)
    {
        Header = p->payload;
        
        LocalAddress.Type = IP_ADDRESS_V4;
        LocalAddress.Address.IPv4Address = Header->SrcAddr;
        
        RemoteAddress.Type = IP_ADDRESS_V4;
        RemoteAddress.Address.IPv4Address = Header->DstAddr;
    }
    else 
    {
        return ERR_IF;
    }

    IPInitializePacket(&Packet, LocalAddress.Type);

    if (!(NCE = RouteGetRouteToDestination(&RemoteAddress)))
    {
        return ERR_RTE;
    }
    
    NdisStatus = AllocatePacketWithBuffer(&Packet.NdisPacket, NULL, p->tot_len);
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return ERR_MEM;
    }

    GetDataPtr(Packet.NdisPacket, 0, (PCHAR*)&Packet.Header, &Packet.TotalSize);
    Packet.MappedHeader = TRUE;

    if (p->tot_len != p->len ||
        Packet.TotalSize != p->len)
    {
        DbgPrint("TCPSendDataCallback: tot_len = %u, len = %u, TotalSize = %u\n",
                 p->tot_len, p->len, Packet.TotalSize);
        ASSERT(p->tot_len == p->len);
        ASSERT(Packet.TotalSize == p->len);
    }

    RtlCopyMemory(Packet.Header, p->payload, p->len);

    Packet.HeaderSize = sizeof(IPv4_HEADER);
    Packet.TotalSize = p->tot_len;
    Packet.SrcAddr = LocalAddress;
    Packet.DstAddr = RemoteAddress;

    NdisStatus = IPSendDatagram(&Packet, NCE);
    if (!NT_SUCCESS(NdisStatus))
        return ERR_RTE;

    return 0;
}

VOID
TCPUpdateInterfaceLinkStatus(PIP_INTERFACE IF)
{
#if 0
    ULONG OperationalStatus;
    
    GetInterfaceConnectionStatus(IF, &OperationalStatus);
    
    if (OperationalStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
        netif_set_link_up(IF->TCPContext);
    else
        netif_set_link_down(IF->TCPContext);
#endif
}

err_t
TCPInterfaceInit(struct netif *netif)
{
    PIP_INTERFACE IF = netif->state;
    
    netif->hwaddr_len = IF->AddressLength;
    RtlCopyMemory(netif->hwaddr, IF->Address, netif->hwaddr_len);

    netif->output = TCPSendDataCallback;
    netif->mtu = IF->MTU;
    
    netif->name[0] = 'e';
    netif->name[1] = 'n';
    
    netif->flags |= NETIF_FLAG_BROADCAST;
    
    TCPUpdateInterfaceLinkStatus(IF);
    
    TCPUpdateInterfaceIPInformation(IF);

    return 0;
}

VOID
TCPRegisterInterface(PIP_INTERFACE IF)
{
    struct ip_addr ipaddr;
    struct ip_addr netmask;
    struct ip_addr gw;
    
    gw.addr = 0;
    ipaddr.addr = 0;
    netmask.addr = 0;
    
    IF->TCPContext = netif_add(IF->TCPContext, 
                               &ipaddr,
                               &netmask,
                               &gw,
                               IF,
                               TCPInterfaceInit,
                               tcpip_input);
}

VOID
TCPUnregisterInterface(PIP_INTERFACE IF)
{
    netif_remove(IF->TCPContext);
}

VOID
TCPUpdateInterfaceIPInformation(PIP_INTERFACE IF)
{
    struct ip_addr ipaddr;
    struct ip_addr netmask;
    struct ip_addr gw;
    
    gw.addr = 0;
    
    GetInterfaceIPv4Address(IF,
                            ADE_UNICAST,
                            (PULONG)&ipaddr.addr);
    
    GetInterfaceIPv4Address(IF,
                            ADE_ADDRMASK,
                            (PULONG)&netmask.addr);
    
    netif_set_addr(IF->TCPContext, &ipaddr, &netmask, &gw);
    
    if (ipaddr.addr != 0)
    {
        netif_set_up(IF->TCPContext);
        netif_set_default(IF->TCPContext);
    }
    else
    {
        netif_set_down(IF->TCPContext);
    }
}    
