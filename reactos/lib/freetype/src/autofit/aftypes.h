#ifndef __AFTYPES_H__
#define __AFTYPES_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H

FT_BEGIN_HEADER

 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****                D E B U G G I N G                               *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/

#define xxAF_DEBUG

#ifdef AF_DEBUG

#  include <stdio.h>
#  define AF_LOG( x )  printf x

#else

#  define AF_LOG( x )  do ; while ( 0 ) /* nothing */

#endif /* AF_DEBUG */

 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****                U T I L I T Y                                   *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/

  typedef struct AF_WidthRec_
  {
    FT_Pos  org;  /* original position/width in font units              */
    FT_Pos  cur;  /* current/scaled position/width in device sub-pixels */
    FT_Pos  fit;  /* current/fitted position/width in device sub-pixels */

  } AF_WidthRec, *AF_Width;


  FT_LOCAL( void )
  af_sort_pos( FT_UInt   count,
               FT_Pos*   table );

  FT_LOCAL( void )
  af_sort_widths( FT_UInt   count,
                  AF_Width  widths );


 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****                A N G L E   T Y P E S                           *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/

 /*
  *  Angle type. The auto-fitter doesn't need a very high angular accuracy,
  *  and this allows us to speed up some computations considerably with a
  *  light Cordic algorithm (see afangles.c)
  *
  */

  typedef FT_Int    AF_Angle;

#define  AF_ANGLE_PI     128
#define  AF_ANGLE_2PI    (AF_ANGLE_PI*2)
#define  AF_ANGLE_PI2    (AF_ANGLE_PI/2)
#define  AF_ANGLE_PI4    (AF_ANGLE_PI/4)

 /*
  *  compute the angle of a given 2-D vector
  *
  */
  FT_LOCAL( AF_Angle )
  af_angle_atan( FT_Pos  dx,
                 FT_Pos  dy );


 /*
  *  computes "angle2 - angle1", the result is always within
  *  the range [ -AF_ANGLE_PI .. AF_ANGLE_PI-1 ]
  *
  */
  FT_LOCAL( AF_Angle )
  af_angle_diff( AF_Angle  angle1,
                 AF_Angle  angle2 );


 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****                O U T L I N E S                                 *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/

 /* opaque handle to glyph-specific hints. see "afhints.h" for more
  * details
  */
  typedef struct AF_GlyphHintsRec_*     AF_GlyphHints;

 /* this structure is used to model an input glyph outline to
  * the auto-hinter. The latter will set the "hints" field
  * depending on the glyph's script
  */
  typedef struct AF_OutlineRec_
  {
    FT_Face          face;
    FT_Outline       outline;
    FT_UInt          outline_resolution;

    FT_Int           advance;
    FT_UInt          metrics_resolution;

    AF_GlyphHints    hints;

  } AF_OutlineRec;


 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****                S C A L E R S                                   *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/

 /*
  *  A scaler models the target pixel device that will receive the
  *  auto-hinted glyph image
  *
  */

  typedef enum
  {
    AF_SCALER_FLAG_NO_HORIZONTAL = 1,  /* disable horizontal hinting */
    AF_SCALER_FLAG_NO_VERTICAL   = 2,  /* disable vertical hinting   */
    AF_SCALER_FLAG_NO_ADVANCE    = 4   /* disable advance hinting    */

  } AF_ScalerFlags;


  typedef struct AF_ScalerRec_
  {
    FT_Face         face;         /* source font face                        */
    FT_Fixed        x_scale;      /* from font units to 1/64th device pixels */
    FT_Fixed        y_scale;      /* from font units to 1/64th device pixels */
    FT_Pos          x_delta;      /* in 1/64th device pixels                 */
    FT_Pos          y_delta;      /* in 1/64th device pixels                 */
    FT_Render_Mode  render_mode;  /* monochrome, anti-aliased, LCD, etc..    */
    FT_UInt32       flags;        /* additionnal control flags, see above    */

  } AF_ScalerRec, *AF_Scaler;



 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****                S C R I P T S                                   *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/

 /*
  *  the list of know scripts. Each different script correspond to the
  *  following information:
  *
  *   - a set of Unicode ranges to test weither the face supports the
  *     script
  *
  *   - a specific global analyzer that will compute global metrics
  *     specific to the script.
  *
  *   - a specific glyph analyzer that will compute segments and
  *     edges for each glyph covered by the script
  *
  *   - a specific grid-fitting algorithm that will distort the
  *     scaled glyph outline according to the results of the glyph
  *     analyzer
  *
  *  note that a given analyzer and/or grid-fitting algorithm can be
  *  used by more than one script
  */
  typedef enum
  {
    AF_SCRIPT_NONE  = 0,
    AF_SCRIPT_LATIN = 1,
    /* add new scripts here. don't forget to update the list in "afglobal.c" */

    AF_SCRIPT_MAX   /* do not remove */

  } AF_Script;



  typedef struct AF_ScriptClassRec_ const*  AF_ScriptClass;

  typedef struct AF_ScriptMetricsRec_
  {
    AF_ScriptClass    clazz;
    AF_ScalerRec      scaler;

  } AF_ScriptMetricsRec, *AF_ScriptMetrics;


 /* this function parses a FT_Face to compute global metrics for
  * a specific script
  */
  typedef FT_Error  (*AF_Script_InitMetricsFunc)( AF_ScriptMetrics   metrics,
                                                  FT_Face            face );

  typedef void      (*AF_Script_ScaleMetricsFunc)( AF_ScriptMetrics  metrics,
                                                   AF_Scaler         scaler );

  typedef void      (*AF_Script_DoneMetricsFunc)( AF_ScriptMetrics   metrics );


  typedef FT_Error  (*AF_Script_InitHintsFunc)( AF_GlyphHints     hints,
                                                FT_Outline*       outline,
                                                AF_ScriptMetrics  metrics );

  typedef void      (*AF_Script_ApplyHintsFunc)( AF_GlyphHints     hints,
                                                 FT_Outline*       outline,
                                                 AF_ScriptMetrics  metrics );


  typedef struct AF_Script_UniRangeRec_
  {
    FT_UInt32    first;
    FT_UInt32    last;

  } AF_Script_UniRangeRec;

  typedef const AF_Script_UniRangeRec *  AF_Script_UniRange;

  typedef struct AF_ScriptClassRec_
  {
    AF_Script                   script;
    AF_Script_UniRange          script_uni_ranges;  /* last must be { 0, 0 } */

    FT_UInt                     script_metrics_size;
    AF_Script_InitMetricsFunc   script_metrics_init;
    AF_Script_ScaleMetricsFunc  script_metrics_scale;
    AF_Script_DoneMetricsFunc   script_metrics_done;

    AF_Script_InitHintsFunc     script_hints_init;
    AF_Script_ApplyHintsFunc    script_hints_apply;

  } AF_ScriptClassRec;


/* */

FT_END_HEADER

#endif /* __AFTYPES_H__ */
