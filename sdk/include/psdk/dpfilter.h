/*
 * dpfilter.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Created by Timo Kreuzer <timo.kreuzer@reactos.org>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#define DPFLTR_ERROR_LEVEL                  0
#define DPFLTR_WARNING_LEVEL                1
#define DPFLTR_TRACE_LEVEL                  2
#define DPFLTR_INFO_LEVEL                   3
#define DPFLTR_MASK                         0x80000000

typedef enum _DPFLTR_TYPE {
  DPFLTR_SYSTEM_ID = 0,
  DPFLTR_SMSS_ID = 1,
  DPFLTR_SETUP_ID = 2,
  DPFLTR_NTFS_ID = 3,
  DPFLTR_FSTUB_ID = 4,
  DPFLTR_CRASHDUMP_ID = 5,
  DPFLTR_CDAUDIO_ID = 6,
  DPFLTR_CDROM_ID = 7,
  DPFLTR_CLASSPNP_ID = 8,
  DPFLTR_DISK_ID = 9,
  DPFLTR_REDBOOK_ID = 10,
  DPFLTR_STORPROP_ID = 11,
  DPFLTR_SCSIPORT_ID = 12,
  DPFLTR_SCSIMINIPORT_ID = 13,
  DPFLTR_CONFIG_ID = 14,
  DPFLTR_I8042PRT_ID = 15,
  DPFLTR_SERMOUSE_ID = 16,
  DPFLTR_LSERMOUS_ID = 17,
  DPFLTR_KBDHID_ID = 18,
  DPFLTR_MOUHID_ID = 19,
  DPFLTR_KBDCLASS_ID = 20,
  DPFLTR_MOUCLASS_ID = 21,
  DPFLTR_TWOTRACK_ID = 22,
  DPFLTR_WMILIB_ID = 23,
  DPFLTR_ACPI_ID = 24,
  DPFLTR_AMLI_ID = 25,
  DPFLTR_HALIA64_ID = 26,
  DPFLTR_VIDEO_ID = 27,
  DPFLTR_SVCHOST_ID = 28,
  DPFLTR_VIDEOPRT_ID = 29,
  DPFLTR_TCPIP_ID = 30,
  DPFLTR_DMSYNTH_ID = 31,
  DPFLTR_NTOSPNP_ID = 32,
  DPFLTR_FASTFAT_ID = 33,
  DPFLTR_SAMSS_ID = 34,
  DPFLTR_PNPMGR_ID = 35,
  DPFLTR_NETAPI_ID = 36,
  DPFLTR_SCSERVER_ID = 37,
  DPFLTR_SCCLIENT_ID = 38,
  DPFLTR_SERIAL_ID = 39,
  DPFLTR_SERENUM_ID = 40,
  DPFLTR_UHCD_ID = 41,
  DPFLTR_RPCPROXY_ID = 42,
  DPFLTR_AUTOCHK_ID = 43,
  DPFLTR_DCOMSS_ID = 44,
  DPFLTR_UNIMODEM_ID = 45,
  DPFLTR_SIS_ID = 46,
  DPFLTR_FLTMGR_ID = 47,
  DPFLTR_WMICORE_ID = 48,
  DPFLTR_BURNENG_ID = 49,
  DPFLTR_IMAPI_ID = 50,
  DPFLTR_SXS_ID = 51,
  DPFLTR_FUSION_ID = 52,
  DPFLTR_IDLETASK_ID = 53,
  DPFLTR_SOFTPCI_ID = 54,
  DPFLTR_TAPE_ID = 55,
  DPFLTR_MCHGR_ID = 56,
  DPFLTR_IDEP_ID = 57,
  DPFLTR_PCIIDE_ID = 58,
  DPFLTR_FLOPPY_ID = 59,
  DPFLTR_FDC_ID = 60,
  DPFLTR_TERMSRV_ID = 61,
  DPFLTR_W32TIME_ID = 62,
  DPFLTR_PREFETCHER_ID = 63,
  DPFLTR_RSFILTER_ID = 64,
  DPFLTR_FCPORT_ID = 65,
  DPFLTR_PCI_ID = 66,
  DPFLTR_DMIO_ID = 67,
  DPFLTR_DMCONFIG_ID = 68,
  DPFLTR_DMADMIN_ID = 69,
  DPFLTR_WSOCKTRANSPORT_ID = 70,
  DPFLTR_VSS_ID = 71,
  DPFLTR_PNPMEM_ID = 72,
  DPFLTR_PROCESSOR_ID = 73,
  DPFLTR_DMSERVER_ID = 74,
  DPFLTR_SR_ID = 75,
  DPFLTR_INFINIBAND_ID = 76,
  DPFLTR_IHVDRIVER_ID = 77,
  DPFLTR_IHVVIDEO_ID = 78,
  DPFLTR_IHVAUDIO_ID = 79,
  DPFLTR_IHVNETWORK_ID = 80,
  DPFLTR_IHVSTREAMING_ID = 81,
  DPFLTR_IHVBUS_ID = 82,
  DPFLTR_HPS_ID = 83,
  DPFLTR_RTLTHREADPOOL_ID = 84,
  DPFLTR_LDR_ID = 85,
  DPFLTR_TCPIP6_ID = 86,
  DPFLTR_ISAPNP_ID = 87,
  DPFLTR_SHPC_ID = 88,
  DPFLTR_STORPORT_ID = 89,
  DPFLTR_STORMINIPORT_ID = 90,
  DPFLTR_PRINTSPOOLER_ID = 91,
  DPFLTR_VSSDYNDISK_ID = 92,
  DPFLTR_VERIFIER_ID = 93,
  DPFLTR_VDS_ID = 94,
  DPFLTR_VDSBAS_ID = 95,
  DPFLTR_VDSDYN_ID = 96,
  DPFLTR_VDSDYNDR_ID = 97,
  DPFLTR_VDSLDR_ID = 98,
  DPFLTR_VDSUTIL_ID = 99,
  DPFLTR_DFRGIFC_ID = 100,
  DPFLTR_DEFAULT_ID = 101,
  DPFLTR_MM_ID = 102,
  DPFLTR_DFSC_ID = 103,
  DPFLTR_WOW64_ID = 104,
  DPFLTR_ALPC_ID = 105,
  DPFLTR_WDI_ID = 106,
  DPFLTR_PERFLIB_ID = 107,
  DPFLTR_KTM_ID = 108,
  DPFLTR_IOSTRESS_ID = 109,
  DPFLTR_HEAP_ID = 110,
  DPFLTR_WHEA_ID = 111,
  DPFLTR_USERGDI_ID = 112,
  DPFLTR_MMCSS_ID = 113,
  DPFLTR_TPM_ID = 114,
  DPFLTR_THREADORDER_ID = 115,
  DPFLTR_ENVIRON_ID = 116,
  DPFLTR_EMS_ID = 117,
  DPFLTR_WDT_ID = 118,
  DPFLTR_FVEVOL_ID = 119,
  DPFLTR_NDIS_ID = 120,
  DPFLTR_NVCTRACE_ID = 121,
  DPFLTR_LUAFV_ID = 122,
  DPFLTR_APPCOMPAT_ID = 123,
  DPFLTR_USBSTOR_ID = 124,
  DPFLTR_SBP2PORT_ID = 125,
  DPFLTR_COVERAGE_ID = 126,
  DPFLTR_CACHEMGR_ID = 127,
  DPFLTR_MOUNTMGR_ID = 128,
  DPFLTR_CFR_ID = 129,
  DPFLTR_TXF_ID = 130,
  DPFLTR_KSECDD_ID = 131,
  DPFLTR_FLTREGRESS_ID = 132,
  DPFLTR_MPIO_ID = 133,
  DPFLTR_MSDSM_ID = 134,
  DPFLTR_UDFS_ID = 135,
  DPFLTR_PSHED_ID = 136,
  DPFLTR_STORVSP_ID = 137,
  DPFLTR_LSASS_ID = 138,
  DPFLTR_SSPICLI_ID = 139,
  DPFLTR_CNG_ID = 140,
  DPFLTR_EXFAT_ID = 141,
  DPFLTR_FILETRACE_ID = 142,
  DPFLTR_XSAVE_ID = 143,
  DPFLTR_SE_ID = 144,
  DPFLTR_DRIVEEXTENDER_ID = 145,
  DPFLTR_ENDOFTABLE_ID
} DPFLTR_TYPE;
