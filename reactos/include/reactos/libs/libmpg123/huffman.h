/*
	huffman.h: huffman tables ... recalcualted to work with optimized decoder scheme (MH)

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp

	probably we could save a few bytes of memory, because the 
	smaller tables are often the part of a bigger table
*/


#ifndef _MPG123_HUFFMAN_H_
#define _MPG123_HUFFMAN_H_

struct newhuff 
{
  unsigned int linbits;
  const short *table;
};

static const short tab0[] = 
{ 
   0
};

static const short tab1[] =
{
  -5,  -3,  -1,  17,   1,  16,   0
};

static const short tab2[] =
{
 -15, -11,  -9,  -5,  -3,  -1,  34,   2,  18,  -1,  33,  32,  17,  -1,   1,
  16,   0
};

static const short tab3[] =
{
 -13, -11,  -9,  -5,  -3,  -1,  34,   2,  18,  -1,  33,  32,  16,  17,  -1,
   1,   0
};

static const short tab5[] =
{
 -29, -25, -23, -15,  -7,  -5,  -3,  -1,  51,  35,  50,  49,  -3,  -1,  19,
   3,  -1,  48,  34,  -3,  -1,  18,  33,  -1,   2,  32,  17,  -1,   1,  16,
   0
};

static const short tab6[] =
{
 -25, -19, -13,  -9,  -5,  -3,  -1,  51,   3,  35,  -1,  50,  48,  -1,  19,
  49,  -3,  -1,  34,   2,  18,  -3,  -1,  33,  32,   1,  -1,  17,  -1,  16,
   0
};

static const short tab7[] =
{
 -69, -65, -57, -39, -29, -17, -11,  -7,  -3,  -1,  85,  69,  -1,  84,  83,
  -1,  53,  68,  -3,  -1,  37,  82,  21,  -5,  -1,  81,  -1,   5,  52,  -1,
  80,  -1,  67,  51,  -5,  -3,  -1,  36,  66,  20,  -1,  65,  64, -11,  -7,
  -3,  -1,   4,  35,  -1,  50,   3,  -1,  19,  49,  -3,  -1,  48,  34,  18,
  -5,  -1,  33,  -1,   2,  32,  17,  -1,   1,  16,   0
};

static const short tab8[] =
{
 -65, -63, -59, -45, -31, -19, -13,  -7,  -5,  -3,  -1,  85,  84,  69,  83,
  -3,  -1,  53,  68,  37,  -3,  -1,  82,   5,  21,  -5,  -1,  81,  -1,  52,
  67,  -3,  -1,  80,  51,  36,  -5,  -3,  -1,  66,  20,  65,  -3,  -1,   4,
  64,  -1,  35,  50,  -9,  -7,  -3,  -1,  19,  49,  -1,   3,  48,  34,  -1,
   2,  32,  -1,  18,  33,  17,  -3,  -1,   1,  16,   0
};

static const short tab9[] =
{
 -63, -53, -41, -29, -19, -11,  -5,  -3,  -1,  85,  69,  53,  -1,  83,  -1,
  84,   5,  -3,  -1,  68,  37,  -1,  82,  21,  -3,  -1,  81,  52,  -1,  67,
  -1,  80,   4,  -7,  -3,  -1,  36,  66,  -1,  51,  64,  -1,  20,  65,  -5,
  -3,  -1,  35,  50,  19,  -1,  49,  -1,   3,  48,  -5,  -3,  -1,  34,   2,
  18,  -1,  33,  32,  -3,  -1,  17,   1,  -1,  16,   0
};

static const short tab10[] =
{
-125,-121,-111, -83, -55, -35, -21, -13,  -7,  -3,  -1, 119, 103,  -1, 118,
  87,  -3,  -1, 117, 102,  71,  -3,  -1, 116,  86,  -1, 101,  55,  -9,  -3,
  -1, 115,  70,  -3,  -1,  85,  84,  99,  -1,  39, 114, -11,  -5,  -3,  -1,
 100,   7, 112,  -1,  98,  -1,  69,  53,  -5,  -1,   6,  -1,  83,  68,  23,
 -17,  -5,  -1, 113,  -1,  54,  38,  -5,  -3,  -1,  37,  82,  21,  -1,  81,
  -1,  52,  67,  -3,  -1,  22,  97,  -1,  96,  -1,   5,  80, -19, -11,  -7,
  -3,  -1,  36,  66,  -1,  51,   4,  -1,  20,  65,  -3,  -1,  64,  35,  -1,
  50,   3,  -3,  -1,  19,  49,  -1,  48,  34,  -7,  -3,  -1,  18,  33,  -1,
   2,  32,  17,  -1,   1,  16,   0
};

static const short tab11[] =
{
-121,-113, -89, -59, -43, -27, -17,  -7,  -3,  -1, 119, 103,  -1, 118, 117,
  -3,  -1, 102,  71,  -1, 116,  -1,  87,  85,  -5,  -3,  -1,  86, 101,  55,
  -1, 115,  70,  -9,  -7,  -3,  -1,  69,  84,  -1,  53,  83,  39,  -1, 114,
  -1, 100,   7,  -5,  -1, 113,  -1,  23, 112,  -3,  -1,  54,  99,  -1,  96,
  -1,  68,  37, -13,  -7,  -5,  -3,  -1,  82,   5,  21,  98,  -3,  -1,  38,
   6,  22,  -5,  -1,  97,  -1,  81,  52,  -5,  -1,  80,  -1,  67,  51,  -1,
  36,  66, -15, -11,  -7,  -3,  -1,  20,  65,  -1,   4,  64,  -1,  35,  50,
  -1,  19,  49,  -5,  -3,  -1,   3,  48,  34,  33,  -5,  -1,  18,  -1,   2,
  32,  17,  -3,  -1,   1,  16,   0
};

static const short tab12[] =
{
-115, -99, -73, -45, -27, -17,  -9,  -5,  -3,  -1, 119, 103, 118,  -1,  87,
 117,  -3,  -1, 102,  71,  -1, 116, 101,  -3,  -1,  86,  55,  -3,  -1, 115,
  85,  39,  -7,  -3,  -1, 114,  70,  -1, 100,  23,  -5,  -1, 113,  -1,   7,
 112,  -1,  54,  99, -13,  -9,  -3,  -1,  69,  84,  -1,  68,  -1,   6,   5,
  -1,  38,  98,  -5,  -1,  97,  -1,  22,  96,  -3,  -1,  53,  83,  -1,  37,
  82, -17,  -7,  -3,  -1,  21,  81,  -1,  52,  67,  -5,  -3,  -1,  80,   4,
  36,  -1,  66,  20,  -3,  -1,  51,  65,  -1,  35,  50, -11,  -7,  -5,  -3,
  -1,  64,   3,  48,  19,  -1,  49,  34,  -1,  18,  33,  -7,  -5,  -3,  -1,
   2,  32,   0,  17,  -1,   1,  16
};

static const short tab13[] =
{
-509,-503,-475,-405,-333,-265,-205,-153,-115, -83, -53, -35, -21, -13,  -9,
  -7,  -5,  -3,  -1, 254, 252, 253, 237, 255,  -1, 239, 223,  -3,  -1, 238,
 207,  -1, 222, 191,  -9,  -3,  -1, 251, 206,  -1, 220,  -1, 175, 233,  -1,
 236, 221,  -9,  -5,  -3,  -1, 250, 205, 190,  -1, 235, 159,  -3,  -1, 249,
 234,  -1, 189, 219, -17,  -9,  -3,  -1, 143, 248,  -1, 204,  -1, 174, 158,
  -5,  -1, 142,  -1, 127, 126, 247,  -5,  -1, 218,  -1, 173, 188,  -3,  -1,
 203, 246, 111, -15,  -7,  -3,  -1, 232,  95,  -1, 157, 217,  -3,  -1, 245,
 231,  -1, 172, 187,  -9,  -3,  -1,  79, 244,  -3,  -1, 202, 230, 243,  -1,
  63,  -1, 141, 216, -21,  -9,  -3,  -1,  47, 242,  -3,  -1, 110, 156,  15,
  -5,  -3,  -1, 201,  94, 171,  -3,  -1, 125, 215,  78, -11,  -5,  -3,  -1,
 200, 214,  62,  -1, 185,  -1, 155, 170,  -1,  31, 241, -23, -13,  -5,  -1,
 240,  -1, 186, 229,  -3,  -1, 228, 140,  -1, 109, 227,  -5,  -1, 226,  -1,
  46,  14,  -1,  30, 225, -15,  -7,  -3,  -1, 224,  93,  -1, 213, 124,  -3,
  -1, 199,  77,  -1, 139, 184,  -7,  -3,  -1, 212, 154,  -1, 169, 108,  -1,
 198,  61, -37, -21,  -9,  -5,  -3,  -1, 211, 123,  45,  -1, 210,  29,  -5,
  -1, 183,  -1,  92, 197,  -3,  -1, 153, 122, 195,  -7,  -5,  -3,  -1, 167,
 151,  75, 209,  -3,  -1,  13, 208,  -1, 138, 168, -11,  -7,  -3,  -1,  76,
 196,  -1, 107, 182,  -1,  60,  44,  -3,  -1, 194,  91,  -3,  -1, 181, 137,
  28, -43, -23, -11,  -5,  -1, 193,  -1, 152,  12,  -1, 192,  -1, 180, 106,
  -5,  -3,  -1, 166, 121,  59,  -1, 179,  -1, 136,  90, -11,  -5,  -1,  43,
  -1, 165, 105,  -1, 164,  -1, 120, 135,  -5,  -1, 148,  -1, 119, 118, 178,
 -11,  -3,  -1,  27, 177,  -3,  -1,  11, 176,  -1, 150,  74,  -7,  -3,  -1,
  58, 163,  -1,  89, 149,  -1,  42, 162, -47, -23,  -9,  -3,  -1,  26, 161,
  -3,  -1,  10, 104, 160,  -5,  -3,  -1, 134,  73, 147,  -3,  -1,  57,  88,
  -1, 133, 103,  -9,  -3,  -1,  41, 146,  -3,  -1,  87, 117,  56,  -5,  -1,
 131,  -1, 102,  71,  -3,  -1, 116,  86,  -1, 101, 115, -11,  -3,  -1,  25,
 145,  -3,  -1,   9, 144,  -1,  72, 132,  -7,  -5,  -1, 114,  -1,  70, 100,
  40,  -1, 130,  24, -41, -27, -11,  -5,  -3,  -1,  55,  39,  23,  -1, 113,
  -1,  85,   7,  -7,  -3,  -1, 112,  54,  -1,  99,  69,  -3,  -1,  84,  38,
  -1,  98,  53,  -5,  -1, 129,  -1,   8, 128,  -3,  -1,  22,  97,  -1,   6,
  96, -13,  -9,  -5,  -3,  -1,  83,  68,  37,  -1,  82,   5,  -1,  21,  81,
  -7,  -3,  -1,  52,  67,  -1,  80,  36,  -3,  -1,  66,  51,  20, -19, -11,
  -5,  -1,  65,  -1,   4,  64,  -3,  -1,  35,  50,  19,  -3,  -1,  49,   3,
  -1,  48,  34,  -3,  -1,  18,  33,  -1,   2,  32,  -3,  -1,  17,   1,  16,
   0
};

static const short tab15[] =
{
-495,-445,-355,-263,-183,-115, -77, -43, -27, -13,  -7,  -3,  -1, 255, 239,
  -1, 254, 223,  -1, 238,  -1, 253, 207,  -7,  -3,  -1, 252, 222,  -1, 237,
 191,  -1, 251,  -1, 206, 236,  -7,  -3,  -1, 221, 175,  -1, 250, 190,  -3,
  -1, 235, 205,  -1, 220, 159, -15,  -7,  -3,  -1, 249, 234,  -1, 189, 219,
  -3,  -1, 143, 248,  -1, 204, 158,  -7,  -3,  -1, 233, 127,  -1, 247, 173,
  -3,  -1, 218, 188,  -1, 111,  -1, 174,  15, -19, -11,  -3,  -1, 203, 246,
  -3,  -1, 142, 232,  -1,  95, 157,  -3,  -1, 245, 126,  -1, 231, 172,  -9,
  -3,  -1, 202, 187,  -3,  -1, 217, 141,  79,  -3,  -1, 244,  63,  -1, 243,
 216, -33, -17,  -9,  -3,  -1, 230,  47,  -1, 242,  -1, 110, 240,  -3,  -1,
  31, 241,  -1, 156, 201,  -7,  -3,  -1,  94, 171,  -1, 186, 229,  -3,  -1,
 125, 215,  -1,  78, 228, -15,  -7,  -3,  -1, 140, 200,  -1,  62, 109,  -3,
  -1, 214, 227,  -1, 155, 185,  -7,  -3,  -1,  46, 170,  -1, 226,  30,  -5,
  -1, 225,  -1,  14, 224,  -1,  93, 213, -45, -25, -13,  -7,  -3,  -1, 124,
 199,  -1,  77, 139,  -1, 212,  -1, 184, 154,  -7,  -3,  -1, 169, 108,  -1,
 198,  61,  -1, 211, 210,  -9,  -5,  -3,  -1,  45,  13,  29,  -1, 123, 183,
  -5,  -1, 209,  -1,  92, 208,  -1, 197, 138, -17,  -7,  -3,  -1, 168,  76,
  -1, 196, 107,  -5,  -1, 182,  -1, 153,  12,  -1,  60, 195,  -9,  -3,  -1,
 122, 167,  -1, 166,  -1, 192,  11,  -1, 194,  -1,  44,  91, -55, -29, -15,
  -7,  -3,  -1, 181,  28,  -1, 137, 152,  -3,  -1, 193,  75,  -1, 180, 106,
  -5,  -3,  -1,  59, 121, 179,  -3,  -1, 151, 136,  -1,  43,  90, -11,  -5,
  -1, 178,  -1, 165,  27,  -1, 177,  -1, 176, 105,  -7,  -3,  -1, 150,  74,
  -1, 164, 120,  -3,  -1, 135,  58, 163, -17,  -7,  -3,  -1,  89, 149,  -1,
  42, 162,  -3,  -1,  26, 161,  -3,  -1,  10, 160, 104,  -7,  -3,  -1, 134,
  73,  -1, 148,  57,  -5,  -1, 147,  -1, 119,   9,  -1,  88, 133, -53, -29,
 -13,  -7,  -3,  -1,  41, 103,  -1, 118, 146,  -1, 145,  -1,  25, 144,  -7,
  -3,  -1,  72, 132,  -1,  87, 117,  -3,  -1,  56, 131,  -1, 102,  71,  -7,
  -3,  -1,  40, 130,  -1,  24, 129,  -7,  -3,  -1, 116,   8,  -1, 128,  86,
  -3,  -1, 101,  55,  -1, 115,  70, -17,  -7,  -3,  -1,  39, 114,  -1, 100,
  23,  -3,  -1,  85, 113,  -3,  -1,   7, 112,  54,  -7,  -3,  -1,  99,  69,
  -1,  84,  38,  -3,  -1,  98,  22,  -3,  -1,   6,  96,  53, -33, -19,  -9,
  -5,  -1,  97,  -1,  83,  68,  -1,  37,  82,  -3,  -1,  21,  81,  -3,  -1,
   5,  80,  52,  -7,  -3,  -1,  67,  36,  -1,  66,  51,  -1,  65,  -1,  20,
   4,  -9,  -3,  -1,  35,  50,  -3,  -1,  64,   3,  19,  -3,  -1,  49,  48,
  34,  -9,  -7,  -3,  -1,  18,  33,  -1,   2,  32,  17,  -3,  -1,   1,  16,
   0
};

static const short tab16[] =
{
-509,-503,-461,-323,-103, -37, -27, -15,  -7,  -3,  -1, 239, 254,  -1, 223,
 253,  -3,  -1, 207, 252,  -1, 191, 251,  -5,  -1, 175,  -1, 250, 159,  -3,
  -1, 249, 248, 143,  -7,  -3,  -1, 127, 247,  -1, 111, 246, 255,  -9,  -5,
  -3,  -1,  95, 245,  79,  -1, 244, 243, -53,  -1, 240,  -1,  63, -29, -19,
 -13,  -7,  -5,  -1, 206,  -1, 236, 221, 222,  -1, 233,  -1, 234, 217,  -1,
 238,  -1, 237, 235,  -3,  -1, 190, 205,  -3,  -1, 220, 219, 174, -11,  -5,
  -1, 204,  -1, 173, 218,  -3,  -1, 126, 172, 202,  -5,  -3,  -1, 201, 125,
  94, 189, 242, -93,  -5,  -3,  -1,  47,  15,  31,  -1, 241, -49, -25, -13,
  -5,  -1, 158,  -1, 188, 203,  -3,  -1, 142, 232,  -1, 157, 231,  -7,  -3,
  -1, 187, 141,  -1, 216, 110,  -1, 230, 156, -13,  -7,  -3,  -1, 171, 186,
  -1, 229, 215,  -1,  78,  -1, 228, 140,  -3,  -1, 200,  62,  -1, 109,  -1,
 214, 155, -19, -11,  -5,  -3,  -1, 185, 170, 225,  -1, 212,  -1, 184, 169,
  -5,  -1, 123,  -1, 183, 208, 227,  -7,  -3,  -1,  14, 224,  -1,  93, 213,
  -3,  -1, 124, 199,  -1,  77, 139, -75, -45, -27, -13,  -7,  -3,  -1, 154,
 108,  -1, 198,  61,  -3,  -1,  92, 197,  13,  -7,  -3,  -1, 138, 168,  -1,
 153,  76,  -3,  -1, 182, 122,  60, -11,  -5,  -3,  -1,  91, 137,  28,  -1,
 192,  -1, 152, 121,  -1, 226,  -1,  46,  30, -15,  -7,  -3,  -1, 211,  45,
  -1, 210, 209,  -5,  -1,  59,  -1, 151, 136,  29,  -7,  -3,  -1, 196, 107,
  -1, 195, 167,  -1,  44,  -1, 194, 181, -23, -13,  -7,  -3,  -1, 193,  12,
  -1,  75, 180,  -3,  -1, 106, 166, 179,  -5,  -3,  -1,  90, 165,  43,  -1,
 178,  27, -13,  -5,  -1, 177,  -1,  11, 176,  -3,  -1, 105, 150,  -1,  74,
 164,  -5,  -3,  -1, 120, 135, 163,  -3,  -1,  58,  89,  42, -97, -57, -33,
 -19, -11,  -5,  -3,  -1, 149, 104, 161,  -3,  -1, 134, 119, 148,  -5,  -3,
  -1,  73,  87, 103, 162,  -5,  -1,  26,  -1,  10, 160,  -3,  -1,  57, 147,
  -1,  88, 133,  -9,  -3,  -1,  41, 146,  -3,  -1, 118,   9,  25,  -5,  -1,
 145,  -1, 144,  72,  -3,  -1, 132, 117,  -1,  56, 131, -21, -11,  -5,  -3,
  -1, 102,  40, 130,  -3,  -1,  71, 116,  24,  -3,  -1, 129, 128,  -3,  -1,
   8,  86,  55,  -9,  -5,  -1, 115,  -1, 101,  70,  -1,  39, 114,  -5,  -3,
  -1, 100,  85,   7,  23, -23, -13,  -5,  -1, 113,  -1, 112,  54,  -3,  -1,
  99,  69,  -1,  84,  38,  -3,  -1,  98,  22,  -1,  97,  -1,   6,  96,  -9,
  -5,  -1,  83,  -1,  53,  68,  -1,  37,  82,  -1,  81,  -1,  21,   5, -33,
 -23, -13,  -7,  -3,  -1,  52,  67,  -1,  80,  36,  -3,  -1,  66,  51,  20,
  -5,  -1,  65,  -1,   4,  64,  -1,  35,  50,  -3,  -1,  19,  49,  -3,  -1,
   3,  48,  34,  -3,  -1,  18,  33,  -1,   2,  32,  -3,  -1,  17,   1,  16,
   0
};

static const short tab24[] =
{
-451,-117, -43, -25, -15,  -7,  -3,  -1, 239, 254,  -1, 223, 253,  -3,  -1,
 207, 252,  -1, 191, 251,  -5,  -1, 250,  -1, 175, 159,  -1, 249, 248,  -9,
  -5,  -3,  -1, 143, 127, 247,  -1, 111, 246,  -3,  -1,  95, 245,  -1,  79,
 244, -71,  -7,  -3,  -1,  63, 243,  -1,  47, 242,  -5,  -1, 241,  -1,  31,
 240, -25,  -9,  -1,  15,  -3,  -1, 238, 222,  -1, 237, 206,  -7,  -3,  -1,
 236, 221,  -1, 190, 235,  -3,  -1, 205, 220,  -1, 174, 234, -15,  -7,  -3,
  -1, 189, 219,  -1, 204, 158,  -3,  -1, 233, 173,  -1, 218, 188,  -7,  -3,
  -1, 203, 142,  -1, 232, 157,  -3,  -1, 217, 126,  -1, 231, 172, 255,-235,
-143, -77, -45, -25, -15,  -7,  -3,  -1, 202, 187,  -1, 141, 216,  -5,  -3,
  -1,  14, 224,  13, 230,  -5,  -3,  -1, 110, 156, 201,  -1,  94, 186,  -9,
  -5,  -1, 229,  -1, 171, 125,  -1, 215, 228,  -3,  -1, 140, 200,  -3,  -1,
  78,  46,  62, -15,  -7,  -3,  -1, 109, 214,  -1, 227, 155,  -3,  -1, 185,
 170,  -1, 226,  30,  -7,  -3,  -1, 225,  93,  -1, 213, 124,  -3,  -1, 199,
  77,  -1, 139, 184, -31, -15,  -7,  -3,  -1, 212, 154,  -1, 169, 108,  -3,
  -1, 198,  61,  -1, 211,  45,  -7,  -3,  -1, 210,  29,  -1, 123, 183,  -3,
  -1, 209,  92,  -1, 197, 138, -17,  -7,  -3,  -1, 168, 153,  -1,  76, 196,
  -3,  -1, 107, 182,  -3,  -1, 208,  12,  60,  -7,  -3,  -1, 195, 122,  -1,
 167,  44,  -3,  -1, 194,  91,  -1, 181,  28, -57, -35, -19,  -7,  -3,  -1,
 137, 152,  -1, 193,  75,  -5,  -3,  -1, 192,  11,  59,  -3,  -1, 176,  10,
  26,  -5,  -1, 180,  -1, 106, 166,  -3,  -1, 121, 151,  -3,  -1, 160,   9,
 144,  -9,  -3,  -1, 179, 136,  -3,  -1,  43,  90, 178,  -7,  -3,  -1, 165,
  27,  -1, 177, 105,  -1, 150, 164, -17,  -9,  -5,  -3,  -1,  74, 120, 135,
  -1,  58, 163,  -3,  -1,  89, 149,  -1,  42, 162,  -7,  -3,  -1, 161, 104,
  -1, 134, 119,  -3,  -1,  73, 148,  -1,  57, 147, -63, -31, -15,  -7,  -3,
  -1,  88, 133,  -1,  41, 103,  -3,  -1, 118, 146,  -1,  25, 145,  -7,  -3,
  -1,  72, 132,  -1,  87, 117,  -3,  -1,  56, 131,  -1, 102,  40, -17,  -7,
  -3,  -1, 130,  24,  -1,  71, 116,  -5,  -1, 129,  -1,   8, 128,  -1,  86,
 101,  -7,  -5,  -1,  23,  -1,   7, 112, 115,  -3,  -1,  55,  39, 114, -15,
  -7,  -3,  -1,  70, 100,  -1,  85, 113,  -3,  -1,  54,  99,  -1,  69,  84,
  -7,  -3,  -1,  38,  98,  -1,  22,  97,  -5,  -3,  -1,   6,  96,  53,  -1,
  83,  68, -51, -37, -23, -15,  -9,  -3,  -1,  37,  82,  -1,  21,  -1,   5,
  80,  -1,  81,  -1,  52,  67,  -3,  -1,  36,  66,  -1,  51,  20,  -9,  -5,
  -1,  65,  -1,   4,  64,  -1,  35,  50,  -1,  19,  49,  -7,  -5,  -3,  -1,
   3,  48,  34,  18,  -1,  33,  -1,   2,  32,  -3,  -1,  17,   1,  -1,  16,
   0
};

static const short tab_c0[] =
{
 -29, -21, -13,  -7,  -3,  -1,  11,  15,  -1,  13,  14,  -3,  -1,   7,   5,
   9,  -3,  -1,   6,   3,  -1,  10,  12,  -3,  -1,   2,   1,  -1,   4,   8,
   0
};

static const short tab_c1[] =
{
 -15,  -7,  -3,  -1,  15,  14,  -1,  13,  12,  -3,  -1,  11,  10,  -1,   9,
   8,  -7,  -3,  -1,   7,   6,  -1,   5,   4,  -3,  -1,   3,   2,  -1,   1,
   0
};



static const struct newhuff ht[] = 
{
 { /* 0 */ 0 , tab0  } ,
 { /* 2 */ 0 , tab1  } ,
 { /* 3 */ 0 , tab2  } ,
 { /* 3 */ 0 , tab3  } ,
 { /* 0 */ 0 , tab0  } ,
 { /* 4 */ 0 , tab5  } ,
 { /* 4 */ 0 , tab6  } ,
 { /* 6 */ 0 , tab7  } ,
 { /* 6 */ 0 , tab8  } ,
 { /* 6 */ 0 , tab9  } ,
 { /* 8 */ 0 , tab10 } ,
 { /* 8 */ 0 , tab11 } ,
 { /* 8 */ 0 , tab12 } ,
 { /* 16 */ 0 , tab13 } ,
 { /* 0  */ 0 , tab0  } ,
 { /* 16 */ 0 , tab15 } ,

 { /* 16 */ 1 , tab16 } ,
 { /* 16 */ 2 , tab16 } ,
 { /* 16 */ 3 , tab16 } ,
 { /* 16 */ 4 , tab16 } ,
 { /* 16 */ 6 , tab16 } ,
 { /* 16 */ 8 , tab16 } ,
 { /* 16 */ 10, tab16 } ,
 { /* 16 */ 13, tab16 } ,
 { /* 16 */ 4 , tab24 } ,
 { /* 16 */ 5 , tab24 } ,
 { /* 16 */ 6 , tab24 } ,
 { /* 16 */ 7 , tab24 } ,
 { /* 16 */ 8 , tab24 } ,
 { /* 16 */ 9 , tab24 } ,
 { /* 16 */ 11, tab24 } ,
 { /* 16 */ 13, tab24 }
};

static const struct newhuff htc[] = 
{
 { /* 1 , 1 , */ 0 , tab_c0 } ,
 { /* 1 , 1 , */ 0 , tab_c1 }
};


#endif
