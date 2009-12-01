/*
 * PROJECT:     ws2_32.dll API tests
 * LICENSE:     GPLv2 or any later version
 * FILE:        apitests/ws2_32/helpers.c
 * PURPOSE:     Helper functions for the socket tests
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "ws2_32.h"

int CreateSocket(PTESTINFO pti, SOCKET* psck)
{
    /* Create the socket */
    *psck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    TEST(*psck != INVALID_SOCKET);

    if(*psck == INVALID_SOCKET)
    {
        printf("Winsock error code is %u\n", WSAGetLastError());
        WSACleanup();
        return APISTATUS_ASSERTION_FAILED;
    }

    return APISTATUS_NORMAL;
}

int ConnectToReactOSWebsite(PTESTINFO pti, SOCKET sck)
{
    int iResult;
    struct hostent* host;
    struct sockaddr_in sa;

    /* Connect to "www.reactos.org" */
    host = gethostbyname("www.reactos.org");

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = *(u_long*)host->h_addr_list[0];
    sa.sin_port = htons(80);

    SCKTEST(connect(sck, (struct sockaddr *)&sa, sizeof(sa)));

    return APISTATUS_NORMAL;
}

int GetRequestAndWait(PTESTINFO pti, SOCKET sck)
{
    const char szGetRequest[] = "GET / HTTP/1.0\r\n\r\n";
    int iResult;
    struct fd_set readable;

    /* Send the GET request */
    SCKTEST(send(sck, szGetRequest, strlen(szGetRequest), 0));
    TEST(iResult == strlen(szGetRequest));

    /* Shutdown the SEND connection */
    SCKTEST(shutdown(sck, SD_SEND));

    /* Wait until we're ready to read */
    FD_ZERO(&readable);
    FD_SET(sck, &readable);

    SCKTEST(select(0, &readable, NULL, NULL, NULL));

    return APISTATUS_NORMAL;
}
