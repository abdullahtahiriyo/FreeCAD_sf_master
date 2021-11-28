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

namespace Sketcher {
    enum ConstraintType : int;
    enum class PointPos : int;
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
 * Analysis performs analysis such as maximum boundingbox magnitude of all geometries and maximum curvature of BSplines
 */
class GeometryCoinConverter {
// These internal private classes are used to parametrize the conversion of geometry into points and line sets (see template method convert)
private:
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
    /** Constructs an GeometryCoinConverter responsible for
     * generating the points and line sets for drawing the geometry
     * defined by a GeometryLayer into the coin nodes provided by
     * GeometryLayerNodes.
     *
     * @param geometrylayernodes: The coin nodes to be populated with
     * the geometry
     *
     * @param drawingparameters: Parameters for drawing the overlay information
     */
    GeometryCoinConverter(  GeometryLayerNodes & geometrylayernodes,
                            DrawingParameters & drawingparameters );

    /**
    * converts the geometry defined by GeometryLayer into the coin nodes.
    *
    * @param geometry: the geometry to be processed
    */
    void convert(const GeometryLayer & geolayer);

    /**
    * returns the maximum of the vertical and horizontal magnitudes of the
    * coordinates of the points and lines added to coin by this layer (local responsibility).
    */
    float getBoundingBoxMaxMagnitude();

    /**
    * returns the Comb representation scale that should be used to represent
    * the B-Splines of this layer (local responsibility).
    */
    double getCombRepresentationScale();

    /**
    * returns the GeoIds of BSpline geometries
    */
    auto getBSplineGeoIds(){ return std::move(bsplineGeoIds);}

    /**
    * returns the local mapping of CurveIds of this layer and GeoIds.
    * CurveIds are the position in the layer occupied by the curves.
    */
    auto getCurveMap(){ return std::move(CurvIdToGeoId);}

    /**
    * returns the local mapping of PointIds of this layer and GeoIds.
    * PointIds are the position in the layer occupied by the points.
    */
    auto getPointMap(){ return std::move(PointIdToGeoId);}

    auto getReversePointMap() { return std::move(GeoIdPointPosToPointId);}

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
    std::map<std::pair<int, Sketcher::PointPos>, int> GeoIdPointPosToPointId;

};


} // namespace SketcherGui


#endif // SKETCHERGUI_GeometryCoinConverter_H

