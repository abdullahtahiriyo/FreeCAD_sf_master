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


#ifndef SKETCHERGUI_GeometryCoinConverter_H
#define SKETCHERGUI_GeometryCoinConverter_H

#include <vector>

namespace Base {
    template< typename T >
    class Vector3;

    class Vector2d;
}

namespace Part {
    class Geometry;
}

namespace SketcherGui {
    struct GeometryLayer;
    struct GeometryLayerNodes;
    struct DrawingParameters;

/** @brief      Class for creating the Geometry layer into coin nodes
 *  @details
 * Responsibility:
 * To create and update GeometryLayer nodes provided as constructor parameter
 * for the provided geometry, taking into account the drawing parameters provided as
 * constructor parameters.
 *
 * Interface:
 * A single entry point convert(), performing the following flow:
 *
 * [Geometry] => Analysis => construct drawing elements => Create mappings GeoId coin => populate coin nodes
 *
 * Analysis perform analysis such as maximum boundingbox magnitude of all geometries and maximum curvature of BSplines
 */
class GeometryCoinConverter {
    enum class PointsMode {
    InsertSingle,
    InsertStartEnd,
    InsertStartEndMid,
    InsertMidOnly
    };

enum class CurveMode {
    NoCurve,
    StartEndPointsOnly,
    ClosedCurve,
    OpenCurve
    };

enum class AnalyseMode {
    BoundingBoxMagnitude,
    BoundingBoxMagnitudeAndBSplineCurvature
    };

public:
    GeometryCoinConverter(  GeometryLayerNodes & geometrylayernodes,
                            DrawingParameters & drawingparameters );

    void convert(const GeometryLayer & geolayer);

    float getBoundingBoxMaxMagnitude();
    double getCombRepresentationScale();
    auto getBSplineGeoIds(){ return std::move(bsplineGeoIds);}

    auto getCurveMap(){ return std::move(CurvIdToGeoId);}
    auto getPointMap(){ return std::move(PointIdToGeoId);}


private:
    template < typename GeoType, PointsMode pointmode, CurveMode curvemode, AnalyseMode analysemode >
    void convert(const Part::Geometry * geometry);

private:
    GeometryLayerNodes & geometryLayerNodes;

    std::vector<Base::Vector3d> Coords;
    std::vector<Base::Vector3d> Points;
    std::vector<unsigned int> Index;

    // Parameters
    DrawingParameters & drawingParameters;

    // measurements
    float boundingBoxMaxMagnitude = 100;
    double combrepscale = 0; // the repscale that would correspond to this comb based only on this calculation.
    std::vector<int> bsplineGeoIds;

    // Mappings coin geoId
    std::vector<int> CurvIdToGeoId;
    std::vector<int> PointIdToGeoId;

};


} // namespace SketcherGui


#endif // SKETCHERGUI_GeometryCoinConverter_H

