/***************************************************************************/
/*                                                                         */
/*  ftcbasic.c                                                             */
/*                                                                         */
/*    The FreeType basic cache interface (body).                           */
/*                                                                         */
/*  Copyright 2003, 2004 by                                                */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_CACHE_H
#include FT_CACHE_INTERNAL_GLYPH_H
#include FT_CACHE_INTERNAL_IMAGE_H
#include FT_CACHE_INTERNAL_SBITS_H
#include FT_INTERNAL_MEMORY_H

#include "ftcerror.h"


  /*
   *  Basic Families
   *
   */
  typedef struct  FTC_BasicAttrRec_
  {
    FTC_ScalerRec  scaler;
    FT_UInt        load_flags;

  } FTC_BasicAttrRec, *FTC_BasicAttrs;

#define FTC_BASIC_ATTR_COMPARE( a, b )                           \
          ( FTC_SCALER_COMPARE( &(a)->scaler, &(b)->scaler ) &&  \
            (a)->load_flags == (b)->load_flags               )

#define FTC_BASIC_ATTR_HASH( a )                                   \
          ( FTC_SCALER_HASH( &(a)->scaler ) + 31*(a)->load_flags )


  typedef struct  FTC_BasicQueryRec_
  {
    FTC_GQueryRec     gquery;
    FTC_BasicAttrRec  attrs;

  } FTC_BasicQueryRec, *FTC_BasicQuery;


  typedef struct  FTC_BasicFamilyRec_
  {
    FTC_FamilyRec     family;
    FTC_BasicAttrRec  attrs;

  } FTC_BasicFamilyRec, *FTC_BasicFamily;


  static FT_Bool
  ftc_basic_family_compare( FTC_BasicFamily  family,
                            FTC_BasicQuery   query )
  {
    return FT_BOOL( FTC_BASIC_ATTR_COMPARE( &family->attrs, &query->attrs ) );
  }


  static FT_Error
  ftc_basic_family_init( FTC_BasicFamily  family,
                         FTC_BasicQuery   query,
                         FTC_Cache        cache )
  {
    FTC_Family_Init( FTC_FAMILY( family ), cache );
    family->attrs = query->attrs;
    return 0;
  }


  static FT_UInt
  ftc_basic_family_get_count( FTC_BasicFamily  family,
                              FTC_Manager      manager )
  {
    FT_Error  error;
    FT_Face   face;
    FT_UInt   result = 0;


    error = FTC_Manager_LookupFace( manager, family->attrs.scaler.face_id,
                                    &face );
    if ( !error )
      result = face->num_glyphs;

    return result;
  }


  static FT_Error
  ftc_basic_family_load_bitmap( FTC_BasicFamily  family,
                                FT_UInt          gindex,
                                FTC_Manager      manager,
                                FT_Face         *aface )
  {
    FT_Error  error;
    FT_Size   size;


    error = FTC_Manager_LookupSize( manager, &family->attrs.scaler, &size );
    if ( !error )
    {
      FT_Face  face = size->face;


      error = FT_Load_Glyph( face, gindex,
                             family->attrs.load_flags | FT_LOAD_RENDER );
      if ( !error )
        *aface = face;
    }

    return error;
  }


  static FT_Error
  ftc_basic_family_load_glyph( FTC_BasicFamily  family,
                               FT_UInt          gindex,
                               FTC_Cache        cache,
                               FT_Glyph        *aglyph )
  {
    FT_Error    error;
    FTC_Scaler  scaler = &family->attrs.scaler;
    FT_Face     face;
    FT_Size     size;


    /* we will now load the glyph image */
    error = FTC_Manager_LookupSize( cache->manager,
                                    scaler,
                                    &size );
    if ( !error )
    {
      face = size->face;

      error = FT_Load_Glyph( face, gindex, family->attrs.load_flags );
      if ( !error )
      {
        if ( face->glyph->format == FT_GLYPH_FORMAT_BITMAP  ||
             face->glyph->format == FT_GLYPH_FORMAT_OUTLINE )
        {
          /* ok, copy it */
          FT_Glyph  glyph;


          error = FT_Get_Glyph( face->glyph, &glyph );
          if ( !error )
          {
            *aglyph = glyph;
            goto Exit;
          }
        }
        else
          error = FTC_Err_Invalid_Argument;
      }
    }

  Exit:
    return error;
  }


  static FT_Bool
  ftc_basic_gnode_compare_faceid( FTC_GNode   gnode,
                                  FTC_FaceID  face_id,
                                  FTC_Cache   cache )
  {
    FTC_BasicFamily  family = (FTC_BasicFamily)gnode->family;
    FT_Bool          result;


    result = FT_BOOL( family->attrs.scaler.face_id == face_id );
    if ( result )
    {
      /* we must call this function to avoid this node from appearing
       * in later lookups with the same face_id!
       */
      FTC_GNode_UnselectFamily( gnode, cache );
    }
    return result;
  }


 /*
  *
  * basic image cache
  *
  */

  static const FTC_IFamilyClassRec  ftc_basic_image_family_class =
  {
    {
      sizeof( FTC_BasicFamilyRec ),
      (FTC_MruNode_CompareFunc)ftc_basic_family_compare,
      (FTC_MruNode_InitFunc)   ftc_basic_family_init,
      (FTC_MruNode_ResetFunc)  NULL,
      (FTC_MruNode_DoneFunc)   NULL
    },
    (FTC_IFamily_LoadGlyphFunc)ftc_basic_family_load_glyph
  };


  static const FTC_GCacheClassRec  ftc_basic_image_cache_class =
  {
    {
      (FTC_Node_NewFunc)    FTC_INode_New,
      (FTC_Node_WeightFunc) FTC_INode_Weight,
      (FTC_Node_CompareFunc)FTC_GNode_Compare,
      (FTC_Node_CompareFunc)ftc_basic_gnode_compare_faceid,
      (FTC_Node_FreeFunc)   FTC_INode_Free,

      sizeof( FTC_GCacheRec ),
      (FTC_Cache_InitFunc)  FTC_GCache_Init,
      (FTC_Cache_DoneFunc)  FTC_GCache_Done
    },
    (FTC_MruListClass)&ftc_basic_image_family_class
  };


  FT_EXPORT_DEF( FT_Error )
  FTC_ImageCache_New( FTC_Manager      manager,
                      FTC_ImageCache  *acache )
  {
    return FTC_GCache_New( manager, &ftc_basic_image_cache_class,
                           (FTC_GCache*)acache );
  }


  /* documentation is in ftcimage.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_ImageCache_Lookup( FTC_ImageCache  cache,
                         FTC_ImageType   type,
                         FT_UInt         gindex,
                         FT_Glyph       *aglyph,
                         FTC_Node       *anode )
  {
    FTC_BasicQueryRec  query;
    FTC_INode          node;
    FT_Error           error;
    FT_UInt32          hash;


    /* some argument checks are delayed to FTC_Cache_Lookup */
    if ( !aglyph )
    {
      error = FTC_Err_Invalid_Argument;
      goto Exit;
    }

    *aglyph = NULL;
    if ( anode )
      *anode  = NULL;

    query.attrs.scaler.face_id = type->face_id;
    query.attrs.scaler.width   = type->width;
    query.attrs.scaler.height  = type->height;
    query.attrs.scaler.pixel   = 1;
    query.attrs.load_flags     = type->flags;

    query.attrs.scaler.x_res   = 0;  /* make compilers happy */
    query.attrs.scaler.y_res   = 0;

    hash = FTC_BASIC_ATTR_HASH( &query.attrs ) + gindex;

#if 1  /* inlining is about 50% faster! */
    FTC_GCACHE_LOOKUP_CMP( cache,
                           ftc_basic_family_compare,
                           FTC_GNode_Compare,
                           hash, gindex,
                           &query,
                           node,
                           error );
#else
    error = FTC_GCache_Lookup( FTC_GCACHE( cache ),
                               hash, gindex,
                               FTC_GQUERY( &query ),
                               (FTC_Node*) &node );
#endif
    if ( !error )
    {
      *aglyph = FTC_INODE( node )->glyph;

      if ( anode )
      {
        *anode = FTC_NODE( node );
        FTC_NODE( node )->ref_count++;
      }
    }

  Exit:
    return error;
  }


 /*
  *
  * basic small bitmap cache
  *
  */


  static const FTC_SFamilyClassRec  ftc_basic_sbit_family_class =
  {
    {
      sizeof( FTC_BasicFamilyRec ),
      (FTC_MruNode_CompareFunc)ftc_basic_family_compare,
      (FTC_MruNode_InitFunc)   ftc_basic_family_init,
      (FTC_MruNode_ResetFunc)  NULL,
      (FTC_MruNode_DoneFunc)   NULL
    },
    (FTC_SFamily_GetCountFunc) ftc_basic_family_get_count,
    (FTC_SFamily_LoadGlyphFunc)ftc_basic_family_load_bitmap
  };


  static const FTC_GCacheClassRec  ftc_basic_sbit_cache_class =
  {
    {
      (FTC_Node_NewFunc)    FTC_SNode_New,
      (FTC_Node_WeightFunc) FTC_SNode_Weight,
      (FTC_Node_CompareFunc)FTC_SNode_Compare,
      (FTC_Node_CompareFunc)ftc_basic_gnode_compare_faceid,
      (FTC_Node_FreeFunc)   FTC_SNode_Free,

      sizeof( FTC_GCacheRec ),
      (FTC_Cache_InitFunc)  FTC_GCache_Init,
      (FTC_Cache_DoneFunc)  FTC_GCache_Done
    },
    (FTC_MruListClass)&ftc_basic_sbit_family_class
  };


  FT_EXPORT_DEF( FT_Error )
  FTC_SBitCache_New( FTC_Manager     manager,
                     FTC_SBitCache  *acache )
  {
    return FTC_GCache_New( manager, &ftc_basic_sbit_cache_class,
                           (FTC_GCache*)acache );
  }


  FT_EXPORT_DEF( FT_Error )
  FTC_SBitCache_Lookup( FTC_SBitCache  cache,
                        FTC_ImageType  type,
                        FT_UInt        gindex,
                        FTC_SBit      *ansbit,
                        FTC_Node      *anode )
  {
    FT_Error           error;
    FTC_BasicQueryRec  query;
    FTC_SNode          node;
    FT_UInt32          hash;


    if ( anode )
      *anode = NULL;

    /* other argument checks delayed to FTC_Cache_Lookup */
    if ( !ansbit )
      return FTC_Err_Invalid_Argument;

    *ansbit = NULL;

    query.attrs.scaler.face_id = type->face_id;
    query.attrs.scaler.width   = type->width;
    query.attrs.scaler.height  = type->height;
    query.attrs.scaler.pixel   = 1;
    query.attrs.load_flags     = type->flags;

    query.attrs.scaler.x_res   = 0;  /* make compilers happy */
    query.attrs.scaler.y_res   = 0;

    /* beware, the hash must be the same for all glyph ranges! */
    hash = FTC_BASIC_ATTR_HASH( &query.attrs ) +
           gindex / FTC_SBIT_ITEMS_PER_NODE;

#if 1  /* inlining is about 50% faster! */
    FTC_GCACHE_LOOKUP_CMP( cache,
                           ftc_basic_family_compare,
                           FTC_SNode_Compare,
                           hash, gindex,
                           &query,
                           node,
                           error );
#else
    error = FTC_GCache_Lookup( FTC_GCACHE( cache ),
                               hash,
                               gindex,
                               FTC_GQUERY( &query ),
                               (FTC_Node*)&node );
#endif
    if ( error )
      goto Exit;

    *ansbit = node->sbits + ( gindex - FTC_GNODE( node )->gindex );

    if ( anode )
    {
      *anode = FTC_NODE( node );
      FTC_NODE( node )->ref_count++;
    }

  Exit:
    return error;
  }


/* END */
