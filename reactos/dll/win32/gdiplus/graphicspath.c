/*
 * Copyright (C) 2007 Google (Evan Stade)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

/* make sure path has enough space for len more points */
static BOOL lengthen_path(GpPath *path, INT len)
{
    /* initial allocation */
    if(path->datalen == 0){
        path->datalen = len * 2;

        path->pathdata.Points = GdipAlloc(path->datalen * sizeof(PointF));
        if(!path->pathdata.Points)   return FALSE;

        path->pathdata.Types = GdipAlloc(path->datalen);
        if(!path->pathdata.Types){
            GdipFree(path->pathdata.Points);
            return FALSE;
        }
    }
    /* reallocation, double size of arrays */
    else if(path->datalen - path->pathdata.Count < len){
        while(path->datalen - path->pathdata.Count < len)
            path->datalen *= 2;

        path->pathdata.Points = HeapReAlloc(GetProcessHeap(), 0,
            path->pathdata.Points, path->datalen * sizeof(PointF));
        if(!path->pathdata.Points)  return FALSE;

        path->pathdata.Types = HeapReAlloc(GetProcessHeap(), 0,
            path->pathdata.Types, path->datalen);
        if(!path->pathdata.Types)   return FALSE;
    }

    return TRUE;
}

GpStatus WINGDIPAPI GdipAddPathArc(GpPath *path, REAL x1, REAL y1, REAL x2,
    REAL y2, REAL startAngle, REAL sweepAngle)
{
    INT count, old_count, i;

    if(!path)
        return InvalidParameter;

    count = arc2polybezier(NULL, x1, y1, x2, y2, startAngle, sweepAngle);

    if(count == 0)
        return Ok;
    if(!lengthen_path(path, count))
        return OutOfMemory;

    old_count = path->pathdata.Count;
    arc2polybezier(&path->pathdata.Points[old_count], x1, y1, x2, y2,
                   startAngle, sweepAngle);

    for(i = 0; i < count; i++){
        path->pathdata.Types[old_count + i] = PathPointTypeBezier;
    }

    path->pathdata.Types[old_count] =
        (path->newfigure ? PathPointTypeStart : PathPointTypeLine);
    path->newfigure = FALSE;
    path->pathdata.Count += count;

    return Ok;
}

GpStatus WINGDIPAPI GdipAddPathBezierI(GpPath *path, INT x1, INT y1, INT x2,
    INT y2, INT x3, INT y3, INT x4, INT y4)
{
    INT old_count;

    if(!path)
        return InvalidParameter;

    if(!lengthen_path(path, 4))
        return OutOfMemory;

    old_count = path->pathdata.Count;

    path->pathdata.Points[old_count].X = (REAL) x1;
    path->pathdata.Points[old_count].Y = (REAL) y1;
    path->pathdata.Points[old_count + 1].X = (REAL) x2;
    path->pathdata.Points[old_count + 1].Y = (REAL) y2;
    path->pathdata.Points[old_count + 2].X = (REAL) x3;
    path->pathdata.Points[old_count + 2].Y = (REAL) y3;
    path->pathdata.Points[old_count + 3].X = (REAL) x4;
    path->pathdata.Points[old_count + 3].Y = (REAL) y4;

    path->pathdata.Types[old_count] =
        (path->newfigure ? PathPointTypeStart : PathPointTypeLine);
    path->pathdata.Types[old_count + 1] = PathPointTypeBezier;
    path->pathdata.Types[old_count + 2] = PathPointTypeBezier;
    path->pathdata.Types[old_count + 3] = PathPointTypeBezier;

    path->newfigure = FALSE;
    path->pathdata.Count += 4;

    return Ok;
}

GpStatus WINGDIPAPI GdipAddPathBeziers(GpPath *path, GDIPCONST GpPointF *points,
    INT count)
{
    INT i, old_count;

    if(!path || !points || ((count - 1) % 3))
        return InvalidParameter;

    if(!lengthen_path(path, count))
        return OutOfMemory;

    old_count = path->pathdata.Count;

    for(i = 0; i < count; i++){
        path->pathdata.Points[old_count + i].X = points[i].X;
        path->pathdata.Points[old_count + i].Y = points[i].Y;
        path->pathdata.Types[old_count + i] = PathPointTypeBezier;
    }

    path->pathdata.Types[old_count] =
        (path->newfigure ? PathPointTypeStart : PathPointTypeLine);
    path->newfigure = FALSE;
    path->pathdata.Count += count;

    return Ok;
}

GpStatus WINGDIPAPI GdipAddPathEllipse(GpPath *path, REAL x, REAL y, REAL width,
    REAL height)
{
    INT old_count, numpts;

    if(!path)
        return InvalidParameter;

    if(!lengthen_path(path, MAX_ARC_PTS))
        return OutOfMemory;

    old_count = path->pathdata.Count;
    if((numpts = arc2polybezier(&path->pathdata.Points[old_count],  x, y, width,
                               height, 0.0, 360.0)) != MAX_ARC_PTS){
        ERR("expected %d points but got %d\n", MAX_ARC_PTS, numpts);
        return GenericError;
    }

    memset(&path->pathdata.Types[old_count + 1], PathPointTypeBezier,
           MAX_ARC_PTS - 1);

    /* An ellipse is an instrinsic figure (always its own subpath). */
    path->pathdata.Types[old_count] = PathPointTypeStart;
    path->pathdata.Types[old_count + MAX_ARC_PTS - 1] |= PathPointTypeCloseSubpath;
    path->newfigure = TRUE;
    path->pathdata.Count += MAX_ARC_PTS;

    return Ok;
}

GpStatus WINGDIPAPI GdipAddPathLine2(GpPath *path, GDIPCONST GpPointF *points,
    INT count)
{
    INT i, old_count;

    if(!path || !points)
        return InvalidParameter;

    if(!lengthen_path(path, count))
        return OutOfMemory;

    old_count = path->pathdata.Count;

    for(i = 0; i < count; i++){
        path->pathdata.Points[old_count + i].X = points[i].X;
        path->pathdata.Points[old_count + i].Y = points[i].Y;
        path->pathdata.Types[old_count + i] = PathPointTypeLine;
    }

    if(path->newfigure){
        path->pathdata.Types[old_count] = PathPointTypeStart;
        path->newfigure = FALSE;
    }

    path->pathdata.Count += count;

    return Ok;
}

GpStatus WINGDIPAPI GdipAddPathLineI(GpPath *path, INT x1, INT y1, INT x2, INT y2)
{
    INT old_count;

    if(!path)
        return InvalidParameter;

    if(!lengthen_path(path, 2))
        return OutOfMemory;

    old_count = path->pathdata.Count;

    path->pathdata.Points[old_count].X = (REAL) x1;
    path->pathdata.Points[old_count].Y = (REAL) y1;
    path->pathdata.Points[old_count + 1].X = (REAL) x2;
    path->pathdata.Points[old_count + 1].Y = (REAL) y2;

    path->pathdata.Types[old_count] =
        (path->newfigure ? PathPointTypeStart : PathPointTypeLine);
    path->pathdata.Types[old_count + 1] = PathPointTypeLine;

    path->newfigure = FALSE;
    path->pathdata.Count += 2;

    return Ok;
}

GpStatus WINGDIPAPI GdipAddPathPath(GpPath *path, GDIPCONST GpPath* addingPath,
    BOOL connect)
{
    INT old_count, count;

    if(!path || !addingPath)
        return InvalidParameter;

    old_count = path->pathdata.Count;
    count = addingPath->pathdata.Count;

    if(!lengthen_path(path, count))
        return OutOfMemory;

    memcpy(&path->pathdata.Points[old_count], addingPath->pathdata.Points,
           count * sizeof(GpPointF));
    memcpy(&path->pathdata.Types[old_count], addingPath->pathdata.Types, count);

    if(path->newfigure || !connect)
        path->pathdata.Types[old_count] = PathPointTypeStart;
    else
        path->pathdata.Types[old_count] = PathPointTypeLine;

    path->newfigure = FALSE;
    path->pathdata.Count += count;

    return Ok;
}

GpStatus WINGDIPAPI GdipClonePath(GpPath* path, GpPath **clone)
{
    if(!path || !clone)
        return InvalidParameter;

    *clone = GdipAlloc(sizeof(GpPath));
    if(!*clone) return OutOfMemory;

    **clone = *path;

    (*clone)->pathdata.Points = GdipAlloc(path->datalen * sizeof(PointF));
    (*clone)->pathdata.Types = GdipAlloc(path->datalen);
    if(!(*clone)->pathdata.Points || !(*clone)->pathdata.Types){
        GdipFree(*clone);
        GdipFree((*clone)->pathdata.Points);
        GdipFree((*clone)->pathdata.Types);
        return OutOfMemory;
    }

    memcpy((*clone)->pathdata.Points, path->pathdata.Points,
           path->datalen * sizeof(PointF));
    memcpy((*clone)->pathdata.Types, path->pathdata.Types, path->datalen);

    return Ok;
}

GpStatus WINGDIPAPI GdipClosePathFigure(GpPath* path)
{
    if(!path)
        return InvalidParameter;

    if(path->pathdata.Count > 0){
        path->pathdata.Types[path->pathdata.Count - 1] |= PathPointTypeCloseSubpath;
        path->newfigure = TRUE;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipClosePathFigures(GpPath* path)
{
    INT i;

    if(!path)
        return InvalidParameter;

    for(i = 1; i < path->pathdata.Count; i++){
        if(path->pathdata.Types[i] == PathPointTypeStart)
            path->pathdata.Types[i-1] |= PathPointTypeCloseSubpath;
    }

    path->newfigure = TRUE;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePath(GpFillMode fill, GpPath **path)
{
    if(!path)
        return InvalidParameter;

    *path = GdipAlloc(sizeof(GpPath));
    if(!*path)  return OutOfMemory;

    (*path)->fill = fill;
    (*path)->newfigure = TRUE;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePath2(GDIPCONST GpPointF* points,
    GDIPCONST BYTE* types, INT count, GpFillMode fill, GpPath **path)
{
    if(!path)
        return InvalidParameter;

    *path = GdipAlloc(sizeof(GpPath));
    if(!*path)  return OutOfMemory;

    (*path)->pathdata.Points = GdipAlloc(count * sizeof(PointF));
    (*path)->pathdata.Types = GdipAlloc(count);

    if(!(*path)->pathdata.Points || !(*path)->pathdata.Types){
        GdipFree((*path)->pathdata.Points);
        GdipFree((*path)->pathdata.Types);
        GdipFree(*path);
        return OutOfMemory;
    }

    memcpy((*path)->pathdata.Points, points, count * sizeof(PointF));
    memcpy((*path)->pathdata.Types, types, count);
    (*path)->pathdata.Count = count;
    (*path)->datalen = count;

    (*path)->fill = fill;
    (*path)->newfigure = TRUE;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeletePath(GpPath *path)
{
    if(!path)
        return InvalidParameter;

    GdipFree(path->pathdata.Points);
    GdipFree(path->pathdata.Types);
    GdipFree(path);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathFillMode(GpPath *path, GpFillMode *fillmode)
{
    if(!path || !fillmode)
        return InvalidParameter;

    *fillmode = path->fill;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathPoints(GpPath *path, GpPointF* points, INT count)
{
    if(!path)
        return InvalidParameter;

    if(count < path->pathdata.Count)
        return InsufficientBuffer;

    memcpy(points, path->pathdata.Points, path->pathdata.Count * sizeof(GpPointF));

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathTypes(GpPath *path, BYTE* types, INT count)
{
    if(!path)
        return InvalidParameter;

    if(count < path->pathdata.Count)
        return InsufficientBuffer;

    memcpy(types, path->pathdata.Types, path->pathdata.Count);

    return Ok;
}

/* Windows expands the bounding box to the maximum possible bounding box
 * for a given pen.  For example, if a line join can extend past the point
 * it's joining by x units, the bounding box is extended by x units in every
 * direction (even though this is too conservative for most cases). */
GpStatus WINGDIPAPI GdipGetPathWorldBounds(GpPath* path, GpRectF* bounds,
    GDIPCONST GpMatrix *matrix, GDIPCONST GpPen *pen)
{
    GpPointF * points, temp_pts[4];
    INT count, i;
    REAL path_width = 1.0, width, height, temp, low_x, low_y, high_x, high_y;

    /* Matrix and pen can be null. */
    if(!path || !bounds)
        return InvalidParameter;

    /* If path is empty just return. */
    count = path->pathdata.Count;
    if(count == 0){
        bounds->X = bounds->Y = bounds->Width = bounds->Height = 0.0;
        return Ok;
    }

    points = path->pathdata.Points;

    low_x = high_x = points[0].X;
    low_y = high_y = points[0].Y;

    for(i = 1; i < count; i++){
        low_x = min(low_x, points[i].X);
        low_y = min(low_y, points[i].Y);
        high_x = max(high_x, points[i].X);
        high_y = max(high_y, points[i].Y);
    }

    width = high_x - low_x;
    height = high_y - low_y;

    /* This looks unusual but it's the only way I can imitate windows. */
    if(matrix){
        temp_pts[0].X = low_x;
        temp_pts[0].Y = low_y;
        temp_pts[1].X = low_x;
        temp_pts[1].Y = high_y;
        temp_pts[2].X = high_x;
        temp_pts[2].Y = high_y;
        temp_pts[3].X = high_x;
        temp_pts[3].Y = low_y;

        GdipTransformMatrixPoints((GpMatrix*)matrix, temp_pts, 4);
        low_x = temp_pts[0].X;
        low_y = temp_pts[0].Y;

        for(i = 1; i < 4; i++){
            low_x = min(low_x, temp_pts[i].X);
            low_y = min(low_y, temp_pts[i].Y);
        }

        temp = width;
        width = height * fabs(matrix->matrix[2]) + width * fabs(matrix->matrix[0]);
        height = height * fabs(matrix->matrix[3]) + temp * fabs(matrix->matrix[1]);
    }

    if(pen){
        path_width = pen->width / 2.0;

        if(count > 2)
            path_width = max(path_width,  pen->width * pen->miterlimit / 2.0);
        /* FIXME: this should probably also check for the startcap */
        if(pen->endcap & LineCapNoAnchor)
            path_width = max(path_width,  pen->width * 2.2);

        low_x -= path_width;
        low_y -= path_width;
        width += 2.0 * path_width;
        height += 2.0 * path_width;
    }

    bounds->X = low_x;
    bounds->Y = low_y;
    bounds->Width = width;
    bounds->Height = height;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPointCount(GpPath *path, INT *count)
{
    if(!path)
        return InvalidParameter;

    *count = path->pathdata.Count;

    return Ok;
}

GpStatus WINGDIPAPI GdipIsOutlineVisiblePathPointI(GpPath* path, INT x, INT y,
    GpPen *pen, GpGraphics *graphics, BOOL *result)
{
    static int calls;

    if(!path || !pen)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipStartPathFigure(GpPath *path)
{
    if(!path)
        return InvalidParameter;

    path->newfigure = TRUE;

    return Ok;
}

GpStatus WINGDIPAPI GdipResetPath(GpPath *path)
{
    if(!path)
        return InvalidParameter;

    path->pathdata.Count = 0;
    path->newfigure = TRUE;
    path->fill = FillModeAlternate;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathFillMode(GpPath *path, GpFillMode fill)
{
    if(!path)
        return InvalidParameter;

    path->fill = fill;

    return Ok;
}

GpStatus WINGDIPAPI GdipTransformPath(GpPath *path, GpMatrix *matrix)
{
    if(!path)
        return InvalidParameter;

    if(path->pathdata.Count == 0)
        return Ok;

    return GdipTransformMatrixPoints(matrix, path->pathdata.Points,
                                     path->pathdata.Count);
}
