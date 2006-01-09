#ifndef __CPL_RESOURCE_H
#define __CPL_RESOURCE_H

/* metrics */
#define PROPSHEETWIDTH  246
#define PROPSHEETHEIGHT 228
#define PROPSHEETPADDING        6
#define SYSTEM_COLUMN   (0 * PROPSHEETPADDING)
// this is not supported by the MS Resource compiler:
//#define LABELLINE(x)    0 //(((PROPSHEETPADDING + 2) * x) + (x + 2))

#define ICONSIZE        16

/* ids */

#define IDI_CPLSYSTEM	100
#define IDI_HORIZONTAL	101
#define IDI_VERTICAL	102
#define IDI_NETSTAT		103

#define IDD_PROPPAGENETWORK	100
#define IDD_CARDPROPERTIES  101
#define IDD_NETPROPERTIES	102
#define IDD_TCPIPPROPERTIES 103

#define IDS_CPLSYSTEMNAME	1001
#define IDS_CPLSYSTEMDESCRIPTION	2001
#define IDS_ERROR			3001
#define IDS_ENTER_VALID_IPADDRESS	3002
#define IDS_ENTER_VALID_SUBNET		3003
#define IDS_CANNOT_LOAD_CONFIG		3004
#define IDS_CANNOT_CREATE_PROPSHEET	3005
#define IDS_OUT_OF_MEMORY               3006
#define IDS_CANNOT_SAVE_CHANGES         3007


/* controls */
#define IDC_NETCARDLIST		100
#define IDC_ADD				101
#define IDC_REMOVE			102
#define IDC_PROPERTIES		103

#define IDC_NETCARDNAME		104
#define IDC_CONFIGURE		105
#define IDC_COMPONENTSLIST	106
#define IDC_INSTALL			107
#define IDC_UNINSTALL		108
//#define IDC_PROPERTIES		109
#define IDC_DESCRIPTION		110
#define IDC_SHOWTASKBAR		111
#define IDC_SEND			112
#define IDC_RECEIVED		113
#define IDC_ENDISABLE		114
#define IDC_USEDHCP			115
#define IDC_NODHCP			116
#define IDC_AUTODNS			117
#define IDC_FIXEDDNS		118
#define IDC_ADVANCED		119
#define IDC_IPADDR			120
#define IDC_SUBNETMASK		121
#define IDC_DEFGATEWAY		122
#define IDC_DNS1			123
#define IDC_DNS2			124
#endif /* __CPL_RESOURCE_H */

/* EOF */
