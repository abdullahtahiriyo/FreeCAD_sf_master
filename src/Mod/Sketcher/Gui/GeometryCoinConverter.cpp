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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoFont.h>

# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoText2.h>
#endif  // #ifndef _PreComp_

#include <Mod/Part/App/Geometry.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include <Base/UnitsApi.h>

#include "EditModeCoinManagerParameters.h"

#include "Mod/Sketcher/App/Constraint.h"

#include "GeometryCoinConverter.h"


using namespace SketcherGui;

GeometryCoinConverter::GeometryCoinConverter(   GeometryLayerNodes & geometrylayernodes,
                                                DrawingParameters & drawingparameters,
                                                GeometryLayerParameters& geometryLayerParams,
                                                CoinMapping & coinMap ):
    geometryLayerNodes(geometrylayernodes),
    drawingParameters(drawingparameters),
    geometryLayerParameters(geometryLayerParams),
    coinMapping(coinMap)
{}

void GeometryCoinConverter::convert(const Sketcher::GeoListFacade & geolistfacade)
{

    // measurements
    bsplineGeoIds.clear();

    // end information layer
    Coords.clear();
    Points.clear();
    Index.clear();

    coinMapping.clear();

    pointCounter.clear();
    curveCounter.clear();

    for(int l=0; l<geometryLayerParameters.Layers; l++){
        Coords.emplace_back();
        Points.emplace_back();
        Index.emplace_back();

        coinMapping.CurvIdToGeoId.emplace_back();
        coinMapping.PointIdToGeoId.emplace_back();
        coinMapping.PointIdToVertexId.emplace_back();
    }

    pointCounter.resize(geometryLayerParameters.Layers,0);
    curveCounter.resize(geometryLayerParameters.Layers,0);

    // RootPoint
    // TODO: RootPoint is here added in layer0. However, this layer may be hidden. The point should,
    // when that functionality is provided, be added to the first visible layer, or may even a new
    // empty layer.
    Points[0].emplace_back(0.,0.,0.);
    coinMapping.PointIdToGeoId[0].push_back(-1); // root point
    coinMapping.PointIdToVertexId[0].push_back(-1); // VertexId is the reference used for point selection/preselection

    auto setTracking = [this] (int geoId, int layerId, GeometryCoinConverter::PointsMode pointmode, int numberCurves) {
        // TODO: This routine only works for one layer, for multiple layers the tracking is yet TBD
        int numberPoints = 0;

        if(pointmode == PointsMode::InsertSingle) {
            numberPoints = 1;

            coinMapping.GeoElementId2SetId.emplace( std::piecewise_construct,
                                                    std::forward_as_tuple(geoId, Sketcher::PointPos::start),
                                                    std::forward_as_tuple(pointCounter[layerId], layerId));
        }
        else if (pointmode == PointsMode::InsertStartEnd) {
            numberPoints = 2;

            coinMapping.GeoElementId2SetId.emplace( std::piecewise_construct,
                                                    std::forward_as_tuple(geoId, Sketcher::PointPos::start),
                                                    std::forward_as_tuple(pointCounter[layerId]++, layerId));

            coinMapping.GeoElementId2SetId.emplace( std::piecewise_construct,
                                                    std::forward_as_tuple(geoId, Sketcher::PointPos::end),
                                                    std::forward_as_tuple(pointCounter[layerId]++, layerId));
        }
        else if (pointmode == PointsMode::InsertMidOnly) {
            numberPoints = 1;

            coinMapping.GeoElementId2SetId.emplace( std::piecewise_construct,
                                                    std::forward_as_tuple(geoId, Sketcher::PointPos::mid),
                                                    std::forward_as_tuple(pointCounter[layerId], layerId));


        }
        else if (pointmode == PointsMode::InsertStartEndMid) {
            numberPoints = 3;

            coinMapping.GeoElementId2SetId.emplace( std::piecewise_construct,
                                                    std::forward_as_tuple(geoId, Sketcher::PointPos::start),
                                                    std::forward_as_tuple(pointCounter[layerId]++, layerId));

            coinMapping.GeoElementId2SetId.emplace( std::piecewise_construct,
                                                    std::forward_as_tuple(geoId, Sketcher::PointPos::end),
                                                    std::forward_as_tuple(pointCounter[layerId]++, layerId));

            coinMapping.GeoElementId2SetId.emplace( std::piecewise_construct,
                                                    std::forward_as_tuple(geoId, Sketcher::PointPos::mid),
                                                    std::forward_as_tuple(pointCounter[layerId]++, layerId));
        }

        for(int i = 0; i < numberPoints; i++) {
            coinMapping.PointIdToGeoId[layerId].push_back(geoId);
            coinMapping.PointIdToVertexId[layerId].push_back(vertexCounter++);
        }

        for(int i = 0; i < numberCurves; i++)
            coinMapping.CurvIdToGeoId[layerId].push_back(geoId);
    };

    // currently the whole geometrylist is processed in a single layer
    for (size_t i = 0 ; i < geolistfacade.geomlist.size()- 2; i++) {

        const auto GeoId = geolistfacade.getGeoIdFromGeomListIndex(i);
        const auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        const auto type = geom->getGeometry()->getTypeId();
        auto layerId = geom->getGeometryLayerId();

        if (type == Part::GeomPoint::getClassTypeId()) { // add a point
            convert< Part::GeomPoint,
                            GeometryCoinConverter::PointsMode::InsertSingle,
                            GeometryCoinConverter::CurveMode::NoCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>(geom);
            setTracking(GeoId, layerId, GeometryCoinConverter::PointsMode::InsertSingle, 0);
        }
        else if (type == Part::GeomLineSegment::getClassTypeId()) { // add a line
            convert< Part::GeomLineSegment,
                            GeometryCoinConverter::PointsMode::InsertStartEnd,
                            GeometryCoinConverter::CurveMode::StartEndPointsOnly,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>(geom);
            setTracking(GeoId, layerId, GeometryCoinConverter::PointsMode::InsertStartEnd, 1);
        }
        else if (type.isDerivedFrom(Part::GeomConic::getClassTypeId())) { // add a closed curve conic
            convert< Part::GeomConic,
                            GeometryCoinConverter::PointsMode::InsertMidOnly,
                            GeometryCoinConverter::CurveMode::ClosedCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>(geom);
            setTracking(GeoId, layerId, GeometryCoinConverter::PointsMode::InsertMidOnly, 1);
        }
        else if (type.isDerivedFrom(Part::GeomArcOfConic::getClassTypeId())) { // add an arc of conic
            convert< Part::GeomArcOfConic,
                            GeometryCoinConverter::PointsMode::InsertStartEndMid,
                            GeometryCoinConverter::CurveMode::OpenCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>(geom);
            setTracking(GeoId, layerId, GeometryCoinConverter::PointsMode::InsertStartEndMid, 1);
        }
        else if (type == Part::GeomBSplineCurve::getClassTypeId()) { // add a bspline (a bounded curve that is not a conic)
            convert< Part::GeomBSplineCurve,
                            GeometryCoinConverter::PointsMode::InsertStartEnd,
                            GeometryCoinConverter::CurveMode::OpenCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitudeAndBSplineCurvature>(geom);
            setTracking(GeoId, layerId, GeometryCoinConverter::PointsMode::InsertStartEnd, 1);
            bsplineGeoIds.push_back(GeoId);
        }
    }

    for(int l=0 ; l < geometryLayerParameters.Layers ; l++) {

        // Coin Nodes Editing
        geometryLayerNodes.CurvesCoordinate[l]->point.setNum(Coords[l].size());
        geometryLayerNodes.CurveSet[l]->numVertices.setNum(Index[l].size());
        geometryLayerNodes.CurvesMaterials[l]->diffuseColor.setNum(Index[l].size());
        geometryLayerNodes.PointsCoordinate[l]->point.setNum(Points[l].size());
        geometryLayerNodes.PointsMaterials[l]->diffuseColor.setNum(Points[l].size());

        SbVec3f *verts = geometryLayerNodes.CurvesCoordinate[l]->point.startEditing();
        int32_t *index = geometryLayerNodes.CurveSet[l]->numVertices.startEditing();
        SbVec3f *pverts = geometryLayerNodes.PointsCoordinate[l]->point.startEditing();

        int i=0; // setting up the line set
        for (std::vector<Base::Vector3d>::const_iterator it = Coords[l].begin(); it != Coords[l].end(); ++it,i++)
            verts[i].setValue(it->x,it->y,drawingParameters.zLowLines);

        i=0; // setting up the indexes of the line set
        for (std::vector<unsigned int>::const_iterator it = Index[l].begin(); it != Index[l].end(); ++it,i++)
            index[i] = *it;

        i=0; // setting up the point set
        for (std::vector<Base::Vector3d>::const_iterator it = Points[l].begin(); it != Points[l].end(); ++it,i++)
            pverts[i].setValue(it->x,it->y,drawingParameters.zLowPoints);

        geometryLayerNodes.CurvesCoordinate[l]->point.finishEditing();
        geometryLayerNodes.CurveSet[l]->numVertices.finishEditing();
        geometryLayerNodes.PointsCoordinate[l]->point.finishEditing();
    }
}

template < typename GeoType, GeometryCoinConverter::PointsMode pointmode, GeometryCoinConverter::CurveMode curvemode, GeometryCoinConverter::AnalyseMode analysemode >
void GeometryCoinConverter::convert(const Sketcher::GeometryFacade * geometryfacade) {
    auto geo = static_cast<const GeoType *>(geometryfacade->getGeometry());
    auto layerId = geometryfacade->getGeometryLayerId();

    auto addPoint = [&dMg = boundingBoxMaxMagnitude] (auto & pushvector, Base::Vector3d point) {

        if constexpr (analysemode == AnalyseMode::BoundingBoxMagnitude || analysemode == AnalyseMode::BoundingBoxMagnitudeAndBSplineCurvature) {
            dMg = dMg>std::abs(point.x)?dMg:std::abs(point.x);
            dMg = dMg>std::abs(point.y)?dMg:std::abs(point.y);
            pushvector.push_back(point);
        }
    };

    // Points
    if constexpr (pointmode == PointsMode::InsertSingle) {
        addPoint(Points[layerId], geo->getPoint());
    }
    else if constexpr (pointmode == PointsMode::InsertStartEnd) {
        addPoint(Points[layerId], geo->getStartPoint());
        addPoint(Points[layerId], geo->getEndPoint());
    }
    else if constexpr (pointmode == PointsMode::InsertStartEndMid) {
        // All in this group are Trimmed Curves (see Geometry.h)
        addPoint(Points[layerId], geo->getStartPoint(/*emulateCCW=*/true));
        addPoint(Points[layerId], geo->getEndPoint(/*emulateCCW=*/true));
        addPoint(Points[layerId], geo->getCenter());
    }
    else if constexpr (pointmode == PointsMode::InsertMidOnly) {
        addPoint(Points[layerId], geo->getCenter());
    }

    // Curves
    if constexpr (curvemode == CurveMode::StartEndPointsOnly) {
        addPoint(Coords[layerId], geo->getStartPoint());
        addPoint(Coords[layerId], geo->getEndPoint());
        Index[layerId].push_back(2);
    }
    else if constexpr (curvemode == CurveMode::ClosedCurve) {
        double segment = (geo->getLastParameter() - geo->getFirstParameter()) / drawingParameters.curvedEdgeCountSegments;

        for (int i=0; i < drawingParameters.curvedEdgeCountSegments; i++) {
            Base::Vector3d pnt = geo->value(i*segment);
            addPoint(Coords[layerId], pnt);
        }

        Base::Vector3d pnt = geo->value(0);
        addPoint(Coords[layerId], pnt);

        Index[layerId].push_back(drawingParameters.curvedEdgeCountSegments+1);
    }
    else if constexpr (curvemode == CurveMode::OpenCurve) {

        double segment = (geo->getLastParameter() - geo->getFirstParameter()) / drawingParameters.curvedEdgeCountSegments;

        for (int i=0; i < drawingParameters.curvedEdgeCountSegments; i++) {
            Base::Vector3d pnt = geo->value(geo->getFirstParameter() + i*segment);
            addPoint(Coords[layerId], pnt);
        }

        Base::Vector3d pnt = geo->value(geo->getLastParameter());
            addPoint(Coords[layerId], pnt);

        Index[layerId].push_back(drawingParameters.curvedEdgeCountSegments+1);

        if constexpr (analysemode == AnalyseMode::BoundingBoxMagnitudeAndBSplineCurvature) {
            //***************************************************************************************************************
            // global information gathering for geometry information layer

            std::vector<Base::Vector3d> poles = geo->getPoles();

            Base::Vector3d midp = Base::Vector3d(0,0,0);

            for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it) {
                midp += (*it);
            }

            midp /= poles.size();

            double firstparam = geo->getFirstParameter();
            double lastparam =  geo->getLastParameter();

            const int ndiv = poles.size()>4?poles.size()*16:64;
            double step = (lastparam - firstparam ) / (ndiv -1);

            std::vector<double> paramlist(ndiv);
            std::vector<Base::Vector3d> pointatcurvelist(ndiv);
            std::vector<double> curvaturelist(ndiv);
            std::vector<Base::Vector3d> normallist(ndiv);

            double maxcurv = 0;
            double maxdisttocenterofmass = 0;

            for (int i = 0; i < ndiv; i++) {
                paramlist[i] = firstparam + i * step;
                pointatcurvelist[i] = geo->pointAtParameter(paramlist[i]);

                try {
                    curvaturelist[i] = geo->curvatureAt(paramlist[i]);
                }
                catch(Base::CADKernelError &e) {
                    // it is "just" a visualisation matter OCC could not calculate the curvature
                    // terminating here would mean that the other shapes would not be drawn.
                    // Solution: Report the issue and set dummy curvature to 0
                    e.ReportException();
                    Base::Console().Error("Curvature graph for B-Spline with GeoId=%d could not be calculated.\n", 666); // TODO: Fix identification of curve.
                    curvaturelist[i] = 0;
                }

                if (curvaturelist[i] > maxcurv)
                    maxcurv = curvaturelist[i];

                double tempf = ( pointatcurvelist[i] - midp ).Length();

                if (tempf > maxdisttocenterofmass)
                    maxdisttocenterofmass = tempf;

            }

            double temprepscale = 0;
            if (maxcurv > 0)
                temprepscale = (0.5 * maxdisttocenterofmass) / maxcurv; // just a factor to make a comb reasonably visible

            if (temprepscale > combrepscale)
                combrepscale = temprepscale;

        }

    }

}

float GeometryCoinConverter::getBoundingBoxMaxMagnitude()
{
    return boundingBoxMaxMagnitude;
}

double GeometryCoinConverter::getCombRepresentationScale()
{
    return combrepscale;
}
