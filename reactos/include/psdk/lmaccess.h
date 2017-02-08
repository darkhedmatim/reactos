#ifndef _LMACCESS_H
#define _LMACCESS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <lmcons.h>
#define GROUP_SPECIALGRP_USERS L"USERS"
#define GROUP_SPECIALGRP_ADMINS L"ADMINS"
#define GROUP_SPECIALGRP_GUESTS L"GUESTS"
#define GROUP_SPECIALGRP_LOCAL L"LOCAL"
#define ACCESS_LETTERS "RWCXDAP "
#define NETLOGON_CONTROL_QUERY 1
#define NETLOGON_CONTROL_REPLICATE 2
#define NETLOGON_CONTROL_SYNCHRONIZE 3
#define NETLOGON_CONTROL_PDC_REPLICATE 4
#define NETLOGON_CONTROL_REDISCOVER 5
#define NETLOGON_CONTROL_TC_QUERY 6
#define NETLOGON_CONTROL_BACKUP_CHANGE_LOG 65532
#define NETLOGON_CONTROL_TRUNCATE_LOG 65533
#define NETLOGON_CONTROL_SET_DBFLAG 65534
#define NETLOGON_CONTROL_BREAKPOINT 65535
#define UF_SCRIPT 1
#define UF_ACCOUNTDISABLE 2
#define UF_HOMEDIR_REQUIRED 8
#define UF_LOCKOUT 16
#define UF_PASSWD_NOTREQD 32
#define UF_PASSWD_CANT_CHANGE 64
#define UF_TEMP_DUPLICATE_ACCOUNT 256
#define UF_NORMAL_ACCOUNT 512
#define UF_INTERDOMAIN_TRUST_ACCOUNT 2048
#define UF_WORKSTATION_TRUST_ACCOUNT 4096
#define UF_SERVER_TRUST_ACCOUNT 8192
#define UF_MNS_LOGON_ACCOUNT 131072
#define UF_MACHINE_ACCOUNT_MASK (UF_INTERDOMAIN_TRUST_ACCOUNT|UF_WORKSTATION_TRUST_ACCOUNT|UF_SERVER_TRUST_ACCOUNT)
#define UF_ACCOUNT_TYPE_MASK (UF_TEMP_DUPLICATE_ACCOUNT|UF_NORMAL_ACCOUNT|UF_INTERDOMAIN_TRUST_ACCOUNT|UF_WORKSTATION_TRUST_ACCOUNT|UF_SERVER_TRUST_ACCOUNT)
#define UF_DONT_EXPIRE_PASSWD 65536
#define UF_SETTABLE_BITS (UF_SCRIPT|UF_ACCOUNTDISABLE|UF_LOCKOUT|UF_HOMEDIR_REQUIRED|UF_PASSWD_NOTREQD|UF_PASSWD_CANT_CHANGE|UF_ACCOUNT_TYPE_MASK|UF_DONT_EXPIRE_PASSWD)
#define FILTER_TEMP_DUPLICATE_ACCOUNT 1
#define FILTER_NORMAL_ACCOUNT 2
#define FILTER_INTERDOMAIN_TRUST_ACCOUNT 8
#define FILTER_WORKSTATION_TRUST_ACCOUNT 16
#define FILTER_SERVER_TRUST_ACCOUNT 32
#define LG_INCLUDE_INDIRECT 1
#define AF_OP_PRINT 1
#define AF_OP_COMM 2
#define AF_OP_SERVER 4
#define AF_OP_ACCOUNTS 8
#define AF_SETTABLE_BITS (AF_OP_PRINT|AF_OP_COMM|AF_OP_SERVER|AF_OP_ACCOUNTS)
#define UAS_ROLE_STANDALONE 0
#define UAS_ROLE_MEMBER 1
#define UAS_ROLE_BACKUP 2
#define UAS_ROLE_PRIMARY 3
#define USER_NAME_PARMNUM 1
#define USER_PASSWORD_PARMNUM 3
#define USER_PASSWORD_AGE_PARMNUM 4
#define USER_PRIV_PARMNUM 5
#define USER_HOME_DIR_PARMNUM 6
#define USER_COMMENT_PARMNUM 7
#define USER_FLAGS_PARMNUM 8
#define USER_SCRIPT_PATH_PARMNUM 9
#define USER_AUTH_FLAGS_PARMNUM 10
#define USER_FULL_NAME_PARMNUM 11
#define USER_USR_COMMENT_PARMNUM 12
#define USER_PARMS_PARMNUM 13
#define USER_WORKSTATIONS_PARMNUM 14
#define USER_LAST_LOGON_PARMNUM 15
#define USER_LAST_LOGOFF_PARMNUM 16
#define USER_ACCT_EXPIRES_PARMNUM 17
#define USER_MAX_STORAGE_PARMNUM 18
#define USER_UNITS_PER_WEEK_PARMNUM 19
#define USER_LOGON_HOURS_PARMNUM 20
#define USER_PAD_PW_COUNT_PARMNUM 21
#define USER_NUM_LOGONS_PARMNUM 22
#define USER_LOGON_SERVER_PARMNUM 23
#define USER_COUNTRY_CODE_PARMNUM 24
#define USER_CODE_PAGE_PARMNUM 25
#define USER_PRIMARY_GROUP_PARMNUM 51
#define USER_PROFILE 52
#define USER_PROFILE_PARMNUM 52
#define USER_HOME_DIR_DRIVE_PARMNUM 53
#define USER_NAME_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_NAME_PARMNUM)
#define USER_PASSWORD_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_PASSWORD_PARMNUM)
#define USER_PASSWORD_AGE_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_PASSWORD_AGE_PARMNUM)
#define USER_PRIV_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_PRIV_PARMNUM)
#define USER_HOME_DIR_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_HOME_DIR_PARMNUM)
#define USER_COMMENT_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_COMMENT_PARMNUM)
#define USER_FLAGS_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_FLAGS_PARMNUM)
#define USER_SCRIPT_PATH_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_SCRIPT_PATH_PARMNUM)
#define USER_AUTH_FLAGS_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_AUTH_FLAGS_PARMNUM)
#define USER_FULL_NAME_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_FULL_NAME_PARMNUM)
#define USER_USR_COMMENT_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_USR_COMMENT_PARMNUM)
#define USER_PARMS_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_PARMS_PARMNUM)
#define USER_WORKSTATIONS_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_WORKSTATIONS_PARMNUM)
#define USER_LAST_LOGON_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_LAST_LOGON_PARMNUM)
#define USER_LAST_LOGOFF_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_LAST_LOGOFF_PARMNUM)
#define USER_ACCT_EXPIRES_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_ACCT_EXPIRES_PARMNUM)
#define USER_MAX_STORAGE_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_MAX_STORAGE_PARMNUM)
#define USER_UNITS_PER_WEEK_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_UNITS_PER_WEEK_PARMNUM)
#define USER_LOGON_HOURS_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_LOGON_HOURS_PARMNUM)
#define USER_PAD_PW_COUNT_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_PAD_PW_COUNT_PARMNUM)
#define USER_NUM_LOGONS_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_NUM_LOGONS_PARMNUM)
#define USER_LOGON_SERVER_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_LOGON_SERVER_PARMNUM)
#define USER_COUNTRY_CODE_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_COUNTRY_CODE_PARMNUM)
#define USER_CODE_PAGE_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_CODE_PAGE_PARMNUM)
#define USER_PRIMARY_GROUP_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_PRIMARY_GROUP_PARMNUM)
#define USER_POSIX_ID_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_POSIX_ID_PARMNUM)
#define USER_HOME_DIR_DRIVE_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+USER_HOME_DIR_DRIVE_PARMNUM)
#define NULL_USERSETINFO_PASSWD " "
#define TIMEQ_FOREVER ((ULONG)-1)
#define USER_MAXSTORAGE_UNLIMITED ((ULONG)-1)
#define USER_NO_LOGOFF ((ULONG)-1L)
#define UNITS_PER_DAY 24
#define UNITS_PER_WEEK  168
#define USER_PRIV_MASK 3
#define USER_PRIV_GUEST 0
#define USER_PRIV_USER 1
#define USER_PRIV_ADMIN 2
#define MAX_PASSWD_LEN PWLEN
#define DEF_MIN_PWLEN 6
#define DEF_PWUNIQUENESS 5
#define DEF_MAX_PWHIST 8
#define DEF_MAX_PWAGE TIMEQ_FOREVER
#define DEF_MIN_PWAGE 0
#define DEF_FORCE_LOGOFF (ULONG)0xffffffff
#define DEF_MAX_BADPW 0
#define ONE_DAY 86400
#define VALIDATED_LOGON 0
#define PASSWORD_EXPIRED 2
#define NON_VALIDATED_LOGON 3
#define VALID_LOGOFF 1
#define MODALS_MIN_PASSWD_LEN_PARMNUM 1
#define MODALS_MAX_PASSWD_AGE_PARMNUM 2
#define MODALS_MIN_PASSWD_AGE_PARMNUM 3
#define MODALS_FORCE_LOGOFF_PARMNUM 4
#define MODALS_PASSWD_HIST_LEN_PARMNUM 5
#define MODALS_ROLE_PARMNUM 6
#define MODALS_PRIMARY_PARMNUM 7
#define MODALS_DOMAIN_NAME_PARMNUM 8
#define MODALS_DOMAIN_ID_PARMNUM 9
#define MODALS_LOCKOUT_DURATION_PARMNUM 10
#define MODALS_LOCKOUT_OBSERVATION_WINDOW_PARMNUM 11
#define MODALS_LOCKOUT_THRESHOLD_PARMNUM 12
#define MODALS_MIN_PASSWD_LEN_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+MODALS_MIN_PASSWD_LEN_PARMNUM)
#define MODALS_MAX_PASSWD_AGE_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+MODALS_MAX_PASSWD_AGE_PARMNUM)
#define MODALS_MIN_PASSWD_AGE_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+MODALS_MIN_PASSWD_AGE_PARMNUM)
#define MODALS_FORCE_LOGOFF_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+MODALS_FORCE_LOGOFF_PARMNUM)
#define MODALS_PASSWD_HIST_LEN_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+MODALS_PASSWD_HIST_LEN_PARMNUM)
#define MODALS_ROLE_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+MODALS_ROLE_PARMNUM)
#define MODALS_PRIMARY_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+MODALS_PRIMARY_PARMNUM)
#define MODALS_DOMAIN_NAME_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+MODALS_DOMAIN_NAME_PARMNUM)
#define MODALS_DOMAIN_ID_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+MODALS_DOMAIN_ID_PARMNUM)
#define GROUPIDMASK 0x8000
#define GROUP_ALL_PARMNUM 0
#define GROUP_NAME_PARMNUM 1
#define GROUP_COMMENT_PARMNUM 2
#define GROUP_ATTRIBUTES_PARMNUM 3
#define GROUP_ALL_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+GROUP_ALL_PARMNUM)
#define GROUP_NAME_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+GROUP_NAME_PARMNUM)
#define GROUP_COMMENT_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+GROUP_COMMENT_PARMNUM)
#define GROUP_ATTRIBUTES_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+GROUP_ATTRIBUTES_PARMNUM)
#define GROUP_POSIX_ID_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+GROUP_POSIX_ID_PARMNUM)
#define LOCALGROUP_NAME_PARMNUM 1
#define LOCALGROUP_COMMENT_PARMNUM 2
#define MAXPERMENTRIES 64
#define ACCESS_NONE 0
#define ACCESS_ALL (ACCESS_READ|ACCESS_WRITE|ACCESS_CREATE|ACCESS_EXEC|ACCESS_DELETE|ACCESS_ATRIB|ACCESS_PERM)
#define ACCESS_READ 1
#define ACCESS_WRITE 2
#define ACCESS_CREATE 4
#define ACCESS_EXEC 8
#define ACCESS_DELETE 16
#define ACCESS_ATRIB 32
#define ACCESS_PERM 64
#define ACCESS_GROUP 0x8000
#define ACCESS_AUDIT 1
#define ACCESS_SUCCESS_OPEN 16
#define ACCESS_SUCCESS_WRITE 32
#define ACCESS_SUCCESS_DELETE 64
#define ACCESS_SUCCESS_ACL 128
#define ACCESS_SUCCESS_MASK 240
#define ACCESS_FAIL_OPEN 256
#define ACCESS_FAIL_WRITE 512
#define ACCESS_FAIL_DELETE 1024
#define ACCESS_FAIL_ACL 2048
#define ACCESS_FAIL_MASK 3840
#define ACCESS_FAIL_SHIFT 4
#define ACCESS_RESOURCE_NAME_PARMNUM 1
#define ACCESS_ATTR_PARMNUM 2
#define ACCESS_COUNT_PARMNUM 3
#define ACCESS_ACCESS_LIST_PARMNUM 4
#define ACCESS_RESOURCE_NAME_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+ACCESS_RESOURCE_NAME_PARMNUM)
#define ACCESS_ATTR_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+ACCESS_ATTR_PARMNUM)
#define ACCESS_COUNT_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+ACCESS_COUNT_PARMNUM)
#define ACCESS_ACCESS_LIST_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+ACCESS_ACCESS_LIST_PARMNUM)
#define NETLOGON_REPLICATION_NEEDED 1
#define NETLOGON_REPLICATION_IN_PROGRESS 2
#define NETLOGON_FULL_SYNC_REPLICATION 4
#define NETLOGON_REDO_NEEDED 8

typedef struct _USER_INFO_0 { LPWSTR usri0_name; }USER_INFO_0,*PUSER_INFO_0,*LPUSER_INFO_0;
typedef struct _USER_INFO_1 {
	LPWSTR usri1_name;
	LPWSTR usri1_password;
	DWORD usri1_password_age;
	DWORD usri1_priv;
	LPWSTR usri1_home_dir;
	LPWSTR usri1_comment;
	DWORD usri1_flags;
	LPWSTR usri1_script_path;
}USER_INFO_1,*PUSER_INFO_1,*LPUSER_INFO_1;
typedef struct _USER_INFO_2 {
	LPWSTR usri2_name;
	LPWSTR usri2_password;
	DWORD usri2_password_age;
	DWORD usri2_priv;
	LPWSTR usri2_home_dir;
	LPWSTR usri2_comment;
	DWORD usri2_flags;
	LPWSTR usri2_script_path;
	DWORD usri2_auth_flags;
	LPWSTR usri2_full_name;
	LPWSTR usri2_usr_comment;
	LPWSTR usri2_parms;
	LPWSTR usri2_workstations;
	DWORD usri2_last_logon;
	DWORD usri2_last_logoff;
	DWORD usri2_acct_expires;
	DWORD usri2_max_storage;
	DWORD usri2_units_per_week;
	PBYTE usri2_logon_hours;
	DWORD usri2_bad_pw_count;
	DWORD usri2_num_logons;
	LPWSTR usri2_logon_server;
	DWORD usri2_country_code;
	DWORD usri2_code_page;
}USER_INFO_2,*PUSER_INFO_2,*LPUSER_INFO_2;
typedef struct _USER_INFO_3 {
	LPWSTR usri3_name;
	LPWSTR usri3_password;
	DWORD usri3_password_age;
	DWORD usri3_priv;
	LPWSTR usri3_home_dir;
	LPWSTR usri3_comment;
	DWORD usri3_flags;
	LPWSTR usri3_script_path;
	DWORD usri3_auth_flags;
	LPWSTR usri3_full_name;
	LPWSTR usri3_usr_comment;
	LPWSTR usri3_parms;
	LPWSTR usri3_workstations;
	DWORD usri3_last_logon;
	DWORD usri3_last_logoff;
	DWORD usri3_acct_expires;
	DWORD usri3_max_storage;
	DWORD usri3_units_per_week;
	PBYTE usri3_logon_hours;
	DWORD usri3_bad_pw_count;
	DWORD usri3_num_logons;
	LPWSTR usri3_logon_server;
	DWORD usri3_country_code;
	DWORD usri3_code_page;
	DWORD usri3_user_id;
	DWORD usri3_primary_group_id;
	LPWSTR usri3_profile;
	LPWSTR usri3_home_dir_drive;
	DWORD usri3_password_expired;
}USER_INFO_3,*PUSER_INFO_3,*LPUSER_INFO_3;
typedef struct _USER_INFO_4 {
    LPWSTR usri4_name;
    LPWSTR usri4_password;
    DWORD  usri4_password_age;
    DWORD  usri4_priv;
    LPWSTR usri4_home_dir;
    LPWSTR usri4_comment;
    DWORD  usri4_flags;
    LPWSTR usri4_script_path;
    DWORD  usri4_auth_flags;
    LPWSTR usri4_full_name;
    LPWSTR usri4_usr_comment;
    LPWSTR usri4_parms;
    LPWSTR usri4_workstations;
    DWORD  usri4_last_logon;
    DWORD  usri4_last_logoff;
    DWORD  usri4_acct_expires;
    DWORD  usri4_max_storage;
    DWORD  usri4_units_per_week;
    PBYTE  usri4_logon_hours;
    DWORD  usri4_bad_pw_count;
    DWORD  usri4_num_logons;
    LPWSTR usri4_logon_server;
    DWORD  usri4_country_code;
    DWORD  usri4_code_page;
    PSID   usri4_user_sid;
    DWORD  usri4_primary_group_id;
    LPWSTR usri4_profile;
    LPWSTR usri4_home_dir_drive;
    DWORD  usri4_password_expired;
} USER_INFO_4, *PUSER_INFO_4, *LPUSER_INFO_4;
typedef struct _USER_INFO_10 {
	LPWSTR usri10_name;
	LPWSTR usri10_comment;
	LPWSTR usri10_usr_comment;
	LPWSTR usri10_full_name;
}USER_INFO_10,*PUSER_INFO_10,*LPUSER_INFO_10;
typedef struct _USER_INFO_11 {
	LPWSTR usri11_name;
	LPWSTR usri11_comment;
	LPWSTR usri11_usr_comment;
	LPWSTR usri11_full_name;
	DWORD usri11_priv;
	DWORD usri11_auth_flags;
	DWORD usri11_password_age;
	LPWSTR usri11_home_dir;
	LPWSTR usri11_parms;
	DWORD usri11_last_logon;
	DWORD usri11_last_logoff;
	DWORD usri11_bad_pw_count;
	DWORD usri11_num_logons;
	LPWSTR usri11_logon_server;
	DWORD usri11_country_code;
	LPWSTR usri11_workstations;
	DWORD usri11_max_storage;
	DWORD usri11_units_per_week;
	PBYTE usri11_logon_hours;
	DWORD usri11_code_page;
}USER_INFO_11,*PUSER_INFO_11,*LPUSER_INFO_11;
typedef struct _USER_INFO_20 {
	LPWSTR usri20_name;
	LPWSTR usri20_full_name;
	LPWSTR usri20_comment;
	DWORD usri20_flags;
	DWORD usri20_user_id;
}USER_INFO_20,*PUSER_INFO_20,*LPUSER_INFO_20;
typedef struct _USER_INFO_21 {
	BYTE usri21_password[ENCRYPTED_PWLEN];
}USER_INFO_21,*PUSER_INFO_21,*LPUSER_INFO_21;
typedef struct _USER_INFO_22 {
	LPWSTR usri22_name;
	BYTE usri22_password[ENCRYPTED_PWLEN];
	DWORD usri22_password_age;
	DWORD usri22_priv;
	LPWSTR usri22_home_dir;
	LPWSTR usri22_comment;
	DWORD usri22_flags;
	LPWSTR usri22_script_path;
	DWORD usri22_auth_flags;
	LPWSTR usri22_full_name;
	LPWSTR usri22_usr_comment;
	LPWSTR usri22_parms;
	LPWSTR usri22_workstations;
	DWORD usri22_last_logon;
	DWORD usri22_last_logoff;
	DWORD usri22_acct_expires;
	DWORD usri22_max_storage;
	DWORD usri22_units_per_week;
	PBYTE usri22_logon_hours;
	DWORD usri22_bad_pw_count;
	DWORD usri22_num_logons;
	LPWSTR usri22_logon_server;
	DWORD usri22_country_code;
	DWORD usri22_code_page;
}USER_INFO_22,*PUSER_INFO_22,*LPUSER_INFO_22;
typedef struct _USER_INFO_23 {
    LPWSTR usri23_name;
    LPWSTR usri23_full_name;
    LPWSTR usri23_comment;
    DWORD  usri23_flags;
    PSID   usri23_user_sid;
} USER_INFO_23, *PUSER_INFO_23, *LPUSER_INFO_23;
typedef struct _USER_INFO_1003 {
	LPWSTR usri1003_password;
} USER_INFO_1003,*PUSER_INFO_1003,*LPUSER_INFO_1003;
typedef struct _USER_INFO_1005 {
	DWORD usri1005_priv;
} USER_INFO_1005,*PUSER_INFO_1005,*LPUSER_INFO_1005;
typedef struct _USER_INFO_1006 {
	LPWSTR usri1006_home_dir;
} USER_INFO_1006,*PUSER_INFO_1006,*LPUSER_INFO_1006;
typedef struct _USER_INFO_1007 {
	LPWSTR usri1007_comment;
} USER_INFO_1007,*PUSER_INFO_1007,*LPUSER_INFO_1007;
typedef struct _USER_INFO_1008 {
	DWORD usri1008_flags;
} USER_INFO_1008,*PUSER_INFO_1008,*LPUSER_INFO_1008;
typedef struct _USER_INFO_1009 {
	LPWSTR usri1009_script_path;
} USER_INFO_1009,*PUSER_INFO_1009,*LPUSER_INFO_1009;
typedef struct _USER_INFO_1010 {
	DWORD usri1010_auth_flags;
} USER_INFO_1010,*PUSER_INFO_1010,*LPUSER_INFO_1010;
typedef struct _USER_INFO_1011 {
	LPWSTR usri1011_full_name;
} USER_INFO_1011,*PUSER_INFO_1011,*LPUSER_INFO_1011;
typedef struct _USER_INFO_1012 {
	LPWSTR usri1012_usr_comment;
} USER_INFO_1012,*PUSER_INFO_1012,*LPUSER_INFO_1012;
typedef struct _USER_INFO_1013 {
	LPWSTR usri1013_parms;
} USER_INFO_1013,*PUSER_INFO_1013,*LPUSER_INFO_1013;
typedef struct _USER_INFO_1014 {
	LPWSTR usri1014_workstations;
} USER_INFO_1014,*PUSER_INFO_1014,*LPUSER_INFO_1014;
typedef struct _USER_INFO_1017 {
	DWORD usri1017_acct_expires;
} USER_INFO_1017,*PUSER_INFO_1017,*LPUSER_INFO_1017;
typedef struct _USER_INFO_1018 {
	DWORD usri1018_max_storage;
} USER_INFO_1018,*PUSER_INFO_1018,*LPUSER_INFO_1018;
typedef struct _USER_INFO_1020 {
	DWORD usri1020_units_per_week;
	PBYTE usri1020_logon_hours;
} USER_INFO_1020,*PUSER_INFO_1020,*LPUSER_INFO_1020;
typedef struct _USER_INFO_1023 {
	LPWSTR usri1023_logon_server;
} USER_INFO_1023,*PUSER_INFO_1023,*LPUSER_INFO_1023;
typedef struct _USER_INFO_1024 {
	DWORD usri1024_country_code;
} USER_INFO_1024,*PUSER_INFO_1024,*LPUSER_INFO_1024;
typedef struct _USER_INFO_1025 {
	DWORD usri1025_code_page;
} USER_INFO_1025,*PUSER_INFO_1025,*LPUSER_INFO_1025;
typedef struct _USER_INFO_1051 {
	DWORD usri1051_primary_group_id;
} USER_INFO_1051,*PUSER_INFO_1051,*LPUSER_INFO_1051;
typedef struct _USER_INFO_1052 {
	LPWSTR usri1052_profile;
} USER_INFO_1052,*PUSER_INFO_1052,*LPUSER_INFO_1052;
typedef struct _USER_INFO_1053 {
	LPWSTR usri1053_home_dir_drive;
} USER_INFO_1053,*PUSER_INFO_1053,*LPUSER_INFO_1053;
typedef struct _USER_MODALS_INFO_0 {
	DWORD usrmod0_min_passwd_len;
	DWORD usrmod0_max_passwd_age;
	DWORD usrmod0_min_passwd_age;
	DWORD usrmod0_force_logoff;
	DWORD usrmod0_password_hist_len;
}USER_MODALS_INFO_0,*PUSER_MODALS_INFO_0,*LPUSER_MODALS_INFO_0;
typedef struct _USER_MODALS_INFO_1 {
	DWORD usrmod1_role;
	LPWSTR usrmod1_primary;
}USER_MODALS_INFO_1,*PUSER_MODALS_INFO_1,*LPUSER_MODALS_INFO_1;
typedef struct _USER_MODALS_INFO_2 {
	LPWSTR usrmod2_domain_name;
	PSID usrmod2_domain_id;
}USER_MODALS_INFO_2,*PUSER_MODALS_INFO_2,*LPUSER_MODALS_INFO_2;
typedef struct _USER_MODALS_INFO_3 {
	DWORD usrmod3_lockout_duration;
	DWORD usrmod3_lockout_observation_window;
	DWORD usrmod3_lockout_threshold;
}USER_MODALS_INFO_3,*PUSER_MODALS_INFO_3,*LPUSER_MODALS_INFO_3;
typedef struct _USER_MODALS_INFO_1001 {
	DWORD usrmod1001_min_passwd_len;
} USER_MODALS_INFO_1001,*PUSER_MODALS_INFO_1001,*LPUSER_MODALS_INFO_1001;
typedef struct _USER_MODALS_INFO_1002 {
	DWORD usrmod1002_max_passwd_age;
} USER_MODALS_INFO_1002,*PUSER_MODALS_INFO_1002,*LPUSER_MODALS_INFO_1002;
typedef struct _USER_MODALS_INFO_1003 {
	DWORD usrmod1003_min_passwd_age;
} USER_MODALS_INFO_1003,*PUSER_MODALS_INFO_1003,*LPUSER_MODALS_INFO_1003;
typedef struct _USER_MODALS_INFO_1004 {
	DWORD usrmod1004_force_logoff;
} USER_MODALS_INFO_1004,*PUSER_MODALS_INFO_1004,*LPUSER_MODALS_INFO_1004;
typedef struct _USER_MODALS_INFO_1005 {
	DWORD usrmod1005_password_hist_len;
} USER_MODALS_INFO_1005,*PUSER_MODALS_INFO_1005,*LPUSER_MODALS_INFO_1005;
typedef struct _USER_MODALS_INFO_1006 {
	DWORD usrmod1006_role;
} USER_MODALS_INFO_1006,*PUSER_MODALS_INFO_1006,*LPUSER_MODALS_INFO_1006;
typedef struct _USER_MODALS_INFO_1007 {
	LPWSTR usrmod1007_primary;
} USER_MODALS_INFO_1007,*PUSER_MODALS_INFO_1007,*LPUSER_MODALS_INFO_1007;
typedef struct _GROUP_INFO_0 {
	LPWSTR grpi0_name;
}GROUP_INFO_0,*PGROUP_INFO_0,*LPGROUP_INFO_0;
typedef struct _GROUP_INFO_1 {
	LPWSTR grpi1_name;
	LPWSTR grpi1_comment;
}GROUP_INFO_1,*PGROUP_INFO_1,*LPGROUP_INFO_1;
typedef struct _GROUP_INFO_2 {
	LPWSTR grpi2_name;
	LPWSTR grpi2_comment;
	DWORD grpi2_group_id;
	DWORD grpi2_attributes;
}GROUP_INFO_2,*PGROUP_INFO_2;
typedef struct _GROUP_INFO_1002 {
	LPWSTR grpi1002_comment;
} GROUP_INFO_1002,*PGROUP_INFO_1002,*LPGROUP_INFO_1002;
typedef struct _GROUP_INFO_1005 {
	DWORD grpi1005_attributes;
} GROUP_INFO_1005,*PGROUP_INFO_1005,*LPGROUP_INFO_1005;
typedef struct _GROUP_USERS_INFO_0 {
	LPWSTR grui0_name;
} GROUP_USERS_INFO_0,*PGROUP_USERS_INFO_0,*LPGROUP_USERS_INFO_0;
typedef struct _GROUP_USERS_INFO_1 {
	LPWSTR grui1_name;
	DWORD grui1_attributes;
} GROUP_USERS_INFO_1,*PGROUP_USERS_INFO_1,*LPGROUP_USERS_INFO_1;
typedef struct _LOCALGROUP_INFO_0 {
	LPWSTR lgrpi0_name;
}LOCALGROUP_INFO_0,*PLOCALGROUP_INFO_0,*LPLOCALGROUP_INFO_0;
typedef struct _LOCALGROUP_INFO_1 {
	LPWSTR lgrpi1_name;
	LPWSTR lgrpi1_comment;
}LOCALGROUP_INFO_1,*PLOCALGROUP_INFO_1,*LPLOCALGROUP_INFO_1;
typedef struct _LOCALGROUP_INFO_1002 {
	LPWSTR lgrpi1002_comment;
}LOCALGROUP_INFO_1002,*PLOCALGROUP_INFO_1002,*LPLOCALGROUP_INFO_1002;
typedef struct _LOCALGROUP_MEMBERS_INFO_0 {
	PSID lgrmi0_sid;
} LOCALGROUP_MEMBERS_INFO_0,*PLOCALGROUP_MEMBERS_INFO_0,*LPLOCALGROUP_MEMBERS_INFO_0;
typedef struct _LOCALGROUP_MEMBERS_INFO_1 {
	PSID lgrmi1_sid;
	SID_NAME_USE lgrmi1_sidusage;
	LPWSTR lgrmi1_name;
} LOCALGROUP_MEMBERS_INFO_1,*PLOCALGROUP_MEMBERS_INFO_1,*LPLOCALGROUP_MEMBERS_INFO_1;
typedef struct _LOCALGROUP_MEMBERS_INFO_2 {
	PSID lgrmi2_sid;
	SID_NAME_USE lgrmi2_sidusage;
	LPWSTR lgrmi2_domainandname;
} LOCALGROUP_MEMBERS_INFO_2,*PLOCALGROUP_MEMBERS_INFO_2,*LPLOCALGROUP_MEMBERS_INFO_2;
typedef struct _LOCALGROUP_MEMBERS_INFO_3 {
	LPWSTR lgrmi3_domainandname;
} LOCALGROUP_MEMBERS_INFO_3,*PLOCALGROUP_MEMBERS_INFO_3,
*LPLOCALGROUP_MEMBERS_INFO_3;
typedef struct _LOCALGROUP_USERS_INFO_0 {
	LPWSTR lgrui0_name;
} LOCALGROUP_USERS_INFO_0,*PLOCALGROUP_USERS_INFO_0,*LPLOCALGROUP_USERS_INFO_0;
typedef struct _NET_DISPLAY_USER {
	LPWSTR usri1_name;
	LPWSTR usri1_comment;
	DWORD usri1_flags;
	LPWSTR usri1_full_name;
	DWORD usri1_user_id;
	DWORD usri1_next_index;
} NET_DISPLAY_USER,*PNET_DISPLAY_USER;
typedef struct _NET_DISPLAY_MACHINE {
	LPWSTR usri2_name;
	LPWSTR usri2_comment;
	DWORD usri2_flags;
	DWORD usri2_user_id;
	DWORD usri2_next_index;
} NET_DISPLAY_MACHINE,*PNET_DISPLAY_MACHINE;
typedef struct _NET_DISPLAY_GROUP {
	LPWSTR grpi3_name;
	LPWSTR grpi3_comment;
	DWORD grpi3_group_id;
	DWORD grpi3_attributes;
	DWORD grpi3_next_index;
} NET_DISPLAY_GROUP,*PNET_DISPLAY_GROUP;
typedef struct _ACCESS_INFO_0 {
	LPTSTR acc0_resource_name;
}ACCESS_INFO_0,*PACCESS_INFO_0,*LPACCESS_INFO_0;
typedef struct _ACCESS_INFO_1 {
	LPTSTR acc1_resource_name;
	DWORD acc1_attr;
	DWORD acc1_count;
}ACCESS_INFO_1,*PACCESS_INFO_1,*LPACCESS_INFO_1;
typedef struct _ACCESS_INFO_1002 {
	DWORD acc1002_attr;
} ACCESS_INFO_1002,*PACCESS_INFO_1002,*LPACCESS_INFO_1002;
typedef struct _ACCESS_LIST {
	LPTSTR acl_ugname;
	DWORD acl_access;
}ACCESS_LIST,*PACCESS_LIST,*LPACCESS_LIST;
typedef struct _NETLOGON_INFO_1 {
	DWORD netlog1_flags;
	NET_API_STATUS netlog1_pdc_connection_status;
} NETLOGON_INFO_1,*PNETLOGON_INFO_1;
typedef struct _NETLOGON_INFO_2 {
	DWORD netlog2_flags;
	NET_API_STATUS netlog2_pdc_connection_status;
	LPWSTR netlog2_trusted_dc_name;
	NET_API_STATUS netlog2_tc_connection_status;
} NETLOGON_INFO_2,*PNETLOGON_INFO_2;
typedef struct _NETLOGON_INFO_3 {
	DWORD netlog3_flags;
	DWORD netlog3_logon_attempts;
	DWORD netlog3_reserved1;
	DWORD netlog3_reserved2;
	DWORD netlog3_reserved3;
	DWORD netlog3_reserved4;
	DWORD netlog3_reserved5;
} NETLOGON_INFO_3,*PNETLOGON_INFO_3;

#ifndef _NTDEF_
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
#endif

#if 0
/* MS has these defined, but the RxNetAccessXX functions aren't documented nor do
   they exist in any headers */
#define NetAccessAdd RxNetAccessAdd
#define NetAccessEnum RxNetAccessEnum
#define NetAccessGetInfo RxNetAccessGetInfo
#define NetAccessSetInfo RxNetAccessSetInfo
#define NetAccessDel RxNetAccessDel
#define NetAccessGetUserPerms RxNetAccessGetUserPerms
/* These are obsolete */
NET_API_STATUS WINAPI NetAccessAdd(LPCWSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetAccessEnum(LPCWSTR,LPCWSTR,DWORD,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI NetAccessGetInfo(LPCWSTR,LPCWSTR,DWORD,PBYTE*);
NET_API_STATUS WINAPI NetAccessSetInfo(LPCWSTR,LPCWSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetAccessDel(LPCWSTR,LPCWSTR);
NET_API_STATUS WINAPI NetAccessGetUserPerms(LPCWSTR,LPCWSTR,LPCWSTR,PDWORD);
#endif

NET_API_STATUS WINAPI NetUserAdd(LPCWSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetUserEnum(LPCWSTR,DWORD,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI NetUserGetInfo(LPCWSTR,LPCWSTR,DWORD,PBYTE*);
NET_API_STATUS WINAPI NetUserSetInfo(LPCWSTR,LPCWSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetUserDel(LPCWSTR,LPCWSTR);
NET_API_STATUS WINAPI NetUserGetGroups(LPCWSTR,LPCWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI NetUserSetGroups(LPCWSTR,LPCWSTR,DWORD,PBYTE,DWORD);
NET_API_STATUS WINAPI NetUserGetLocalGroups(LPCWSTR,LPCWSTR,DWORD,DWORD,PBYTE*,DWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI NetUserModalsGet(LPCWSTR,DWORD,PBYTE*);
NET_API_STATUS WINAPI NetUserModalsSet(LPCWSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetUserChangePassword(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);
NET_API_STATUS WINAPI NetGroupAdd(LPCWSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetGroupAddUser(LPCWSTR,LPCWSTR,LPCWSTR);
NET_API_STATUS WINAPI NetGroupEnum(LPCWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD_PTR);
NET_API_STATUS WINAPI NetGroupGetInfo(LPCWSTR,LPCWSTR,DWORD,PBYTE*);
NET_API_STATUS WINAPI NetGroupSetInfo(LPCWSTR,LPCWSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetGroupDel(LPCWSTR,LPCWSTR);
NET_API_STATUS WINAPI NetGroupDelUser(LPCWSTR,LPCWSTR,LPCWSTR);
NET_API_STATUS WINAPI NetGroupGetUsers(LPCWSTR,LPCWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD_PTR);
NET_API_STATUS WINAPI NetGroupSetUsers(LPCWSTR,LPCWSTR,DWORD,PBYTE,DWORD);
NET_API_STATUS WINAPI NetLocalGroupAdd(LPCWSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetLocalGroupAddMember(LPCWSTR,LPCWSTR,PSID);
NET_API_STATUS WINAPI NetLocalGroupEnum(LPCWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD_PTR);
NET_API_STATUS WINAPI NetLocalGroupGetInfo(LPCWSTR,LPCWSTR,DWORD,PBYTE*);
NET_API_STATUS WINAPI NetLocalGroupSetInfo(LPCWSTR,LPCWSTR,DWORD,PBYTE,PDWORD);
NET_API_STATUS WINAPI NetLocalGroupDel(LPCWSTR,LPCWSTR);
NET_API_STATUS WINAPI NetLocalGroupDelMember(LPCWSTR,LPCWSTR,PSID);
NET_API_STATUS WINAPI NetLocalGroupGetMembers(LPCWSTR,LPCWSTR,DWORD,PBYTE*,DWORD,
PDWORD,PDWORD,PDWORD_PTR);
NET_API_STATUS WINAPI NetLocalGroupSetMembers(LPCWSTR,LPCWSTR,DWORD,PBYTE,DWORD);
NET_API_STATUS WINAPI NetLocalGroupAddMembers(LPCWSTR,LPCWSTR,DWORD,PBYTE,DWORD);
NET_API_STATUS WINAPI NetLocalGroupDelMembers(LPCWSTR,LPCWSTR,DWORD,PBYTE,DWORD);
NET_API_STATUS WINAPI NetQueryDisplayInformation(LPCWSTR,DWORD,DWORD,DWORD,DWORD,PDWORD,PVOID*);
NET_API_STATUS WINAPI NetGetDisplayInformationIndex(LPCWSTR,DWORD,LPCWSTR,PDWORD);
NET_API_STATUS WINAPI NetGetDCName(LPCWSTR,LPCWSTR,PBYTE*);
NET_API_STATUS WINAPI NetGetAnyDCName(LPCWSTR,LPCWSTR,PBYTE*);
NET_API_STATUS WINAPI I_NetLogonControl(LPCWSTR,DWORD,DWORD,PBYTE*);
NET_API_STATUS WINAPI I_NetLogonControl2(LPCWSTR,DWORD,DWORD,PBYTE,PBYTE*);
NTSTATUS WINAPI NetEnumerateTrustedDomains(LPWSTR,LPWSTR*);
#ifdef __cplusplus
}
#endif
#endif
