/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winhttp.h"
#include "wincrypt.h"
#include "winreg.h"
#define COBJMACROS
#include "ole2.h"
#include "dispex.h"
#include "activscp.h"

#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

#define DEFAULT_RESOLVE_TIMEOUT     0
#define DEFAULT_CONNECT_TIMEOUT     20000
#define DEFAULT_SEND_TIMEOUT        30000
#define DEFAULT_RECEIVE_TIMEOUT     30000

static const WCHAR global_funcsW[] = {'g','l','o','b','a','l','_','f','u','n','c','s',0};
static const WCHAR dns_resolveW[] = {'d','n','s','_','r','e','s','o','l','v','e',0};

void set_last_error( DWORD error )
{
    /* FIXME */
    SetLastError( error );
}

DWORD get_last_error( void )
{
    /* FIXME */
    return GetLastError();
}

void send_callback( object_header_t *hdr, DWORD status, LPVOID info, DWORD buflen )
{
    TRACE("%p, 0x%08x, %p, %u\n", hdr, status, info, buflen);

    if (hdr->callback && (hdr->notify_mask & status)) hdr->callback( hdr->handle, hdr->context, status, info, buflen );
}

/***********************************************************************
 *          WinHttpCheckPlatform (winhttp.@)
 */
BOOL WINAPI WinHttpCheckPlatform( void )
{
    TRACE("\n");
    return TRUE;
}

/***********************************************************************
 *          session_destroy (internal)
 */
static void session_destroy( object_header_t *hdr )
{
    session_t *session = (session_t *)hdr;
    struct list *item, *next;
    domain_t *domain;

    TRACE("%p\n", session);

    LIST_FOR_EACH_SAFE( item, next, &session->cookie_cache )
    {
        domain = LIST_ENTRY( item, domain_t, entry );
        delete_domain( domain );
    }
    heap_free( session->agent );
    heap_free( session->proxy_server );
    heap_free( session->proxy_bypass );
    heap_free( session->proxy_username );
    heap_free( session->proxy_password );
    heap_free( session );
#ifdef __REACTOS__
    WSACleanup();
#endif
}

static BOOL session_query_option( object_header_t *hdr, DWORD option, LPVOID buffer, LPDWORD buflen )
{
    session_t *session = (session_t *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_REDIRECT_POLICY:
    {
        if (!buffer || *buflen < sizeof(DWORD))
        {
            *buflen = sizeof(DWORD);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        *(DWORD *)buffer = hdr->redirect_policy;
        *buflen = sizeof(DWORD);
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        *(DWORD *)buffer = session->resolve_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        *(DWORD *)buffer = session->connect_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    case WINHTTP_OPTION_SEND_TIMEOUT:
        *(DWORD *)buffer = session->send_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        *(DWORD *)buffer = session->recv_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    default:
        FIXME("unimplemented option %u\n", option);
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

static BOOL session_set_option( object_header_t *hdr, DWORD option, LPVOID buffer, DWORD buflen )
{
    session_t *session = (session_t *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_PROXY:
    {
        WINHTTP_PROXY_INFO *pi = buffer;

        FIXME("%u %s %s\n", pi->dwAccessType, debugstr_w(pi->lpszProxy), debugstr_w(pi->lpszProxyBypass));
        return TRUE;
    }
    case WINHTTP_OPTION_REDIRECT_POLICY:
    {
        DWORD policy;

        if (buflen != sizeof(policy))
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        policy = *(DWORD *)buffer;
        TRACE("0x%x\n", policy);
        hdr->redirect_policy = policy;
        return TRUE;
    }
    case WINHTTP_OPTION_DISABLE_FEATURE:
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        session->resolve_timeout = *(DWORD *)buffer;
        return TRUE;
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        session->connect_timeout = *(DWORD *)buffer;
        return TRUE;
    case WINHTTP_OPTION_SEND_TIMEOUT:
        session->send_timeout = *(DWORD *)buffer;
        return TRUE;
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        session->recv_timeout = *(DWORD *)buffer;
        return TRUE;
    default:
        FIXME("unimplemented option %u\n", option);
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

static const object_vtbl_t session_vtbl =
{
    session_destroy,
    session_query_option,
    session_set_option
};

/***********************************************************************
 *          WinHttpOpen (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpen( LPCWSTR agent, DWORD access, LPCWSTR proxy, LPCWSTR bypass, DWORD flags )
{
    session_t *session;
    HINTERNET handle = NULL;
#ifdef __REACTOS__
    WSADATA wsaData;
    int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (error) ERR("WSAStartup failed: %d\n", error);
#endif

    TRACE("%s, %u, %s, %s, 0x%08x\n", debugstr_w(agent), access, debugstr_w(proxy), debugstr_w(bypass), flags);

    if (!(session = heap_alloc_zero( sizeof(session_t) ))) return NULL;

    session->hdr.type = WINHTTP_HANDLE_TYPE_SESSION;
    session->hdr.vtbl = &session_vtbl;
    session->hdr.flags = flags;
    session->hdr.refs = 1;
    session->hdr.redirect_policy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    list_init( &session->hdr.children );
    session->resolve_timeout = DEFAULT_RESOLVE_TIMEOUT;
    session->connect_timeout = DEFAULT_CONNECT_TIMEOUT;
    session->send_timeout = DEFAULT_SEND_TIMEOUT;
    session->recv_timeout = DEFAULT_RECEIVE_TIMEOUT;
    list_init( &session->cookie_cache );

    if (agent && !(session->agent = strdupW( agent ))) goto end;
    if (access == WINHTTP_ACCESS_TYPE_DEFAULT_PROXY)
    {
        WINHTTP_PROXY_INFO info;

        WinHttpGetDefaultProxyConfiguration( &info );
        session->access = info.dwAccessType;
        if (info.lpszProxy && !(session->proxy_server = strdupW( info.lpszProxy )))
        {
            GlobalFree( (LPWSTR)info.lpszProxy );
            GlobalFree( (LPWSTR)info.lpszProxyBypass );
            goto end;
        }
        if (info.lpszProxyBypass && !(session->proxy_bypass = strdupW( info.lpszProxyBypass )))
        {
            GlobalFree( (LPWSTR)info.lpszProxy );
            GlobalFree( (LPWSTR)info.lpszProxyBypass );
            goto end;
        }
    }
    else if (access == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
    {
        session->access = access;
        if (proxy && !(session->proxy_server = strdupW( proxy ))) goto end;
        if (bypass && !(session->proxy_bypass = strdupW( bypass ))) goto end;
    }

    if (!(handle = alloc_handle( &session->hdr ))) goto end;
    session->hdr.handle = handle;

end:
    release_object( &session->hdr );
    TRACE("returning %p\n", handle);
    return handle;
}

/***********************************************************************
 *          connect_destroy (internal)
 */
static void connect_destroy( object_header_t *hdr )
{
    connect_t *connect = (connect_t *)hdr;

    TRACE("%p\n", connect);

    release_object( &connect->session->hdr );

    heap_free( connect->hostname );
    heap_free( connect->servername );
    heap_free( connect->username );
    heap_free( connect->password );
    heap_free( connect );
}

static BOOL connect_query_option( object_header_t *hdr, DWORD option, LPVOID buffer, LPDWORD buflen )
{
    connect_t *connect = (connect_t *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_PARENT_HANDLE:
    {
        if (!buffer || *buflen < sizeof(HINTERNET))
        {
            *buflen = sizeof(HINTERNET);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        *(HINTERNET *)buffer = ((object_header_t *)connect->session)->handle;
        *buflen = sizeof(HINTERNET);
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        *(DWORD *)buffer = connect->session->resolve_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        *(DWORD *)buffer = connect->session->connect_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    case WINHTTP_OPTION_SEND_TIMEOUT:
        *(DWORD *)buffer = connect->session->send_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        *(DWORD *)buffer = connect->session->recv_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    default:
        FIXME("unimplemented option %u\n", option);
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

static const object_vtbl_t connect_vtbl =
{
    connect_destroy,
    connect_query_option,
    NULL
};

static BOOL domain_matches(LPCWSTR server, LPCWSTR domain)
{
    static const WCHAR localW[] = { '<','l','o','c','a','l','>',0 };
    BOOL ret = FALSE;

    if (!strcmpiW( domain, localW ) && !strchrW( server, '.' ))
        ret = TRUE;
    else if (*domain == '*')
    {
        if (domain[1] == '.')
        {
            LPCWSTR dot;

            /* For a hostname to match a wildcard, the last domain must match
             * the wildcard exactly.  E.g. if the wildcard is *.a.b, and the
             * hostname is www.foo.a.b, it matches, but a.b does not.
             */
            dot = strchrW( server, '.' );
            if (dot)
            {
                int len = strlenW( dot + 1 );

                if (len > strlenW( domain + 2 ))
                {
                    LPCWSTR ptr;

                    /* The server's domain is longer than the wildcard, so it
                     * could be a subdomain.  Compare the last portion of the
                     * server's domain.
                     */
                    ptr = dot + len + 1 - strlenW( domain + 2 );
                    if (!strcmpiW( ptr, domain + 2 ))
                    {
                        /* This is only a match if the preceding character is
                         * a '.', i.e. that it is a matching domain.  E.g.
                         * if domain is '*.b.c' and server is 'www.ab.c' they
                         * do not match.
                         */
                        ret = *(ptr - 1) == '.';
                    }
                }
                else
                    ret = !strcmpiW( dot + 1, domain + 2 );
            }
        }
    }
    else
        ret = !strcmpiW( server, domain );
    return ret;
}

/* Matches INTERNET_MAX_HOST_NAME_LENGTH in wininet.h, also RFC 1035 */
#define MAX_HOST_NAME_LENGTH 256

static BOOL should_bypass_proxy(session_t *session, LPCWSTR server)
{
    LPCWSTR ptr;
    BOOL ret = FALSE;

    if (!session->proxy_bypass) return FALSE;
    ptr = session->proxy_bypass;
    do {
        LPCWSTR tmp = ptr;

        ptr = strchrW( ptr, ';' );
        if (!ptr)
            ptr = strchrW( tmp, ' ' );
        if (ptr)
        {
            if (ptr - tmp < MAX_HOST_NAME_LENGTH)
            {
                WCHAR domain[MAX_HOST_NAME_LENGTH];

                memcpy( domain, tmp, (ptr - tmp) * sizeof(WCHAR) );
                domain[ptr - tmp] = 0;
                ret = domain_matches( server, domain );
            }
            ptr += 1;
        }
        else if (*tmp)
            ret = domain_matches( server, tmp );
    } while (ptr && !ret);
    return ret;
}

BOOL set_server_for_hostname( connect_t *connect, LPCWSTR server, INTERNET_PORT port )
{
    session_t *session = connect->session;
    BOOL ret = TRUE;

    if (session->proxy_server && !should_bypass_proxy(session, server))
    {
        LPCWSTR colon;

        if ((colon = strchrW( session->proxy_server, ':' )))
        {
            if (!connect->servername || strncmpiW( connect->servername,
                session->proxy_server, colon - session->proxy_server - 1 ))
            {
                heap_free( connect->servername );
                connect->resolved = FALSE;
                if (!(connect->servername = heap_alloc(
                    (colon - session->proxy_server + 1) * sizeof(WCHAR) )))
                {
                    ret = FALSE;
                    goto end;
                }
                memcpy( connect->servername, session->proxy_server,
                    (colon - session->proxy_server) * sizeof(WCHAR) );
                connect->servername[colon - session->proxy_server] = 0;
                if (*(colon + 1))
                    connect->serverport = atoiW( colon + 1 );
                else
                    connect->serverport = INTERNET_DEFAULT_PORT;
            }
        }
        else
        {
            if (!connect->servername || strcmpiW( connect->servername,
                session->proxy_server ))
            {
                heap_free( connect->servername );
                connect->resolved = FALSE;
                if (!(connect->servername = strdupW( session->proxy_server )))
                {
                    ret = FALSE;
                    goto end;
                }
                connect->serverport = INTERNET_DEFAULT_PORT;
            }
        }
    }
    else if (server)
    {
        heap_free( connect->servername );
        connect->resolved = FALSE;
        if (!(connect->servername = strdupW( server )))
        {
            ret = FALSE;
            goto end;
        }
        connect->serverport = port;
    }
end:
    return ret;
}

/***********************************************************************
 *          WinHttpConnect (winhttp.@)
 */
HINTERNET WINAPI WinHttpConnect( HINTERNET hsession, LPCWSTR server, INTERNET_PORT port, DWORD reserved )
{
    connect_t *connect;
    session_t *session;
    HINTERNET hconnect = NULL;

    TRACE("%p, %s, %u, %x\n", hsession, debugstr_w(server), port, reserved);

    if (!server)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (!(session = (session_t *)grab_object( hsession )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (session->hdr.type != WINHTTP_HANDLE_TYPE_SESSION)
    {
        release_object( &session->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(connect = heap_alloc_zero( sizeof(connect_t) )))
    {
        release_object( &session->hdr );
        return NULL;
    }
    connect->hdr.type = WINHTTP_HANDLE_TYPE_CONNECT;
    connect->hdr.vtbl = &connect_vtbl;
    connect->hdr.refs = 1;
    connect->hdr.flags = session->hdr.flags;
    connect->hdr.callback = session->hdr.callback;
    connect->hdr.notify_mask = session->hdr.notify_mask;
    connect->hdr.context = session->hdr.context;
    list_init( &connect->hdr.children );

    addref_object( &session->hdr );
    connect->session = session;
    list_add_head( &session->hdr.children, &connect->hdr.entry );

    if (!(connect->hostname = strdupW( server ))) goto end;
    connect->hostport = port;
    if (!set_server_for_hostname( connect, server, port )) goto end;

    if (!(hconnect = alloc_handle( &connect->hdr ))) goto end;
    connect->hdr.handle = hconnect;

    send_callback( &session->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hconnect, sizeof(hconnect) );

end:
    release_object( &connect->hdr );
    release_object( &session->hdr );
    TRACE("returning %p\n", hconnect);
    return hconnect;
}

/***********************************************************************
 *          request_destroy (internal)
 */
static void request_destroy( object_header_t *hdr )
{
    request_t *request = (request_t *)hdr;
    unsigned int i;

    TRACE("%p\n", request);

    release_object( &request->connect->hdr );

    heap_free( request->verb );
    heap_free( request->path );
    heap_free( request->version );
    heap_free( request->raw_headers );
    heap_free( request->status_text );
    for (i = 0; i < request->num_headers; i++)
    {
        heap_free( request->headers[i].field );
        heap_free( request->headers[i].value );
    }
    heap_free( request->headers );
    for (i = 0; i < request->num_accept_types; i++) heap_free( request->accept_types[i] );
    heap_free( request->accept_types );
    heap_free( request );
}

static void str_to_buffer( WCHAR *buffer, const WCHAR *str, LPDWORD buflen )
{
    int len = 0;
    if (str) len = strlenW( str );
    if (buffer && *buflen > len)
    {
        if (str) memcpy( buffer, str, len * sizeof(WCHAR) );
        buffer[len] = 0;
    }
    *buflen = len * sizeof(WCHAR);
}

static WCHAR *blob_to_str( DWORD encoding, CERT_NAME_BLOB *blob )
{
    WCHAR *ret;
    DWORD size, format = CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG;

    size = CertNameToStrW( encoding, blob, format, NULL, 0 );
    if ((ret = LocalAlloc( 0, size * sizeof(WCHAR) )))
        CertNameToStrW( encoding, blob, format, ret, size );

    return ret;
}

static BOOL request_query_option( object_header_t *hdr, DWORD option, LPVOID buffer, LPDWORD buflen )
{
    request_t *request = (request_t *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_SECURITY_FLAGS:
    {
        DWORD flags;
        int bits;

        if (!buffer || *buflen < sizeof(flags))
        {
            *buflen = sizeof(flags);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        flags = 0;
        if (hdr->flags & WINHTTP_FLAG_SECURE) flags |= SECURITY_FLAG_SECURE;
        flags |= request->netconn.security_flags;
        bits = netconn_get_cipher_strength( &request->netconn );
        if (bits >= 128)
            flags |= SECURITY_FLAG_STRENGTH_STRONG;
        else if (bits >= 56)
            flags |= SECURITY_FLAG_STRENGTH_MEDIUM;
        else
            flags |= SECURITY_FLAG_STRENGTH_WEAK;
        *(DWORD *)buffer = flags;
        *buflen = sizeof(flags);
        return TRUE;
    }
    case WINHTTP_OPTION_SERVER_CERT_CONTEXT:
    {
        const CERT_CONTEXT *cert;

        if (!buffer || *buflen < sizeof(cert))
        {
            *buflen = sizeof(cert);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        if (!(cert = netconn_get_certificate( &request->netconn ))) return FALSE;
        *(CERT_CONTEXT **)buffer = (CERT_CONTEXT *)cert;
        *buflen = sizeof(cert);
        return TRUE;
    }
    case WINHTTP_OPTION_SECURITY_CERTIFICATE_STRUCT:
    {
        const CERT_CONTEXT *cert;
        const CRYPT_OID_INFO *oidInfo;
        WINHTTP_CERTIFICATE_INFO *ci = buffer;

        FIXME("partial stub\n");

        if (!buffer || *buflen < sizeof(*ci))
        {
            *buflen = sizeof(*ci);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        if (!(cert = netconn_get_certificate( &request->netconn ))) return FALSE;

        ci->ftExpiry = cert->pCertInfo->NotAfter;
        ci->ftStart  = cert->pCertInfo->NotBefore;
        ci->lpszSubjectInfo = blob_to_str( cert->dwCertEncodingType, &cert->pCertInfo->Subject );
        ci->lpszIssuerInfo  = blob_to_str( cert->dwCertEncodingType, &cert->pCertInfo->Issuer );
        ci->lpszProtocolName      = NULL;
        oidInfo = CryptFindOIDInfo( CRYPT_OID_INFO_OID_KEY,
                                    cert->pCertInfo->SignatureAlgorithm.pszObjId,
                                    0 );
        if (oidInfo)
            ci->lpszSignatureAlgName = (LPWSTR)oidInfo->pwszName;
        else
            ci->lpszSignatureAlgName  = NULL;
        ci->lpszEncryptionAlgName = NULL;
        ci->dwKeySize = netconn_get_cipher_strength( &request->netconn );

        CertFreeCertificateContext( cert );
        *buflen = sizeof(*ci);
        return TRUE;
    }
    case WINHTTP_OPTION_SECURITY_KEY_BITNESS:
    {
        if (!buffer || *buflen < sizeof(DWORD))
        {
            *buflen = sizeof(DWORD);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        *(DWORD *)buffer = netconn_get_cipher_strength( &request->netconn );
        *buflen = sizeof(DWORD);
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        *(DWORD *)buffer = request->resolve_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        *(DWORD *)buffer = request->connect_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    case WINHTTP_OPTION_SEND_TIMEOUT:
        *(DWORD *)buffer = request->send_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        *(DWORD *)buffer = request->recv_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_USERNAME:
        str_to_buffer( buffer, request->connect->username, buflen );
        return TRUE;

    case WINHTTP_OPTION_PASSWORD:
        str_to_buffer( buffer, request->connect->password, buflen );
        return TRUE;

    case WINHTTP_OPTION_PROXY_USERNAME:
        str_to_buffer( buffer, request->connect->session->proxy_username, buflen );
        return TRUE;

    case WINHTTP_OPTION_PROXY_PASSWORD:
        str_to_buffer( buffer, request->connect->session->proxy_password, buflen );
        return TRUE;

    default:
        FIXME("unimplemented option %u\n", option);
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

static WCHAR *buffer_to_str( WCHAR *buffer, DWORD buflen )
{
    WCHAR *ret;
    if ((ret = heap_alloc( (buflen + 1) * sizeof(WCHAR))))
    {
        memcpy( ret, buffer, buflen * sizeof(WCHAR) );
        ret[buflen] = 0;
        return ret;
    }
    set_last_error( ERROR_OUTOFMEMORY );
    return NULL;
}

static BOOL request_set_option( object_header_t *hdr, DWORD option, LPVOID buffer, DWORD buflen )
{
    request_t *request = (request_t *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_PROXY:
    {
        WINHTTP_PROXY_INFO *pi = buffer;

        FIXME("%u %s %s\n", pi->dwAccessType, debugstr_w(pi->lpszProxy), debugstr_w(pi->lpszProxyBypass));
        return TRUE;
    }
    case WINHTTP_OPTION_DISABLE_FEATURE:
    {
        DWORD disable;

        if (buflen != sizeof(DWORD))
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        disable = *(DWORD *)buffer;
        TRACE("0x%x\n", disable);
        hdr->disable_flags |= disable;
        return TRUE;
    }
    case WINHTTP_OPTION_AUTOLOGON_POLICY:
    {
        DWORD policy;

        if (buflen != sizeof(DWORD))
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        policy = *(DWORD *)buffer;
        TRACE("0x%x\n", policy);
        hdr->logon_policy = policy;
        return TRUE;
    }
    case WINHTTP_OPTION_REDIRECT_POLICY:
    {
        DWORD policy;

        if (buflen != sizeof(DWORD))
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        policy = *(DWORD *)buffer;
        TRACE("0x%x\n", policy);
        hdr->redirect_policy = policy;
        return TRUE;
    }
    case WINHTTP_OPTION_SECURITY_FLAGS:
    {
        DWORD flags;

        if (buflen < sizeof(DWORD))
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        flags = *(DWORD *)buffer;
        TRACE("0x%x\n", flags);
        if (!(flags & (SECURITY_FLAG_IGNORE_CERT_CN_INVALID   |
                       SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                       SECURITY_FLAG_IGNORE_UNKNOWN_CA        |
                       SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE)))
        {
            set_last_error( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        request->netconn.security_flags = flags;
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        request->resolve_timeout = *(DWORD *)buffer;
        return TRUE;
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        request->connect_timeout = *(DWORD *)buffer;
        return TRUE;
    case WINHTTP_OPTION_SEND_TIMEOUT:
        request->send_timeout = *(DWORD *)buffer;
        return TRUE;
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        request->recv_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_USERNAME:
    {
        connect_t *connect = request->connect;

        heap_free( connect->username );
        if (!(connect->username = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_PASSWORD:
    {
        connect_t *connect = request->connect;

        heap_free( connect->password );
        if (!(connect->password = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_PROXY_USERNAME:
    {
        session_t *session = request->connect->session;

        heap_free( session->proxy_username );
        if (!(session->proxy_username = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_PROXY_PASSWORD:
    {
        session_t *session = request->connect->session;

        heap_free( session->proxy_password );
        if (!(session->proxy_password = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    default:
        FIXME("unimplemented option %u\n", option);
        set_last_error( ERROR_INVALID_PARAMETER );
        return TRUE;
    }
}

static const object_vtbl_t request_vtbl =
{
    request_destroy,
    request_query_option,
    request_set_option
};

static BOOL store_accept_types( request_t *request, const WCHAR **accept_types )
{
    const WCHAR **types = accept_types;
    int i;

    if (!types) return TRUE;
    while (*types)
    {
        request->num_accept_types++;
        types++;
    }
    if (!request->num_accept_types) return TRUE;
    if (!(request->accept_types = heap_alloc( request->num_accept_types * sizeof(WCHAR *))))
    {
        request->num_accept_types = 0;
        return FALSE;
    }
    types = accept_types;
    for (i = 0; i < request->num_accept_types; i++)
    {
        if (!(request->accept_types[i] = strdupW( *types )))
        {
            for (; i >= 0; i--) heap_free( request->accept_types[i] );
            heap_free( request->accept_types );
            request->accept_types = NULL;
            request->num_accept_types = 0;
            return FALSE;
        }
        types++;
    }
    return TRUE;
}

/***********************************************************************
 *          WinHttpOpenRequest (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpenRequest( HINTERNET hconnect, LPCWSTR verb, LPCWSTR object, LPCWSTR version,
                                     LPCWSTR referrer, LPCWSTR *types, DWORD flags )
{
    request_t *request;
    connect_t *connect;
    HINTERNET hrequest = NULL;

    TRACE("%p, %s, %s, %s, %s, %p, 0x%08x\n", hconnect, debugstr_w(verb), debugstr_w(object),
          debugstr_w(version), debugstr_w(referrer), types, flags);

    if(types && TRACE_ON(winhttp)) {
        const WCHAR **iter;

        TRACE("accept types:\n");
        for(iter = types; *iter; iter++)
            TRACE("    %s\n", debugstr_w(*iter));
    }

    if (!(connect = (connect_t *)grab_object( hconnect )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (connect->hdr.type != WINHTTP_HANDLE_TYPE_CONNECT)
    {
        release_object( &connect->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(request = heap_alloc_zero( sizeof(request_t) )))
    {
        release_object( &connect->hdr );
        return NULL;
    }
    request->hdr.type = WINHTTP_HANDLE_TYPE_REQUEST;
    request->hdr.vtbl = &request_vtbl;
    request->hdr.refs = 1;
    request->hdr.flags = flags;
    request->hdr.callback = connect->hdr.callback;
    request->hdr.notify_mask = connect->hdr.notify_mask;
    request->hdr.context = connect->hdr.context;
    list_init( &request->hdr.children );

    addref_object( &connect->hdr );
    request->connect = connect;
    list_add_head( &connect->hdr.children, &request->hdr.entry );

    if (!netconn_init( &request->netconn, request->hdr.flags & WINHTTP_FLAG_SECURE )) goto end;
    request->resolve_timeout = connect->session->resolve_timeout;
    request->connect_timeout = connect->session->connect_timeout;
    request->send_timeout = connect->session->send_timeout;
    request->recv_timeout = connect->session->recv_timeout;

    if (!verb || !verb[0]) verb = getW;
    if (!(request->verb = strdupW( verb ))) goto end;

    if (object)
    {
        WCHAR *path, *p;
        unsigned int len;

        len = strlenW( object ) + 1;
        if (object[0] != '/') len++;
        if (!(p = path = heap_alloc( len * sizeof(WCHAR) ))) goto end;

        if (object[0] != '/') *p++ = '/';
        strcpyW( p, object );
        request->path = path;
    }
    else if (!(request->path = strdupW( slashW ))) goto end;

    if (!version || !version[0]) version = http1_1;
    if (!(request->version = strdupW( version ))) goto end;
    if (!(store_accept_types( request, types ))) goto end;

    if (!(hrequest = alloc_handle( &request->hdr ))) goto end;
    request->hdr.handle = hrequest;

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hrequest, sizeof(hrequest) );

end:
    release_object( &request->hdr );
    release_object( &connect->hdr );
    TRACE("returning %p\n", hrequest);
    return hrequest;
}

/***********************************************************************
 *          WinHttpCloseHandle (winhttp.@)
 */
BOOL WINAPI WinHttpCloseHandle( HINTERNET handle )
{
    object_header_t *hdr;

    TRACE("%p\n", handle);

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    release_object( hdr );
    free_handle( handle );
    return TRUE;
}

static BOOL query_option( object_header_t *hdr, DWORD option, LPVOID buffer, LPDWORD buflen )
{
    BOOL ret = FALSE;

    if (!buflen)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (option)
    {
    case WINHTTP_OPTION_CONTEXT_VALUE:
    {
        if (!buffer || *buflen < sizeof(DWORD_PTR))
        {
            *buflen = sizeof(DWORD_PTR);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        *(DWORD_PTR *)buffer = hdr->context;
        *buflen = sizeof(DWORD_PTR);
        return TRUE;
    }
    default:
        if (hdr->vtbl->query_option) ret = hdr->vtbl->query_option( hdr, option, buffer, buflen );
        else
        {
            FIXME("unimplemented option %u\n", option);
            set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
            return FALSE;
        }
        break;
    }
    return ret;
}

/***********************************************************************
 *          WinHttpQueryOption (winhttp.@)
 */
BOOL WINAPI WinHttpQueryOption( HINTERNET handle, DWORD option, LPVOID buffer, LPDWORD buflen )
{
    BOOL ret = FALSE;
    object_header_t *hdr;

    TRACE("%p, %u, %p, %p\n", handle, option, buffer, buflen);

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    ret = query_option( hdr, option, buffer, buflen );

    release_object( hdr );
    return ret;
}

static BOOL set_option( object_header_t *hdr, DWORD option, LPVOID buffer, DWORD buflen )
{
    BOOL ret = TRUE;

    if (!buffer)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (option)
    {
    case WINHTTP_OPTION_CONTEXT_VALUE:
    {
        if (buflen != sizeof(DWORD_PTR))
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        hdr->context = *(DWORD_PTR *)buffer;
        return TRUE;
    }
    default:
        if (hdr->vtbl->set_option) ret = hdr->vtbl->set_option( hdr, option, buffer, buflen );
        else
        {
            FIXME("unimplemented option %u\n", option);
            set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
            return FALSE;
        }
        break;
    }
    return ret;
}

/***********************************************************************
 *          WinHttpSetOption (winhttp.@)
 */
BOOL WINAPI WinHttpSetOption( HINTERNET handle, DWORD option, LPVOID buffer, DWORD buflen )
{
    BOOL ret = FALSE;
    object_header_t *hdr;

    TRACE("%p, %u, %p, %u\n", handle, option, buffer, buflen);

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    ret = set_option( hdr, option, buffer, buflen );

    release_object( hdr );
    return ret;
}

static char *get_computer_name( COMPUTER_NAME_FORMAT format )
{
    char *ret;
    DWORD size = 0;

    GetComputerNameExA( format, NULL, &size );
    if (GetLastError() != ERROR_MORE_DATA) return NULL;
    if (!(ret = heap_alloc( size ))) return NULL;
    if (!GetComputerNameExA( format, ret, &size ))
    {
        heap_free( ret );
        return NULL;
    }
    return ret;
}

static BOOL is_domain_suffix( const char *domain, const char *suffix )
{
    int len_domain = strlen( domain ), len_suffix = strlen( suffix );

    if (len_suffix > len_domain) return FALSE;
    if (!strcasecmp( domain + len_domain - len_suffix, suffix )) return TRUE;
    return FALSE;
}

static void printf_addr( const WCHAR *fmt, WCHAR *buf, struct sockaddr_in *addr )
{
    sprintfW( buf, fmt,
              (unsigned int)(ntohl( addr->sin_addr.s_addr ) >> 24 & 0xff),
              (unsigned int)(ntohl( addr->sin_addr.s_addr ) >> 16 & 0xff),
              (unsigned int)(ntohl( addr->sin_addr.s_addr ) >> 8 & 0xff),
              (unsigned int)(ntohl( addr->sin_addr.s_addr ) & 0xff) );
}

static WCHAR *build_wpad_url( const struct addrinfo *ai )
{
    static const WCHAR fmtW[] =
        {'h','t','t','p',':','/','/','%','u','.','%','u','.','%','u','.','%','u',
         '/','w','p','a','d','.','d','a','t',0};
    WCHAR *ret;

    while (ai && ai->ai_family != AF_INET) ai = ai->ai_next;
    if (!ai) return NULL;

    if (!(ret = GlobalAlloc( 0, sizeof(fmtW) + 12 * sizeof(WCHAR) ))) return NULL;
    printf_addr( fmtW, ret, (struct sockaddr_in *)ai->ai_addr );
    return ret;
}

/***********************************************************************
 *          WinHttpDetectAutoProxyConfigUrl (winhttp.@)
 */
BOOL WINAPI WinHttpDetectAutoProxyConfigUrl( DWORD flags, LPWSTR *url )
{
    BOOL ret = FALSE;

    TRACE("0x%08x, %p\n", flags, url);

    if (!flags || !url)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (flags & WINHTTP_AUTO_DETECT_TYPE_DHCP) FIXME("discovery via DHCP not supported\n");
    if (flags & WINHTTP_AUTO_DETECT_TYPE_DNS_A)
    {
#ifdef HAVE_GETADDRINFO
        char *fqdn, *domain, *p;

        if (!(fqdn = get_computer_name( ComputerNamePhysicalDnsFullyQualified ))) return FALSE;
        if (!(domain = get_computer_name( ComputerNamePhysicalDnsDomain )))
        {
            heap_free( fqdn );
            return FALSE;
        }
        p = fqdn;
        while ((p = strchr( p, '.' )) && is_domain_suffix( p + 1, domain ))
        {
            struct addrinfo *ai;
            char *name;
            int res;

            if (!(name = heap_alloc( sizeof("wpad") + strlen(p) )))
            {
                heap_free( fqdn );
                heap_free( domain );
                return FALSE;
            }
            strcpy( name, "wpad" );
            strcat( name, p );
            res = getaddrinfo( name, NULL, NULL, &ai );
            heap_free( name );
            if (!res)
            {
                *url = build_wpad_url( ai );
                freeaddrinfo( ai );
                if (*url)
                {
                    TRACE("returning %s\n", debugstr_w(*url));
                    ret = TRUE;
                    break;
                }
            }
            p++;
        }
        heap_free( domain );
        heap_free( fqdn );
#else
    FIXME("getaddrinfo not found at build time\n");
#endif
    }
    if (!ret) set_last_error( ERROR_WINHTTP_AUTODETECTION_FAILED );
    return ret;
}

static const WCHAR Connections[] = {
    'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s','\\',
    'C','o','n','n','e','c','t','i','o','n','s',0 };
static const WCHAR WinHttpSettings[] = {
    'W','i','n','H','t','t','p','S','e','t','t','i','n','g','s',0 };
static const DWORD WINHTTP_SETTINGS_MAGIC = 0x18;
static const DWORD WININET_SETTINGS_MAGIC = 0x46;
static const DWORD PROXY_TYPE_DIRECT         = 1;
static const DWORD PROXY_TYPE_PROXY          = 2;
static const DWORD PROXY_USE_PAC_SCRIPT      = 4;
static const DWORD PROXY_AUTODETECT_SETTINGS = 8;

struct connection_settings_header
{
    DWORD magic;
    DWORD unknown; /* always zero? */
    DWORD flags;   /* one or more of PROXY_* */
};

static inline void copy_char_to_wchar_sz(const BYTE *src, DWORD len, WCHAR *dst)
{
    const BYTE *begin;

    for (begin = src; src - begin < len; src++, dst++)
        *dst = *src;
    *dst = 0;
}

/***********************************************************************
 *          WinHttpGetDefaultProxyConfiguration (winhttp.@)
 */
BOOL WINAPI WinHttpGetDefaultProxyConfiguration( WINHTTP_PROXY_INFO *info )
{
    LONG l;
    HKEY key;
    BOOL got_from_reg = FALSE, direct = TRUE;
    char *envproxy;

    TRACE("%p\n", info);

    l = RegOpenKeyExW( HKEY_LOCAL_MACHINE, Connections, 0, KEY_READ, &key );
    if (!l)
    {
        DWORD type, size = 0;

        l = RegQueryValueExW( key, WinHttpSettings, NULL, &type, NULL, &size );
        if (!l && type == REG_BINARY &&
            size >= sizeof(struct connection_settings_header) + 2 * sizeof(DWORD))
        {
            BYTE *buf = heap_alloc( size );

            if (buf)
            {
                struct connection_settings_header *hdr =
                    (struct connection_settings_header *)buf;
                DWORD *len = (DWORD *)(hdr + 1);

                l = RegQueryValueExW( key, WinHttpSettings, NULL, NULL, buf,
                    &size );
                if (!l && hdr->magic == WINHTTP_SETTINGS_MAGIC &&
                    hdr->unknown == 0)
                {
                    if (hdr->flags & PROXY_TYPE_PROXY)
                    {
                       BOOL sane = FALSE;
                       LPWSTR proxy = NULL;
                       LPWSTR proxy_bypass = NULL;

                        /* Sanity-check length of proxy string */
                        if ((BYTE *)len - buf + *len <= size)
                        {
                            sane = TRUE;
                            proxy = GlobalAlloc( 0, (*len + 1) * sizeof(WCHAR) );
                            if (proxy)
                                copy_char_to_wchar_sz( (BYTE *)(len + 1), *len, proxy );
                            len = (DWORD *)((BYTE *)(len + 1) + *len);
                        }
                        if (sane)
                        {
                            /* Sanity-check length of proxy bypass string */
                            if ((BYTE *)len - buf + *len <= size)
                            {
                                proxy_bypass = GlobalAlloc( 0, (*len + 1) * sizeof(WCHAR) );
                                if (proxy_bypass)
                                    copy_char_to_wchar_sz( (BYTE *)(len + 1), *len, proxy_bypass );
                            }
                            else
                            {
                                sane = FALSE;
                                GlobalFree( proxy );
                                proxy = NULL;
                            }
                        }
                        info->lpszProxy = proxy;
                        info->lpszProxyBypass = proxy_bypass;
                        if (sane)
                        {
                            got_from_reg = TRUE;
                            direct = FALSE;
                            info->dwAccessType =
                                WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                            TRACE("http proxy (from registry) = %s, bypass = %s\n",
                                debugstr_w(info->lpszProxy),
                                debugstr_w(info->lpszProxyBypass));
                        }
                    }
                }
                heap_free( buf );
            }
        }
        RegCloseKey( key );
    }
    if (!got_from_reg && (envproxy = getenv( "http_proxy" )))
    {
        char *colon, *http_proxy;

        if ((colon = strchr( envproxy, ':' )))
        {
            if (*(colon + 1) == '/' && *(colon + 2) == '/')
            {
                static const char http[] = "http://";

                /* It's a scheme, check that it's http */
                if (!strncmp( envproxy, http, strlen( http ) ))
                    http_proxy = envproxy + strlen( http );
                else
                {
                    WARN("unsupported scheme in $http_proxy: %s\n", envproxy);
                    http_proxy = NULL;
                }
            }
            else
                http_proxy = envproxy;
        }
        else
            http_proxy = envproxy;
        if (http_proxy)
        {
            WCHAR *http_proxyW;
            int len;

            len = MultiByteToWideChar( CP_UNIXCP, 0, http_proxy, -1, NULL, 0 );
            if ((http_proxyW = GlobalAlloc( 0, len * sizeof(WCHAR))))
            {
                MultiByteToWideChar( CP_UNIXCP, 0, http_proxy, -1, http_proxyW, len );
                direct = FALSE;
                info->dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                info->lpszProxy = http_proxyW;
                info->lpszProxyBypass = NULL;
                TRACE("http proxy (from environment) = %s\n",
                    debugstr_w(info->lpszProxy));
            }
        }
    }
    if (direct)
    {
        info->dwAccessType    = WINHTTP_ACCESS_TYPE_NO_PROXY;
        info->lpszProxy       = NULL;
        info->lpszProxyBypass = NULL;
    }
    return TRUE;
}

/***********************************************************************
 *          WinHttpGetIEProxyConfigForCurrentUser (winhttp.@)
 */
BOOL WINAPI WinHttpGetIEProxyConfigForCurrentUser( WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *config )
{
    static const WCHAR settingsW[] =
        {'D','e','f','a','u','l','t','C','o','n','n','e','c','t','i','o','n','S','e','t','t','i','n','g','s',0};
    HKEY hkey = NULL;
    struct connection_settings_header *hdr = NULL;
    DWORD type, offset, len, size = 0;
    BOOL ret = FALSE;

    TRACE("%p\n", config);

    if (!config)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    memset( config, 0, sizeof(*config) );
    config->fAutoDetect = TRUE;

    if (RegOpenKeyExW( HKEY_CURRENT_USER, Connections, 0, KEY_READ, &hkey ) ||
        RegQueryValueExW( hkey, settingsW, NULL, &type, NULL, &size ) ||
        type != REG_BINARY || size < sizeof(struct connection_settings_header))
    {
        ret = TRUE;
        goto done;
    }
    if (!(hdr = heap_alloc( size ))) goto done;
    if (RegQueryValueExW( hkey, settingsW, NULL, &type, (BYTE *)hdr, &size ) ||
        hdr->magic != WININET_SETTINGS_MAGIC)
    {
        ret = TRUE;
        goto done;
    }

    config->fAutoDetect = (hdr->flags & PROXY_AUTODETECT_SETTINGS) != 0;
    offset = sizeof(*hdr);
    if (offset + sizeof(DWORD) > size) goto done;
    len = *(DWORD *)((char *)hdr + offset);
    offset += sizeof(DWORD);
    if (len && hdr->flags & PROXY_TYPE_PROXY)
    {
        if (!(config->lpszProxy = GlobalAlloc( 0, (len + 1) * sizeof(WCHAR) ))) goto done;
        copy_char_to_wchar_sz( (const BYTE *)hdr + offset , len, config->lpszProxy );
    }
    offset += len;
    if (offset + sizeof(DWORD) > size) goto done;
    len = *(DWORD *)((char *)hdr + offset);
    offset += sizeof(DWORD);
    if (len && (hdr->flags & PROXY_TYPE_PROXY))
    {
        if (!(config->lpszProxyBypass = GlobalAlloc( 0, (len + 1) * sizeof(WCHAR) ))) goto done;
        copy_char_to_wchar_sz( (const BYTE *)hdr + offset , len, config->lpszProxyBypass );
    }
    offset += len;
    if (offset + sizeof(DWORD) > size) goto done;
    len = *(DWORD *)((char *)hdr + offset);
    offset += sizeof(DWORD);
    if (len && (hdr->flags & PROXY_USE_PAC_SCRIPT))
    {
        if (!(config->lpszAutoConfigUrl = GlobalAlloc( 0, (len + 1) * sizeof(WCHAR) ))) goto done;
        copy_char_to_wchar_sz( (const BYTE *)hdr + offset , len, config->lpszAutoConfigUrl );
    }
    ret = TRUE;

done:
    RegCloseKey( hkey );
    heap_free( hdr );
    if (!ret)
    {
        heap_free( config->lpszAutoConfigUrl );
        config->lpszAutoConfigUrl = NULL;
        heap_free( config->lpszProxy );
        config->lpszProxy = NULL;
        heap_free( config->lpszProxyBypass );
        config->lpszProxyBypass = NULL;
    }
    return ret;
}

static HRESULT WINAPI dispex_QueryInterface(
    IDispatchEx *iface, REFIID riid, void **ppv )
{
    *ppv = NULL;

    if (IsEqualGUID( riid, &IID_IUnknown )  ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IDispatchEx ))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI dispex_AddRef(
    IDispatchEx *iface )
{
    return 2;
}

static ULONG WINAPI dispex_Release(
    IDispatchEx *iface )
{
    return 1;
}

static HRESULT WINAPI dispex_GetTypeInfoCount(
    IDispatchEx *iface, UINT *info )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetTypeInfo(
    IDispatchEx *iface, UINT info, LCID lcid, ITypeInfo **type_info )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetIDsOfNames(
    IDispatchEx *iface, REFIID riid, LPOLESTR *names, UINT count, LCID lcid, DISPID *id )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_Invoke(
    IDispatchEx *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
    DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep, UINT *err )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_DeleteMemberByName(
    IDispatchEx *iface, BSTR name, DWORD flags )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_DeleteMemberByDispID(
    IDispatchEx *iface, DISPID id )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetMemberProperties(
    IDispatchEx *iface, DISPID id, DWORD flags_fetch, DWORD *flags )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetMemberName(
    IDispatchEx *iface, DISPID id, BSTR *name )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetNextDispID(
    IDispatchEx *iface, DWORD flags, DISPID id, DISPID *next )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetNameSpaceParent(
    IDispatchEx *iface, IUnknown **unk )
{
    return E_NOTIMPL;
}

#define DISPID_GLOBAL_DNSRESOLVE  0x1000

static HRESULT WINAPI dispex_GetDispID(
    IDispatchEx *iface, BSTR name, DWORD flags, DISPID *id )
{
    if (!strcmpW( name, dns_resolveW ))
    {
        *id = DISPID_GLOBAL_DNSRESOLVE;
        return S_OK;
    }
    return DISP_E_UNKNOWNNAME;
}

static HRESULT dns_resolve( const WCHAR *hostname, VARIANT *result )
{
#ifdef HAVE_GETADDRINFO
        static const WCHAR fmtW[] = {'%','u','.','%','u','.','%','u','.','%','u',0};
        WCHAR addr[16];
        struct addrinfo *ai, *elem;
        char *hostnameA;
        int res;

        if (hostname[0])
            hostnameA = strdupWA( hostname );
        else
            hostnameA = get_computer_name( ComputerNamePhysicalDnsFullyQualified );

        if (!hostnameA) return E_OUTOFMEMORY;
        res = getaddrinfo( hostnameA, NULL, NULL, &ai );
        heap_free( hostnameA );
        if (res) return S_FALSE;

        elem = ai;
        while (elem && elem->ai_family != AF_INET) elem = elem->ai_next;
        if (!elem)
        {
            freeaddrinfo( ai );
            return S_FALSE;
        }
        printf_addr( fmtW, addr, (struct sockaddr_in *)elem->ai_addr );
        freeaddrinfo( ai );
        V_VT( result ) = VT_BSTR;
        V_BSTR( result ) = SysAllocString( addr );
        return S_OK;
#else
        FIXME("getaddrinfo not found at build time\n");
        return S_FALSE;
#endif
}

static HRESULT WINAPI dispex_InvokeEx(
    IDispatchEx *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
    VARIANT *result, EXCEPINFO *exep, IServiceProvider *caller )
{
    if (id == DISPID_GLOBAL_DNSRESOLVE)
    {
        if (params->cArgs != 1) return DISP_E_BADPARAMCOUNT;
        if (V_VT(&params->rgvarg[0]) != VT_BSTR) return DISP_E_BADVARTYPE;
        return dns_resolve( V_BSTR(&params->rgvarg[0]), result );
    }
    return DISP_E_MEMBERNOTFOUND;
}

static const IDispatchExVtbl dispex_vtbl =
{
    dispex_QueryInterface,
    dispex_AddRef,
    dispex_Release,
    dispex_GetTypeInfoCount,
    dispex_GetTypeInfo,
    dispex_GetIDsOfNames,
    dispex_Invoke,
    dispex_GetDispID,
    dispex_InvokeEx,
    dispex_DeleteMemberByName,
    dispex_DeleteMemberByDispID,
    dispex_GetMemberProperties,
    dispex_GetMemberName,
    dispex_GetNextDispID,
    dispex_GetNameSpaceParent
};

static IDispatchEx global_dispex = { &dispex_vtbl };

static HRESULT WINAPI site_QueryInterface(
    IActiveScriptSite *iface, REFIID riid, void **ppv )
{
    *ppv = NULL;

    if (IsEqualGUID( &IID_IUnknown, riid ))
        *ppv = iface;
    else if (IsEqualGUID( &IID_IActiveScriptSite, riid ))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef( (IUnknown *)*ppv );
    return S_OK;
}

static ULONG WINAPI site_AddRef(
    IActiveScriptSite *iface )
{
    return 2;
}

static ULONG WINAPI site_Release(
    IActiveScriptSite *iface )
{
    return 1;
}

static HRESULT WINAPI site_GetLCID(
    IActiveScriptSite *iface, LCID *lcid )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_GetItemInfo(
    IActiveScriptSite *iface, LPCOLESTR name, DWORD mask,
    IUnknown **item, ITypeInfo **type_info )
{
    if (!strcmpW( name, global_funcsW ) && mask == SCRIPTINFO_IUNKNOWN)
    {
        *item = (IUnknown *)&global_dispex;
        return S_OK;
    }
    return E_NOTIMPL;
}

static HRESULT WINAPI site_GetDocVersionString(
    IActiveScriptSite *iface, BSTR *version )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnScriptTerminate(
    IActiveScriptSite *iface, const VARIANT *result, const EXCEPINFO *info )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnStateChange(
    IActiveScriptSite *iface, SCRIPTSTATE state )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnScriptError(
    IActiveScriptSite *iface, IActiveScriptError *error )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnEnterScript(
    IActiveScriptSite *iface )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnLeaveScript(
    IActiveScriptSite *iface )
{
    return E_NOTIMPL;
}

static const IActiveScriptSiteVtbl site_vtbl =
{
    site_QueryInterface,
    site_AddRef,
    site_Release,
    site_GetLCID,
    site_GetItemInfo,
    site_GetDocVersionString,
    site_OnScriptTerminate,
    site_OnStateChange,
    site_OnScriptError,
    site_OnEnterScript,
    site_OnLeaveScript
};

static IActiveScriptSite script_site = { &site_vtbl };

static BOOL parse_script_result( VARIANT result, WINHTTP_PROXY_INFO *info )
{
    static const WCHAR proxyW[] = {'P','R','O','X','Y'};
    const WCHAR *p;
    WCHAR *q;
    int len;

    info->dwAccessType    = WINHTTP_ACCESS_TYPE_NO_PROXY;
    info->lpszProxy       = NULL;
    info->lpszProxyBypass = NULL;

    if (V_VT( &result ) != VT_BSTR) return TRUE;
    TRACE("%s\n", debugstr_w( V_BSTR( &result ) ));

    p = V_BSTR( &result );
    while (*p == ' ') p++;
    len = strlenW( p );
    if (len >= 5 && !memicmpW( p, proxyW, sizeof(proxyW)/sizeof(WCHAR) ))
    {
        p += 5;
        while (*p == ' ') p++;
        if (!*p || *p == ';') return TRUE;
        if (!(info->lpszProxy = q = strdupW( p ))) return FALSE;
        info->dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        for (; *q; q++)
        {
            if (*q == ' ' || *q == ';')
            {
                *q = 0;
                break;
            }
        }
    }
    return TRUE;
}

static BSTR include_pac_utils( BSTR script )
{
    static const WCHAR pacjsW[] = {'p','a','c','.','j','s',0};
    HMODULE hmod = GetModuleHandleA( "winhttp.dll" );
    HRSRC rsrc;
    DWORD size;
    const char *data;
    BSTR ret;
    int len;

    if (!(rsrc = FindResourceW( hmod, pacjsW, (LPCWSTR)40 ))) return NULL;
    size = SizeofResource( hmod, rsrc );
    data = LoadResource( hmod, rsrc );

    len = MultiByteToWideChar( CP_ACP, 0, data, size, NULL, 0 );
    if (!(ret = SysAllocStringLen( NULL, len + SysStringLen( script ) + 1 ))) return NULL;
    MultiByteToWideChar( CP_ACP, 0, data, size, ret, len );
    ret[len] = 0;
    strcatW( ret, script );
    return ret;
}

static BOOL run_script( const BSTR script, const WCHAR *url, WINHTTP_PROXY_INFO *info )
{
    static const WCHAR jscriptW[] = {'J','S','c','r','i','p','t',0};
    static const WCHAR findproxyW[] = {'F','i','n','d','P','r','o','x','y','F','o','r','U','R','L',0};
    IActiveScriptParse *parser = NULL;
    IActiveScript *engine = NULL;
    IDispatch *dispatch = NULL;
    BOOL ret = FALSE;
    CLSID clsid;
    DISPID dispid;
    BSTR func = NULL, hostname = NULL, full_script = NULL;
    URL_COMPONENTSW uc;
    VARIANT args[2], result;
    DISPPARAMS params;
    HRESULT hr, init;

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    if (!WinHttpCrackUrl( url, 0, 0, &uc )) return FALSE;
    if (!(hostname = SysAllocStringLen( NULL, uc.dwHostNameLength + 1 ))) return FALSE;
    memcpy( hostname, uc.lpszHostName, uc.dwHostNameLength * sizeof(WCHAR) );
    hostname[uc.dwHostNameLength] = 0;

    init = CoInitialize( NULL );
    hr = CLSIDFromProgID( jscriptW, &clsid );
    if (hr != S_OK) goto done;

    hr = CoCreateInstance( &clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                           &IID_IActiveScript, (void **)&engine );
    if (hr != S_OK) goto done;

    hr = IActiveScript_QueryInterface( engine, &IID_IActiveScriptParse, (void **)&parser );
    if (hr != S_OK) goto done;

    hr = IActiveScriptParse64_InitNew( parser );
    if (hr != S_OK) goto done;

    hr = IActiveScript_SetScriptSite( engine, &script_site );
    if (hr != S_OK) goto done;

    hr = IActiveScript_AddNamedItem( engine, global_funcsW, SCRIPTITEM_GLOBALMEMBERS );
    if (hr != S_OK) goto done;

    if (!(full_script = include_pac_utils( script ))) goto done;

    hr = IActiveScriptParse64_ParseScriptText( parser, full_script, NULL, NULL, NULL, 0, 0, 0, NULL, NULL );
    if (hr != S_OK) goto done;

    hr = IActiveScript_SetScriptState( engine, SCRIPTSTATE_STARTED );
    if (hr != S_OK) goto done;

    hr = IActiveScript_GetScriptDispatch( engine, NULL, &dispatch );
    if (hr != S_OK) goto done;

    if (!(func = SysAllocString( findproxyW ))) goto done;
    hr = IDispatch_GetIDsOfNames( dispatch, &IID_NULL, &func, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
    if (hr != S_OK) goto done;

    V_VT( &args[0] ) = VT_BSTR;
    V_BSTR( &args[0] ) = hostname;
    V_VT( &args[1] ) = VT_BSTR;
    V_BSTR( &args[1] ) = SysAllocString( url );

    params.rgvarg = args;
    params.rgdispidNamedArgs = NULL;
    params.cArgs = 2;
    params.cNamedArgs = 0;
    hr = IDispatch_Invoke( dispatch, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
                           &params, &result, NULL, NULL );
    VariantClear( &args[1] );
    if (hr != S_OK)
    {
        WARN("script failed 0x%08x\n", hr);
        goto done;
    }
    ret = parse_script_result( result, info );

done:
    SysFreeString( full_script );
    SysFreeString( hostname );
    SysFreeString( func );
    if (dispatch) IDispatch_Release( dispatch );
    if (parser) IUnknown_Release( parser );
    if (engine) IActiveScript_Release( engine );
    if (SUCCEEDED( init )) CoUninitialize();
    if (!ret) set_last_error( ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT );
    return ret;
}

static BSTR download_script( const WCHAR *url )
{
    static const WCHAR typeW[] = {'*','/','*',0};
    static const WCHAR *acceptW[] = {typeW, NULL};
    HINTERNET ses, con = NULL, req = NULL;
    WCHAR *hostname;
    URL_COMPONENTSW uc;
    DWORD size = 4096, offset, to_read, bytes_read, flags = 0;
    char *tmp, *buffer = NULL;
    BSTR script = NULL;
    int len;

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    if (!WinHttpCrackUrl( url, 0, 0, &uc )) return NULL;
    if (!(hostname = heap_alloc( (uc.dwHostNameLength + 1) * sizeof(WCHAR) ))) return NULL;
    memcpy( hostname, uc.lpszHostName, uc.dwHostNameLength * sizeof(WCHAR) );
    hostname[uc.dwHostNameLength] = 0;

    if (!(ses = WinHttpOpen( NULL, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0 ))) goto done;
    if (!(con = WinHttpConnect( ses, hostname, uc.nPort, 0 ))) goto done;
    if (uc.nScheme == INTERNET_SCHEME_HTTPS) flags |= WINHTTP_FLAG_SECURE;
    if (!(req = WinHttpOpenRequest( con, NULL, uc.lpszUrlPath, NULL, NULL, acceptW, flags ))) goto done;
    if (!WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 )) goto done;
    if (!(WinHttpReceiveResponse( req, 0 ))) goto done;

    if (!(buffer = heap_alloc( size ))) goto done;
    to_read = size;
    offset = 0;
    for (;;)
    {
        if (!WinHttpReadData( req, buffer + offset, to_read, &bytes_read )) goto done;
        if (!bytes_read) break;
        to_read -= bytes_read;
        offset += bytes_read;
        if (!to_read)
        {
            to_read = size;
            size *= 2;
            if (!(tmp = heap_realloc( buffer, size ))) goto done;
            buffer = tmp;
        }
    }
    len = MultiByteToWideChar( CP_ACP, 0, buffer, offset, NULL, 0 );
    if (!(script = SysAllocStringLen( NULL, len ))) goto done;
    MultiByteToWideChar( CP_ACP, 0, buffer, offset, script, len );
    script[len] = 0;

done:
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
    heap_free( buffer );
    heap_free( hostname );
    if (!script) set_last_error( ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT );
    return script;
}

/***********************************************************************
 *          WinHttpGetProxyForUrl (winhttp.@)
 */
BOOL WINAPI WinHttpGetProxyForUrl( HINTERNET hsession, LPCWSTR url, WINHTTP_AUTOPROXY_OPTIONS *options,
                                   WINHTTP_PROXY_INFO *info )
{
    WCHAR *detected_pac_url = NULL;
    const WCHAR *pac_url;
    session_t *session;
    BSTR script;
    BOOL ret = FALSE;

    TRACE("%p, %s, %p, %p\n", hsession, debugstr_w(url), options, info);

    if (!(session = (session_t *)grab_object( hsession )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (session->hdr.type != WINHTTP_HANDLE_TYPE_SESSION)
    {
        release_object( &session->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }
    if (!url || !options || !info ||
        !(options->dwFlags & (WINHTTP_AUTOPROXY_AUTO_DETECT|WINHTTP_AUTOPROXY_CONFIG_URL)) ||
        ((options->dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT) && !options->dwAutoDetectFlags) ||
        ((options->dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT) &&
         (options->dwFlags & WINHTTP_AUTOPROXY_CONFIG_URL)))
    {
        release_object( &session->hdr );
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (options->dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT &&
        !WinHttpDetectAutoProxyConfigUrl( options->dwAutoDetectFlags, &detected_pac_url ))
    {
        set_last_error( ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR );
        goto done;
    }
    if (options->dwFlags & WINHTTP_AUTOPROXY_CONFIG_URL) pac_url = options->lpszAutoConfigUrl;
    else pac_url = detected_pac_url;

    if (!(script = download_script( pac_url ))) goto done;
    ret = run_script( script, url, info );
    SysFreeString( script );

done:
    GlobalFree( detected_pac_url );
    release_object( &session->hdr );
    return ret;
}

/***********************************************************************
 *          WinHttpSetDefaultProxyConfiguration (winhttp.@)
 */
BOOL WINAPI WinHttpSetDefaultProxyConfiguration( WINHTTP_PROXY_INFO *info )
{
    LONG l;
    HKEY key;
    BOOL ret = FALSE;
    const WCHAR *src;

    TRACE("%p\n", info);

    if (!info)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (info->dwAccessType)
    {
    case WINHTTP_ACCESS_TYPE_NO_PROXY:
        break;
    case WINHTTP_ACCESS_TYPE_NAMED_PROXY:
        if (!info->lpszProxy)
        {
            set_last_error( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        /* Only ASCII characters are allowed */
        for (src = info->lpszProxy; *src; src++)
            if (*src > 0x7f)
            {
                set_last_error( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
        if (info->lpszProxyBypass)
        {
            for (src = info->lpszProxyBypass; *src; src++)
                if (*src > 0x7f)
                {
                    set_last_error( ERROR_INVALID_PARAMETER );
                    return FALSE;
                }
        }
        break;
    default:
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    l = RegCreateKeyExW( HKEY_LOCAL_MACHINE, Connections, 0, NULL, 0,
        KEY_WRITE, NULL, &key, NULL );
    if (!l)
    {
        DWORD size = sizeof(struct connection_settings_header) + 2 * sizeof(DWORD);
        BYTE *buf;

        if (info->dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
        {
            size += strlenW( info->lpszProxy );
            if (info->lpszProxyBypass)
                size += strlenW( info->lpszProxyBypass );
        }
        buf = heap_alloc( size );
        if (buf)
        {
            struct connection_settings_header *hdr =
                (struct connection_settings_header *)buf;
            DWORD *len = (DWORD *)(hdr + 1);

            hdr->magic = WINHTTP_SETTINGS_MAGIC;
            hdr->unknown = 0;
            if (info->dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
            {
                BYTE *dst;

                hdr->flags = PROXY_TYPE_PROXY;
                *len++ = strlenW( info->lpszProxy );
                for (dst = (BYTE *)len, src = info->lpszProxy; *src;
                    src++, dst++)
                    *dst = *src;
                len = (DWORD *)dst;
                if (info->lpszProxyBypass)
                {
                    *len++ = strlenW( info->lpszProxyBypass );
                    for (dst = (BYTE *)len, src = info->lpszProxyBypass; *src;
                        src++, dst++)
                        *dst = *src;
                }
                else
                    *len++ = 0;
            }
            else
            {
                hdr->flags = PROXY_TYPE_DIRECT;
                *len++ = 0;
                *len++ = 0;
            }
            l = RegSetValueExW( key, WinHttpSettings, 0, REG_BINARY, buf, size );
            if (!l)
                ret = TRUE;
            heap_free( buf );
        }
        RegCloseKey( key );
    }
    return ret;
}

/***********************************************************************
 *          WinHttpSetStatusCallback (winhttp.@)
 */
WINHTTP_STATUS_CALLBACK WINAPI WinHttpSetStatusCallback( HINTERNET handle, WINHTTP_STATUS_CALLBACK callback,
                                                         DWORD flags, DWORD_PTR reserved )
{
    object_header_t *hdr;
    WINHTTP_STATUS_CALLBACK ret;

    TRACE("%p, %p, 0x%08x, 0x%lx\n", handle, callback, flags, reserved);

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return WINHTTP_INVALID_STATUS_CALLBACK;
    }
    ret = hdr->callback;
    hdr->callback = callback;
    hdr->notify_mask = flags;

    release_object( hdr );
    return ret;
}

/***********************************************************************
 *          WinHttpSetTimeouts (winhttp.@)
 */
BOOL WINAPI WinHttpSetTimeouts( HINTERNET handle, int resolve, int connect, int send, int receive )
{
    BOOL ret = TRUE;
    object_header_t *hdr;
    request_t *request;
    session_t *session;

    TRACE("%p, %d, %d, %d, %d\n", handle, resolve, connect, send, receive);

    if (resolve < -1 || connect < -1 || send < -1 || receive < -1)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    switch(hdr->type)
    {
        case WINHTTP_HANDLE_TYPE_REQUEST:
            request = (request_t *)hdr;
            request->connect_timeout = connect;

            if (resolve < 0) resolve = 0;
            request->resolve_timeout = resolve;

            if (send < 0) send = 0;
            request->send_timeout = send;

            if (receive < 0) receive = 0;
            request->recv_timeout = receive;

            if (netconn_connected( &request->netconn ))
            {
                if (netconn_set_timeout( &request->netconn, TRUE, send )) ret = FALSE;
                if (netconn_set_timeout( &request->netconn, FALSE, receive )) ret = FALSE;
            }

            release_object( &request->hdr );
            break;

        case WINHTTP_HANDLE_TYPE_SESSION:
            session = (session_t *)hdr;
            session->connect_timeout = connect;

            if (resolve < 0) resolve = 0;
            session->resolve_timeout = resolve;

            if (send < 0) send = 0;
            session->send_timeout = send;

            if (receive < 0) receive = 0;
            session->recv_timeout = receive;
            break;

        default:
            release_object( hdr );
            set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
            return FALSE;
    }
    return ret;
}

static const WCHAR wkday[7][4] =
    {{'S','u','n', 0}, {'M','o','n', 0}, {'T','u','e', 0}, {'W','e','d', 0},
     {'T','h','u', 0}, {'F','r','i', 0}, {'S','a','t', 0}};
static const WCHAR month[12][4] =
    {{'J','a','n', 0}, {'F','e','b', 0}, {'M','a','r', 0}, {'A','p','r', 0},
     {'M','a','y', 0}, {'J','u','n', 0}, {'J','u','l', 0}, {'A','u','g', 0},
     {'S','e','p', 0}, {'O','c','t', 0}, {'N','o','v', 0}, {'D','e','c', 0}};

/***********************************************************************
 *           WinHttpTimeFromSystemTime (WININET.@)
 */
BOOL WINAPI WinHttpTimeFromSystemTime( const SYSTEMTIME *time, LPWSTR string )
{
    static const WCHAR format[] =
        {'%','s',',',' ','%','0','2','d',' ','%','s',' ','%','4','d',' ','%','0',
         '2','d',':','%','0','2','d',':','%','0','2','d',' ','G','M','T', 0};

    TRACE("%p, %p\n", time, string);

    if (!time || !string) return FALSE;

    sprintfW( string, format,
              wkday[time->wDayOfWeek],
              time->wDay,
              month[time->wMonth - 1],
              time->wYear,
              time->wHour,
              time->wMinute,
              time->wSecond );

    return TRUE;
}

/***********************************************************************
 *           WinHttpTimeToSystemTime (WININET.@)
 */
BOOL WINAPI WinHttpTimeToSystemTime( LPCWSTR string, SYSTEMTIME *time )
{
    unsigned int i;
    const WCHAR *s = string;
    WCHAR *end;

    TRACE("%s, %p\n", debugstr_w(string), time);

    if (!string || !time) return FALSE;

    /* Windows does this too */
    GetSystemTime( time );

    /*  Convert an RFC1123 time such as 'Fri, 07 Jan 2005 12:06:35 GMT' into
     *  a SYSTEMTIME structure.
     */

    while (*s && !isalphaW( *s )) s++;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0') return TRUE;
    time->wDayOfWeek = 7;

    for (i = 0; i < 7; i++)
    {
        if (toupperW( wkday[i][0] ) == toupperW( s[0] ) &&
            toupperW( wkday[i][1] ) == toupperW( s[1] ) &&
            toupperW( wkday[i][2] ) == toupperW( s[2] ) )
        {
            time->wDayOfWeek = i;
            break;
        }
    }

    if (time->wDayOfWeek > 6) return TRUE;
    while (*s && !isdigitW( *s )) s++;
    time->wDay = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isalphaW( *s )) s++;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0') return TRUE;
    time->wMonth = 0;

    for (i = 0; i < 12; i++)
    {
        if (toupperW( month[i][0]) == toupperW( s[0] ) &&
            toupperW( month[i][1]) == toupperW( s[1] ) &&
            toupperW( month[i][2]) == toupperW( s[2] ) )
        {
            time->wMonth = i + 1;
            break;
        }
    }
    if (time->wMonth == 0) return TRUE;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wYear = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wHour = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wMinute = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wSecond = strtolW( s, &end, 10 );

    time->wMilliseconds = 0;
    return TRUE;
}
