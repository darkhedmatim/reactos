#ifndef _DBT_H
#define _DBT_H

#ifdef __cplusplus
extern "C" {
#endif
#define DBT_NO_DISK_SPACE	0x47
#define DBT_CONFIGMGPRIVATE	0x7FFF
#define DBT_DEVICEARRIVAL	0x8000
#define DBT_DEVICEQUERYREMOVE	0x8001
#define DBT_DEVICEQUERYREMOVEFAILED	0x8002
#define DBT_DEVICEREMOVEPENDING	0x8003
#define DBT_DEVICEREMOVECOMPLETE	0x8004
#define DBT_DEVICETYPESPECIFIC	0x8005
#define DBT_DEVTYP_OEM	0
#define DBT_DEVTYP_DEVNODE	1
#define DBT_DEVTYP_VOLUME	2
#define DBT_DEVTYP_PORT	3
#define DBT_DEVTYP_NET	4
#define DBT_DEVTYP_DEVICEINTERFACE  5
#define DBT_APPYBEGIN 0
#define DBT_APPYEND 1
#define DBT_DEVNODES_CHANGED 7
#define DBT_QUERYCHANGECONFIG 0x17
#define DBT_CONFIGCHANGED 0x18
#define DBT_CONFIGCHANGECANCELED 0x19
#define DBT_MONITORCHANGE 0x1B
#define DBT_SHELLLOGGEDON 32
#define DBT_CONFIGMGAPI32 34
#define DBT_VXDINITCOMPLETE 35
#define DBT_VOLLOCKQUERYLOCK 0x8041
#define DBT_VOLLOCKLOCKTAKEN 0x8042
#define DBT_VOLLOCKLOCKFAILED 0x8043
#define DBT_VOLLOCKQUERYUNLOCK 0x8044
#define DBT_VOLLOCKLOCKRELEASED 0x8045
#define DBT_VOLLOCKUNLOCKFAILED 0x8046
#define DBT_USERDEFINED 0xFFFF
#define DBTF_MEDIA	1
#define DBTF_NET	2

/* Also defined in winuser.h */
#define BSM_ALLCOMPONENTS	0
#define BSM_APPLICATIONS	8
#define BSM_ALLDESKTOPS		16
#define BSM_INSTALLABLEDRIVERS	4
#define BSM_NETDRIVER	2
#define BSM_VXDS	1
#define BSF_FLUSHDISK 0x00000004
#define BSF_FORCEIFHUNG 0x00000020
#define BSF_IGNORECURRENTTASK 0x00000002
#define BSF_NOHANG 0x00000008
#define BSF_NOTIMEOUTIFNOTHUNG 0x00000040
#define BSF_POSTMESSAGE 0x00000010
#define BSF_QUERY 0x00000001
#if (_WIN32_WINNT >= 0x0500)
#define BSF_ALLOWSFW 0x00000080
#define BSF_SENDNOTIFYMESSAGE 0x00000100
#endif /* (_WIN32_WINNT >= 0x0500) */
#if (_WIN32_WINNT >= 0x0501)
#define BSF_LUID 0x00000400
#define BSF_RETURNHDESK 0x00000200
#endif /* (_WIN32_WINNT >= 0x0501) */

#define BSF_MSGSRV32ISOK_BIT 31
#define BSF_MSGSRV32ISOK 0x80000000

typedef struct _DEV_BROADCAST_HDR {
	DWORD dbch_size;
	DWORD dbch_devicetype;
	DWORD dbch_reserved;
} DEV_BROADCAST_HDR,*PDEV_BROADCAST_HDR;
typedef struct _DEV_BROADCAST_OEM {
	DWORD dbco_size;
	DWORD dbco_devicetype;
	DWORD dbco_reserved;
	DWORD dbco_identifier;
	DWORD dbco_suppfunc;
} DEV_BROADCAST_OEM,*PDEV_BROADCAST_OEM;
typedef struct _DEV_BROADCAST_PORT_A {
	DWORD dbcp_size;
	DWORD dbcp_devicetype;
	DWORD dbcp_reserved;
	char dbcp_name[1];
} DEV_BROADCAST_PORT_A, *PDEV_BROADCAST_PORT_A;
typedef struct _DEV_BROADCAST_PORT_W {
	DWORD dbcp_size;
	DWORD dbcp_devicetype;
	DWORD dbcp_reserved;
	wchar_t dbcp_name[1];
} DEV_BROADCAST_PORT_W, *PDEV_BROADCAST_PORT_W;
typedef struct _DEV_BROADCAST_USERDEFINED {
	struct _DEV_BROADCAST_HDR dbud_dbh;
	char dbud_szName[1];
} DEV_BROADCAST_USERDEFINED;
typedef struct _DEV_BROADCAST_VOLUME {
	DWORD dbcv_size;
	DWORD dbcv_devicetype;
	DWORD dbcv_reserved;
	DWORD dbcv_unitmask;
	WORD dbcv_flags;
} DEV_BROADCAST_VOLUME,*PDEV_BROADCAST_VOLUME;
typedef struct _DEV_BROADCAST_DEVICEINTERFACE {
    DWORD dbcc_size;
    DWORD dbcc_devicetype;
    DWORD dbcc_reserved;
    GUID dbcc_classguid;
    TCHAR dbcc_name[1];
} DEV_BROADCAST_DEVICEINTERFACE, *PDEV_BROADCAST_DEVICEINTERFACE;

#ifdef UNICODE
typedef DEV_BROADCAST_PORT_W DEV_BROADCAST_PORT, *PDEV_BROADCAST_PORT;
#else
typedef DEV_BROADCAST_PORT_A DEV_BROADCAST_PORT, *PDEV_BROADCAST_PORT;
#endif

#ifdef __cplusplus
}
#endif
#endif
