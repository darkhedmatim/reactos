/*
* PROJECT:         ReactOS api tests
* LICENSE:         GPL - See COPYING in the top level directory
* PURPOSE:         Test for WSAAsync
* PROGRAMMERS:     Miroslav Mastny 
*/

#include <apitest.h>

#include <stdio.h>
#include <windows.h>
#include <winsock2.h>

#define SVR_PORT 5000
#define WAIT_TIMEOUT_ 10000
#define EXIT_FLAGS (FD_ACCEPT|FD_CONNECT)

START_TEST(WSAAsync)
{
    WSADATA    WsaData;
    SOCKET     ServerSocket = INVALID_SOCKET,
               ClientSocket = INVALID_SOCKET;
    WSAEVENT   ServerEvent = WSA_INVALID_EVENT,
               ClientEvent = WSA_INVALID_EVENT;
    struct hostent *ent = NULL;
    struct sockaddr_in server_addr_in;
    struct sockaddr_in addr_remote;
    struct sockaddr_in addr_con_loc;
    int nConRes;
    int addrsize, len;
    WSAEVENT fEvents[2];
    SOCKET fSockets[2];
    SOCKET sockaccept;
    WSANETWORKEVENTS WsaNetworkEvents;
    ULONG ulValue = 1;
    DWORD dwWait;
    DWORD dwFlags = 0;

    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
    {
        skip("WSAStartup failed\n");
        return;
    }

    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ServerEvent  = WSACreateEvent();
    ClientEvent  = WSACreateEvent();

    if (ServerSocket == INVALID_SOCKET)
    {
        skip("ERROR: Server socket creation failed\n");
        return;
    }
    if (ClientSocket == INVALID_SOCKET)
    {
        skip("ERROR: Client socket creation failed\n");
        closesocket(ServerSocket);
        return;
    }
    if (ServerEvent == WSA_INVALID_EVENT)
    {
        skip("ERROR: Server WSAEvent creation failed\n");
        closesocket(ClientSocket);
        closesocket(ServerSocket);
        return;
    }
    if (ClientEvent == WSA_INVALID_EVENT)
    {
        skip("ERROR: Client WSAEvent creation failed\n");
        WSACloseEvent(ServerEvent);
        closesocket(ClientSocket);
        closesocket(ServerSocket);
        return;
    }
    ent = gethostbyname("127.0.0.1");
    if (ent == NULL)
    {
        ok(ent != NULL, "ERROR: gethostbyname '127.0.0.1' failed, trying 'localhost'\n");
        ent = gethostbyname("localhost");

        if (ent == NULL)
        {
            skip("ERROR: gethostbyname 'localhost' failed\n");
            goto done;
        }
    }

    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_port   = htons(SVR_PORT);
    memcpy(&server_addr_in.sin_addr.S_un.S_addr, ent->h_addr_list[0], 4);

    // server inialialization
    trace("Initializing server and client connections ...\n");
    ok(bind(ServerSocket, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in)) == 0, "ERROR: server bind failed\n");
    ok(ioctlsocket(ServerSocket, FIONBIO, &ulValue) == 0, "ERROR: server ioctlsocket FIONBIO failed\n");
    ok(WSAEventSelect(ServerSocket, ServerEvent, FD_ACCEPT | FD_CLOSE) == 0, "ERROR: server accept EventSelect failed\n");

    // client inialialization
    ok(WSAEventSelect(ClientSocket, ClientEvent, FD_CONNECT | FD_CLOSE) == 0, "ERROR: client EventSelect failed\n");
    ok(ioctlsocket(ClientSocket, FIONBIO, &ulValue) == 0, "ERROR: client ioctlsocket FIONBIO failed\n");

    // listen
    trace("Starting server listening mode ...\n");
    ok(listen(ServerSocket, SOMAXCONN) == 0, "ERROR: cannot initialize server listen\n");

    trace("Starting client to server connection ...\n");
    // connect
    nConRes = connect(ClientSocket, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in));
    ok(nConRes == SOCKET_ERROR, "ERROR: client connect() result is not SOCKET_ERROR\n");
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "ERROR: client connect() last error is not WSAEWOULDBLOCK\n");

    fSockets[0] = ServerSocket;
    fSockets[1] = ClientSocket;

    fEvents[0] = ServerEvent;
    fEvents[1] = ClientEvent;

    while (dwFlags != EXIT_FLAGS)
    {
        dwWait = WaitForMultipleObjects(2, fEvents, FALSE, WAIT_TIMEOUT_);

        ok(dwWait == WAIT_OBJECT_0 || // server socket event
           dwWait == WAIT_OBJECT_0+1, // client socket event
           "Unknown event received %ld\n", dwWait);
        if (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_OBJECT_0+1)
        {
            skip("ERROR: Connection timeout\n");
            break;
        }

        WSAEnumNetworkEvents(fSockets[dwWait-WAIT_OBJECT_0], fEvents[dwWait-WAIT_OBJECT_0], &WsaNetworkEvents);

        if ((WsaNetworkEvents.lNetworkEvents & FD_ACCEPT) != 0)
        {// connection accepted
            trace("Event FD_ACCEPT...\n");
            ok(WsaNetworkEvents.iErrorCode[FD_ACCEPT_BIT] == 0, "Error on accept %d\n", WsaNetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
            if (WsaNetworkEvents.iErrorCode[FD_ACCEPT_BIT] == 0)
            {
                addrsize = sizeof(addr_remote);
                sockaccept = accept(fSockets[dwWait - WAIT_OBJECT_0], (struct sockaddr*)&addr_remote, &addrsize);
                ok(sockaccept != INVALID_SOCKET, "ERROR: Connection accept function failed, error %d\n", WSAGetLastError());
                dwFlags |= FD_ACCEPT;
            }
        }
 
        if ((WsaNetworkEvents.lNetworkEvents & FD_CONNECT) != 0)
        {// client connected
            trace("Event FD_CONNECT...\n");
            ok(WsaNetworkEvents.iErrorCode[FD_CONNECT_BIT] == 0, "Error on connect %d\n", WsaNetworkEvents.iErrorCode[FD_CONNECT_BIT]);
            if (WsaNetworkEvents.iErrorCode[FD_CONNECT_BIT] == 0)
            {
                len = sizeof(addr_con_loc);
                ok(getsockname(fSockets[dwWait - WAIT_OBJECT_0], (struct sockaddr*)&addr_con_loc, &len) == 0, "\n");
                dwFlags |= FD_CONNECT;
            }
        }
    }

done:
    WSACloseEvent(ServerEvent);
    WSACloseEvent(ClientEvent);
    closesocket(ServerSocket);
    closesocket(ClientSocket);

    WSACleanup();
}
