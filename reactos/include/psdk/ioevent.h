/*
 * ioevent.h
 *
 * PnP Event Notification GUIDs
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

#ifndef _IOEVENT_H
#define _IOEVENT_H

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(GUID_IO_VOLUME_CHANGE,
  0x7373654AL, 0x812A, 0x11D0, 0xBE, 0xC7, 0x08, 0x00, 0x2B, 0xE2, 0x09, 0x2F);
DEFINE_GUID(GUID_IO_VOLUME_DISMOUNT,
  0xD16A55E8L, 0x1059, 0x11D2, 0x8F, 0xFD, 0x00, 0xA0, 0xC9, 0xA0, 0x6D, 0x32);
DEFINE_GUID(GUID_IO_VOLUME_DISMOUNT_FAILED,
  0xE3C5B178L, 0x105D, 0x11D2, 0x8F, 0xFD, 0x00, 0xA0, 0xC9, 0xA0, 0x6D, 0x32);
DEFINE_GUID(GUID_IO_VOLUME_MOUNT,
  0xB5804878L, 0x1A96, 0x11D2, 0x8F, 0xFD, 0x00, 0xA0, 0xC9, 0xA0, 0x6D, 0x32);
DEFINE_GUID(GUID_IO_VOLUME_LOCK,
  0x50708874L, 0xC9AF, 0x11D1, 0x8F, 0xEF, 0x00, 0xA0, 0xC9, 0xA0, 0x6D, 0x32);
DEFINE_GUID(GUID_IO_VOLUME_LOCK_FAILED,
  0xAE2EED10L, 0x0BA8, 0x11D2, 0x8F, 0xFB, 0x00, 0xA0, 0xC9, 0xA0, 0x6D, 0x32);
DEFINE_GUID(GUID_IO_VOLUME_UNLOCK,
  0x9A8C3D68L, 0xD0CB, 0x11D1, 0x8F, 0xEF, 0x00, 0xA0, 0xC9, 0xA0, 0x6D, 0x32);
DEFINE_GUID(GUID_IO_VOLUME_NAME_CHANGE,
  0x2DE97F83, 0x4C06, 0x11D2, 0xA5, 0x32, 0x00, 0x60, 0x97, 0x13, 0x05, 0x5A);
DEFINE_GUID(GUID_IO_VOLUME_PREPARING_EJECT,
  0xC79EB16E, 0x0DAC, 0x4E7A, 0xA8, 0x6C, 0xB2, 0x5C, 0xEE, 0xAA, 0x88, 0xF6);
DEFINE_GUID(GUID_IO_VOLUME_PHYSICAL_CONFIGURATION_CHANGE,
  0x2DE97F84, 0x4C06, 0x11D2, 0xA5, 0x32, 0x00, 0x60, 0x97, 0x13, 0x05, 0x5A);
DEFINE_GUID(GUID_IO_VOLUME_FVE_STATUS_CHANGE,
  0x062998B2, 0xEE1F, 0x4B6A, 0xB8, 0x57, 0xE7, 0x6C, 0xBB, 0xE9, 0xA6, 0xDA);
DEFINE_GUID(GUID_IO_VOLUME_DEVICE_INTERFACE,
  0x53F5630D, 0xB6BF, 0x11D0, 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B);
DEFINE_GUID(GUID_IO_VOLUME_CHANGE_SIZE,
  0x3A1625BE, 0xAD03, 0x49F1, 0x8E, 0xF8, 0x6B, 0xBA, 0xC1, 0x82, 0xD1, 0xFD);
DEFINE_GUID(GUID_IO_MEDIA_ARRIVAL,
  0xD07433C0, 0xA98E, 0x11D2, 0x91, 0x7A, 0x00, 0xA0, 0xC9, 0x06, 0x8F, 0xF3);
DEFINE_GUID(GUID_IO_MEDIA_REMOVAL,
  0xD07433C1, 0xA98E, 0x11D2, 0x91, 0x7A, 0x00, 0xA0, 0xC9, 0x06, 0x8F, 0xF3);
DEFINE_GUID(GUID_IO_CDROM_EXCLUSIVE_LOCK,
  0xBC56C139, 0x7A10, 0x47EE, 0xA2, 0x94, 0x4C, 0x6A, 0x38, 0xF0, 0x14, 0x9A);
DEFINE_GUID(GUID_IO_CDROM_EXCLUSIVE_UNLOCK,
  0xA3B6D27D, 0x5E35, 0x4885, 0x81, 0xE5, 0xEE, 0x18, 0xC0, 0x0E, 0xD7, 0x79);
DEFINE_GUID(GUID_IO_DEVICE_BECOMING_READY,
  0xD07433F0, 0xA98E, 0x11D2, 0x91, 0x7A, 0x00, 0xA0, 0xC9, 0x06, 0x8F, 0xF3);
DEFINE_GUID(GUID_IO_DEVICE_EXTERNAL_REQUEST,
  0xD07433D0, 0xA98E, 0x11D2, 0x91, 0x7A, 0x00, 0xA0, 0xC9, 0x06, 0x8F, 0xF3);
DEFINE_GUID(GUID_IO_MEDIA_EJECT_REQUEST,
  0xD07433D1, 0xA98E, 0x11D2, 0x91, 0x7A, 0x00, 0xA0, 0xC9, 0x06, 0x8F, 0xF3);
DEFINE_GUID(GUID_IO_DRIVE_REQUIRES_CLEANING,
  0x7207877C, 0x90ED, 0x44E5, 0xA0, 0x00, 0x81, 0x42, 0x8D, 0x4C, 0x79, 0xBB);
DEFINE_GUID(GUID_IO_TAPE_ERASE,
  0x852D11EB, 0x4BB8, 0x4507, 0x9D, 0x9B, 0x41, 0x7C, 0xC2, 0xB1, 0xB4, 0x38);
DEFINE_GUID(GUID_IO_DISK_CLONE_ARRIVAL,
  0x6A61885B, 0x7C39, 0x43DD, 0x9B, 0x56, 0xB8, 0xAC, 0x22, 0xA5, 0x49, 0xAA);
DEFINE_GUID(GUID_IO_DISK_LAYOUT_CHANGE,
  0x11DFF54C, 0x8469, 0x41F9, 0xB3, 0xDE, 0xEF, 0x83, 0x64, 0x87, 0xC5, 0x4A);

#ifdef __cplusplus
}
#endif

#endif /* _IOEVENT_H */
