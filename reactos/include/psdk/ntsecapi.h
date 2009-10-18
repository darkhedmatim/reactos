#ifndef _NTSECAPI_H
#define _NTSECAPI_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define KERB_WRAP_NO_ENCRYPT 0x80000001
#define LOGON_GUEST 1
#define LOGON_NOENCRYPTION 2
#define LOGON_CACHED_ACCOUNT 4
#define LOGON_USED_LM_PASSWORD 8
#define LOGON_EXTRA_SIDS 32
#define LOGON_SUBAUTH_SESSION_KEY 64
#define LOGON_SERVER_TRUST_ACCOUNT 128
#define LOGON_NTLMV2_ENABLED 256
#define LOGON_RESOURCE_GROUPS 512
#define LOGON_PROFILE_PATH_RETURNED 1024
#define LOGON_GRACE_LOGON 16777216
#define LSA_MODE_PASSWORD_PROTECTED 1
#define LSA_MODE_INDIVIDUAL_ACCOUNTS 2
#define LSA_MODE_MANDATORY_ACCESS 3
#define LSA_MODE_LOG_FULL 4
#define LSA_SUCCESS(x) ((LONG)(x)>=0)
#define MICROSOFT_KERBEROS_NAME_A "Kerberos"
#define MICROSOFT_KERBEROS_NAME_W L"Kerberos"
#define MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT 32
#define MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT 2048
#define MSV1_0_CHALLENGE_LENGTH 8
#define MSV1_0_CLEARTEXT_PASSWORD_ALLOWED 2
#define MSV1_0_CRED_LM_PRESENT 1
#define MSV1_0_CRED_NT_PRESENT 2
#define MSV1_0_CRED_VERSION 0
#define MSV1_0_DONT_TRY_GUEST_ACCOUNT 16
#define MSV1_0_LANMAN_SESSION_KEY_LENGTH 8
#define MSV1_0_MAX_NTLM3_LIFE 1800
#define MSV1_0_MAX_AVL_SIZE 64000
#define MSV1_0_MNS_LOGON 16777216
#define MSV1_0_NTLM3_RESPONSE_LENGTH 16
#define MSV1_0_NTLM3_OWF_LENGTH 16
#define MSV1_0_NTLM3_INPUT_LENGTH (sizeof(MSV1_0_NTLM3_RESPONSE)-MSV1_0_NTLM3_RESPONSE_LENGTH)
#define MSV1_0_OWF_PASSWORD_LENGTH 16
#define MSV1_0_PACKAGE_NAME "MICROSOFT_AUTHENTICATION_PACKAGE_V1_0"
#define MSV1_0_PACKAGE_NAMEW L"MICROSOFT_AUTHENTICATION_PACKAGE_V1_0"
#define MSV1_0_PACKAGE_NAMEW_LENGTH sizeof(MSV1_0_PACKAGE_NAMEW)-sizeof(WCHAR)
#define MSV1_0_RETURN_USER_PARAMETERS 8
#define MSV1_0_RETURN_PASSWORD_EXPIRY 64
#define MSV1_0_RETURN_PROFILE_PATH 512
#define MSV1_0_SUBAUTHENTICATION_DLL_EX 1048576
#define MSV1_0_SUBAUTHENTICATION_DLL 0xff000000
#define MSV1_0_SUBAUTHENTICATION_DLL_SHIFT 24
#define MSV1_0_SUBAUTHENTICATION_DLL_RAS 2
#define MSV1_0_SUBAUTHENTICATION_DLL_IIS 132
#define MSV1_0_SUBAUTHENTICATION_FLAGS 0xff000000
#define MSV1_0_SUBAUTHENTICATION_KEY "System\\CurrentControlSet\\Control\\Lsa\\MSV1_0"
#define MSV1_0_SUBAUTHENTICATION_VALUE "Auth"
#define MSV1_0_TRY_GUEST_ACCOUNT_ONLY 256
#define MSV1_0_TRY_SPECIFIED_DOMAIN_ONLY 1024
#define MSV1_0_UPDATE_LOGON_STATISTICS 4
#define MSV1_0_USE_CLIENT_CHALLENGE 128
#define MSV1_0_USER_SESSION_KEY_LENGTH 16
#define POLICY_VIEW_LOCAL_INFORMATION 1
#define POLICY_VIEW_AUDIT_INFORMATION 2
#define POLICY_GET_PRIVATE_INFORMATION 4
#define POLICY_TRUST_ADMIN 8
#define POLICY_CREATE_ACCOUNT 16
#define POLICY_CREATE_SECRET 32
#define POLICY_CREATE_PRIVILEGE 64
#define POLICY_SET_DEFAULT_QUOTA_LIMITS 128
#define POLICY_SET_AUDIT_REQUIREMENTS 256
#define POLICY_AUDIT_LOG_ADMIN 512
#define POLICY_SERVER_ADMIN 1024
#define POLICY_LOOKUP_NAMES 2048
#define POLICY_READ (STANDARD_RIGHTS_READ|6)
#define POLICY_WRITE (STANDARD_RIGHTS_WRITE|2040)
#define POLICY_EXECUTE (STANDARD_RIGHTS_EXECUTE|2049)
#define POLICY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|4095)
#define POLICY_AUDIT_EVENT_UNCHANGED 0
#define POLICY_AUDIT_EVENT_SUCCESS 1
#define POLICY_AUDIT_EVENT_FAILURE 2
#define POLICY_AUDIT_EVENT_NONE 4
#define POLICY_AUDIT_EVENT_MASK 7
#define POLICY_LOCATION_LOCAL 1
#define POLICY_LOCATION_DS 2
#define POLICY_MACHINE_POLICY_LOCAL 0
#define POLICY_MACHINE_POLICY_DEFAULTED 1
#define POLICY_MACHINE_POLICY_EXPLICIT 2
#define POLICY_MACHINE_POLICY_UNKNOWN 0xFFFFFFFF
#define POLICY_QOS_SCHANEL_REQUIRED 1
#define POLICY_QOS_OUTBOUND_INTEGRITY 2
#define POLICY_QOS_OUTBOUND_CONFIDENTIALITY 4
#define POLICY_QOS_INBOUND_INTEGREITY 8
#define POLICY_QOS_INBOUND_CONFIDENTIALITY 16
#define POLICY_QOS_ALLOW_LOCAL_ROOT_CERT_STORE 32
#define POLICY_QOS_RAS_SERVER_ALLOWED 64
#define POLICY_QOS_DHCP_SERVER_ALLOWD 128
#define POLICY_KERBEROS_FORWARDABLE 1
#define POLICY_KERBEROS_PROXYABLE 2
#define POLICY_KERBEROS_RENEWABLE 4
#define POLICY_KERBEROS_POSTDATEABLE 8
#define SAM_PASSWORD_CHANGE_NOTIFY_ROUTINE "PasswordChangeNotify"
#define SAM_INIT_NOTIFICATION_ROUTINE "InitializeChangeNotify"
#define SAM_PASSWORD_FILTER_ROUTINE "PasswordFilter"
#define SE_INTERACTIVE_LOGON_NAME TEXT("SeInteractiveLogonRight")
#define SE_NETWORK_LOGON_NAME TEXT("SeNetworkLogonRight")
#define SE_BATCH_LOGON_NAME TEXT("SeBatchLogonRight")
#define SE_SERVICE_LOGON_NAME TEXT("SeServiceLogonRight")
#define TRUST_ATTRIBUTE_NON_TRANSITIVE 1
#define TRUST_ATTRIBUTE_UPLEVEL_ONLY 2
#define TRUST_ATTRIBUTE_TREE_PARENT 4194304
#define TRUST_ATTRIBUTES_VALID  -16580609
#define TRUST_AUTH_TYPE_NONE 0
#define TRUST_AUTH_TYPE_NT4OWF 1
#define TRUST_AUTH_TYPE_CLEAR 2
#define TRUST_DIRECTION_DISABLED 0
#define TRUST_DIRECTION_INBOUND 1
#define TRUST_DIRECTION_OUTBOUND 2
#define TRUST_DIRECTION_BIDIRECTIONAL 3
#define TRUST_TYPE_DOWNLEVEL 1
#define TRUST_TYPE_UPLEVEL 2
#define TRUST_TYPE_MIT 3
#define TRUST_TYPE_DCE 4

#define SCESTATUS_SUCCESS             0L
#define SCESTATUS_INVALID_PARAMETER   1L
#define SCESTATUS_RECORD_NOT_FOUND    2L
#define SCESTATUS_INVALID_DATA        3L
#define SCESTATUS_OBJECT_EXISTS       4L
#define SCESTATUS_BUFFER_TOO_SMALL    5L
#define SCESTATUS_PROFILE_NOT_FOUND   6L
#define SCESTATUS_BAD_FORMAT          7L
#define SCESTATUS_NOT_ENOUGH_RESOURCE 8L
#define SCESTATUS_ACCESS_DENIED       9L
#define SCESTATUS_CANT_DELETE         10L
#define SCESTATUS_PREFIX_OVERFLOW     11L
#define SCESTATUS_OTHER_ERROR         12L
#define SCESTATUS_ALREADY_RUNNING     13L
#define SCESTATUS_SERVICE_NOT_SUPPORT 14L
#define SCESTATUS_MOD_NOT_FOUND       15L
#define SCESTATUS_EXCEPTION_IN_SERVER 16L
#define SCESTATUS_NO_TEMPLATE_GIVEN   17L
#define SCESTATUS_NO_MAPPING          18L
#define SCESTATUS_TRUST_FAIL          19L

#if !defined(_NTDEF_)
typedef LONG NTSTATUS, *PNTSTATUS;
#endif

#if defined (_NTDEF_)
typedef UNICODE_STRING LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;
typedef STRING LSA_STRING, *PLSA_STRING;
typedef OBJECT_ATTRIBUTES LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;

#else

typedef struct _LSA_UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
#ifdef MIDL_PASS
  [size_is(MaximumLength / 2), length_is(Length / 2)]
#endif
  PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;

typedef struct _LSA_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PCHAR Buffer;
} LSA_STRING, *PLSA_STRING;

typedef struct _LSA_OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PLSA_UNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;
  PVOID SecurityQualityOfService;
} LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;

typedef LSA_UNICODE_STRING UNICODE_STRING, *PUNICODE_STRING;
typedef LSA_STRING STRING, *PSTRING ;

#endif

typedef enum _KERB_LOGON_SUBMIT_TYPE
{
  KerbInteractiveLogon = 2,
  KerbS4ULogon = 12,
  KerbTicketLogon = 10,
  KerbTicketUnlockLogon = 11
} KERB_LOGON_SUBMIT_TYPE, *PKERB_LOGON_SUBMIT_TYPE;
typedef enum _MSV1_0_LOGON_SUBMIT_TYPE {
  MsV1_0InteractiveLogon = 2,
  MsV1_0Lm20Logon,
  MsV1_0NetworkLogon,
  MsV1_0SubAuthLogon,
  MsV1_0WorkstationUnlockLogon = 7
} MSV1_0_LOGON_SUBMIT_TYPE, *PMSV1_0_LOGON_SUBMIT_TYPE;
typedef enum _MSV1_0_PROFILE_BUFFER_TYPE {
  MsV1_0InteractiveProfile = 2,
  MsV1_0Lm20LogonProfile,
  MsV1_0SmartCardProfile
} MSV1_0_PROFILE_BUFFER_TYPE, *PMSV1_0_PROFILE_BUFFER_TYPE;
typedef enum {
  MsvAvEOL,
  MsvAvNbComputerName,
  MsvAvNbDomainName,
  MsvAvDnsComputerName,
  MsvAvDnsDomainName
} MSV1_0_AVID;
typedef enum _MSV1_0_PROTOCOL_MESSAGE_TYPE {
  MsV1_0Lm20ChallengeRequest = 0,
  MsV1_0Lm20GetChallengeResponse,
  MsV1_0EnumerateUsers,
  MsV1_0GetUserInfo,
  MsV1_0ReLogonUsers,
  MsV1_0ChangePassword,
  MsV1_0ChangeCachedPassword,
  MsV1_0GenericPassthrough,
  MsV1_0CacheLogon,
  MsV1_0SubAuth,
  MsV1_0DeriveCredential,
  MsV1_0CacheLookup
} MSV1_0_PROTOCOL_MESSAGE_TYPE, *PMSV1_0_PROTOCOL_MESSAGE_TYPE;
typedef enum _POLICY_LSA_SERVER_ROLE {
  PolicyServerRoleBackup = 2,
  PolicyServerRolePrimary
} POLICY_LSA_SERVER_ROLE, *PPOLICY_LSA_SERVER_ROLE;
typedef enum _POLICY_SERVER_ENABLE_STATE {
  PolicyServerEnabled = 2,
  PolicyServerDisabled
} POLICY_SERVER_ENABLE_STATE, *PPOLICY_SERVER_ENABLE_STATE;
typedef enum _POLICY_INFORMATION_CLASS {
  PolicyAuditLogInformation = 1,
  PolicyAuditEventsInformation,
  PolicyPrimaryDomainInformation,
  PolicyPdAccountInformation,
  PolicyAccountDomainInformation,
  PolicyLsaServerRoleInformation,
  PolicyReplicaSourceInformation,
  PolicyDefaultQuotaInformation,
  PolicyModificationInformation,
  PolicyAuditFullSetInformation,
  PolicyAuditFullQueryInformation,
  PolicyDnsDomainInformation,
  PolicyEfsInformation
} POLICY_INFORMATION_CLASS, *PPOLICY_INFORMATION_CLASS;
typedef enum _POLICY_AUDIT_EVENT_TYPE {
  AuditCategorySystem,
  AuditCategoryLogon,
  AuditCategoryObjectAccess,
  AuditCategoryPrivilegeUse,
  AuditCategoryDetailedTracking,
  AuditCategoryPolicyChange,
  AuditCategoryAccountManagement,
  AuditCategoryDirectoryServiceAccess,
  AuditCategoryAccountLogon
} POLICY_AUDIT_EVENT_TYPE, *PPOLICY_AUDIT_EVENT_TYPE;
typedef enum _POLICY_LOCAL_INFORMATION_CLASS {
  PolicyLocalAuditEventsInformation = 1,
  PolicyLocalPdAccountInformation,
  PolicyLocalAccountDomainInformation,
  PolicyLocalLsaServerRoleInformation,
  PolicyLocalReplicaSourceInformation,
  PolicyLocalModificationInformation,
  PolicyLocalAuditFullSetInformation,
  PolicyLocalAuditFullQueryInformation,
  PolicyLocalDnsDomainInformation,
  PolicyLocalIPSecReferenceInformation,
  PolicyLocalMachinePasswordInformation,
  PolicyLocalQualityOfServiceInformation,
  PolicyLocalPolicyLocationInformation
} POLICY_LOCAL_INFORMATION_CLASS, *PPOLICY_LOCAL_INFORMATION_CLASS;
typedef enum _POLICY_DOMAIN_INFORMATION_CLASS {
  PolicyDomainIPSecReferenceInformation = 1,
  PolicyDomainQualityOfServiceInformation,
  PolicyDomainEfsInformation,
  PolicyDomainPublicKeyInformation,
  PolicyDomainPasswordPolicyInformation,
  PolicyDomainLockoutInformation,
  PolicyDomainKerberosTicketInformation
} POLICY_DOMAIN_INFORMATION_CLASS, *PPOLICY_DOMAIN_INFORMATION_CLASS;
typedef enum _POLICY_NOTIFICATION_INFORMATION_CLASS {
  PolicyNotifyAuditEventsInformation = 1,
  PolicyNotifyAccountDomainInformation,
  PolicyNotifyServerRoleInformation,
  PolicyNotifyDnsDomainInformation,
  PolicyNotifyDomainEfsInformation,
  PolicyNotifyDomainKerberosTicketInformation,
  PolicyNotifyMachineAccountPasswordInformation
} POLICY_NOTIFICATION_INFORMATION_CLASS, *PPOLICY_NOTIFICATION_INFORMATION_CLASS;
typedef enum _SECURITY_LOGON_TYPE {
  Interactive = 2,
  Network,
  Batch,
  Service,
  Proxy,
  Unlock
} SECURITY_LOGON_TYPE, *PSECURITY_LOGON_TYPE;
typedef struct _SECURITY_LOGON_SESSION_DATA {
  ULONG Size;
  LUID LogonId;
  LSA_UNICODE_STRING UserName;
  LSA_UNICODE_STRING LogonDomain;
  LSA_UNICODE_STRING AuthenticationPackage;
  ULONG LogonType;
  ULONG Session;
  PSID Sid;
  LARGE_INTEGER LogonTime;
  LSA_UNICODE_STRING LogonServer;
  LSA_UNICODE_STRING DnsDomainName;
  LSA_UNICODE_STRING Upn;
} SECURITY_LOGON_SESSION_DATA, *PSECURITY_LOGON_SESSION_DATA;
typedef enum _TRUSTED_INFORMATION_CLASS {
  TrustedDomainNameInformation = 1,
  TrustedControllersInformation,
  TrustedPosixOffsetInformation,
  TrustedPasswordInformation,
  TrustedDomainInformationBasic,
  TrustedDomainInformationEx,
  TrustedDomainAuthInformation,
  TrustedDomainFullInformation
} TRUSTED_INFORMATION_CLASS, *PTRUSTED_INFORMATION_CLASS;
typedef enum _LSA_FOREST_TRUST_RECORD_TYPE {
  ForestTrustTopLevelName,
  ForestTrustTopLevelNameEx,
  ForestTrustDomainInfo,
  ForestTrustRecordTypeLast = ForestTrustDomainInfo
} LSA_FOREST_TRUST_RECORD_TYPE;
typedef enum _LSA_FOREST_TRUST_COLLISION_RECORD_TYPE {
  CollisionTdo,
  CollisionXref,
  CollisionOther
} LSA_FOREST_TRUST_COLLISION_RECORD_TYPE;
typedef struct _DOMAIN_PASSWORD_INFORMATION {
  USHORT MinPasswordLength;
  USHORT PasswordHistoryLength;
  ULONG PasswordProperties;
  LARGE_INTEGER MaxPasswordAge;
  LARGE_INTEGER MinPasswordAge;
} DOMAIN_PASSWORD_INFORMATION, *PDOMAIN_PASSWORD_INFORMATION;
typedef ULONG LSA_ENUMERATION_HANDLE, *PLSA_ENUMERATION_HANDLE;
typedef struct _LSA_ENUMERATION_INFORMATION {
  PSID Sid;
} LSA_ENUMERATION_INFORMATION, *PLSA_ENUMERATION_INFORMATION;
typedef ULONG LSA_OPERATIONAL_MODE, *PLSA_OPERATIONAL_MODE;

typedef struct _LSA_FOREST_TRUST_DOMAIN_INFO {
  PSID Sid;
  LSA_UNICODE_STRING DnsName;
  LSA_UNICODE_STRING NetbiosName;
} LSA_FOREST_TRUST_DOMAIN_INFO, *PLSA_FOREST_TRUST_DOMAIN_INFO;
typedef struct _LSA_FOREST_TRUST_BINARY_DATA {
  ULONG Length;
  PUCHAR Buffer;
} LSA_FOREST_TRUST_BINARY_DATA, *PLSA_FOREST_TRUST_BINARY_DATA;
typedef struct _LSA_FOREST_TRUST_RECORD {
  ULONG Flags;
  LSA_FOREST_TRUST_RECORD_TYPE ForestTrustType;
  LARGE_INTEGER Time;
  union {
    LSA_UNICODE_STRING TopLevelName;
    LSA_FOREST_TRUST_DOMAIN_INFO DomainInfo;
    LSA_FOREST_TRUST_BINARY_DATA Data;
  } ForestTrustData;
} LSA_FOREST_TRUST_RECORD, *PLSA_FOREST_TRUST_RECORD;
typedef struct _LSA_FOREST_TRUST_INFORMATION {
  ULONG RecordCount;
  PLSA_FOREST_TRUST_RECORD *Entries;
} LSA_FOREST_TRUST_INFORMATION, *PLSA_FOREST_TRUST_INFORMATION;
typedef struct _LSA_FOREST_TRUST_COLLISION_RECORD {
  ULONG Index;
  LSA_FOREST_TRUST_COLLISION_RECORD_TYPE Type;
  ULONG Flags;
  LSA_UNICODE_STRING Name;
} LSA_FOREST_TRUST_COLLISION_RECORD, *PLSA_FOREST_TRUST_COLLISION_RECORD;
typedef struct _LSA_FOREST_TRUST_COLLISION_INFORMATION {
  ULONG RecordCount;
  PLSA_FOREST_TRUST_COLLISION_RECORD *Entries;
} LSA_FOREST_TRUST_COLLISION_INFORMATION, *PLSA_FOREST_TRUST_COLLISION_INFORMATION;
typedef struct _LSA_TRUST_INFORMATION {
  LSA_UNICODE_STRING Name;
  PSID Sid;
} LSA_TRUST_INFORMATION, *PLSA_TRUST_INFORMATION;
typedef struct _LSA_REFERENCED_DOMAIN_LIST {
  ULONG Entries;
  PLSA_TRUST_INFORMATION Domains;
} LSA_REFERENCED_DOMAIN_LIST, *PLSA_REFERENCED_DOMAIN_LIST;
typedef struct _LSA_TRANSLATED_SID {
  SID_NAME_USE Use;
  ULONG RelativeId;
  LONG DomainIndex;
} LSA_TRANSLATED_SID, *PLSA_TRANSLATED_SID;
typedef struct _LSA_TRANSLATED_SID2 {
  SID_NAME_USE Use;
  PSID Sid;
  LONG DomainIndex;
  ULONG Flags;
} LSA_TRANSLATED_SID2, *PLSA_TRANSLATED_SID2;
typedef struct _LSA_TRANSLATED_NAME {
  SID_NAME_USE Use;
  LSA_UNICODE_STRING Name;
  LONG DomainIndex;
} LSA_TRANSLATED_NAME, *PLSA_TRANSLATED_NAME;

typedef struct _KERB_INTERACTIVE_LOGON {
  KERB_LOGON_SUBMIT_TYPE MessageType;
  UNICODE_STRING LogonDomainName;
  UNICODE_STRING UserName;
  UNICODE_STRING Password;
} KERB_INTERACTIVE_LOGON, *PKERB_INTERACTIVE_LOGON;
typedef struct _MSV1_0_INTERACTIVE_LOGON {
  MSV1_0_LOGON_SUBMIT_TYPE MessageType;
  UNICODE_STRING LogonDomainName;
  UNICODE_STRING UserName;
  UNICODE_STRING Password;
} MSV1_0_INTERACTIVE_LOGON, *PMSV1_0_INTERACTIVE_LOGON;
typedef struct _MSV1_0_INTERACTIVE_PROFILE {
  MSV1_0_PROFILE_BUFFER_TYPE MessageType;
  USHORT LogonCount;
  USHORT BadPasswordCount;
  LARGE_INTEGER LogonTime;
  LARGE_INTEGER LogoffTime;
  LARGE_INTEGER KickOffTime;
  LARGE_INTEGER PasswordLastSet;
  LARGE_INTEGER PasswordCanChange;
  LARGE_INTEGER PasswordMustChange;
  UNICODE_STRING LogonScript;
  UNICODE_STRING HomeDirectory;
  UNICODE_STRING FullName;
  UNICODE_STRING ProfilePath;
  UNICODE_STRING HomeDirectoryDrive;
  UNICODE_STRING LogonServer;
  ULONG UserFlags;
} MSV1_0_INTERACTIVE_PROFILE, *PMSV1_0_INTERACTIVE_PROFILE;
typedef struct _MSV1_0_LM20_LOGON {
  MSV1_0_LOGON_SUBMIT_TYPE MessageType;
  UNICODE_STRING LogonDomainName;
  UNICODE_STRING UserName;
  UNICODE_STRING Workstation;
  UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
  STRING CaseSensitiveChallengeResponse;
  STRING CaseInsensitiveChallengeResponse;
  ULONG ParameterControl;
} MSV1_0_LM20_LOGON, * PMSV1_0_LM20_LOGON;
typedef struct _MSV1_0_SUBAUTH_LOGON{ /* W2K only */
  MSV1_0_LOGON_SUBMIT_TYPE MessageType;
  UNICODE_STRING LogonDomainName;
  UNICODE_STRING UserName;
  UNICODE_STRING Workstation;
  UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
  STRING AuthenticationInfo1;
  STRING AuthenticationInfo2;
  ULONG ParameterControl;
  ULONG SubAuthPackageId;
} MSV1_0_SUBAUTH_LOGON, * PMSV1_0_SUBAUTH_LOGON;
typedef struct _MSV1_0_LM20_LOGON_PROFILE {
  MSV1_0_PROFILE_BUFFER_TYPE MessageType;
  LARGE_INTEGER KickOffTime;
  LARGE_INTEGER LogoffTime;
  ULONG UserFlags;
  UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
  UNICODE_STRING LogonDomainName;
  UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
  UNICODE_STRING LogonServer;
  UNICODE_STRING UserParameters;
} MSV1_0_LM20_LOGON_PROFILE, * PMSV1_0_LM20_LOGON_PROFILE;
typedef struct _MSV1_0_SUPPLEMENTAL_CREDENTIAL {
  ULONG Version;
  ULONG Flags;
  UCHAR LmPassword[MSV1_0_OWF_PASSWORD_LENGTH];
  UCHAR NtPassword[MSV1_0_OWF_PASSWORD_LENGTH];
} MSV1_0_SUPPLEMENTAL_CREDENTIAL, *PMSV1_0_SUPPLEMENTAL_CREDENTIAL;
typedef struct _MSV1_0_NTLM3_RESPONSE {
  UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH];
  UCHAR RespType;
  UCHAR HiRespType;
  USHORT Flags;
  ULONG MsgWord;
  ULONGLONG TimeStamp;
  UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH];
  ULONG AvPairsOff;
  UCHAR Buffer[1];
} MSV1_0_NTLM3_RESPONSE, *PMSV1_0_NTLM3_RESPONSE;
typedef struct  _MSV1_0_AV_PAIR {
  USHORT AvId;
  USHORT AvLen;
} MSV1_0_AV_PAIR, *PMSV1_0_AV_PAIR;
typedef struct _MSV1_0_CHANGEPASSWORD_REQUEST {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  UNICODE_STRING DomainName;
  UNICODE_STRING AccountName;
  UNICODE_STRING OldPassword;
  UNICODE_STRING NewPassword;
  BOOLEAN Impersonating;
} MSV1_0_CHANGEPASSWORD_REQUEST, *PMSV1_0_CHANGEPASSWORD_REQUEST;
typedef struct _MSV1_0_CHANGEPASSWORD_RESPONSE {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  BOOLEAN PasswordInfoValid;
  DOMAIN_PASSWORD_INFORMATION DomainPasswordInfo;
} MSV1_0_CHANGEPASSWORD_RESPONSE, *PMSV1_0_CHANGEPASSWORD_RESPONSE;
typedef struct _MSV1_0_SUBAUTH_REQUEST{
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  ULONG SubAuthPackageId;
  ULONG SubAuthInfoLength;
  PUCHAR SubAuthSubmitBuffer;
} MSV1_0_SUBAUTH_REQUEST, *PMSV1_0_SUBAUTH_REQUEST;
typedef struct _MSV1_0_SUBAUTH_RESPONSE{
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  ULONG SubAuthInfoLength;
  PUCHAR SubAuthReturnBuffer;
} MSV1_0_SUBAUTH_RESPONSE, *PMSV1_0_SUBAUTH_RESPONSE;
#define MSV1_0_DERIVECRED_TYPE_SHA1 0
typedef struct _MSV1_0_DERIVECRED_REQUEST {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  LUID LogonId;
  ULONG DeriveCredType;
  ULONG DeriveCredInfoLength;
  UCHAR DeriveCredSubmitBuffer[1];
} MSV1_0_DERIVECRED_REQUEST, *PMSV1_0_DERIVECRED_REQUEST;
typedef struct _MSV1_0_DERIVECRED_RESPONSE {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  ULONG DeriveCredInfoLength;
  UCHAR DeriveCredReturnBuffer[1];
} MSV1_0_DERIVECRED_RESPONSE, *PMSV1_0_DERIVECRED_RESPONSE;
typedef ULONG POLICY_AUDIT_EVENT_OPTIONS, *PPOLICY_AUDIT_EVENT_OPTIONS;
typedef struct _POLICY_PRIVILEGE_DEFINITION {
  LSA_UNICODE_STRING Name;
  LUID LocalValue;
} POLICY_PRIVILEGE_DEFINITION, *PPOLICY_PRIVILEGE_DEFINITION;
typedef struct _POLICY_AUDIT_LOG_INFO {
  ULONG AuditLogPercentFull;
  ULONG MaximumLogSize;
  LARGE_INTEGER AuditRetentionPeriod;
  BOOLEAN AuditLogFullShutdownInProgress;
  LARGE_INTEGER TimeToShutdown;
  ULONG NextAuditRecordId;
} POLICY_AUDIT_LOG_INFO, *PPOLICY_AUDIT_LOG_INFO;
typedef struct _POLICY_AUDIT_EVENTS_INFO {
  BOOLEAN AuditingMode;
  PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
  ULONG MaximumAuditEventCount;
} POLICY_AUDIT_EVENTS_INFO, *PPOLICY_AUDIT_EVENTS_INFO;
typedef struct _POLICY_ACCOUNT_DOMAIN_INFO {
  LSA_UNICODE_STRING DomainName;
  PSID DomainSid;
} POLICY_ACCOUNT_DOMAIN_INFO, *PPOLICY_ACCOUNT_DOMAIN_INFO;
typedef struct _POLICY_PRIMARY_DOMAIN_INFO {
  LSA_UNICODE_STRING Name;
  PSID Sid;
} POLICY_PRIMARY_DOMAIN_INFO, *PPOLICY_PRIMARY_DOMAIN_INFO;
typedef struct _POLICY_DNS_DOMAIN_INFO {
  LSA_UNICODE_STRING Name;
  LSA_UNICODE_STRING DnsDomainName;
  LSA_UNICODE_STRING DnsTreeName;
  GUID DomainGuid;
  PSID Sid;
} POLICY_DNS_DOMAIN_INFO, *PPOLICY_DNS_DOMAIN_INFO;
typedef struct _POLICY_PD_ACCOUNT_INFO {
  LSA_UNICODE_STRING Name;
} POLICY_PD_ACCOUNT_INFO, *PPOLICY_PD_ACCOUNT_INFO;
typedef struct _POLICY_LSA_SERVER_ROLE_INFO {
  POLICY_LSA_SERVER_ROLE LsaServerRole;
} POLICY_LSA_SERVER_ROLE_INFO, *PPOLICY_LSA_SERVER_ROLE_INFO;
typedef struct _POLICY_REPLICA_SOURCE_INFO {
  LSA_UNICODE_STRING ReplicaSource;
  LSA_UNICODE_STRING ReplicaAccountName;
} POLICY_REPLICA_SOURCE_INFO, *PPOLICY_REPLICA_SOURCE_INFO;
typedef struct _POLICY_DEFAULT_QUOTA_INFO {
  QUOTA_LIMITS QuotaLimits;
} POLICY_DEFAULT_QUOTA_INFO, *PPOLICY_DEFAULT_QUOTA_INFO;
typedef struct _POLICY_MODIFICATION_INFO {
  LARGE_INTEGER ModifiedId;
  LARGE_INTEGER DatabaseCreationTime;
} POLICY_MODIFICATION_INFO, *PPOLICY_MODIFICATION_INFO;
typedef struct _POLICY_AUDIT_FULL_SET_INFO {
  BOOLEAN ShutDownOnFull;
} POLICY_AUDIT_FULL_SET_INFO, *PPOLICY_AUDIT_FULL_SET_INFO;
typedef struct _POLICY_AUDIT_FULL_QUERY_INFO {
  BOOLEAN ShutDownOnFull;
  BOOLEAN LogIsFull;
} POLICY_AUDIT_FULL_QUERY_INFO, *PPOLICY_AUDIT_FULL_QUERY_INFO;
typedef struct _POLICY_EFS_INFO {
  ULONG InfoLength;
  PUCHAR EfsBlob;
} POLICY_EFS_INFO, *PPOLICY_EFS_INFO;
typedef struct _POLICY_LOCAL_IPSEC_REFERENCE_INFO {
  LSA_UNICODE_STRING ObjectPath;
} POLICY_LOCAL_IPSEC_REFERENCE_INFO, *PPOLICY_LOCAL_IPSEC_REFERENCE_INFO;
typedef struct _POLICY_LOCAL_MACHINE_PASSWORD_INFO {
  LARGE_INTEGER PasswordChangeInterval;
} POLICY_LOCAL_MACHINE_PASSWORD_INFO, *PPOLICY_LOCAL_MACHINE_PASSWORD_INFO;
typedef struct _POLICY_LOCAL_POLICY_LOCATION_INFO {
  ULONG PolicyLocation;
} POLICY_LOCAL_POLICY_LOCATION_INFO, *PPOLICY_LOCAL_POLICY_LOCATION_INFO;
typedef struct _POLICY_LOCAL_QUALITY_OF_SERVICE_INFO {
  ULONG QualityOfService;
} POLICY_LOCAL_QUALITY_OF_SERVICE_INFO, *PPOLICY_LOCAL_QUALITY_OF_SERVICE_INFO;
typedef struct _POLICY_LOCAL_QUALITY_OF_SERVICE_INFO POLICY_DOMAIN_QUALITY_OF_SERVICE_INFO;
typedef struct _POLICY_LOCAL_QUALITY_OF_SERVICE_INFO *PPOLICY_DOMAIN_QUALITY_OF_SERVICE_INFO;
typedef struct _POLICY_DOMAIN_PUBLIC_KEY_INFO {
  ULONG InfoLength;
  PUCHAR PublicKeyInfo;
} POLICY_DOMAIN_PUBLIC_KEY_INFO, *PPOLICY_DOMAIN_PUBLIC_KEY_INFO;
typedef struct _POLICY_DOMAIN_LOCKOUT_INFO {
  LARGE_INTEGER LockoutDuration;
  LARGE_INTEGER LockoutObservationWindow;
  USHORT LockoutThreshold;
} POLICY_DOMAIN_LOCKOUT_INFO, *PPOLICY_DOMAIN_LOCKOUT_INFO;
typedef struct _POLICY_DOMAIN_PASSWORD_INFO {
  USHORT MinPasswordLength;
  USHORT PasswordHistoryLength;
  ULONG PasswordProperties;
  LARGE_INTEGER MaxPasswordAge;
  LARGE_INTEGER MinPasswordAge;
} POLICY_DOMAIN_PASSWORD_INFO, *PPOLICY_DOMAIN_PASSWORD_INFO;
typedef struct _POLICY_DOMAIN_KERBEROS_TICKET_INFO {
  ULONG AuthenticationOptions;
  LARGE_INTEGER MinTicketAge;
  LARGE_INTEGER MaxTicketAge;
  LARGE_INTEGER MaxRenewAge;
  LARGE_INTEGER ProxyLifetime;
  LARGE_INTEGER ForceLogoff;
} POLICY_DOMAIN_KERBEROS_TICKET_INFO, *PPOLICY_DOMAIN_KERBEROS_TICKET_INFO;
typedef PVOID LSA_HANDLE, *PLSA_HANDLE;
typedef struct _TRUSTED_DOMAIN_NAME_INFO {
  LSA_UNICODE_STRING Name;
} TRUSTED_DOMAIN_NAME_INFO, *PTRUSTED_DOMAIN_NAME_INFO;
typedef struct _TRUSTED_CONTROLLERS_INFO {
  ULONG Entries;
  PLSA_UNICODE_STRING Names;
} TRUSTED_CONTROLLERS_INFO, *PTRUSTED_CONTROLLERS_INFO;
typedef struct _TRUSTED_POSIX_OFFSET_INFO {
  ULONG Offset;
} TRUSTED_POSIX_OFFSET_INFO, *PTRUSTED_POSIX_OFFSET_INFO;
typedef struct _TRUSTED_PASSWORD_INFO {
  LSA_UNICODE_STRING Password;
  LSA_UNICODE_STRING OldPassword;
} TRUSTED_PASSWORD_INFO, *PTRUSTED_PASSWORD_INFO;
typedef  LSA_TRUST_INFORMATION TRUSTED_DOMAIN_INFORMATION_BASIC;
typedef PLSA_TRUST_INFORMATION *PTRUSTED_DOMAIN_INFORMATION_BASIC;
typedef struct _TRUSTED_DOMAIN_INFORMATION_EX {
  LSA_UNICODE_STRING Name;
  LSA_UNICODE_STRING FlatName;
  PSID Sid;
  ULONG TrustDirection;
  ULONG TrustType;
  ULONG TrustAttributes;
} TRUSTED_DOMAIN_INFORMATION_EX, *PTRUSTED_DOMAIN_INFORMATION_EX;
typedef struct _LSA_AUTH_INFORMATION {
  LARGE_INTEGER LastUpdateTime;
  ULONG AuthType;
  ULONG AuthInfoLength;
  PUCHAR AuthInfo;
} LSA_AUTH_INFORMATION, *PLSA_AUTH_INFORMATION;
typedef struct _TRUSTED_DOMAIN_AUTH_INFORMATION {
  ULONG IncomingAuthInfos;
  PLSA_AUTH_INFORMATION IncomingAuthenticationInformation;
  PLSA_AUTH_INFORMATION IncomingPreviousAuthenticationInformation;
  ULONG OutgoingAuthInfos;
  PLSA_AUTH_INFORMATION OutgoingAuthenticationInformation;
  PLSA_AUTH_INFORMATION OutgoingPreviousAuthenticationInformation;
} TRUSTED_DOMAIN_AUTH_INFORMATION, *PTRUSTED_DOMAIN_AUTH_INFORMATION;
typedef struct _TRUSTED_DOMAIN_FULL_INFORMATION {
  TRUSTED_DOMAIN_INFORMATION_EX Information;
  TRUSTED_POSIX_OFFSET_INFO PosixOffset;
  TRUSTED_DOMAIN_AUTH_INFORMATION AuthInformation;
} TRUSTED_DOMAIN_FULL_INFORMATION, *PTRUSTED_DOMAIN_FULL_INFORMATION;
NTSTATUS NTAPI LsaAddAccountRights(LSA_HANDLE,PSID,PLSA_UNICODE_STRING,ULONG);
NTSTATUS NTAPI LsaCallAuthenticationPackage(HANDLE,ULONG,PVOID,ULONG,PVOID*,
                            PULONG,PNTSTATUS);
NTSTATUS NTAPI LsaClose(LSA_HANDLE);
NTSTATUS NTAPI LsaConnectUntrusted(PHANDLE);
NTSTATUS NTAPI LsaCreateTrustedDomainEx(LSA_HANDLE,
                            PTRUSTED_DOMAIN_INFORMATION_EX,
                            PTRUSTED_DOMAIN_AUTH_INFORMATION,ACCESS_MASK,
                            PLSA_HANDLE);
NTSTATUS NTAPI LsaDeleteTrustedDomain(LSA_HANDLE,PSID);
NTSTATUS NTAPI LsaDeregisterLogonProcess(HANDLE);
NTSTATUS NTAPI LsaEnumerateAccountRights(LSA_HANDLE,PSID,PLSA_UNICODE_STRING*,PULONG);
NTSTATUS NTAPI LsaEnumerateAccountsWithUserRight(LSA_HANDLE,PLSA_UNICODE_STRING,
                            PVOID*,PULONG);
NTSTATUS NTAPI LsaEnumerateTrustedDomains(LSA_HANDLE,PLSA_ENUMERATION_HANDLE,
                            PVOID*,ULONG,PULONG);
NTSTATUS NTAPI LsaEnumerateTrustedDomainsEx(LSA_HANDLE,PLSA_ENUMERATION_HANDLE,
                            PVOID*,ULONG,PULONG);
NTSTATUS NTAPI LsaFreeMemory(PVOID);
NTSTATUS NTAPI LsaFreeReturnBuffer(PVOID);
NTSTATUS NTAPI LsaLogonUser(HANDLE,PLSA_STRING,SECURITY_LOGON_TYPE,ULONG,PVOID,
                            ULONG,PTOKEN_GROUPS,PTOKEN_SOURCE,PVOID*,PULONG,
                            PLUID,PHANDLE,PQUOTA_LIMITS,PNTSTATUS);
NTSTATUS NTAPI LsaLookupAuthenticationPackage(HANDLE,PLSA_STRING,PULONG);
NTSTATUS NTAPI LsaLookupNames(LSA_HANDLE,ULONG,PLSA_UNICODE_STRING,
                            PLSA_REFERENCED_DOMAIN_LIST*,PLSA_TRANSLATED_SID*);
NTSTATUS NTAPI LsaLookupNames2(LSA_HANDLE,ULONG,ULONG,PLSA_UNICODE_STRING,
                            PLSA_REFERENCED_DOMAIN_LIST*,PLSA_TRANSLATED_SID2*);
NTSTATUS NTAPI LsaLookupSids(LSA_HANDLE,ULONG,PSID*,
                            PLSA_REFERENCED_DOMAIN_LIST*,PLSA_TRANSLATED_NAME*);
ULONG NTAPI LsaNtStatusToWinError(NTSTATUS);
NTSTATUS NTAPI LsaOpenPolicy(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,
                            ACCESS_MASK,PLSA_HANDLE);
NTSTATUS NTAPI LsaQueryDomainInformationPolicy(LSA_HANDLE,
                            POLICY_DOMAIN_INFORMATION_CLASS,PVOID*);
NTSTATUS NTAPI LsaQueryInformationPolicy(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);
NTSTATUS NTAPI LsaQueryLocalInformationPolicy(LSA_HANDLE,
                            POLICY_LOCAL_INFORMATION_CLASS,PVOID*);
NTSTATUS NTAPI LsaQueryTrustedDomainInfo(LSA_HANDLE,PSID,
                            TRUSTED_INFORMATION_CLASS,PVOID*);
NTSTATUS NTAPI LsaQueryTrustedDomainInfoByName(LSA_HANDLE,PLSA_UNICODE_STRING,
                            TRUSTED_INFORMATION_CLASS,PVOID*);
NTSTATUS NTAPI LsaRegisterLogonProcess(PLSA_STRING,PHANDLE,PLSA_OPERATIONAL_MODE);
NTSTATUS NTAPI LsaRemoveAccountRights(LSA_HANDLE,PSID,BOOLEAN,
                            PLSA_UNICODE_STRING,ULONG);
NTSTATUS NTAPI LsaRetrievePrivateData(LSA_HANDLE,PLSA_UNICODE_STRING,
                            PLSA_UNICODE_STRING*);
NTSTATUS NTAPI LsaSetDomainInformationPolicy(LSA_HANDLE,
                            POLICY_DOMAIN_INFORMATION_CLASS,PVOID);
NTSTATUS NTAPI LsaSetInformationPolicy(LSA_HANDLE,POLICY_INFORMATION_CLASS, PVOID);
NTSTATUS NTAPI LsaSetLocalInformationPolicy(LSA_HANDLE,
                            POLICY_LOCAL_INFORMATION_CLASS,PVOID);
NTSTATUS NTAPI LsaSetTrustedDomainInformation(LSA_HANDLE,PSID,
                            TRUSTED_INFORMATION_CLASS,PVOID);
NTSTATUS NTAPI LsaSetTrustedDomainInfoByName(LSA_HANDLE,PLSA_UNICODE_STRING,
                            TRUSTED_INFORMATION_CLASS,PVOID);
NTSTATUS NTAPI LsaStorePrivateData(LSA_HANDLE,PLSA_UNICODE_STRING,
                            PLSA_UNICODE_STRING);
typedef NTSTATUS (*PSAM_PASSWORD_NOTIFICATION_ROUTINE)(PUNICODE_STRING,
                            ULONG,PUNICODE_STRING);
typedef BOOLEAN (*PSAM_INIT_NOTIFICATION_ROUTINE)(void);
typedef BOOLEAN (*PSAM_PASSWORD_FILTER_ROUTINE)(PUNICODE_STRING,PUNICODE_STRING,
                            PUNICODE_STRING,BOOLEAN);
#ifdef __cplusplus
}
#endif
#endif /* _NTSECAPI_H */
