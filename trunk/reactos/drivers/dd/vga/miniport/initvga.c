#include <ntddk.h>
#include <debug.h>
#include "vgavideo.h"

void outxay(USHORT ad, UCHAR x, UCHAR y)
{
  USHORT xy = (x << 8) + y;

  VideoPortWritePortUshort((PUSHORT)ad, xy);
}

void setMode(VideoMode mode)
{
  unsigned char x;
  unsigned int y, c, a, m, n;

  VideoPortWritePortUchar((PUCHAR)MISC, mode.Misc);
  VideoPortWritePortUchar((PUCHAR)STATUS, 0);
  VideoPortWritePortUchar((PUCHAR)FEATURE, mode.Feature);

  for(x=0; x<5; x++)
  {
    outxay(SEQ, mode.Seq[x], x);
  }

  VideoPortWritePortUshort((PUSHORT)CRTC, 0x11);
  VideoPortWritePortUshort((PUSHORT)CRTC, (mode.Crtc[0x11] & 0x7f));

  for(x=0; x<25; x++)
  {
    outxay(CRTC, mode.Crtc[x], x);
  }

  for(x=0; x<9; x++)
  {
    outxay(GRAPHICS, mode.Gfx[x], x);
  }

  x=VideoPortReadPortUchar((PUCHAR)FEATURE);

  for(x=0; x<21; x++)
  {
    VideoPortWritePortUchar((PUCHAR)ATTRIB, x);
    VideoPortWritePortUchar((PUCHAR)ATTRIB, mode.Attrib[x]);
  }

  x=VideoPortReadPortUchar((PUCHAR)STATUS);

  VideoPortWritePortUchar((PUCHAR)ATTRIB, 0x20);
}

VideoMode Mode12 = {
    0xa000, /* 0xe3, */ 0xc3, 0x00,

    {0x02, 0x01, 0x0f, 0x00, 0x06 },

    {0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x59, 0xea, 0x8c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3,
     0xff},

    {0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x05, 0x0f, 0xff},

    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
     0x0c, 0x0d, 0x0e, 0x0f, 0x81, 0x00, 0x0f, 0x00, 0x00}
};

VideoMode Mode13 = {
    0xa000, 0x63, 0x00,

    {0x03, 0x01, 0x0f, 0x00, 0x0e},

    {0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f, 0x00, 0x41, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x9c, 0x0e, 0x8f, 0x28, 0x40, 0x96, 0xb9, 0xa3,
     0xff},

    {0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x07, 0x0f, 0xff},

    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
     0x0c, 0x0d, 0x0e, 0x0f, 0x41, 0x00, 0x0f, 0x00, 0x00}
};

VideoMode Mode3 = {
  0xb800, 0x67, 0x00,
  { 0x03, 0x00, 0x3, 0x00, 0x2 },
  
  { 0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x1f, 0x00, 0x4f, 0x0e, 0x0f,
    0x00, 0x00, 0x00, 0x00, 0x9c, 0x0e, 0x8f, 0x28, 0x1f, 0x96, 0xb9, 0xa3,
    0xff },
  
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x00, 0xff },
  
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x0c, 0x00, 0x0f, 0x08, 0x00 }
};

void InitVGAMode()
{
   int i;

   // FIXME: Use Vidport to map the memory properly
   vidmem = (char *)(0xd0000000 + 0xa0000);
   setMode(Mode12);

   WRITE_PORT_USHORT((PUSHORT)0x3C4, 0x0f02); // index=MASK MAP, write to all bitplanes
   i = vidmem[0];
//   VideoPortZeroMemory(vidmem, 38400);
   VideoPortZeroMemory(vidmem, 64000);

   vgaPreCalc();
}


VOID  VGAResetDevice(OUT PSTATUS_BLOCK  StatusBlock)
{
  char *vidmem;
  HANDLE Event;
  OBJECT_ATTRIBUTES Attr;
  UNICODE_STRING Name;
  NTSTATUS Status;
  
  CHECKPOINT;
  Event = 0;
  setMode( Mode3 );
  RtlInitUnicodeString( &Name, L"\\TextConsoleRefreshEvent" );
  InitializeObjectAttributes( &Attr, &Name, 0, 0, 0 );
  Status = NtOpenEvent( &Event, STANDARD_RIGHTS_ALL, &Attr );
  if( !NT_SUCCESS( Status ) )
    DbgPrint( "VGA: Failed to open refresh event\n" );
  else {
    NtSetEvent( Event, 1 );
    NtClose( Event );
  }
}



