/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            vga.c
 * PURPOSE:         VGA hardware emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "vga.h"

#include "io.h"
#include "bios.h"

/* PRIVATE VARIABLES **********************************************************/

static CONST DWORD MemoryBase[]  = { 0xA0000, 0xA0000, 0xB0000, 0xB8000 };
static CONST DWORD MemoryLimit[] = { 0xAFFFF, 0xAFFFF, 0xB7FFF, 0xBFFFF };

#define USE_REACTOS_COLORS
// #define USE_DOSBOX_COLORS

#if defined(USE_REACTOS_COLORS)

// ReactOS colors
static CONST COLORREF VgaDefaultPalette[VGA_MAX_COLORS] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0x00), RGB(0x10, 0x10, 0x10), RGB(0x20, 0x20, 0x20), RGB(0x35, 0x35, 0x35),
    RGB(0x45, 0x45, 0x45), RGB(0x55, 0x55, 0x55), RGB(0x65, 0x65, 0x65), RGB(0x75, 0x75, 0x75),
    RGB(0x8A, 0x8A, 0x8A), RGB(0x9A, 0x9A, 0x9A), RGB(0xAA, 0xAA, 0xAA), RGB(0xBA, 0xBA, 0xBA),
    RGB(0xCA, 0xCA, 0xCA), RGB(0xDF, 0xDF, 0xDF), RGB(0xEF, 0xEF, 0xEF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0xFF), RGB(0x41, 0x00, 0xFF), RGB(0x82, 0x00, 0xFF), RGB(0xBE, 0x00, 0xFF),
    RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0x00, 0xBE), RGB(0xFF, 0x00, 0x82), RGB(0xFF, 0x00, 0x41),
    RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x41, 0x00), RGB(0xFF, 0x82, 0x00), RGB(0xFF, 0xBE, 0x00),
    RGB(0xFF, 0xFF, 0x00), RGB(0xBE, 0xFF, 0x00), RGB(0x82, 0xFF, 0x00), RGB(0x41, 0xFF, 0x00),
    RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0x41), RGB(0x00, 0xFF, 0x82), RGB(0x00, 0xFF, 0xBE),
    RGB(0x00, 0xFF, 0xFF), RGB(0x00, 0xBE, 0xFF), RGB(0x00, 0x82, 0xFF), RGB(0x00, 0x41, 0xFF),
    RGB(0x82, 0x82, 0xFF), RGB(0x9E, 0x82, 0xFF), RGB(0xBE, 0x82, 0xFF), RGB(0xDF, 0x82, 0xFF),
    RGB(0xFF, 0x82, 0xFF), RGB(0xFF, 0x82, 0xDF), RGB(0xFF, 0x82, 0xBE), RGB(0xFF, 0x82, 0x9E),
    RGB(0xFF, 0x82, 0x82), RGB(0xFF, 0x9E, 0x82), RGB(0xFF, 0xBE, 0x82), RGB(0xFF, 0xDF, 0x82),
    RGB(0xFF, 0xFF, 0x82), RGB(0xDF, 0xFF, 0x82), RGB(0xBE, 0xFF, 0x82), RGB(0x9E, 0xFF, 0x82),
    RGB(0x82, 0xFF, 0x82), RGB(0x82, 0xFF, 0x9E), RGB(0x82, 0xFF, 0xBE), RGB(0x82, 0xFF, 0xDF),
    RGB(0x82, 0xFF, 0xFF), RGB(0x82, 0xDF, 0xFF), RGB(0x82, 0xBE, 0xFF), RGB(0x82, 0x9E, 0xFF),
    RGB(0xBA, 0xBA, 0xFF), RGB(0xCA, 0xBA, 0xFF), RGB(0xDF, 0xBA, 0xFF), RGB(0xEF, 0xBA, 0xFF),
    RGB(0xFF, 0xBA, 0xFF), RGB(0xFF, 0xBA, 0xEF), RGB(0xFF, 0xBA, 0xDF), RGB(0xFF, 0xBA, 0xCA),
    RGB(0xFF, 0xBA, 0xBA), RGB(0xFF, 0xCA, 0xBA), RGB(0xFF, 0xDF, 0xBA), RGB(0xFF, 0xEF, 0xBA),
    RGB(0xFF, 0xFF, 0xBA), RGB(0xEF, 0xFF, 0xBA), RGB(0xDF, 0xFF, 0xBA), RGB(0xCA, 0xFF, 0xBA),
    RGB(0xBA, 0xFF, 0xBA), RGB(0xBA, 0xFF, 0xCA), RGB(0xBA, 0xFF, 0xDF), RGB(0xBA, 0xFF, 0xEF),
    RGB(0xBA, 0xFF, 0xFF), RGB(0xBA, 0xEF, 0xFF), RGB(0xBA, 0xDF, 0xFF), RGB(0xBA, 0xCA, 0xFF),
    RGB(0x00, 0x00, 0x71), RGB(0x1C, 0x00, 0x71), RGB(0x39, 0x00, 0x71), RGB(0x55, 0x00, 0x71),
    RGB(0x71, 0x00, 0x71), RGB(0x71, 0x00, 0x55), RGB(0x71, 0x00, 0x39), RGB(0x71, 0x00, 0x1C),
    RGB(0x71, 0x00, 0x00), RGB(0x71, 0x1C, 0x00), RGB(0x71, 0x39, 0x00), RGB(0x71, 0x55, 0x00),
    RGB(0x71, 0x71, 0x00), RGB(0x55, 0x71, 0x00), RGB(0x39, 0x71, 0x00), RGB(0x1C, 0x71, 0x00),
    RGB(0x00, 0x71, 0x00), RGB(0x00, 0x71, 0x1C), RGB(0x00, 0x71, 0x39), RGB(0x00, 0x71, 0x55),
    RGB(0x00, 0x71, 0x71), RGB(0x00, 0x55, 0x71), RGB(0x00, 0x39, 0x71), RGB(0x00, 0x1C, 0x71),
    RGB(0x39, 0x39, 0x71), RGB(0x45, 0x39, 0x71), RGB(0x55, 0x39, 0x71), RGB(0x61, 0x39, 0x71),
    RGB(0x71, 0x39, 0x71), RGB(0x71, 0x39, 0x61), RGB(0x71, 0x39, 0x55), RGB(0x71, 0x39, 0x45),
    RGB(0x71, 0x39, 0x39), RGB(0x71, 0x45, 0x39), RGB(0x71, 0x55, 0x39), RGB(0x71, 0x61, 0x39),
    RGB(0x71, 0x71, 0x39), RGB(0x61, 0x71, 0x39), RGB(0x55, 0x71, 0x39), RGB(0x45, 0x71, 0x39),
    RGB(0x39, 0x71, 0x39), RGB(0x39, 0x71, 0x45), RGB(0x39, 0x71, 0x55), RGB(0x39, 0x71, 0x61),
    RGB(0x39, 0x71, 0x71), RGB(0x39, 0x61, 0x71), RGB(0x39, 0x55, 0x71), RGB(0x39, 0x45, 0x71),
    RGB(0x51, 0x51, 0x71), RGB(0x59, 0x51, 0x71), RGB(0x61, 0x51, 0x71), RGB(0x69, 0x51, 0x71),
    RGB(0x71, 0x51, 0x71), RGB(0x71, 0x51, 0x69), RGB(0x71, 0x51, 0x61), RGB(0x71, 0x51, 0x59),
    RGB(0x71, 0x51, 0x51), RGB(0x71, 0x59, 0x51), RGB(0x71, 0x61, 0x51), RGB(0x71, 0x69, 0x51),
    RGB(0x71, 0x71, 0x51), RGB(0x69, 0x71, 0x51), RGB(0x61, 0x71, 0x51), RGB(0x59, 0x71, 0x51),
    RGB(0x51, 0x71, 0x51), RGB(0x51, 0x71, 0x59), RGB(0x51, 0x71, 0x61), RGB(0x51, 0x71, 0x69),
    RGB(0x51, 0x71, 0x71), RGB(0x51, 0x69, 0x71), RGB(0x51, 0x61, 0x71), RGB(0x51, 0x59, 0x71),
    RGB(0x00, 0x00, 0x41), RGB(0x10, 0x00, 0x41), RGB(0x20, 0x00, 0x41), RGB(0x31, 0x00, 0x41),
    RGB(0x41, 0x00, 0x41), RGB(0x41, 0x00, 0x31), RGB(0x41, 0x00, 0x20), RGB(0x41, 0x00, 0x10),
    RGB(0x41, 0x00, 0x00), RGB(0x41, 0x10, 0x00), RGB(0x41, 0x20, 0x00), RGB(0x41, 0x31, 0x00),
    RGB(0x41, 0x41, 0x00), RGB(0x31, 0x41, 0x00), RGB(0x20, 0x41, 0x00), RGB(0x10, 0x41, 0x00),
    RGB(0x00, 0x41, 0x00), RGB(0x00, 0x41, 0x10), RGB(0x00, 0x41, 0x20), RGB(0x00, 0x41, 0x31),
    RGB(0x00, 0x41, 0x41), RGB(0x00, 0x31, 0x41), RGB(0x00, 0x20, 0x41), RGB(0x00, 0x10, 0x41),
    RGB(0x20, 0x20, 0x41), RGB(0x28, 0x20, 0x41), RGB(0x31, 0x20, 0x41), RGB(0x39, 0x20, 0x41),
    RGB(0x41, 0x20, 0x41), RGB(0x41, 0x20, 0x39), RGB(0x41, 0x20, 0x31), RGB(0x41, 0x20, 0x28),
    RGB(0x41, 0x20, 0x20), RGB(0x41, 0x28, 0x20), RGB(0x41, 0x31, 0x20), RGB(0x41, 0x39, 0x20),
    RGB(0x41, 0x41, 0x20), RGB(0x39, 0x41, 0x20), RGB(0x31, 0x41, 0x20), RGB(0x28, 0x41, 0x20),
    RGB(0x20, 0x41, 0x20), RGB(0x20, 0x41, 0x28), RGB(0x20, 0x41, 0x31), RGB(0x20, 0x41, 0x39),
    RGB(0x20, 0x41, 0x41), RGB(0x20, 0x39, 0x41), RGB(0x20, 0x31, 0x41), RGB(0x20, 0x28, 0x41),
    RGB(0x2D, 0x2D, 0x41), RGB(0x31, 0x2D, 0x41), RGB(0x35, 0x2D, 0x41), RGB(0x3D, 0x2D, 0x41),
    RGB(0x41, 0x2D, 0x41), RGB(0x41, 0x2D, 0x3D), RGB(0x41, 0x2D, 0x35), RGB(0x41, 0x2D, 0x31),
    RGB(0x41, 0x2D, 0x2D), RGB(0x41, 0x31, 0x2D), RGB(0x41, 0x35, 0x2D), RGB(0x41, 0x3D, 0x2D),
    RGB(0x41, 0x41, 0x2D), RGB(0x3D, 0x41, 0x2D), RGB(0x35, 0x41, 0x2D), RGB(0x31, 0x41, 0x2D),
    RGB(0x2D, 0x41, 0x2D), RGB(0x2D, 0x41, 0x31), RGB(0x2D, 0x41, 0x35), RGB(0x2D, 0x41, 0x3D),
    RGB(0x2D, 0x41, 0x41), RGB(0x2D, 0x3D, 0x41), RGB(0x2D, 0x35, 0x41), RGB(0x2D, 0x31, 0x41),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00)
};

#elif defined(USE_DOSBOX_COLORS)

// DOSBox colors
static CONST COLORREF VgaDefaultPalette[VGA_MAX_COLORS] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0x00), RGB(0x14, 0x14, 0x14), RGB(0x20, 0x20, 0x20), RGB(0x2C, 0x2C, 0x2C),
    RGB(0x38, 0x38, 0x38), RGB(0x45, 0x45, 0x45), RGB(0x51, 0x51, 0x51), RGB(0x61, 0x61, 0x61),
    RGB(0x71, 0x71, 0x71), RGB(0x82, 0x82, 0x82), RGB(0x92, 0x92, 0x92), RGB(0xA2, 0xA2, 0xA2),
    RGB(0xB6, 0xB6, 0xB6), RGB(0xCB, 0xCB, 0xCB), RGB(0xE3, 0xE3, 0xE3), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0xFF), RGB(0x41, 0x00, 0xFF), RGB(0x7D, 0x00, 0xFF), RGB(0xBE, 0x00, 0xFF),
    RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0x00, 0xBE), RGB(0xFF, 0x00, 0x7D), RGB(0xFF, 0x00, 0x41),
    RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x41, 0x00), RGB(0xFF, 0x7D, 0x00), RGB(0xFF, 0xBE, 0x00),
    RGB(0xFF, 0xFF, 0x00), RGB(0xBE, 0xFF, 0x00), RGB(0x7D, 0xFF, 0x00), RGB(0x41, 0xFF, 0x00),
    RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0x41), RGB(0x00, 0xFF, 0x7D), RGB(0x00, 0xFF, 0xBE),
    RGB(0x00, 0xFF, 0xFF), RGB(0x00, 0xBE, 0xFF), RGB(0x00, 0x7D, 0xFF), RGB(0x00, 0x41, 0xFF),
    RGB(0x7D, 0x7D, 0xFF), RGB(0x9E, 0x7D, 0xFF), RGB(0xBE, 0x7D, 0xFF), RGB(0xDF, 0x7D, 0xFF),
    RGB(0xFF, 0x7D, 0xFF), RGB(0xFF, 0x7D, 0xDF), RGB(0xFF, 0x7D, 0xBE), RGB(0xFF, 0x7D, 0x9E),

    RGB(0xFF, 0x7D, 0x7D), RGB(0xFF, 0x9E, 0x7D), RGB(0xFF, 0xBE, 0x7D), RGB(0xFF, 0xDF, 0x7D),
    RGB(0xFF, 0xFF, 0x7D), RGB(0xDF, 0xFF, 0x7D), RGB(0xBE, 0xFF, 0x7D), RGB(0x9E, 0xFF, 0x7D),
    RGB(0x7D, 0xFF, 0x7D), RGB(0x7D, 0xFF, 0x9E), RGB(0x7D, 0xFF, 0xBE), RGB(0x7D, 0xFF, 0xDF),
    RGB(0x7D, 0xFF, 0xFF), RGB(0x7D, 0xDF, 0xFF), RGB(0x7D, 0xBE, 0xFF), RGB(0x7D, 0x9E, 0xFF),
    RGB(0xB6, 0xB6, 0xFF), RGB(0xC7, 0xB6, 0xFF), RGB(0xDB, 0xB6, 0xFF), RGB(0xEB, 0xB6, 0xFF),
    RGB(0xFF, 0xB6, 0xFF), RGB(0xFF, 0xB6, 0xEB), RGB(0xFF, 0xB6, 0xDB), RGB(0xFF, 0xB6, 0xC7),
    RGB(0xFF, 0xB6, 0xB6), RGB(0xFF, 0xC7, 0xB6), RGB(0xFF, 0xDB, 0xB6), RGB(0xFF, 0xEB, 0xB6),
    RGB(0xFF, 0xFF, 0xB6), RGB(0xEB, 0xFF, 0xB6), RGB(0xDB, 0xFF, 0xB6), RGB(0xC7, 0xFF, 0xB6),
    RGB(0xB6, 0xFF, 0xB6), RGB(0xB6, 0xFF, 0xC7), RGB(0xB6, 0xFF, 0xDB), RGB(0xB6, 0xFF, 0xEB),
    RGB(0xB6, 0xFF, 0xFF), RGB(0xB6, 0xEB, 0xFF), RGB(0xB6, 0xDB, 0xFF), RGB(0xB6, 0xC7, 0xFF),
    RGB(0x00, 0x00, 0x71), RGB(0x1C, 0x00, 0x71), RGB(0x38, 0x00, 0x71), RGB(0x55, 0x00, 0x71),
    RGB(0x71, 0x00, 0x71), RGB(0x71, 0x00, 0x55), RGB(0x71, 0x00, 0x38), RGB(0x71, 0x00, 0x1C),
    RGB(0x71, 0x00, 0x00), RGB(0x71, 0x1C, 0x00), RGB(0x71, 0x38, 0x00), RGB(0x71, 0x55, 0x00),
    RGB(0x71, 0x71, 0x00), RGB(0x55, 0x71, 0x00), RGB(0x38, 0x71, 0x00), RGB(0x1C, 0x71, 0x00),
    RGB(0x00, 0x71, 0x00), RGB(0x00, 0x71, 0x1C), RGB(0x00, 0x71, 0x38), RGB(0x00, 0x71, 0x55),
    RGB(0x00, 0x71, 0x71), RGB(0x00, 0x55, 0x71), RGB(0x00, 0x38, 0x71), RGB(0x00, 0x1C, 0x71),

    RGB(0x38, 0x38, 0x71), RGB(0x45, 0x38, 0x71), RGB(0x55, 0x38, 0x71), RGB(0x61, 0x38, 0x71),
    RGB(0x71, 0x38, 0x71), RGB(0x71, 0x38, 0x61), RGB(0x71, 0x38, 0x55), RGB(0x71, 0x38, 0x45),
    RGB(0x71, 0x38, 0x38), RGB(0x71, 0x45, 0x38), RGB(0x71, 0x55, 0x38), RGB(0x71, 0x61, 0x38),
    RGB(0x71, 0x71, 0x38), RGB(0x61, 0x71, 0x38), RGB(0x55, 0x71, 0x38), RGB(0x45, 0x71, 0x38),
    RGB(0x38, 0x71, 0x38), RGB(0x38, 0x71, 0x45), RGB(0x38, 0x71, 0x55), RGB(0x38, 0x71, 0x61),
    RGB(0x38, 0x71, 0x71), RGB(0x38, 0x61, 0x71), RGB(0x38, 0x55, 0x71), RGB(0x38, 0x45, 0x71),
    RGB(0x51, 0x51, 0x71), RGB(0x59, 0x51, 0x71), RGB(0x61, 0x51, 0x71), RGB(0x69, 0x51, 0x71),
    RGB(0x71, 0x51, 0x71), RGB(0x71, 0x51, 0x69), RGB(0x71, 0x51, 0x61), RGB(0x71, 0x51, 0x59),
    RGB(0x71, 0x51, 0x51), RGB(0x71, 0x59, 0x51), RGB(0x71, 0x61, 0x51), RGB(0x71, 0x69, 0x51),
    RGB(0x71, 0x71, 0x51), RGB(0x69, 0x71, 0x51), RGB(0x61, 0x71, 0x51), RGB(0x59, 0x71, 0x51),
    RGB(0x51, 0x71, 0x51), RGB(0x51, 0x71, 0x59), RGB(0x51, 0x71, 0x61), RGB(0x51, 0x71, 0x69),
    RGB(0x51, 0x71, 0x71), RGB(0x51, 0x69, 0x71), RGB(0x51, 0x61, 0x71), RGB(0x51, 0x59, 0x71),
    RGB(0x00, 0x00, 0x41), RGB(0x10, 0x00, 0x41), RGB(0x20, 0x00, 0x41), RGB(0x30, 0x00, 0x41),
    RGB(0x41, 0x00, 0x41), RGB(0x41, 0x00, 0x30), RGB(0x41, 0x00, 0x20), RGB(0x41, 0x00, 0x10),
    RGB(0x41, 0x00, 0x00), RGB(0x41, 0x10, 0x00), RGB(0x41, 0x20, 0x00), RGB(0x41, 0x30, 0x00),
    RGB(0x41, 0x41, 0x00), RGB(0x30, 0x41, 0x00), RGB(0x20, 0x41, 0x00), RGB(0x10, 0x41, 0x00),

    RGB(0x00, 0x41, 0x00), RGB(0x00, 0x41, 0x10), RGB(0x00, 0x41, 0x20), RGB(0x00, 0x41, 0x30),
    RGB(0x00, 0x41, 0x41), RGB(0x00, 0x30, 0x41), RGB(0x00, 0x20, 0x41), RGB(0x00, 0x10, 0x41),
    RGB(0x20, 0x20, 0x41), RGB(0x28, 0x20, 0x41), RGB(0x30, 0x20, 0x41), RGB(0x38, 0x20, 0x41),
    RGB(0x41, 0x20, 0x41), RGB(0x41, 0x20, 0x38), RGB(0x41, 0x20, 0x30), RGB(0x41, 0x20, 0x28),
    RGB(0x41, 0x20, 0x20), RGB(0x41, 0x28, 0x20), RGB(0x41, 0x30, 0x20), RGB(0x41, 0x38, 0x20),
    RGB(0x41, 0x41, 0x20), RGB(0x38, 0x41, 0x20), RGB(0x30, 0x41, 0x20), RGB(0x28, 0x41, 0x20),
    RGB(0x20, 0x41, 0x20), RGB(0x20, 0x41, 0x28), RGB(0x20, 0x41, 0x30), RGB(0x20, 0x41, 0x38),
    RGB(0x20, 0x41, 0x41), RGB(0x20, 0x38, 0x41), RGB(0x20, 0x30, 0x41), RGB(0x20, 0x28, 0x41),
    RGB(0x2C, 0x2C, 0x41), RGB(0x30, 0x2C, 0x41), RGB(0x34, 0x2C, 0x41), RGB(0x3C, 0x2C, 0x41),
    RGB(0x41, 0x2C, 0x41), RGB(0x41, 0x2C, 0x3C), RGB(0x41, 0x2C, 0x34), RGB(0x41, 0x2C, 0x30),
    RGB(0x41, 0x2C, 0x2C), RGB(0x41, 0x30, 0x2C), RGB(0x41, 0x34, 0x2C), RGB(0x41, 0x3C, 0x2C),
    RGB(0x41, 0x41, 0x2C), RGB(0x3C, 0x41, 0x2C), RGB(0x34, 0x41, 0x2C), RGB(0x30, 0x41, 0x2C),
    RGB(0x2C, 0x41, 0x2C), RGB(0x2C, 0x41, 0x30), RGB(0x2C, 0x41, 0x34), RGB(0x2C, 0x41, 0x3C),
    RGB(0x2C, 0x41, 0x41), RGB(0x2C, 0x3C, 0x41), RGB(0x2C, 0x34, 0x41), RGB(0x2C, 0x30, 0x41),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00)
};

#endif

static CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;

static BYTE VgaMemory[VGA_NUM_BANKS * VGA_BANK_SIZE];
static LPVOID ConsoleFramebuffer = NULL;
static HANDLE TextConsoleBuffer = NULL;
static HANDLE GraphicsConsoleBuffer = NULL;
static HANDLE ConsoleMutex = NULL;
static HPALETTE PaletteHandle = NULL;
static BOOLEAN DoubleVision = FALSE;

static BYTE VgaLatchRegisters[VGA_NUM_BANKS] = {0, 0, 0, 0};

static BYTE VgaMiscRegister;
static BYTE VgaFeatureRegister;

static BYTE VgaSeqIndex = VGA_SEQ_RESET_REG;
static BYTE VgaSeqRegisters[VGA_SEQ_MAX_REG];

static BYTE VgaCrtcIndex = VGA_CRTC_HORZ_TOTAL_REG;
static BYTE VgaCrtcRegisters[VGA_CRTC_MAX_REG];

static BYTE VgaGcIndex = VGA_GC_RESET_REG;
static BYTE VgaGcRegisters[VGA_GC_MAX_REG];

static BOOLEAN VgaAcLatch = FALSE;
static BOOLEAN VgaAcPalDisable = TRUE;
static BYTE VgaAcIndex = VGA_AC_PAL_0_REG;
static BYTE VgaAcRegisters[VGA_AC_MAX_REG];

// static VGA_REGISTERS VgaRegisters;

static BYTE VgaDacMask = 0xFF;
static WORD VgaDacIndex = 0;
static BOOLEAN VgaDacReadWrite = FALSE;
static BYTE VgaDacRegisters[VGA_PALETTE_SIZE];

static BOOLEAN InVerticalRetrace = FALSE;
static BOOLEAN InHorizontalRetrace = FALSE;

static BOOLEAN NeedsUpdate = FALSE;
static BOOLEAN ModeChanged = TRUE;
static BOOLEAN CursorMoved = FALSE;
static BOOLEAN PaletteChanged = FALSE;

static
enum SCREEN_MODE
{
    TEXT_MODE,
    GRAPHICS_MODE
} ScreenMode = TEXT_MODE;

static SMALL_RECT UpdateRectangle = { 0, 0, 0, 0 };

/* PRIVATE FUNCTIONS **********************************************************/

static inline DWORD VgaGetAddressSize(VOID)
{
    if (VgaCrtcRegisters[VGA_CRTC_UNDERLINE_REG] & VGA_CRTC_UNDERLINE_DWORD)
    {
        /* Double-word addressing */
        return 4; // sizeof(DWORD)
    }
    else if (VgaCrtcRegisters[VGA_CRTC_MODE_CONTROL_REG] & VGA_CRTC_MODE_CONTROL_BYTE)
    {
        /* Byte addressing */
        return 1; // sizeof(BYTE)
    }
    else
    {
        /* Word addressing */
        return 2; // sizeof(WORD)
    }
}

static inline DWORD VgaTranslateReadAddress(DWORD Address)
{
    DWORD Offset = Address - VgaGetVideoBaseAddress();
    BYTE Plane;

    /* Check for chain-4 and odd-even mode */
    if (VgaSeqRegisters[VGA_SEQ_MEM_REG] & VGA_SEQ_MEM_C4)
    {
        /* The lowest two bits are the plane number */
        Plane = Offset & 3;
        Offset >>= 2;
    }
    else if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_OE)
    {
        /* The LSB is the plane number */
        Plane = Offset & 1;
        Offset >>= 1;
    }
    else
    {
        /* Use the read mode */
        Plane = VgaGcRegisters[VGA_GC_READ_MAP_SEL_REG] & 0x03;
    }

    /* Multiply the offset by the address size */
    Offset *= VgaGetAddressSize();
    
    return Offset + Plane * VGA_BANK_SIZE;
}

static inline DWORD VgaTranslateWriteAddress(DWORD Address)
{
    DWORD Offset = Address - VgaGetVideoBaseAddress();

    /* Check for chain-4 and odd-even mode */
    if (VgaSeqRegisters[VGA_SEQ_MEM_REG] & VGA_SEQ_MEM_C4)
    {
        /* Shift the offset to the right by 2 */
        Offset >>= 2;
    }
    else if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_OE)
    {
        /* Shift the offset to the right by 1 */
        Offset >>= 1;
    }

    /* Multiply the offset by the address size */
    Offset *= VgaGetAddressSize();

    /* Return the offset on plane 0 */
    return Offset;
}

static inline BYTE VgaTranslateByteForWriting(BYTE Data, BYTE Plane)
{
    BYTE WriteMode = VgaGcRegisters[VGA_GC_MODE_REG] & 3;
    BYTE BitMask = VgaGcRegisters[VGA_GC_BITMASK_REG];

    if (WriteMode == 1)
    {
        /* In write mode 1 just return the latch register */
        return VgaLatchRegisters[Plane];
    }

    if (WriteMode != 2)
    {
        /* Write modes 0 and 3 rotate the data to the right first */
        BYTE RotateCount = VgaGcRegisters[VGA_GC_ROTATE_REG] & 7;
        Data = LOBYTE(((DWORD)Data >> RotateCount) | ((DWORD)Data << (8 - RotateCount)));
    }
    else
    {
        /* Write mode 2 expands the appropriate bit to all 8 bits */
        Data = (Data & (1 << Plane)) ? 0xFF : 0x00;
    }

    if (WriteMode == 0)
    {
        /*
         * In write mode 0, the enable set/reset register decides if the
         * set/reset bit should be expanded to all 8 bits.
         */
        if (VgaGcRegisters[VGA_GC_ENABLE_RESET_REG] & (1 << Plane))
        {
            /* Copy the bit from the set/reset register to all 8 bits */
            Data = (VgaGcRegisters[VGA_GC_RESET_REG] & (1 << Plane)) ? 0xFF : 0x00;
        }
    }

    if (WriteMode != 3)
    {
        /* Write modes 0 and 2 then perform a logical operation on the data and latch */
        BYTE LogicalOperation = (VgaGcRegisters[VGA_GC_ROTATE_REG] >> 3) & 3;

        if (LogicalOperation == 1) Data &= VgaLatchRegisters[Plane];
        else if (LogicalOperation == 2) Data |= VgaLatchRegisters[Plane];
        else if (LogicalOperation == 3) Data ^= VgaLatchRegisters[Plane];
    }
    else
    {
        /* For write mode 3, we AND the bitmask with the data, which is used as the new bitmask */
        BitMask &= Data;

        /* Then we expand the bit in the set/reset field */
        Data = (VgaGcRegisters[VGA_GC_RESET_REG] & (1 << Plane)) ? 0xFF : 0x00;
    }

    /* Bits cleared in the bitmask are replaced with latch register bits */
    Data = (Data & BitMask) | (VgaLatchRegisters[Plane] & (~BitMask));

    /* Return the byte */
    return Data;
}

static inline VOID VgaMarkForUpdate(SHORT Row, SHORT Column)
{
    /* Check if this is the first time the rectangle is updated */
    if (!NeedsUpdate)
    {
        UpdateRectangle.Left = UpdateRectangle.Top = MAXSHORT;
        UpdateRectangle.Right = UpdateRectangle.Bottom = MINSHORT;
    }

    /* Expand the rectangle to include the point */
    UpdateRectangle.Left = min(UpdateRectangle.Left, Column);
    UpdateRectangle.Right = max(UpdateRectangle.Right, Column);
    UpdateRectangle.Top = min(UpdateRectangle.Top, Row);
    UpdateRectangle.Bottom = max(UpdateRectangle.Bottom, Row);

    /* Set the update request flag */
    NeedsUpdate = TRUE;
}

static VOID VgaWriteSequencer(BYTE Data)
{
    ASSERT(VgaSeqIndex < VGA_SEQ_MAX_REG);

    /* Save the value */
    VgaSeqRegisters[VgaSeqIndex] = Data;
}

static VOID VgaWriteGc(BYTE Data)
{
    ASSERT(VgaGcIndex < VGA_GC_MAX_REG);

    /* Save the value */
    VgaGcRegisters[VgaGcIndex] = Data;

    /* Check the index */
    switch (VgaGcIndex)
    {
        case VGA_GC_MISC_REG:
        {
            /* The GC misc register decides if it's text or graphics mode */
            ModeChanged = TRUE;
            break;
        }
    }
}

static VOID VgaWriteCrtc(BYTE Data)
{
    ASSERT(VgaGcIndex < VGA_CRTC_MAX_REG);

    /* Save the value */
    VgaCrtcRegisters[VgaCrtcIndex] = Data;

    /* Check the index */
    switch (VgaCrtcIndex)
    {
        case VGA_CRTC_END_HORZ_DISP_REG:
        case VGA_CRTC_VERT_DISP_END_REG:
        case VGA_CRTC_OVERFLOW_REG:
        {
            /* The video mode has changed */
            ModeChanged = TRUE;
            break;
        }

        case VGA_CRTC_CURSOR_LOC_LOW_REG:
        case VGA_CRTC_CURSOR_LOC_HIGH_REG:
        case VGA_CRTC_CURSOR_START_REG:
        case VGA_CRTC_CURSOR_END_REG:
        {
            /* Set the cursor moved flag */
            CursorMoved = TRUE;
            break;
        }
    }
}

static VOID VgaWriteDac(BYTE Data)
{
    INT PaletteIndex;
    PALETTEENTRY Entry;

    /* Set the value */
    VgaDacRegisters[VgaDacIndex] = Data;

    /* Find the palette index */
    PaletteIndex = VgaDacIndex / 3;

    /* Fill the entry structure */
    Entry.peRed = VGA_DAC_TO_COLOR(VgaDacRegisters[PaletteIndex * 3]);
    Entry.peGreen = VGA_DAC_TO_COLOR(VgaDacRegisters[PaletteIndex * 3 + 1]);
    Entry.peBlue = VGA_DAC_TO_COLOR(VgaDacRegisters[PaletteIndex * 3 + 2]);
    Entry.peFlags = 0;

    /* Update the palette entry and set the palette change flag */
    SetPaletteEntries(PaletteHandle, PaletteIndex, 1, &Entry);
    PaletteChanged = TRUE;

    /* Update the index */
    VgaDacIndex++;
    VgaDacIndex %= VGA_PALETTE_SIZE;
}

static VOID VgaWriteAc(BYTE Data)
{
    ASSERT(VgaAcIndex < VGA_AC_MAX_REG);

    /* Save the value */
    if (VgaAcIndex <= VGA_AC_PAL_F_REG)
    {
        if (VgaAcPalDisable) return;

        // DbgPrint("    AC Palette Writing %d to index %d\n", Data, VgaAcIndex);
        if (VgaAcRegisters[VgaAcIndex] != Data)
        {
            /* Update the AC register and set the palette change flag */
            VgaAcRegisters[VgaAcIndex] = Data;
            PaletteChanged = TRUE;
        }
    }
    else
    {
        VgaAcRegisters[VgaAcIndex] = Data;
    }
}

static VOID VgaRestoreDefaultPalette(PPALETTEENTRY Entries, USHORT NumOfEntries)
{
    USHORT i;

    /* Copy the colors of the default palette to the DAC and console palette */
    for (i = 0; i < NumOfEntries; i++)
    {
        /* Set the palette entries */
        Entries[i].peRed   = GetRValue(VgaDefaultPalette[i]);
        Entries[i].peGreen = GetGValue(VgaDefaultPalette[i]);
        Entries[i].peBlue  = GetBValue(VgaDefaultPalette[i]);
        Entries[i].peFlags = 0;

        /* Set the DAC registers */
        VgaDacRegisters[i * 3]     = VGA_COLOR_TO_DAC(GetRValue(VgaDefaultPalette[i]));
        VgaDacRegisters[i * 3 + 1] = VGA_COLOR_TO_DAC(GetGValue(VgaDefaultPalette[i]));
        VgaDacRegisters[i * 3 + 2] = VGA_COLOR_TO_DAC(GetBValue(VgaDefaultPalette[i]));
    }
}

static BOOLEAN VgaInitializePalette(VOID)
{
    LPLOGPALETTE Palette;

    /* Allocate storage space for the palette */
    Palette = (LPLOGPALETTE)HeapAlloc(GetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(LOGPALETTE) +
                                        VGA_MAX_COLORS * sizeof(PALETTEENTRY));
    if (Palette == NULL) return FALSE;

    /* Initialize the palette */
    Palette->palVersion = 0x0300;
    Palette->palNumEntries = VGA_MAX_COLORS;

    /* Restore the default palette */
    VgaRestoreDefaultPalette(Palette->palPalEntry, Palette->palNumEntries);

    /* Create the palette */
    PaletteHandle = CreatePalette(Palette);

    /* Free the palette */
    HeapFree(GetProcessHeap(), 0, Palette);

    /* Fail if the palette wasn't successfully created... */
    if (PaletteHandle == NULL) return FALSE;

    /* ... otherwise return success */
    return TRUE;
}

static BOOL VgaEnterGraphicsMode(PCOORD Resolution)
{
    DWORD i;
    CONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo;
    BYTE BitmapInfoBuffer[VGA_BITMAP_INFO_SIZE];
    LPBITMAPINFO BitmapInfo = (LPBITMAPINFO)BitmapInfoBuffer;
    LPWORD PaletteIndex = (LPWORD)(BitmapInfo->bmiColors);

    LONG Width  = Resolution->X;
    LONG Height = Resolution->Y;

    /* Use DoubleVision mode if the resolution is too small */
    if (Width < VGA_MINIMUM_WIDTH && Height < VGA_MINIMUM_HEIGHT)
    {
        DoubleVision = TRUE;
        Width  *= 2;
        Height *= 2;
    }
    else
    {
        DoubleVision = FALSE;
    }

    /* Fill the bitmap info header */
    ZeroMemory(&BitmapInfo->bmiHeader, sizeof(BITMAPINFOHEADER));
    BitmapInfo->bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
    BitmapInfo->bmiHeader.biWidth  = Width;
    BitmapInfo->bmiHeader.biHeight = Height;
    BitmapInfo->bmiHeader.biBitCount = 8;
    BitmapInfo->bmiHeader.biPlanes   = 1;
    BitmapInfo->bmiHeader.biCompression = BI_RGB;
    BitmapInfo->bmiHeader.biSizeImage   = Width * Height /* * 1 == biBitCount / 8 */;

    /* Fill the palette data */
    for (i = 0; i < (VGA_PALETTE_SIZE / 3); i++) PaletteIndex[i] = (WORD)i;

    /* Fill the console graphics buffer info */
    GraphicsBufferInfo.dwBitMapInfoLength = VGA_BITMAP_INFO_SIZE;
    GraphicsBufferInfo.lpBitMapInfo = BitmapInfo;
    GraphicsBufferInfo.dwUsage = DIB_PAL_COLORS;

    /* Create the buffer */
    GraphicsConsoleBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                      NULL,
                                                      CONSOLE_GRAPHICS_BUFFER,
                                                      &GraphicsBufferInfo);
    if (GraphicsConsoleBuffer == INVALID_HANDLE_VALUE) return FALSE;

    /* Save the framebuffer address and mutex */
    ConsoleFramebuffer = GraphicsBufferInfo.lpBitMap;
    ConsoleMutex = GraphicsBufferInfo.hMutex;

    /* Clear the framebuffer */
    ZeroMemory(ConsoleFramebuffer, BitmapInfo->bmiHeader.biSizeImage);

    /* Set the active buffer */
    SetConsoleActiveScreenBuffer(GraphicsConsoleBuffer);

    /* Set the graphics mode palette */
    SetConsolePalette(GraphicsConsoleBuffer,
                      PaletteHandle,
                      SYSPAL_NOSTATIC256);

    /* Set the screen mode flag */
    ScreenMode = GRAPHICS_MODE;

    return TRUE;
}

static VOID VgaLeaveGraphicsMode(VOID)
{
    /* Release the console framebuffer mutex */
    ReleaseMutex(ConsoleMutex);

    /* Switch back to the default console text buffer */
    // SetConsoleActiveScreenBuffer(TextConsoleBuffer);

    /* Cleanup the video data */
    CloseHandle(ConsoleMutex);
    ConsoleMutex = NULL;
    CloseHandle(GraphicsConsoleBuffer);
    GraphicsConsoleBuffer = NULL;
    DoubleVision = FALSE;
}

static BOOL VgaEnterTextMode(PCOORD Resolution)
{
    SMALL_RECT ConRect;

    /* Switch to the text buffer */
    SetConsoleActiveScreenBuffer(TextConsoleBuffer);

    /* Resize the console */
    ConRect.Left   = 0; // ConsoleInfo.srWindow.Left;
    ConRect.Top    = ConsoleInfo.srWindow.Top;
    ConRect.Right  = ConRect.Left + Resolution->X - 1;
    ConRect.Bottom = ConRect.Top  + Resolution->Y - 1;
    /*
     * Use this trick to effectively resize the console buffer and window,
     * because:
     * - SetConsoleScreenBufferSize fails if the new console screen buffer size
     *   is smaller than the current console window size, and:
     * - SetConsoleWindowInfo fails if the new console window size is larger
     *   than the current console screen buffer size.
     */
    SetConsoleScreenBufferSize(TextConsoleBuffer, *Resolution);
    SetConsoleWindowInfo(TextConsoleBuffer, TRUE, &ConRect);
    SetConsoleScreenBufferSize(TextConsoleBuffer, *Resolution);
    /* Update the saved console information */
    GetConsoleScreenBufferInfo(TextConsoleBuffer, &ConsoleInfo);

    /* Allocate a framebuffer */
    ConsoleFramebuffer = HeapAlloc(GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   Resolution->X * Resolution->Y
                                   * sizeof(CHAR_INFO));
    if (ConsoleFramebuffer == NULL)
    {
        DisplayMessage(L"An unexpected error occurred!\n");
        VdmRunning = FALSE;
        return FALSE;
    }

    /*
     * Set the text mode palette.
     *
     * WARNING: This call should fail on Windows (and therefore
     * we get the default palette and our external behaviour is
     * just like Windows' one), but it should success on ReactOS
     * (so that we get console palette changes even for text-mode
     * screen-buffers, which is a new feature on ReactOS).
     */
    SetConsolePalette(TextConsoleBuffer,
                      PaletteHandle,
                      SYSPAL_NOSTATIC256);

    /* Set the screen mode flag */
    ScreenMode = TEXT_MODE;

    return TRUE;
}

static VOID VgaLeaveTextMode(VOID)
{
    /* Free the old framebuffer */
    HeapFree(GetProcessHeap(), 0, ConsoleFramebuffer);
    ConsoleFramebuffer = NULL;
}

static VOID VgaChangeMode(VOID)
{
    COORD Resolution = VgaGetDisplayResolution();

    if (ScreenMode == GRAPHICS_MODE)
    {
        /* Leave the current graphics mode */
        VgaLeaveGraphicsMode();
    }
    else
    {
        /* Leave the current text mode */
        VgaLeaveTextMode();
    }

    /* Check if the new mode is alphanumeric */
    if (!(VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_NOALPHA))
    {
        /* Enter new text mode */
        if (!VgaEnterTextMode(&Resolution))
        {
            DisplayMessage(L"An unexpected VGA error occurred while switching into text mode.");
            VdmRunning = FALSE;
            return;
        }
    }
    else
    {
        /* Enter graphics mode */
        if (!VgaEnterGraphicsMode(&Resolution))
        {
            DisplayMessage(L"An unexpected VGA error occurred while switching into graphics mode.");
            VdmRunning = FALSE;
            return;
        }
    }

    /* Trigger a full update of the screen */
    NeedsUpdate = TRUE;
    UpdateRectangle.Left = 0;
    UpdateRectangle.Top = 0;
    UpdateRectangle.Right = Resolution.X;
    UpdateRectangle.Bottom = Resolution.Y;

    /* Reset the mode change flag */
    ModeChanged = FALSE;
}

static VOID VgaUpdateFramebuffer(VOID)
{
    INT i, j, k;
    COORD Resolution = VgaGetDisplayResolution();
    DWORD AddressSize = VgaGetAddressSize();
    DWORD Address = MAKEWORD(VgaCrtcRegisters[VGA_CRTC_START_ADDR_LOW_REG],
                             VgaCrtcRegisters[VGA_CRTC_START_ADDR_HIGH_REG]);
    DWORD ScanlineSize = (DWORD)VgaCrtcRegisters[VGA_CRTC_OFFSET_REG] * 2;

    /*
     * If console framebuffer is NULL, that means something went wrong
     * earlier and this is the final display refresh.
     */
    if (ConsoleFramebuffer == NULL) return;

    /* Check if this is text mode or graphics mode */
    if (VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_NOALPHA)
    {
        /* Graphics mode */
        PBYTE GraphicsBuffer = (PBYTE)ConsoleFramebuffer;

        /*
         * Synchronize access to the graphics framebuffer
         * with the console framebuffer mutex.
         */
        WaitForSingleObject(ConsoleMutex, INFINITE);

        /* Loop through the scanlines */
        for (i = 0; i < Resolution.Y; i++)
        {
            /* Loop through the pixels */
            for (j = 0; j < Resolution.X; j++)
            {
                BYTE PixelData = 0;

                /* Check the shifting mode */
                if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_SHIFT256)
                {
                    /* 4 bits shifted from each plane */

                    /* Check if this is 16 or 256 color mode */
                    if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT)
                    {
                        /* One byte per pixel */
                        PixelData = VgaMemory[(j % VGA_NUM_BANKS) * VGA_BANK_SIZE
                                              + (Address + (j / VGA_NUM_BANKS))
                                                * AddressSize];
                    }
                    else
                    {
                        /* 4-bits per pixel */

                        PixelData = VgaMemory[(j % VGA_NUM_BANKS) * VGA_BANK_SIZE
                                              + (Address + (j / (VGA_NUM_BANKS * 2)))
                                                * AddressSize];

                        /* Check if we should use the highest 4 bits or lowest 4 */
                        if (((j / VGA_NUM_BANKS) % 2) == 0)
                        {
                            /* Highest 4 */
                            PixelData >>= 4;
                        }
                        else
                        {
                            /* Lowest 4 */
                            PixelData &= 0x0F;
                        }
                    }
                }
                else if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_SHIFTREG)
                {
                    /* Check if this is 16 or 256 color mode */
                    if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT)
                    {
                        // TODO: NOT IMPLEMENTED
                        DPRINT1("8-bit interleaved mode is not implemented!\n");
                    }
                    else
                    {
                        /*
                         * 2 bits shifted from plane 0 and 2 for the first 4 pixels,
                         * then 2 bits shifted from plane 1 and 3 for the next 4
                         */
                        BYTE LowPlaneData = VgaMemory[((j / 4) % 2) * VGA_BANK_SIZE
                                                      + (Address + (j / 8)) * AddressSize];
                        BYTE HighPlaneData = VgaMemory[(((j / 4) % 2) + 2) * VGA_BANK_SIZE
                                                       + (Address + (j / 8)) * AddressSize];

                        /* Extract the two bits from each plane */
                        LowPlaneData = (LowPlaneData >> (6 - ((j % 4) * 2))) & 3;
                        HighPlaneData = (HighPlaneData >> (6 - ((j % 4) * 2))) & 3;

                        /* Combine them into the pixel */
                        PixelData = LowPlaneData | (HighPlaneData << 2);
                    }
                }
                else
                {
                    /* 1 bit shifted from each plane */

                    /* Check if this is 16 or 256 color mode */
                    if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT)
                    {
                        /* 8 bits per pixel, 2 on each plane */

                        for (k = 0; k < VGA_NUM_BANKS; k++)
                        {
                            /* The data is on plane k, 4 pixels per byte */
                            BYTE PlaneData = VgaMemory[k * VGA_BANK_SIZE
                                                       + (Address + (j / VGA_NUM_BANKS))
                                                         * AddressSize];

                            /* The mask of the first bit in the pair */
                            BYTE BitMask = 1 << (((3 - (j % VGA_NUM_BANKS)) * 2) + 1);

                            /* Bits 0, 1, 2 and 3 come from the first bit of the pair */
                            if (PlaneData & BitMask) PixelData |= 1 << k;

                            /* Bits 4, 5, 6 and 7 come from the second bit of the pair */
                            if (PlaneData & (BitMask >> 1)) PixelData |= 1 << (k + 4);
                        }
                    }
                    else
                    {
                        /* 4 bits per pixel, 1 on each plane */

                        for (k = 0; k < VGA_NUM_BANKS; k++)
                        {
                            BYTE PlaneData = VgaMemory[k * VGA_BANK_SIZE
                                                       + (Address + (j / (VGA_NUM_BANKS * 2)))
                                                         * AddressSize];

                            /* If the bit on that plane is set, set it */
                            if (PlaneData & (1 << (7 - (j % 8)))) PixelData |= 1 << k;
                        }
                    }
                }

                if (!(VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT))
                {
                    /*
                     * In 16 color mode, the value is an index to the AC registers
                     * if external palette access is disabled, otherwise (in case
                     * of palette loading) it is a blank pixel.
                     */
                    PixelData = (VgaAcPalDisable ? VgaAcRegisters[PixelData & 0x0F]
                                                 : 0);
                }

                /* Take into account DoubleVision mode when checking for pixel updates */
                if (DoubleVision)
                {
                    /* Now check if the resulting pixel data has changed */
                    if (GraphicsBuffer[(i * Resolution.X * 4) + (j * 2)] != PixelData)
                    {
                        /* Yes, write the new value */
                        GraphicsBuffer[(i * Resolution.X * 4) + (j * 2)] = PixelData;
                        GraphicsBuffer[(i * Resolution.X * 4) + (j * 2 + 1)] = PixelData;
                        GraphicsBuffer[((i * 2 + 1) * Resolution.X * 2) + (j * 2)] = PixelData;
                        GraphicsBuffer[((i * 2 + 1) * Resolution.X * 2) + (j * 2 + 1)] = PixelData;

                        /* Mark the specified pixel as changed */
                        VgaMarkForUpdate(i, j);
                    }
                }
                else
                {
                    /* Now check if the resulting pixel data has changed */
                    if (GraphicsBuffer[i * Resolution.X + j] != PixelData)
                    {
                        /* Yes, write the new value */
                        GraphicsBuffer[i * Resolution.X + j] = PixelData;

                        /* Mark the specified pixel as changed */
                        VgaMarkForUpdate(i, j);
                    }
                }
            }

            /* Move to the next scanline */
            Address += ScanlineSize;
        }

        /*
         * Release the console framebuffer mutex
         * so that we allow for repainting.
         */
        ReleaseMutex(ConsoleMutex);
    }
    else
    {
        /* Text mode */
        DWORD CurrentAddr;
        PCHAR_INFO CharBuffer = (PCHAR_INFO)ConsoleFramebuffer;
        CHAR_INFO CharInfo;

        /* Loop through the scanlines */
        for (i = 0; i < Resolution.Y; i++)
        {
            /* Loop through the characters */
            for (j = 0; j < Resolution.X; j++)
            {
                CurrentAddr = LOWORD((Address + j) * AddressSize);

                /* Plane 0 holds the character itself */
                CharInfo.Char.AsciiChar = VgaMemory[CurrentAddr];

                /* Plane 1 holds the attribute */
                CharInfo.Attributes = VgaMemory[CurrentAddr + VGA_BANK_SIZE];

                /* Now check if the resulting character data has changed */
                if ((CharBuffer[i * Resolution.X + j].Char.AsciiChar != CharInfo.Char.AsciiChar)
                    || (CharBuffer[i * Resolution.X + j].Attributes != CharInfo.Attributes))
                {
                    /* Yes, write the new value */
                    CharBuffer[i * Resolution.X + j] = CharInfo;

                    /* Mark the specified cell as changed */
                    VgaMarkForUpdate(i, j);
                }
            }

            /* Move to the next scanline */
            Address += ScanlineSize;
        }
    }
}

static VOID VgaUpdateTextCursor(VOID)
{
    COORD Position;
    CONSOLE_CURSOR_INFO CursorInfo;
    BYTE CursorStart = VgaCrtcRegisters[VGA_CRTC_CURSOR_START_REG] & 0x3F;
    BYTE CursorEnd = VgaCrtcRegisters[VGA_CRTC_CURSOR_END_REG] & 0x1F;
    DWORD ScanlineSize = (DWORD)VgaCrtcRegisters[VGA_CRTC_OFFSET_REG] * 2;
    BYTE TextSize = 1 + (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & 0x1F);
    WORD Location = MAKEWORD(VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_LOW_REG],
                             VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_HIGH_REG]);

    /* Just return if we are not in text mode */
    if (VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_NOALPHA) return;

    if (CursorStart < CursorEnd)
    {
        /* Visible cursor */
        CursorInfo.bVisible = TRUE;
        CursorInfo.dwSize = (100 * (CursorEnd - CursorStart)) / TextSize;
    }
    else
    {
        /* No cursor */
        CursorInfo.bVisible = FALSE;
        CursorInfo.dwSize = 0;
    }

    /* Add the cursor skew to the location */
    Location += (VgaCrtcRegisters[VGA_CRTC_CURSOR_END_REG] >> 5) & 3;

    /* Find the coordinates of the new position */
    Position.X = (WORD)(Location % ScanlineSize);
    Position.Y = (WORD)(Location / ScanlineSize);

    /* Update the physical cursor */
    SetConsoleCursorInfo(TextConsoleBuffer, &CursorInfo);
    SetConsoleCursorPosition(TextConsoleBuffer, Position);

    /* Reset the cursor move flag */
    CursorMoved = FALSE;
}

static BYTE WINAPI VgaReadPort(ULONG Port)
{
    DPRINT("VgaReadPort: Port 0x%X\n", Port);

    switch (Port)
    {
        case VGA_MISC_READ:
            return VgaMiscRegister;

        case VGA_INSTAT0_READ:
            return 0; // Not implemented

        case VGA_INSTAT1_READ_MONO:
        case VGA_INSTAT1_READ_COLOR:
        {
            BYTE Result = 0;

            /* Reset the AC latch */
            VgaAcLatch = FALSE;

            /* Set a flag if there is a vertical or horizontal retrace */
            if (InVerticalRetrace || InHorizontalRetrace) Result |= VGA_STAT_DD;

            /* Set an additional flag if there was a vertical retrace */
            if (InVerticalRetrace) Result |= VGA_STAT_VRETRACE;

            /* Clear the flags */
            InHorizontalRetrace = InVerticalRetrace = FALSE;

            return Result;
        }

        case VGA_FEATURE_READ:
            return VgaFeatureRegister;

        case VGA_AC_INDEX:
            return VgaAcIndex;

        case VGA_AC_READ:
            return VgaAcRegisters[VgaAcIndex];

        case VGA_SEQ_INDEX:
            return VgaSeqIndex;
        
        case VGA_SEQ_DATA:
            return VgaSeqRegisters[VgaSeqIndex];

        case VGA_DAC_MASK:
            return VgaDacMask;

        case VGA_DAC_READ_INDEX:
            /* This returns the read/write state */
            return (VgaDacReadWrite ? 0 : 3);

        case VGA_DAC_WRITE_INDEX:
            return (VgaDacIndex / 3);

        case VGA_DAC_DATA:
        {
            /* Ignore reads in write mode */
            if (!VgaDacReadWrite)
            {
                BYTE Data = VgaDacRegisters[VgaDacIndex++];
                VgaDacIndex %= VGA_PALETTE_SIZE;
                return Data;
            }

            break;
        }

        case VGA_CRTC_INDEX_MONO:
        case VGA_CRTC_INDEX_COLOR:
            return VgaCrtcIndex;

        case VGA_CRTC_DATA_MONO:
        case VGA_CRTC_DATA_COLOR:
            return VgaCrtcRegisters[VgaCrtcIndex];

        case VGA_GC_INDEX:
            return VgaGcIndex;

        case VGA_GC_DATA:
            return VgaGcRegisters[VgaGcIndex];

        default:
            DPRINT1("VgaReadPort: Unknown port 0x%X\n", Port);
            break;
    }

    return 0;
}

static VOID WINAPI VgaWritePort(ULONG Port, BYTE Data)
{
    DPRINT("VgaWritePort: Port 0x%X, Data 0x%02X\n", Port, Data);

    switch (Port)
    {
        case VGA_MISC_WRITE:
        {
            VgaMiscRegister = Data;

            if (VgaMiscRegister & 0x01)
            {
                /* Color emulation */
                DPRINT1("Color emulation\n");

                /* Register the new I/O Ports */
                RegisterIoPort(0x3D4, VgaReadPort, VgaWritePort);   // VGA_CRTC_INDEX_COLOR
                RegisterIoPort(0x3D5, VgaReadPort, VgaWritePort);   // VGA_CRTC_DATA_COLOR
                RegisterIoPort(0x3DA, VgaReadPort, VgaWritePort);   // VGA_INSTAT1_READ_COLOR, VGA_FEATURE_WRITE_COLOR

                /* Unregister the old ones */
                UnregisterIoPort(0x3B4);    // VGA_CRTC_INDEX_MONO
                UnregisterIoPort(0x3B5);    // VGA_CRTC_DATA_MONO
                UnregisterIoPort(0x3BA);    // VGA_INSTAT1_READ_MONO, VGA_FEATURE_WRITE_MONO
            }
            else
            {
                /* Monochrome emulation */
                DPRINT1("Monochrome emulation\n");

                /* Register the new I/O Ports */
                RegisterIoPort(0x3B4, VgaReadPort, VgaWritePort);   // VGA_CRTC_INDEX_MONO
                RegisterIoPort(0x3B5, VgaReadPort, VgaWritePort);   // VGA_CRTC_DATA_MONO
                RegisterIoPort(0x3BA, VgaReadPort, VgaWritePort);   // VGA_INSTAT1_READ_MONO, VGA_FEATURE_WRITE_MONO

                /* Unregister the old ones */
                UnregisterIoPort(0x3D4);    // VGA_CRTC_INDEX_COLOR
                UnregisterIoPort(0x3D5);    // VGA_CRTC_DATA_COLOR
                UnregisterIoPort(0x3DA);    // VGA_INSTAT1_READ_COLOR, VGA_FEATURE_WRITE_COLOR
            }

            // if (VgaMiscRegister & 0x02) { /* Enable RAM access */ } else { /* Disable RAM access */ }
            break;
        }

        case VGA_FEATURE_WRITE_MONO:
        case VGA_FEATURE_WRITE_COLOR:
        {
            VgaFeatureRegister = Data;
            break;
        }

        case VGA_AC_INDEX:
        // case VGA_AC_WRITE:
        {
            if (!VgaAcLatch)
            {
                /* Change the index */
                BYTE Index = Data & 0x1F;
                if (Index < VGA_AC_MAX_REG) VgaAcIndex = Index;

                /*
                 * Change palette protection by checking for
                 * the Palette Address Source bit.
                 */
                VgaAcPalDisable = (Data & 0x20) ? TRUE : FALSE;
            }
            else
            {
                /* Write the data */
                VgaWriteAc(Data);
            }

            /* Toggle the latch */
            VgaAcLatch = !VgaAcLatch;
            break;
        }

        case VGA_SEQ_INDEX:
        {
            /* Set the sequencer index register */
            if (Data < VGA_SEQ_MAX_REG) VgaSeqIndex = Data;
            break;
        }

        case VGA_SEQ_DATA:
        {
            /* Call the sequencer function */
            VgaWriteSequencer(Data);
            break;
        }

        case VGA_DAC_MASK:
        {
            VgaDacMask = Data;
            break;
        }

        case VGA_DAC_READ_INDEX:
        {
            VgaDacReadWrite = FALSE;
            VgaDacIndex = Data * 3;
            break;
        }

        case VGA_DAC_WRITE_INDEX:
        {
            VgaDacReadWrite = TRUE;
            VgaDacIndex = Data * 3;
            break;
        }

        case VGA_DAC_DATA:
        {
            /* Ignore writes in read mode */
            if (VgaDacReadWrite) VgaWriteDac(Data & 0x3F);
            break;
        }

        case VGA_CRTC_INDEX_MONO:
        case VGA_CRTC_INDEX_COLOR:
        {
            /* Set the CRTC index register */
            if (Data < VGA_CRTC_MAX_REG) VgaCrtcIndex = Data;
            break;
        }

        case VGA_CRTC_DATA_MONO:
        case VGA_CRTC_DATA_COLOR:
        {
            /* Call the CRTC function */
            VgaWriteCrtc(Data);
            break;
        }

        case VGA_GC_INDEX:
        {
            /* Set the GC index register */
            if (Data < VGA_GC_MAX_REG) VgaGcIndex = Data;
            break;
        }

        case VGA_GC_DATA:
        {
            /* Call the GC function */
            VgaWriteGc(Data);
            break;
        }

        default:
            DPRINT1("VgaWritePort: Unknown port 0x%X\n", Port);
            break;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

DWORD VgaGetVideoBaseAddress(VOID)
{
    return MemoryBase[(VgaGcRegisters[VGA_GC_MISC_REG] >> 2) & 0x03];
}

DWORD VgaGetVideoLimitAddress(VOID)
{
    return MemoryLimit[(VgaGcRegisters[VGA_GC_MISC_REG] >> 2) & 0x03];
}

COORD VgaGetDisplayResolution(VOID)
{
    COORD Resolution;
    BYTE MaximumScanLine = 1 + (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & 0x1F);

    /* The low 8 bits are in the display registers */
    Resolution.X = VgaCrtcRegisters[VGA_CRTC_END_HORZ_DISP_REG];
    Resolution.Y = VgaCrtcRegisters[VGA_CRTC_VERT_DISP_END_REG];

    /* Set the top bits from the overflow register */
    if (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VDE8)
    {
        Resolution.Y |= 1 << 8;
    }
    if (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VDE9)
    {
        Resolution.Y |= 1 << 9;
    }

    /* Increase the values by 1 */
    Resolution.X++;
    Resolution.Y++;

    if (VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_NOALPHA)
    {
        /* Multiply the horizontal resolution by the 9/8 dot mode */
        Resolution.X *= (VgaSeqRegisters[VGA_SEQ_CLOCK_REG] & VGA_SEQ_CLOCK_98DM)
                        ? 8 : 9;

        /* The horizontal resolution is halved in 8-bit mode */
        if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT) Resolution.X /= 2;
    }

    if (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & VGA_CRTC_MAXSCANLINE_DOUBLE)
    {
        /* Halve the vertical resolution */
        Resolution.Y >>= 1;
    }
    else
    {
        /* Divide the vertical resolution by the maximum scan line (== font size in text mode) */
        Resolution.Y /= MaximumScanLine;
    }

    /* Return the resolution */
    return Resolution;
}

VOID VgaRefreshDisplay(VOID)
{
    HANDLE ConsoleBufferHandle = NULL;
    COORD Resolution;

    /* Set the vertical retrace flag */
    InVerticalRetrace = TRUE;

    /* If nothing has changed, just return */
    if (!ModeChanged && !CursorMoved && !PaletteChanged && !NeedsUpdate)
        return;

    /* Change the display mode */
    if (ModeChanged) VgaChangeMode();

    /* Change the text cursor location */
    if (CursorMoved) VgaUpdateTextCursor();

    /* Retrieve the current resolution */
    Resolution = VgaGetDisplayResolution();

    if (PaletteChanged)
    {
        /* Trigger a full update of the screen */
        NeedsUpdate = TRUE;
        UpdateRectangle.Left = 0;
        UpdateRectangle.Top = 0;
        UpdateRectangle.Right = Resolution.X;
        UpdateRectangle.Bottom = Resolution.Y;

        PaletteChanged = FALSE;
    }

    /* Update the contents of the framebuffer */
    VgaUpdateFramebuffer();

    /* Ignore if there's nothing to update */
    if (!NeedsUpdate) return;

    DPRINT("Updating screen rectangle (%d, %d, %d, %d)\n",
           UpdateRectangle.Left,
           UpdateRectangle.Top,
           UpdateRectangle.Right,
           UpdateRectangle.Bottom);

    /* Check if this is text mode or graphics mode */
    if (VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_NOALPHA)
    {
        /* Graphics mode */
        ConsoleBufferHandle = GraphicsConsoleBuffer;
    }
    else
    {
        /* Text mode */
        COORD Origin = { UpdateRectangle.Left, UpdateRectangle.Top };
        ConsoleBufferHandle = TextConsoleBuffer;

        /* Write the data to the console */
        WriteConsoleOutputA(TextConsoleBuffer,
                            (PCHAR_INFO)ConsoleFramebuffer,
                            Resolution,
                            Origin,
                            &UpdateRectangle);
    }

    /* In DoubleVision mode, scale the update rectangle */
    if (DoubleVision)
    {
        UpdateRectangle.Left *= 2;
        UpdateRectangle.Top  *= 2;
        UpdateRectangle.Right  = UpdateRectangle.Right  * 2 + 1;
        UpdateRectangle.Bottom = UpdateRectangle.Bottom * 2 + 1;
    }

    /* Redraw the screen */
    InvalidateConsoleDIBits(ConsoleBufferHandle, &UpdateRectangle);

    /* Clear the update flag */
    NeedsUpdate = FALSE;
}

VOID VgaHorizontalRetrace(VOID)
{
    /* Set the flag */
    InHorizontalRetrace = TRUE;
}

VOID VgaReadMemory(DWORD Address, LPBYTE Buffer, DWORD Size)
{
    DWORD i;
    DWORD VideoAddress;

    DPRINT("VgaReadMemory: Address 0x%08X, Size %lu\n", Address, Size);

    /* Ignore if video RAM access is disabled */
    if ((VgaMiscRegister & VGA_MISC_RAM_ENABLED) == 0) return;

    /* Loop through each byte */
    for (i = 0; i < Size; i++)
    {
        VideoAddress = VgaTranslateReadAddress(Address + i);

        /* Load the latch registers */
        VgaLatchRegisters[0] = VgaMemory[LOWORD(VideoAddress)];
        VgaLatchRegisters[1] = VgaMemory[VGA_BANK_SIZE + LOWORD(VideoAddress)];
        VgaLatchRegisters[2] = VgaMemory[(2 * VGA_BANK_SIZE) + LOWORD(VideoAddress)];
        VgaLatchRegisters[3] = VgaMemory[(3 * VGA_BANK_SIZE) + LOWORD(VideoAddress)];

        /* Copy the value to the buffer */
        Buffer[i] = VgaMemory[VideoAddress];
    }
}

VOID VgaWriteMemory(DWORD Address, LPBYTE Buffer, DWORD Size)
{
    DWORD i, j;
    DWORD VideoAddress;

    DPRINT("VgaWriteMemory: Address 0x%08X, Size %lu\n", Address, Size);

    /* Ignore if video RAM access is disabled */
    if ((VgaMiscRegister & VGA_MISC_RAM_ENABLED) == 0) return;

    /* Also ignore if write access to all planes is disabled */
    if ((VgaSeqRegisters[VGA_SEQ_MASK_REG] & 0x0F) == 0x00) return;

    /* Loop through each byte */
    for (i = 0; i < Size; i++)
    {
        VideoAddress = VgaTranslateWriteAddress(Address + i);

        for (j = 0; j < VGA_NUM_BANKS; j++)
        {
            /* Make sure the page is writeable */
            if (!(VgaSeqRegisters[VGA_SEQ_MASK_REG] & (1 << j))) continue;

            /* Check if this is chain-4 mode */
            if (VgaSeqRegisters[VGA_SEQ_MEM_REG] & VGA_SEQ_MEM_C4)
            {
                if (((Address + i) & 3) != j)
                {
                    /* This plane will not be accessed */
                    continue;
                }
            }

            /* Check if this is odd-even mode */
            if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_OE)
            {
                if (((Address + i) & 1) != (j & 1))
                {
                    /* This plane will not be accessed */
                    continue;
                }
            }

            /* Copy the value to the VGA memory */
            VgaMemory[VideoAddress + j * VGA_BANK_SIZE] = VgaTranslateByteForWriting(Buffer[i], j);
        }
    }
}

VOID VgaClearMemory(VOID)
{
    ZeroMemory(VgaMemory, sizeof(VgaMemory));
}

VOID VgaResetPalette(VOID)
{
    PALETTEENTRY Entries[VGA_MAX_COLORS];

    /* Restore the default palette */
    VgaRestoreDefaultPalette(Entries, VGA_MAX_COLORS);
    SetPaletteEntries(PaletteHandle, 0, VGA_MAX_COLORS, Entries);
    PaletteChanged = TRUE;
}

BOOLEAN VgaInitialize(HANDLE TextHandle)
{
    /* Save the default text-mode console output handle */
    if (TextHandle == INVALID_HANDLE_VALUE) return FALSE;
    TextConsoleBuffer = TextHandle;

    /* Save the console information */
    if (!GetConsoleScreenBufferInfo(TextConsoleBuffer, &ConsoleInfo))
    {
        return FALSE;
    }

    /* Initialize the VGA palette and fail if it isn't successfully created */
    if (!VgaInitializePalette()) return FALSE;
    /***/ VgaResetPalette(); /***/

    /* Switch to the text buffer */
    SetConsoleActiveScreenBuffer(TextConsoleBuffer);

    /* Clear the VGA memory */
    VgaClearMemory();

    /* Register the I/O Ports */
    RegisterIoPort(0x3CC, VgaReadPort, NULL);           // VGA_MISC_READ
    RegisterIoPort(0x3C2, VgaReadPort, VgaWritePort);   // VGA_MISC_WRITE, VGA_INSTAT0_READ
    RegisterIoPort(0x3CA, VgaReadPort, NULL);           // VGA_FEATURE_READ
    RegisterIoPort(0x3C0, VgaReadPort, VgaWritePort);   // VGA_AC_INDEX, VGA_AC_WRITE
    RegisterIoPort(0x3C1, VgaReadPort, NULL);           // VGA_AC_READ
    RegisterIoPort(0x3C4, VgaReadPort, VgaWritePort);   // VGA_SEQ_INDEX
    RegisterIoPort(0x3C5, VgaReadPort, VgaWritePort);   // VGA_SEQ_DATA
    RegisterIoPort(0x3C6, VgaReadPort, VgaWritePort);   // VGA_DAC_MASK
    RegisterIoPort(0x3C7, VgaReadPort, VgaWritePort);   // VGA_DAC_READ_INDEX
    RegisterIoPort(0x3C8, VgaReadPort, VgaWritePort);   // VGA_DAC_WRITE_INDEX
    RegisterIoPort(0x3C9, VgaReadPort, VgaWritePort);   // VGA_DAC_DATA
    RegisterIoPort(0x3CE, VgaReadPort, VgaWritePort);   // VGA_GC_INDEX
    RegisterIoPort(0x3CF, VgaReadPort, VgaWritePort);   // VGA_GC_DATA

    /* Return success */
    return TRUE;
}

/* EOF */
