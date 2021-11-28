/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef SKETCHER_GeoEnum_H
#define SKETCHER_GeoEnum_H

namespace Sketcher
{

/** Sketcher Geometry is identified with an index called GeoId.
 *
 * GeoId >= 0 are normal geometry elements
 * GeoId = -1 and -2 are the Horizontal and Vertical axes and the root point
 * GeoId <= -2 are external geometry elements
 * GeoId = -2000 is an undefined or unused geometry id
 *
 * GeoEnum struct provides convenience labels for these GeoIds.
 *
 * However, GeoEnum is not enough to define an element of a Geometry. The most
 * straighforward example is the RootPoint and the Horizontal Axis. Both have
 * the same GeoId (= -1).
 *
 * The same happens for elements of a given geometry. For example, a line has
 * a starting point, an endpoint and an edge. All these share the same GeoId, as the
 * GeoId identifies a geometry and not an element of a geometry.
 *
 * The elements of a given geometry are identified by the PointPos enum. In the case
 * of the root point, it is considered to be the start point of the Horizontal Axis, this
 * is a convention.
 *
 * Therefore, a geometry element (GeoElementId) is univocally defined by the combination of a
 * GeoId and a PointPos.
 *
 * Geometry shapes having more or different elements than those supported by the PointPos
 * struct, such as conics, in particular an arc of ellipse, are called complex geometries. The extra
 * elements of complex geometries are actual separate geometries (focus of an ellipse, line defining the
 * major axis of an ellipse, circle representing the weight of a BSpline), and they are call InternalAlignment
 * geometries.
 */
struct SketcherExport GeoEnum
{
    static const int RtPnt;     // GeoId of the Root Point
    static const int HAxis;     // GeoId of the Horizontal Axis
    static const int VAxis;     // GeoId of the Vertical Axis
    static const int RefExt;    // Starting GeoID of external geometry ( negative geoIds starting at this index)
    static const int GeoUndef;  // GeoId of an undefined Geometry (uninitialised or unused GeoId)
};

/*! PointPos lets us refer to different aspects of a piece of geometry.  sketcher::none refers
 * to an edge itself (eg., for a Perpendicular constraint on two lines). sketcher::start and
 * sketcher::end denote the endpoints of lines or bounded curves.  sketcher::mid denotes
 * geometries with geometrical centers (eg., circle, ellipse). Bare points use 'start'.  More
 * complex geometries like parabola focus or b-spline knots use InternalAlignment constraints
 * in addition to PointPos.
 */
enum class PointPos : int {
    none    = 0,    // Edge of a geometry
    start   = 1,    // Starting point of a geometry
    end     = 2,    // End point of a geometry
    mid     = 3     // Mid point of a geometry
};

class SketcherExport GeoElementId
{
public:
    explicit GeoElementId(int geoId = GeoEnum::GeoUndef, PointPos pos = PointPos::none);

    int GeoId;
    PointPos Pos;
};

} // namespace Sketcher


#endif // SKETCHER_GeoEnum_H

