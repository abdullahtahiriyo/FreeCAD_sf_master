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
# include <Gui/Inventor/SmSwitchboard.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>

# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoDrawStyle.h>

# include <memory>
#endif  // #ifndef _PreComp_

#include "CoinManager.h"

#include "EditData.h"

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Base/Exception.h>


using namespace SketcherGui;
using namespace Sketcher;

//**************************** ParameterObserver nested class ******************************
CoinManager::ParameterObserver::ParameterObserver(CoinManager * pclient): pClient(pclient)
{
    initParameters();
    subscribeToParameters();
}

CoinManager::ParameterObserver::~ParameterObserver()
{
    unsubscribeToParameters();
}

void CoinManager::ParameterObserver::initParameters()
{
    updateCurvedEdgeCountSegmentsParameter();

}

void CoinManager::ParameterObserver::updateCurvedEdgeCountSegmentsParameter()
{
    if(!pClient)
        return;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    int stdcountsegments = hGrp->GetInt("SegmentsPerGeometry", 50);
    // value cannot be smaller than 6
    if (stdcountsegments < 6)
        stdcountsegments = 6;

    pClient->drawingParameters.curvedEdgeCountSegments = stdcountsegments;
}

void CoinManager::ParameterObserver::subscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Attach(this);
}

void CoinManager::ParameterObserver::unsubscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Detach(this);
}

void CoinManager::ParameterObserver::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    (void) rCaller;

    if (strcmp(sReason, "SegmentsPerGeometry") == 0)
        updateCurvedEdgeCountSegmentsParameter();

}

//*************************** GeometryCoinConverter ***************************

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
    BoundingBox,
    BoundingBoxAndBSplineCurvature
};


class GeometryCoinConverter {
public:
    GeometryCoinConverter(std::vector<Base::Vector3d> & points,
                          std::vector<Base::Vector3d> & coords,
                          std::vector<unsigned int> & index,
                          int curvedEdgeCountSegments
                         ): Points(points), Coords(coords), Index(index), CurvedEdgeCountSegments(curvedEdgeCountSegments){}

    template < typename GeoType, PointsMode pointmode, CurveMode curvemode, AnalyseMode analysemode >
    void convert(const Part::Geometry * geometry) {
        auto geo = static_cast<const GeoType *>(geometry);

        auto addPoint = [&dMg = dMg] (auto & pushvector, Base::Vector3d point) {

            if constexpr (analysemode == AnalyseMode::BoundingBox || analysemode == AnalyseMode::BoundingBoxAndBSplineCurvature ) {
                dMg = dMg>std::abs(point.x)?dMg:std::abs(point.x);
                dMg = dMg>std::abs(point.y)?dMg:std::abs(point.y);
                pushvector.push_back(point);
            }
        };

        // Points
        if constexpr (pointmode == PointsMode::InsertSingle) {
            addPoint(Points, geo->getPoint());
        }
        else if constexpr (pointmode == PointsMode::InsertStartEnd) {
            addPoint(Points, geo->getStartPoint());
            addPoint(Points, geo->getEndPoint());
        }
        else if constexpr (pointmode == PointsMode::InsertStartEndMid) {
            // All in this group are Trimmed Curves (see Geometry.h)
            addPoint(Points, geo->getStartPoint(/*emulateCCW=*/true));
            addPoint(Points, geo->getEndPoint(/*emulateCCW=*/true));
            addPoint(Points, geo->getCenter());
        }
        else if constexpr (pointmode == PointsMode::InsertMidOnly) {
            addPoint(Points, geo->getCenter());
        }

        // Curves
        if constexpr (curvemode == CurveMode::StartEndPointsOnly) {
            addPoint(Coords, geo->getStartPoint());
            addPoint(Coords, geo->getEndPoint());
            Index.push_back(2);
        }
        else if constexpr (curvemode == CurveMode::ClosedCurve) {
            double segment = (geo->getLastParameter() - geo->getFirstParameter()) / CurvedEdgeCountSegments;

            for (int i=0; i < CurvedEdgeCountSegments; i++) {
                Base::Vector3d pnt = geo->value(i*segment);
                addPoint(Coords, pnt);
            }

            Base::Vector3d pnt = geo->value(0);
            addPoint(Coords, pnt);

            Index.push_back(CurvedEdgeCountSegments+1);
        }
        else if constexpr (curvemode == CurveMode::OpenCurve) {

            double segment = (geo->getLastParameter() - geo->getFirstParameter()) / CurvedEdgeCountSegments;

            for (int i=0; i < CurvedEdgeCountSegments; i++) {
                Base::Vector3d pnt = geo->value(geo->getFirstParameter() + i*segment);
                addPoint(Coords, pnt);
            }

            Base::Vector3d pnt = geo->value(geo->getLastParameter());
                addPoint(Coords, pnt);

            Index.push_back(CurvedEdgeCountSegments+1);

            if constexpr (analysemode == AnalyseMode::BoundingBoxAndBSplineCurvature) {
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

private:
    std::vector<Base::Vector3d> & Points;
    std::vector<Base::Vector3d> & Coords;
    std::vector<unsigned int> & Index;

    // drawing parameters
    int CurvedEdgeCountSegments;

    // measurements
    float dMg = 100;
    double combrepscale = 0; // the repscale that would correspond to this comb based only on this calculation.

};



//**************************** CoinManager class ******************************
CoinManager::CoinManager(EditData * editdata):edit(editdata) {
    // Create parameter observer and initialise watched parameters
    pObserver = std::make_unique<CoinManager::ParameterObserver>(this);

}

CoinManager::~CoinManager() {}

std::tuple<std::vector<Base::Vector3d>/* Coords*/, std::vector<Base::Vector3d> /*Points;*/, std::vector<unsigned int> /* Index */>
CoinManager::processGeometry(const GeoList & geolist)
{
    const std::vector<Part::Geometry *> *geomlist;
    geomlist = &geolist.geomlist;

    edit->CurvIdToGeoId.clear();
    edit->PointIdToGeoId.clear();

    edit->PointIdToGeoId.push_back(-1); // root point

    std::vector<int> bsplineGeoIds;

    // end information layer

    std::vector<Base::Vector3d> Coords;
    std::vector<Base::Vector3d> Points;
    std::vector<unsigned int> Index;

    GeometryCoinConverter gcconv(Points, Coords, Index, drawingParameters.curvedEdgeCountSegments);

    // RootPoint
    Points.emplace_back(0.,0.,0.);

    int GeoId = 0;
    for (std::vector<Part::Geometry *>::const_iterator it = geomlist->begin(); it != geomlist->end()-2; ++it, GeoId++) {
        if (GeoId >= geolist.intGeoCount)
            GeoId = -geolist.extGeoCount;

        if ((*it)->getTypeId() == Part::GeomPoint::getClassTypeId()) { // add a point
            gcconv.convert<Part::GeomPoint, PointsMode::InsertSingle, CurveMode::NoCurve, AnalyseMode::BoundingBox>((*it));
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) { // add a line
            gcconv.convert<Part::GeomLineSegment, PointsMode::InsertStartEnd, CurveMode::StartEndPointsOnly, AnalyseMode::BoundingBox>((*it));
            edit->CurvIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) { // add a circle
            gcconv.convert<Part::GeomCircle, PointsMode::InsertMidOnly, CurveMode::ClosedCurve, AnalyseMode::BoundingBox>((*it));

            //Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            //Points.push_back(center);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) { // add an ellipse
            gcconv.convert<Part::GeomEllipse, PointsMode::InsertMidOnly, CurveMode::ClosedCurve, AnalyseMode::BoundingBox>((*it));
            edit->CurvIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) { // add an arc
            gcconv.convert<Part::GeomArcOfCircle, PointsMode::InsertStartEndMid, CurveMode::OpenCurve, AnalyseMode::BoundingBox>((*it));
            edit->CurvIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) { // add an arc
            gcconv.convert<Part::GeomArcOfEllipse, PointsMode::InsertStartEndMid, CurveMode::OpenCurve, AnalyseMode::BoundingBox>((*it));
            edit->CurvIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            gcconv.convert<Part::GeomArcOfHyperbola, PointsMode::InsertStartEndMid, CurveMode::OpenCurve, AnalyseMode::BoundingBox>((*it));
            edit->CurvIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            gcconv.convert<Part::GeomArcOfParabola, PointsMode::InsertStartEndMid, CurveMode::OpenCurve, AnalyseMode::BoundingBox>((*it));
            edit->CurvIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) { // add a bspline
            gcconv.convert<Part::GeomBSplineCurve, PointsMode::InsertStartEnd, CurveMode::OpenCurve, AnalyseMode::BoundingBoxAndBSplineCurvature>((*it));

            edit->CurvIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
    }

    edit->CurvesCoordinate->point.setNum(Coords.size());
    edit->CurveSet->numVertices.setNum(Index.size());
    edit->CurvesMaterials->diffuseColor.setNum(Index.size());
    edit->PointsCoordinate->point.setNum(Points.size());
    edit->PointsMaterials->diffuseColor.setNum(Points.size());

    SbVec3f *verts = edit->CurvesCoordinate->point.startEditing();
    int32_t *index = edit->CurveSet->numVertices.startEditing();
    SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();

    int i=0; // setting up the line set
    for (std::vector<Base::Vector3d>::const_iterator it = Coords.begin(); it != Coords.end(); ++it,i++)
        verts[i].setValue(it->x,it->y,drawingParameters.zLowLines);

    i=0; // setting up the indexes of the line set
    for (std::vector<unsigned int>::const_iterator it = Index.begin(); it != Index.end(); ++it,i++)
        index[i] = *it;

    i=0; // setting up the point set
    for (std::vector<Base::Vector3d>::const_iterator it = Points.begin(); it != Points.end(); ++it,i++)
        pverts[i].setValue(it->x,it->y,drawingParameters.zLowPoints);

    edit->CurvesCoordinate->point.finishEditing();
    edit->CurveSet->numVertices.finishEditing();
    edit->PointsCoordinate->point.finishEditing();

    // set cross coordinates
    edit->RootCrossSet->numVertices.set1Value(0,2);
    edit->RootCrossSet->numVertices.set1Value(1,2);

    return std::tuple { Coords, Points, Index};

}

