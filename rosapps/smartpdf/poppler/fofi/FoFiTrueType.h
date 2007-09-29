//========================================================================
//
// FoFiTrueType.h
//
// Copyright 1999-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef FOFITRUETYPE_H
#define FOFITRUETYPE_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include "FoFiBase.h"

class GooString;
class GooHash;
struct TrueTypeTable;
struct TrueTypeCmap;

//------------------------------------------------------------------------
// FoFiTrueType
//------------------------------------------------------------------------

class FoFiTrueType: public FoFiBase {
public:

  // Create a FoFiTrueType object from a memory buffer.
  static FoFiTrueType *make(char *fileA, int lenA, int faceIndexA=0);

  // Create a FoFiTrueType object from a file on disk.
  static FoFiTrueType *load(char *fileName, int faceIndexA=0);

  FoFiTrueType(char *fileA, int lenA, GBool freeFileDataA, int faceIndexA=0);
  virtual ~FoFiTrueType();

  // Return the number of cmaps defined by this font.
  int getNumCmaps();

  // Return the platform ID of the <i>th cmap.
  int getCmapPlatform(int i);

  // Return the encoding ID of the <i>th cmap.
  int getCmapEncoding(int i);

  // Return the index of the cmap for <platform>, <encoding>.  Returns
  // -1 if there is no corresponding cmap.
  int findCmap(int platform, int encoding);

  // Return the GID corresponding to <c> according to the <i>th cmap.
  Gushort mapCodeToGID(int i, int c);

  // Returns the GID corresponding to <name> according to the post
  // table.  Returns 0 if there is no mapping for <name> or if the
  // font does not have a post table.
  int mapNameToGID(char *name);

  // Returns the least restrictive embedding licensing right (as
  // defined by the TrueType spec):
  // * 4: OS/2 table is missing or invalid
  // * 3: installable embedding
  // * 2: editable embedding
  // * 1: preview & print embedding
  // * 0: restricted license embedding
  int getEmbeddingRights();

  // Convert to a Type 42 font, suitable for embedding in a PostScript
  // file.  <psName> will be used as the PostScript font name (so we
  // don't need to depend on the 'name' table in the font).  The
  // <encoding> array specifies the mapping from char codes to names.
  // If <encoding> is NULL, the encoding is unknown or undefined.  The
  // <codeToGID> array specifies the mapping from char codes to GIDs.
  void convertToType42(char *psName, char **encoding,
		       Gushort *codeToGID,
		       FoFiOutputFunc outputFunc, void *outputStream);

  // Convert to a Type 2 CIDFont, suitable for embedding in a
  // PostScript file.  <psName> will be used as the PostScript font
  // name (so we don't need to depend on the 'name' table in the
  // font).  The <cidMap> array maps CIDs to GIDs; it has <nCIDs>
  // entries.
  void convertToCIDType2(char *psName, Gushort *cidMap, int nCIDs,
			 GBool needVerticalMetrics,
			 FoFiOutputFunc outputFunc, void *outputStream);

  // Convert to a Type 0 (but non-CID) composite font, suitable for
  // embedding in a PostScript file.  <psName> will be used as the
  // PostScript font name (so we don't need to depend on the 'name'
  // table in the font).  The <cidMap> array maps CIDs to GIDs; it has
  // <nCIDs> entries.
  void convertToType0(char *psName, Gushort *cidMap, int nCIDs,
		      GBool needVerticalMetrics,
		      FoFiOutputFunc outputFunc, void *outputStream);

  // Write a clean TTF file, filling in missing tables and correcting
  // various other errors.  If <name> is non-NULL, the font is renamed
  // to <name>.  If <codeToGID> is non-NULL, the font is re-encoded,
  // using a Windows Unicode cmap.  If <name> is NULL and the font is
  // complete and correct, it will be written unmodified.
  void writeTTF(FoFiOutputFunc outputFunc, void *outputStream,
		char *name = NULL, Gushort *codeToGID = NULL);

private:

  void cvtEncoding(char **encoding,
		   FoFiOutputFunc outputFunc,
		   void *outputStream);
  void cvtCharStrings(char **encoding,
		      Gushort *codeToGID,
		      FoFiOutputFunc outputFunc,
		      void *outputStream);
  void cvtSfnts(FoFiOutputFunc outputFunc,
		void *outputStream, GooString *name,
		GBool needVerticalMetrics);
  void dumpString(Guchar *s, int length,
		  FoFiOutputFunc outputFunc,
		  void *outputStream);
  Guint computeTableChecksum(Guchar *data, int length);
  void parse();
  void readPostTable();
  int seekTable(char *tag);

  TrueTypeTable *tables;
  int nTables;
  TrueTypeCmap *cmaps;
  int nCmaps;
  int nGlyphs;
  int locaFmt;
  int bbox[4];
  GooHash *nameToGID;

  GBool parsedOk;
  int faceIndex;
};

#endif
