// Taken from FreeType 2 (www.freetype.org)

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/bitmaps.h>
#include "../eng/objects.h"

//#define NDEBUG
#include <win32k/debug1.h>

// #include "grfont.h"
// #include <string.h>

/* font characters */

const unsigned char  font_8x8[2048] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7E, 0x81, 0xA5, 0x81, 0xBD, 0x99, 0x81, 0x7E,
    0x7E, 0xFF, 0xDB, 0xFF, 0xC3, 0xE7, 0xFF, 0x7E,
    0x6C, 0xFE, 0xFE, 0xFE, 0x7C, 0x38, 0x10, 0x00,
    0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x38, 0x10, 0x00,
    0x38, 0x7C, 0x38, 0xFE, 0xFE, 0x92, 0x10, 0x7C,
    0x00, 0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x38, 0x7C,
    0x00, 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x00, 0x00,
    0xFF, 0xFF, 0xE7, 0xC3, 0xC3, 0xE7, 0xFF, 0xFF,
    0x00, 0x3C, 0x66, 0x42, 0x42, 0x66, 0x3C, 0x00,
    0xFF, 0xC3, 0x99, 0xBD, 0xBD, 0x99, 0xC3, 0xFF,
    0x0F, 0x07, 0x0F, 0x7D, 0xCC, 0xCC, 0xCC, 0x78,
    0x3C, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x7E, 0x18,
    0x3F, 0x33, 0x3F, 0x30, 0x30, 0x70, 0xF0, 0xE0,
    0x7F, 0x63, 0x7F, 0x63, 0x63, 0x67, 0xE6, 0xC0,
    0x99, 0x5A, 0x3C, 0xE7, 0xE7, 0x3C, 0x5A, 0x99,
    0x80, 0xE0, 0xF8, 0xFE, 0xF8, 0xE0, 0x80, 0x00,
    0x02, 0x0E, 0x3E, 0xFE, 0x3E, 0x0E, 0x02, 0x00,
    0x18, 0x3C, 0x7E, 0x18, 0x18, 0x7E, 0x3C, 0x18,
    0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x00,
    0x7F, 0xDB, 0xDB, 0x7B, 0x1B, 0x1B, 0x1B, 0x00,
    0x3E, 0x63, 0x38, 0x6C, 0x6C, 0x38, 0x86, 0xFC,
    0x00, 0x00, 0x00, 0x00, 0x7E, 0x7E, 0x7E, 0x00,
    0x18, 0x3C, 0x7E, 0x18, 0x7E, 0x3C, 0x18, 0xFF,
    0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x00,
    0x18, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00,
    0x00, 0x18, 0x0C, 0xFE, 0x0C, 0x18, 0x00, 0x00,
    0x00, 0x30, 0x60, 0xFE, 0x60, 0x30, 0x00, 0x00,
    0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xFE, 0x00, 0x00,
    0x00, 0x24, 0x66, 0xFF, 0x66, 0x24, 0x00, 0x00,
    0x00, 0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00,
    0x6C, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00,
    0x18, 0x7E, 0xC0, 0x7C, 0x06, 0xFC, 0x18, 0x00,
    0x00, 0xC6, 0xCC, 0x18, 0x30, 0x66, 0xC6, 0x00,
    0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00,
    0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x18, 0x30, 0x60, 0x60, 0x60, 0x30, 0x18, 0x00,
    0x60, 0x30, 0x18, 0x18, 0x18, 0x30, 0x60, 0x00,
    0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00,
    0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30,
    0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00,
    0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00,
    0x7C, 0xCE, 0xDE, 0xF6, 0xE6, 0xC6, 0x7C, 0x00,
    0x30, 0x70, 0x30, 0x30, 0x30, 0x30, 0xFC, 0x00,
    0x78, 0xCC, 0x0C, 0x38, 0x60, 0xCC, 0xFC, 0x00,
    0x78, 0xCC, 0x0C, 0x38, 0x0C, 0xCC, 0x78, 0x00,
    0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x1E, 0x00,
    0xFC, 0xC0, 0xF8, 0x0C, 0x0C, 0xCC, 0x78, 0x00,
    0x38, 0x60, 0xC0, 0xF8, 0xCC, 0xCC, 0x78, 0x00,
    0xFC, 0xCC, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00,
    0x78, 0xCC, 0xCC, 0x78, 0xCC, 0xCC, 0x78, 0x00,
    0x78, 0xCC, 0xCC, 0x7C, 0x0C, 0x18, 0x70, 0x00,
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00,
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30,
    0x18, 0x30, 0x60, 0xC0, 0x60, 0x30, 0x18, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x60, 0x30, 0x18, 0x0C, 0x18, 0x30, 0x60, 0x00,
    0x3C, 0x66, 0x0C, 0x18, 0x18, 0x00, 0x18, 0x00,
    0x7C, 0xC6, 0xDE, 0xDE, 0xDC, 0xC0, 0x7C, 0x00,
    0x30, 0x78, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0x00,
    0xFC, 0x66, 0x66, 0x7C, 0x66, 0x66, 0xFC, 0x00,
    0x3C, 0x66, 0xC0, 0xC0, 0xC0, 0x66, 0x3C, 0x00,
    0xF8, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00,
    0xFE, 0x62, 0x68, 0x78, 0x68, 0x62, 0xFE, 0x00,
    0xFE, 0x62, 0x68, 0x78, 0x68, 0x60, 0xF0, 0x00,
    0x3C, 0x66, 0xC0, 0xC0, 0xCE, 0x66, 0x3A, 0x00,
    0xCC, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0xCC, 0x00,
    0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00,
    0x1E, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78, 0x00,
    0xE6, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0xE6, 0x00,
    0xF0, 0x60, 0x60, 0x60, 0x62, 0x66, 0xFE, 0x00,
    0xC6, 0xEE, 0xFE, 0xFE, 0xD6, 0xC6, 0xC6, 0x00,
    0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00,
    0x38, 0x6C, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00,
    0xFC, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00,
    0x7C, 0xC6, 0xC6, 0xC6, 0xD6, 0x7C, 0x0E, 0x00,
    0xFC, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0xE6, 0x00,
    0x7C, 0xC6, 0xE0, 0x78, 0x0E, 0xC6, 0x7C, 0x00,
    0xFC, 0xB4, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFC, 0x00,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00,
    0xC6, 0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0x6C, 0x00,
    0xC6, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0xC6, 0x00,
    0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x30, 0x78, 0x00,
    0xFE, 0xC6, 0x8C, 0x18, 0x32, 0x66, 0xFE, 0x00,
    0x78, 0x60, 0x60, 0x60, 0x60, 0x60, 0x78, 0x00,
    0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00,
    0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00,
    0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00,
    0xE0, 0x60, 0x60, 0x7C, 0x66, 0x66, 0xDC, 0x00,
    0x00, 0x00, 0x78, 0xCC, 0xC0, 0xCC, 0x78, 0x00,
    0x1C, 0x0C, 0x0C, 0x7C, 0xCC, 0xCC, 0x76, 0x00,
    0x00, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00,
    0x38, 0x6C, 0x64, 0xF0, 0x60, 0x60, 0xF0, 0x00,
    0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8,
    0xE0, 0x60, 0x6C, 0x76, 0x66, 0x66, 0xE6, 0x00,
    0x30, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00,
    0x0C, 0x00, 0x1C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78,
    0xE0, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0xE6, 0x00,
    0x70, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00,
    0x00, 0x00, 0xCC, 0xFE, 0xFE, 0xD6, 0xD6, 0x00,
    0x00, 0x00, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0x00,
    0x00, 0x00, 0x78, 0xCC, 0xCC, 0xCC, 0x78, 0x00,
    0x00, 0x00, 0xDC, 0x66, 0x66, 0x7C, 0x60, 0xF0,
    0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0x1E,
    0x00, 0x00, 0xDC, 0x76, 0x62, 0x60, 0xF0, 0x00,
    0x00, 0x00, 0x7C, 0xC0, 0x70, 0x1C, 0xF8, 0x00,
    0x10, 0x30, 0xFC, 0x30, 0x30, 0x34, 0x18, 0x00,
    0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00,
    0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00,
    0x00, 0x00, 0xC6, 0xC6, 0xD6, 0xFE, 0x6C, 0x00,
    0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00,
    0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8,
    0x00, 0x00, 0xFC, 0x98, 0x30, 0x64, 0xFC, 0x00,
    0x1C, 0x30, 0x30, 0xE0, 0x30, 0x30, 0x1C, 0x00,
    0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00,
    0xE0, 0x30, 0x30, 0x1C, 0x30, 0x30, 0xE0, 0x00,
    0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0x00,
    0x7C, 0xC6, 0xC0, 0xC6, 0x7C, 0x0C, 0x06, 0x7C,
    0x00, 0xCC, 0x00, 0xCC, 0xCC, 0xCC, 0x76, 0x00,
    0x1C, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00,
    0x7E, 0x81, 0x3C, 0x06, 0x3E, 0x66, 0x3B, 0x00,
    0xCC, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00,
    0xE0, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00,
    0x30, 0x30, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00,
    0x00, 0x00, 0x7C, 0xC6, 0xC0, 0x78, 0x0C, 0x38,
    0x7E, 0x81, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00,
    0xCC, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00,
    0xE0, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00,
    0xCC, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00,
    0x7C, 0x82, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00,
    0xE0, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00,
    0xC6, 0x10, 0x7C, 0xC6, 0xFE, 0xC6, 0xC6, 0x00,
    0x30, 0x30, 0x00, 0x78, 0xCC, 0xFC, 0xCC, 0x00,
    0x1C, 0x00, 0xFC, 0x60, 0x78, 0x60, 0xFC, 0x00,
    0x00, 0x00, 0x7F, 0x0C, 0x7F, 0xCC, 0x7F, 0x00,
    0x3E, 0x6C, 0xCC, 0xFE, 0xCC, 0xCC, 0xCE, 0x00,
    0x78, 0x84, 0x00, 0x78, 0xCC, 0xCC, 0x78, 0x00,
    0x00, 0xCC, 0x00, 0x78, 0xCC, 0xCC, 0x78, 0x00,
    0x00, 0xE0, 0x00, 0x78, 0xCC, 0xCC, 0x78, 0x00,
    0x78, 0x84, 0x00, 0xCC, 0xCC, 0xCC, 0x76, 0x00,
    0x00, 0xE0, 0x00, 0xCC, 0xCC, 0xCC, 0x76, 0x00,
    0x00, 0xCC, 0x00, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8,
    0xC3, 0x18, 0x3C, 0x66, 0x66, 0x3C, 0x18, 0x00,
    0xCC, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x00,
    0x18, 0x18, 0x7E, 0xC0, 0xC0, 0x7E, 0x18, 0x18,
    0x38, 0x6C, 0x64, 0xF0, 0x60, 0xE6, 0xFC, 0x00,
    0xCC, 0xCC, 0x78, 0x30, 0xFC, 0x30, 0xFC, 0x30,
    0xF8, 0xCC, 0xCC, 0xFA, 0xC6, 0xCF, 0xC6, 0xC3,
    0x0E, 0x1B, 0x18, 0x3C, 0x18, 0x18, 0xD8, 0x70,
    0x1C, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00,
    0x38, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00,
    0x00, 0x1C, 0x00, 0x78, 0xCC, 0xCC, 0x78, 0x00,
    0x00, 0x1C, 0x00, 0xCC, 0xCC, 0xCC, 0x76, 0x00,
    0x00, 0xF8, 0x00, 0xB8, 0xCC, 0xCC, 0xCC, 0x00,
    0xFC, 0x00, 0xCC, 0xEC, 0xFC, 0xDC, 0xCC, 0x00,
    0x3C, 0x6C, 0x6C, 0x3E, 0x00, 0x7E, 0x00, 0x00,
    0x38, 0x6C, 0x6C, 0x38, 0x00, 0x7C, 0x00, 0x00,
    0x18, 0x00, 0x18, 0x18, 0x30, 0x66, 0x3C, 0x00,
    0x00, 0x00, 0x00, 0xFC, 0xC0, 0xC0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFC, 0x0C, 0x0C, 0x00, 0x00,
    0xC6, 0xCC, 0xD8, 0x36, 0x6B, 0xC2, 0x84, 0x0F,
    0xC3, 0xC6, 0xCC, 0xDB, 0x37, 0x6D, 0xCF, 0x03,
    0x18, 0x00, 0x18, 0x18, 0x3C, 0x3C, 0x18, 0x00,
    0x00, 0x33, 0x66, 0xCC, 0x66, 0x33, 0x00, 0x00,
    0x00, 0xCC, 0x66, 0x33, 0x66, 0xCC, 0x00, 0x00,
    0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88,
    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
    0xDB, 0xF6, 0xDB, 0x6F, 0xDB, 0x7E, 0xD7, 0xED,
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18, 0xF8, 0x18, 0x18, 0x18,
    0x18, 0x18, 0xF8, 0x18, 0xF8, 0x18, 0x18, 0x18,
    0x36, 0x36, 0x36, 0x36, 0xF6, 0x36, 0x36, 0x36,
    0x00, 0x00, 0x00, 0x00, 0xFE, 0x36, 0x36, 0x36,
    0x00, 0x00, 0xF8, 0x18, 0xF8, 0x18, 0x18, 0x18,
    0x36, 0x36, 0xF6, 0x06, 0xF6, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x00, 0x00, 0xFE, 0x06, 0xF6, 0x36, 0x36, 0x36,
    0x36, 0x36, 0xF6, 0x06, 0xFE, 0x00, 0x00, 0x00,
    0x36, 0x36, 0x36, 0x36, 0xFE, 0x00, 0x00, 0x00,
    0x18, 0x18, 0xF8, 0x18, 0xF8, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF8, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18, 0x1F, 0x00, 0x00, 0x00,
    0x18, 0x18, 0x18, 0x18, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18, 0x1F, 0x18, 0x18, 0x18,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x18, 0x18, 0x18, 0x18, 0xFF, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x1F, 0x18, 0x1F, 0x18, 0x18, 0x18,
    0x36, 0x36, 0x36, 0x36, 0x37, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x37, 0x30, 0x3F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x3F, 0x30, 0x37, 0x36, 0x36, 0x36,
    0x36, 0x36, 0xF7, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0x00, 0xF7, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x37, 0x30, 0x37, 0x36, 0x36, 0x36,
    0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x36, 0x36, 0xF7, 0x00, 0xF7, 0x36, 0x36, 0x36,
    0x18, 0x18, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x36, 0x36, 0x36, 0x36, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0x00, 0xFF, 0x18, 0x18, 0x18,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x3F, 0x00, 0x00, 0x00,
    0x18, 0x18, 0x1F, 0x18, 0x1F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1F, 0x18, 0x1F, 0x18, 0x18, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0xFF, 0x36, 0x36, 0x36,
    0x18, 0x18, 0xFF, 0x18, 0xFF, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18, 0xF8, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1F, 0x18, 0x18, 0x18,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x76, 0xDC, 0xC8, 0xDC, 0x76, 0x00,
    0x00, 0x78, 0xCC, 0xF8, 0xCC, 0xF8, 0xC0, 0xC0,
    0x00, 0xFC, 0xCC, 0xC0, 0xC0, 0xC0, 0xC0, 0x00,
    0x00, 0x00, 0xFE, 0x6C, 0x6C, 0x6C, 0x6C, 0x00,
    0xFC, 0xCC, 0x60, 0x30, 0x60, 0xCC, 0xFC, 0x00,
    0x00, 0x00, 0x7E, 0xD8, 0xD8, 0xD8, 0x70, 0x00,
    0x00, 0x66, 0x66, 0x66, 0x66, 0x7C, 0x60, 0xC0,
    0x00, 0x76, 0xDC, 0x18, 0x18, 0x18, 0x18, 0x00,
    0xFC, 0x30, 0x78, 0xCC, 0xCC, 0x78, 0x30, 0xFC,
    0x38, 0x6C, 0xC6, 0xFE, 0xC6, 0x6C, 0x38, 0x00,
    0x38, 0x6C, 0xC6, 0xC6, 0x6C, 0x6C, 0xEE, 0x00,
    0x1C, 0x30, 0x18, 0x7C, 0xCC, 0xCC, 0x78, 0x00,
    0x00, 0x00, 0x7E, 0xDB, 0xDB, 0x7E, 0x00, 0x00,
    0x06, 0x0C, 0x7E, 0xDB, 0xDB, 0x7E, 0x60, 0xC0,
    0x38, 0x60, 0xC0, 0xF8, 0xC0, 0x60, 0x38, 0x00,
    0x78, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x00,
    0x00, 0x7E, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00,
    0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x7E, 0x00,
    0x60, 0x30, 0x18, 0x30, 0x60, 0x00, 0xFC, 0x00,
    0x18, 0x30, 0x60, 0x30, 0x18, 0x00, 0xFC, 0x00,
    0x0E, 0x1B, 0x1B, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18, 0x18, 0xD8, 0xD8, 0x70,
    0x18, 0x18, 0x00, 0x7E, 0x00, 0x18, 0x18, 0x00,
    0x00, 0x76, 0xDC, 0x00, 0x76, 0xDC, 0x00, 0x00,
    0x38, 0x6C, 0x6C, 0x38, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x0F, 0x0C, 0x0C, 0x0C, 0xEC, 0x6C, 0x3C, 0x1C,
    0x58, 0x6C, 0x6C, 0x6C, 0x6C, 0x00, 0x00, 0x00,
    0x70, 0x98, 0x30, 0x60, 0xF8, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x3C, 0x3C, 0x3C, 0x3C, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static PSURFOBJ CharCellSurfObj;
static HBITMAP  hCharCellBitmap;

HBITMAP BitmapToSurf(PBITMAPOBJ Bitmap);

// Set things up for a character cell surface
void CreateCellCharSurface()
{
   PBITMAPOBJ pbo;
   HBITMAP    bm;

   hCharCellBitmap = W32kCreateBitmap(8, 8, 1, 8, NULL); // 8x8, 1 plane, 8 bits per pel

   pbo = BITMAPOBJ_HandleToPtr(hCharCellBitmap);
   ASSERT( pbo );
   bm = BitmapToSurf(pbo); // Make the bitmap a surface
   CharCellSurfObj = (PSURFOBJ) AccessUserObject((ULONG) bm);
   BITMAPOBJ_ReleasePtr( hCharCellBitmap );
}

void  grWriteCellChar(PSURFOBJ  target,
                      int        x,
                      int        y,
                      int        charcode,
                      COLORREF   color)
{
   RECTL     DestRect;
   POINTL    SourcePoint;

   char *bigbit, *mover;
   unsigned char thebyte, thebit, idxColor;
   int i, j, charcode8, i8;

   if (charcode < 0 || charcode > 255)
      return;

   DestRect.left   = x;
   DestRect.top    = y;
   DestRect.right  = x+8;
   DestRect.bottom = y+8;

   SourcePoint.x = 0;
   SourcePoint.y = 0;

   bigbit = ExAllocatePool(NonPagedPool, 256);

   // Explode the 1BPP character cell into an 8BPP one due to current GDI limitations.. major FIXME
   charcode8 = charcode * 8;
   mover = bigbit;

   for (i=0; i<8; i++)
   {
      thebyte = font_8x8[charcode8 + i];
      i8 = i*8;
      for (j=8; j>0; j--)
      {
         thebit = thebyte & (1 << j);
         if(thebit>0) thebit = 1;
         *mover = thebit;
         mover++;
      }
   }

   W32kSetBitmapBits(hCharCellBitmap, 256, bigbit);

   // Blt bitmap to screen
   EngBitBlt(target, CharCellSurfObj, NULL, NULL, NULL, &DestRect, &SourcePoint, NULL, NULL, NULL, NULL);

   ExFreePool(bigbit);
}

void  grWriteCellString(PSURFOBJ  target,
                        int         x,
                        int         y,
                        const char* string,
                        COLORREF    color)
{
   while (*string)
   {
      grWriteCellChar(target, x, y, *string++, color);
      x += 8;
   }
}

static int        gr_cursor_x     = 0;
static int        gr_cursor_y     = 0;
static PBITMAPOBJ gr_text_bitmap  = 0;
static int        gr_margin_right = 0;
static int        gr_margin_top   = 0;

void grGotobitmap(PBITMAPOBJ bitmap )
{
   gr_text_bitmap = bitmap;
}

void grSetMargin(int right, int top)
{
   gr_margin_top   = top << 3;
   gr_margin_right = right << 3;
}

void grGotoxy(int x, int y)
{
   gr_cursor_x = x;
   gr_cursor_y = y;
}

void grWrite(const char* string)
{
   if (string)
   {
	  COLORREF color;

//	  color.value = 127;
      grWriteCellString( gr_text_bitmap,
                         gr_margin_right + (gr_cursor_x << 3),
                         gr_margin_top   + (gr_cursor_y << 3),
                         string,
                         color );

      gr_cursor_x += strlen(string);
   }
}

void grLn()
{
   gr_cursor_y ++;
   gr_cursor_x = 0;
}

void grWriteln(const char* string)
{
   grWrite( string );
   grLn();
}
