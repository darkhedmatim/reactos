
#ifndef _NTDDBEEP_
#define _NTDDBEEP_


#ifdef __cplusplus
extern "C" {
#endif

#define DD_BEEP_DEVICE_NAME    "\\Device\\Beep"
#define DD_BEEP_DEVICE_NAME_U  L"\\Device\\Beep"
#define BEEP_FREQUENCY_MINIMUM 0x25
#define BEEP_FREQUENCY_MAXIMUM 0x7FFF
#define IOCTL_BEEP_SET         CTL_CODE(FILE_DEVICE_BEEP, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _BEEP_SET_PARAMETERS 
{
   ULONG Frequency;
   ULONG Duration;
} BEEP_SET_PARAMETERS, *PBEEP_SET_PARAMETERS;

#ifdef __cplusplus
}
#endif
#endif 

