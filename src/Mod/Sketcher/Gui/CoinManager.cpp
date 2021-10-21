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
    OpenCurve,
    Custom
};

enum class AnalyseMode {
    NoAnalysis,
    BoundingBox,
    BoundingBoxAndCurvature
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

        // Points
        if constexpr (pointmode == PointsMode::InsertSingle) {
            Points.push_back(geo->getPoint());
        }
        else if constexpr (pointmode == PointsMode::InsertStartEnd) {
            Points.push_back(geo->getStartPoint());
            Points.push_back(geo->getEndPoint());
        }
        else if constexpr (pointmode == PointsMode::InsertStartEndMid) {
            // All in this group are Trimmed Curves (see Geometry.h)
            Points.push_back(geo->getStartPoint(/*emulateCCW=*/true));
            Points.push_back(geo->getEndPoint(/*emulateCCW=*/true));
            Points.push_back(geo->getCenter());
        }
        else if constexpr (pointmode == PointsMode::InsertMidOnly) {
            Points.push_back(geo->getCenter());
        }

        // Curves
        if constexpr (curvemode == CurveMode::StartEndPointsOnly) {
            Coords.push_back(geo->getStartPoint());
            Coords.push_back(geo->getEndPoint());
            Index.push_back(2);
        }
        else if constexpr (curvemode == CurveMode::ClosedCurve) {

            double segment = (geo->getLastParameter() - geo->getFirstParameter()) / CurvedEdgeCountSegments;

            for (int i=0; i < CurvedEdgeCountSegments; i++) {
                Base::Vector3d pnt = geo->value(i*segment);
                Coords.emplace_back(pnt);
            }

            Base::Vector3d pnt = geo->value(0);
            Coords.emplace_back(pnt);

            Index.push_back(CurvedEdgeCountSegments+1);
        }
        else if constexpr (curvemode == CurveMode::OpenCurve) {

            double segment = (geo->getLastParameter() - geo->getFirstParameter()) / CurvedEdgeCountSegments;

            for (int i=0; i < CurvedEdgeCountSegments; i++) {
                Base::Vector3d pnt = geo->value(geo->getFirstParameter() + i*segment);
                Coords.emplace_back(pnt);
            }

            Base::Vector3d pnt = geo->value(geo->getLastParameter());
                Coords.emplace_back(pnt);

            Index.push_back(CurvedEdgeCountSegments+1);
        }

    }

private:
    std::vector<Base::Vector3d> & Points;
    std::vector<Base::Vector3d> & Coords;
    std::vector<unsigned int> & Index;

    // drawing parameters
    int CurvedEdgeCountSegments;
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

    double combrepscale = 0; // the repscale that would correspond to this comb based only on this calculation.

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
            const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(*it);
            Handle(Geom_Circle) curve = Handle(Geom_Circle)::DownCast(circle->handle());
            auto gf = GeometryFacade::getFacade(circle);

            int countSegments = drawingParameters.curvedEdgeCountSegments;
            Base::Vector3d center = circle->getCenter();

            // BSpline weights have a radius corresponding to the weight value
            // However, in order for them proportional to the B-Spline size,
            // the scenograph has a size scalefactor times the weight
            //
            // This code produces the scaled up version of the geometry for the scenograph
            if(false) { // TODO: Re-enable bspline pole behaviour
/*            if(gf->getInternalType() == InternalType::BSplineControlPoint) {
                for( auto c : getSketchObject()->Constraints.getValues()) {
                    if( c->Type == InternalAlignment && c->AlignmentType == BSplineControlPoint && c->First == GeoId) {
                        auto bspline = dynamic_cast<const Part::GeomBSplineCurve *>((*geomlist)[c->Second]);

                        if(bspline){
                            auto weights = bspline->getWeights();

                            double weight = 1.0;
                            if(c->InternalAlignmentIndex < int(weights.size()))
                                weight =  weights[c->InternalAlignmentIndex];

                            // tentative scaling factor:
                            // proportional to the length of the bspline
                            // inversely proportional to the number of poles
                            double scalefactor = bspline->length(bspline->getFirstParameter(), bspline->getLastParameter())/10.0/weights.size();

                            double vradius = weight*scalefactor;
                            if(!bspline->isRational()) {
                                // OCCT sets the weights to 1.0 if a bspline is non-rational, but if the user has a weight constraint on any
                                // pole it would cause a visual artifact of having a constraint with a different radius and an unscaled circle
                                // so better scale the circles.
                                std::vector<int> polegeoids;
                                polegeoids.reserve(weights.size());

                                for ( auto ic : getSketchObject()->Constraints.getValues()) {
                                    if( ic->Type == InternalAlignment && ic->AlignmentType == BSplineControlPoint && ic->Second == c->Second) {
                                        polegeoids.push_back(ic->First);
                                    }
                                }

                                for ( auto ic : getSketchObject()->Constraints.getValues()) {
                                    if( ic->Type == Weight ) {
                                        auto pos = std::find(polegeoids.begin(), polegeoids.end(), ic->First);

                                        if(pos != polegeoids.end()) {
                                            vradius = ic->getValue() * scalefactor;
                                            break; // one is enough, otherwise it would not be non-rational
                                        }
                                    }
                                }
                            }

                            // virtual circle or radius vradius
                            auto mcurve = [&center, vradius](double param, double &x, double &y) {
                                x = center.x + vradius*cos(param);
                                y = center.y + vradius*sin(param);
                            };

                            double x;
                            double y;
                            for (int i=0; i < countSegments; i++) {
                                double param = 2*M_PI*i/countSegments;
                                mcurve(param,x,y);
                                Coords.emplace_back(x, y, 0);
                            }

                            mcurve(0,x,y);
                            Coords.emplace_back(x, y, 0);

                            // save scale factor for any prospective dragging operation
                            // 1. Solver must be updated, in case a dragging operation starts
                            // 2. if temp geometry is being used (with memory allocation), then the copy we have here must be updated. If
                            //    no temp geometry is being used, then the normal geometry must be updated.
                            {// make solver be ready for a dragging operation
                                auto vpext = std::make_unique<SketcherGui::ViewProviderSketchGeometryExtension>();
                                vpext->setRepresentationFactor(scalefactor);

                                getSketchObject()->updateSolverExtension(GeoId, std::move(vpext));
                            }

                            if(!circle->hasExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()))
                            {
                                // It is ok to add this kind of extension to a const geometry because:
                                // 1. It does not modify the object in a way that affects property state, just ViewProvider representation
                                // 2. If it is lost (for example upon undo), redrawing will reinstate it with the correct value
                                const_cast<Part::GeomCircle *>(circle)->setExtension(std::make_unique<SketcherGui::ViewProviderSketchGeometryExtension>());
                            }

                            auto vpext = std::const_pointer_cast<SketcherGui::ViewProviderSketchGeometryExtension>(
                                            std::static_pointer_cast<const SketcherGui::ViewProviderSketchGeometryExtension>(
                                                circle->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()).lock()));

                            vpext->setRepresentationFactor(scalefactor);
                        }
                        break;
                    }
                }*/
            }
            else {

                /*
                double segment = (2 * M_PI) / countSegments;

                for (int i=0; i < countSegments; i++) {
                    gp_Pnt pnt = curve->Value(i*segment);
                    Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                }

                gp_Pnt pnt = curve->Value(0);
                Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());*/

                gcconv.convert<Part::GeomCircle, PointsMode::InsertMidOnly, CurveMode::ClosedCurve, AnalyseMode::BoundingBox>((*it));
            }

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
            bsplineGeoIds.push_back(GeoId);
            const Part::GeomBSplineCurve *spline = static_cast<const Part::GeomBSplineCurve *>(*it);
            Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(spline->handle());

            Base::Vector3d startp  = spline->getStartPoint();
            Base::Vector3d endp    = spline->getEndPoint();

            double first = curve->FirstParameter();
            double last = curve->LastParameter();
            if (first > last) // if arc is reversed
                std::swap(first, last);

            double range = last-first;
            int countSegments = drawingParameters.curvedEdgeCountSegments;
            double segment = range / countSegments;

            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(first);
                Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                first += segment;
            }

            // end point
            gp_Pnt end = curve->Value(last);
            Coords.emplace_back(end.X(), end.Y(), end.Z());

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(startp);
            Points.push_back(endp);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);

            //***************************************************************************************************************
            // global information gathering for geometry information layer

            std::vector<Base::Vector3d> poles = spline->getPoles();

            Base::Vector3d midp = Base::Vector3d(0,0,0);

            for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it) {
                midp += (*it);
            }

            midp /= poles.size();

            double firstparam = spline->getFirstParameter();
            double lastparam =  spline->getLastParameter();

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
                pointatcurvelist[i] = spline->pointAtParameter(paramlist[i]);

                try {
                    curvaturelist[i] = spline->curvatureAt(paramlist[i]);
                }
                catch(Base::CADKernelError &e) {
                    // it is "just" a visualisation matter OCC could not calculate the curvature
                    // terminating here would mean that the other shapes would not be drawn.
                    // Solution: Report the issue and set dummy curvature to 0
                    e.ReportException();
                    Base::Console().Error("Curvature graph for B-Spline with GeoId=%d could not be calculated.\n", GeoId);
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

    edit->CurvesCoordinate->point.setNum(Coords.size());
    edit->CurveSet->numVertices.setNum(Index.size());
    edit->CurvesMaterials->diffuseColor.setNum(Index.size());
    edit->PointsCoordinate->point.setNum(Points.size());
    edit->PointsMaterials->diffuseColor.setNum(Points.size());

    SbVec3f *verts = edit->CurvesCoordinate->point.startEditing();
    int32_t *index = edit->CurveSet->numVertices.startEditing();
    SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();

    float dMg = 100;

    int i=0; // setting up the line set
    for (std::vector<Base::Vector3d>::const_iterator it = Coords.begin(); it != Coords.end(); ++it,i++) {
        dMg = dMg>std::abs(it->x)?dMg:std::abs(it->x);
        dMg = dMg>std::abs(it->y)?dMg:std::abs(it->y);
        verts[i].setValue(it->x,it->y,drawingParameters.zLowLines);
    }

    i=0; // setting up the indexes of the line set
    for (std::vector<unsigned int>::const_iterator it = Index.begin(); it != Index.end(); ++it,i++)
        index[i] = *it;

    i=0; // setting up the point set
    for (std::vector<Base::Vector3d>::const_iterator it = Points.begin(); it != Points.end(); ++it,i++){
        dMg = dMg>std::abs(it->x)?dMg:std::abs(it->x);
        dMg = dMg>std::abs(it->y)?dMg:std::abs(it->y);
        pverts[i].setValue(it->x,it->y,drawingParameters.zLowPoints);
    }

    edit->CurvesCoordinate->point.finishEditing();
    edit->CurveSet->numVertices.finishEditing();
    edit->PointsCoordinate->point.finishEditing();

    // set cross coordinates
    edit->RootCrossSet->numVertices.set1Value(0,2);
    edit->RootCrossSet->numVertices.set1Value(1,2);

    // This code relies on Part2D, which is generally not updated in no update mode.
    // Additionally it does not relate to the actual sketcher geometry.

    /*
    Base::Console().Log("MinX:%d,MaxX:%d,MinY:%d,MaxY:%d\n",MinX,MaxX,MinY,MaxY);
    // make sure that nine of the numbers are exactly zero because log(0)
    // is not defined
    float xMin = std::abs(MinX) < FLT_EPSILON ? 0.01f : MinX;
    float xMax = std::abs(MaxX) < FLT_EPSILON ? 0.01f : MaxX;
    float yMin = std::abs(MinY) < FLT_EPSILON ? 0.01f : MinY;
    float yMax = std::abs(MaxY) < FLT_EPSILON ? 0.01f : MaxY;
    */



    return std::tuple { Coords, Points, Index};

}

