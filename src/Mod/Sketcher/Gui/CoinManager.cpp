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
# include <Gui/Inventor/SmSwitchboard.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoFont.h>

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
#include <Base/Tools2D.h>
#include <Base/UnitsApi.h>

#include <Gui/Inventor/MarkerBitmaps.h>


using namespace SketcherGui;
using namespace Sketcher;

SbColor CoinManager::DrawingParameters::InformationColor            (0.0f,1.0f,0.0f);     // #00FF00 -> (  0,255,  0)
SbColor CoinManager::DrawingParameters::CreateCurveColor            (0.8f,0.8f,0.8f);     // #CCCCCC -> (204,204,204)




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

    float getBoundingBoxMagnitudeOrder() {return dMg;}
    double getCombRepresentationScale() {return combrepscale;}

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


void CoinManager::processGeometry(const GeoList & geolist)
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

    // Design decision 1
    //
    // I considered refactoring this if-else below into a map of lambdas (dictionary). However, the geometry TypeId is only valid at
    // runtime (at compile time is bad type, as registration is during runtime). This forces to construct the map on each
    // execution, which is not a good trade off.
    //
    // Design decision 2
    //
    // I also considered to move the information about the conversion template parameters to the GeometryCoinConverter class. However,
    // I would also have to move the responsibility to maintain the mapping between GeoIds and coin geometry there. However, I believe
    // the responsibility is of this class under the Single Responsibility Principle.
    auto pushToEdit = [edit = edit] (int geoId, int numberPoints, int numberCurves) {
        for(int i = 0; i < numberPoints; i++)
            edit->PointIdToGeoId.push_back(geoId);

        for(int i = 0; i < numberCurves; i++)
            edit->CurvIdToGeoId.push_back(geoId);

    };

    analysisResults.bsplineGeoIds.clear();

    int GeoId = 0;
    for (std::vector<Part::Geometry *>::const_iterator it = geomlist->begin(); it != geomlist->end()-2; ++it, GeoId++) {
        if (GeoId >= geolist.intGeoCount)
            GeoId = -geolist.extGeoCount;

        if ((*it)->getTypeId() == Part::GeomPoint::getClassTypeId()) { // add a point
            gcconv.convert<Part::GeomPoint, PointsMode::InsertSingle, CurveMode::NoCurve, AnalyseMode::BoundingBox>((*it));
            pushToEdit(GeoId, 1, 0);
        }
        else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) { // add a line
            gcconv.convert<Part::GeomLineSegment, PointsMode::InsertStartEnd, CurveMode::StartEndPointsOnly, AnalyseMode::BoundingBox>((*it));
            pushToEdit(GeoId, 2, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) { // add a circle
            gcconv.convert<Part::GeomCircle, PointsMode::InsertMidOnly, CurveMode::ClosedCurve, AnalyseMode::BoundingBox>((*it));
            pushToEdit(GeoId, 1, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) { // add an ellipse
            gcconv.convert<Part::GeomEllipse, PointsMode::InsertMidOnly, CurveMode::ClosedCurve, AnalyseMode::BoundingBox>((*it));
            pushToEdit(GeoId, 1, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) { // add an arc
            gcconv.convert<Part::GeomArcOfCircle, PointsMode::InsertStartEndMid, CurveMode::OpenCurve, AnalyseMode::BoundingBox>((*it));
            pushToEdit(GeoId, 3, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) { // add an arc
            gcconv.convert<Part::GeomArcOfEllipse, PointsMode::InsertStartEndMid, CurveMode::OpenCurve, AnalyseMode::BoundingBox>((*it));
            pushToEdit(GeoId, 3, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            gcconv.convert<Part::GeomArcOfHyperbola, PointsMode::InsertStartEndMid, CurveMode::OpenCurve, AnalyseMode::BoundingBox>((*it));
            pushToEdit(GeoId, 3, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            gcconv.convert<Part::GeomArcOfParabola, PointsMode::InsertStartEndMid, CurveMode::OpenCurve, AnalyseMode::BoundingBox>((*it));
            pushToEdit(GeoId, 3, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) { // add a bspline
            gcconv.convert<Part::GeomBSplineCurve, PointsMode::InsertStartEnd, CurveMode::OpenCurve, AnalyseMode::BoundingBoxAndBSplineCurvature>((*it));
            pushToEdit(GeoId, 2, 1);
            analysisResults.bsplineGeoIds.push_back(GeoId);
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

    // TODO: THIS NEEDS REFACTORING
    analysisResults.combRepresentationScale = gcconv.getCombRepresentationScale();
    analysisResults.boundingBoxMagnitudeOrder = gcconv.getBoundingBoxMagnitudeOrder();
}

void CoinManager::processGeometryInformationLayer(const GeoList & geolist, bool rebuildinformationlayer)
{
    const int GEOINFO_BSPLINE_DEGREE_POS = 0;
    const int GEOINFO_BSPLINE_DEGREE_TEXT = 3;
    const int GEOINFO_BSPLINE_POLYGON = 1;


    int currentInfoNode = 0;

    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    if ( (analysisResults.combRepresentationScale > (2 * visualisationControlParameters.currentBSplineCombRepresentationScale)) ||
         (analysisResults.combRepresentationScale < (visualisationControlParameters.currentBSplineCombRepresentationScale / 2)))
        visualisationControlParameters.currentBSplineCombRepresentationScale = analysisResults.combRepresentationScale ;

    const std::vector<Part::Geometry *> *geomlist;
    geomlist = &geolist.geomlist;

    auto GeoById = [](const std::vector<Part::Geometry*> GeoList, int Id){
    {
        if (Id >= 0)
            return GeoList[Id];
        else
            return GeoList[GeoList.size()+Id];
        }
    };

    // geometry information layer for bsplines, as they need a second round now that max curvature is known
    for (std::vector<int>::const_iterator it = analysisResults.bsplineGeoIds.begin(); it != analysisResults.bsplineGeoIds.end(); ++it) {

        const Part::Geometry *geo = GeoById(*geomlist, *it);

        const Part::GeomBSplineCurve *spline = static_cast<const Part::GeomBSplineCurve *>(geo);

        //----------------------------------------------------------
        // geometry information layer

        // polynom degree --------------------------------------------------------
        std::vector<Base::Vector3d> poles = spline->getPoles();

        Base::Vector3d midp = Base::Vector3d(0,0,0);

        for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it) {
            midp += (*it);
        }

        midp /= poles.size();

        if (rebuildinformationlayer) {
            SoSwitch *sw = new SoSwitch();

            sw->whichChild = hGrpsk->GetBool("BSplineDegreeVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = new SoSeparator();
            sep->ref();
            // no caching for frequently-changing data structures
            sep->renderCaching = SoSeparator::OFF;

            // every information visual node gets its own material for to-be-implemented preselection and selection
            SoMaterial *mat = new SoMaterial;
            mat->ref();
            mat->diffuseColor = DrawingParameters::InformationColor;

            SoTranslation *translate = new SoTranslation;

            translate->translation.setValue(midp.x, midp.y, drawingParameters.zInfo);

            SoFont *font = new SoFont;
            font->name.setValue("Helvetica");
            font->size.setValue(edit->coinFontSize);

            SoText2 *degreetext = new SoText2;
            degreetext->string = SbString(spline->getDegree());

            sep->addChild(translate);
            sep->addChild(mat);
            sep->addChild(font);
            sep->addChild(degreetext);

            sw->addChild(sep);

            edit->infoGroup->addChild(sw);
            sep->unref();
            mat->unref();
        }
        else {
            SoSwitch *sw = static_cast<SoSwitch *>(edit->infoGroup->getChild(currentInfoNode));

            if (visualisationControlParameters.visibleInformationChanged)
                sw->whichChild = hGrpsk->GetBool("BSplineDegreeVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

            static_cast<SoTranslation *>(sep->getChild(GEOINFO_BSPLINE_DEGREE_POS))->translation.setValue(midp.x, midp.y, drawingParameters.zInfo);

            static_cast<SoText2 *>(sep->getChild(GEOINFO_BSPLINE_DEGREE_TEXT))->string = SbString(spline->getDegree());
        }

        currentInfoNode++; // switch to next node

        // control polygon --------------------------------------------------------
        if (rebuildinformationlayer) {
            SoSwitch *sw = new SoSwitch();

            sw->whichChild = hGrpsk->GetBool("BSplineControlPolygonVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = new SoSeparator();
            sep->ref();
            // no caching for frequently-changing data structures
            sep->renderCaching = SoSeparator::OFF;

            // every information visual node gets its own material for to-be-implemented preselection and selection
            SoMaterial *mat = new SoMaterial;
            mat->ref();
            mat->diffuseColor = drawingParameters.InformationColor;

            SoLineSet *polygon = new SoLineSet;

            SoCoordinate3 *polygoncoords = new SoCoordinate3;

            if (spline->isPeriodic()) {
                polygoncoords->point.setNum(poles.size()+1);
            }
            else {
                polygoncoords->point.setNum(poles.size());
            }

            SbVec3f *vts = polygoncoords->point.startEditing();

            int i=0;
            for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it, i++) {
                vts[i].setValue((*it).x,(*it).y,drawingParameters.zInfo);
            }

            if (spline->isPeriodic()) {
                vts[poles.size()].setValue(poles[0].x,poles[0].y,drawingParameters.zInfo);
            }

            polygoncoords->point.finishEditing();

            sep->addChild(mat);
            sep->addChild(polygoncoords);
            sep->addChild(polygon);

            sw->addChild(sep);

            edit->infoGroup->addChild(sw);
            sep->unref();
            mat->unref();
        }
        else {
            SoSwitch *sw = static_cast<SoSwitch *>(edit->infoGroup->getChild(currentInfoNode));

            if(visualisationControlParameters.visibleInformationChanged)
                sw->whichChild = hGrpsk->GetBool("BSplineControlPolygonVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

            SoCoordinate3 *polygoncoords = static_cast<SoCoordinate3 *>(sep->getChild(GEOINFO_BSPLINE_POLYGON));

            if(spline->isPeriodic()) {
                polygoncoords->point.setNum(poles.size()+1);
            }
            else {
                polygoncoords->point.setNum(poles.size());
            }

            SbVec3f *vts = polygoncoords->point.startEditing();

            int i=0;
            for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it, i++) {
                vts[i].setValue((*it).x,(*it).y,drawingParameters.zInfo);
            }

            if(spline->isPeriodic()) {
                vts[poles.size()].setValue(poles[0].x,poles[0].y,drawingParameters.zInfo);
            }

            polygoncoords->point.finishEditing();

        }
        currentInfoNode++; // switch to next node

        // curvature graph --------------------------------------------------------

        // reimplementation of python source:
        // https://github.com/tomate44/CurvesWB/blob/master/ParametricComb.py
        // by FreeCAD user Chris_G

        double firstparam = spline->getFirstParameter();
        double lastparam =  spline->getLastParameter();

        const int ndiv = poles.size()>4?poles.size()*16:64;
        double step = (lastparam - firstparam ) / (ndiv -1);

        std::vector<double> paramlist(ndiv);
        std::vector<Base::Vector3d> pointatcurvelist(ndiv);
        std::vector<double> curvaturelist(ndiv);
        std::vector<Base::Vector3d> normallist(ndiv);

        for(int i = 0; i < ndiv; i++) {
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
                Base::Console().Error("Curvature graph for B-Spline with GeoId=%d could not be calculated.\n", 0); // TODO: Fix me
                curvaturelist[i] = 0;
            }

            try {
                spline->normalAt(paramlist[i],normallist[i]);
            }
            catch(Base::Exception&) {
                normallist[i] = Base::Vector3d(0,0,0);
            }

        }

        std::vector<Base::Vector3d> pointatcomblist(ndiv);

        for(int i = 0; i < ndiv; i++) {
            pointatcomblist[i] = pointatcurvelist[i] - visualisationControlParameters.currentBSplineCombRepresentationScale * curvaturelist[i] * normallist[i];
        }

        if (rebuildinformationlayer) {
            SoSwitch *sw = new SoSwitch();

            sw->whichChild = hGrpsk->GetBool("BSplineCombVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = new SoSeparator();
            sep->ref();
            // no caching for frequently-changing data structures
            sep->renderCaching = SoSeparator::OFF;

            // every information visual node gets its own material for to-be-implemented preselection and selection
            SoMaterial *mat = new SoMaterial;
            mat->ref();
            mat->diffuseColor = drawingParameters.InformationColor;

            SoLineSet *comblineset = new SoLineSet;

            SoCoordinate3 *combcoords = new SoCoordinate3;

            combcoords->point.setNum(3*ndiv); // 2*ndiv +1 points of ndiv separate segments + ndiv points for last segment
            comblineset->numVertices.setNum(ndiv+1); // ndiv separate segments of radials + 1 segment connecting at comb end

            int32_t *index = comblineset->numVertices.startEditing();
            SbVec3f *vts = combcoords->point.startEditing();

            for(int i = 0; i < ndiv; i++) {
                vts[2*i].setValue(pointatcurvelist[i].x, pointatcurvelist[i].y, drawingParameters.zInfo); // radials
                vts[2*i+1].setValue(pointatcomblist[i].x, pointatcomblist[i].y, drawingParameters.zInfo);
                index[i] = 2;

                vts[2*ndiv+i].setValue(pointatcomblist[i].x, pointatcomblist[i].y, drawingParameters.zInfo); // comb endpoint closing segment
            }

            index[ndiv] = ndiv; // comb endpoint closing segment

            combcoords->point.finishEditing();
            comblineset->numVertices.finishEditing();

            sep->addChild(mat);
            sep->addChild(combcoords);
            sep->addChild(comblineset);

            sw->addChild(sep);

            edit->infoGroup->addChild(sw);
            sep->unref();
            mat->unref();
        }
        else {
            SoSwitch *sw = static_cast<SoSwitch *>(edit->infoGroup->getChild(currentInfoNode));

            if(visualisationControlParameters.visibleInformationChanged)
                sw->whichChild = hGrpsk->GetBool("BSplineCombVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

            SoCoordinate3 *combcoords = static_cast<SoCoordinate3 *>(sep->getChild(GEOINFO_BSPLINE_POLYGON));

            SoLineSet *comblineset = static_cast<SoLineSet *>(sep->getChild(GEOINFO_BSPLINE_POLYGON+1));

            combcoords->point.setNum(3*ndiv); // 2*ndiv +1 points of ndiv separate segments + ndiv points for last segment
            comblineset->numVertices.setNum(ndiv+1); // ndiv separate segments of radials + 1 segment connecting at comb end

            int32_t *index = comblineset->numVertices.startEditing();
            SbVec3f *vts = combcoords->point.startEditing();

            for(int i = 0; i < ndiv; i++) {
                vts[2*i].setValue(pointatcurvelist[i].x, pointatcurvelist[i].y, drawingParameters.zInfo); // radials
                vts[2*i+1].setValue(pointatcomblist[i].x, pointatcomblist[i].y, drawingParameters.zInfo);
                index[i] = 2;

                vts[2*ndiv+i].setValue(pointatcomblist[i].x, pointatcomblist[i].y, drawingParameters.zInfo); // comb endpoint closing segment
            }

            index[ndiv] = ndiv; // comb endpoint closing segment

            combcoords->point.finishEditing();
            comblineset->numVertices.finishEditing();

        }

        currentInfoNode++; // switch to next node

        // knot multiplicity --------------------------------------------------------
        std::vector<double> knots = spline->getKnots();
        std::vector<int> mult = spline->getMultiplicities();

        std::vector<double>::const_iterator itk;
        std::vector<int>::const_iterator itm;


        if (rebuildinformationlayer) {

            for( itk = knots.begin(), itm = mult.begin(); itk != knots.end() && itm != mult.end(); ++itk, ++itm) {

                SoSwitch *sw = new SoSwitch();

                sw->whichChild = hGrpsk->GetBool("BSplineKnotMultiplicityVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

                SoSeparator *sep = new SoSeparator();
                sep->ref();
                // no caching for frequently-changing data structures
                sep->renderCaching = SoSeparator::OFF;

                // every information visual node gets its own material for to-be-implemented preselection and selection
                SoMaterial *mat = new SoMaterial;
                mat->ref();
                mat->diffuseColor = drawingParameters.InformationColor;

                SoTranslation *translate = new SoTranslation;

                Base::Vector3d knotposition = spline->pointAtParameter(*itk);

                translate->translation.setValue(knotposition.x, knotposition.y, drawingParameters.zInfo);

                SoFont *font = new SoFont;
                font->name.setValue("Helvetica");
                font->size.setValue(edit->coinFontSize);

                SoText2 *degreetext = new SoText2;
                degreetext->string = SbString("(") + SbString(*itm) + SbString(")");

                sep->addChild(translate);
                sep->addChild(mat);
                sep->addChild(font);
                sep->addChild(degreetext);

                sw->addChild(sep);

                edit->infoGroup->addChild(sw);
                sep->unref();
                mat->unref();

                currentInfoNode++; // switch to next node
            }
        }
        else {
            for( itk = knots.begin(), itm = mult.begin(); itk != knots.end() && itm != mult.end(); ++itk, ++itm) {
                SoSwitch *sw = static_cast<SoSwitch *>(edit->infoGroup->getChild(currentInfoNode));

                if(visualisationControlParameters.visibleInformationChanged)
                    sw->whichChild = hGrpsk->GetBool("BSplineKnotMultiplicityVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

                SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

                Base::Vector3d knotposition = spline->pointAtParameter(*itk);

                static_cast<SoTranslation *>(sep->getChild(GEOINFO_BSPLINE_DEGREE_POS))->translation.setValue(knotposition.x,knotposition.y,drawingParameters.zInfo);

                static_cast<SoText2 *>(sep->getChild(GEOINFO_BSPLINE_DEGREE_TEXT))->string = SbString("(") + SbString(*itm) + SbString(")");

                currentInfoNode++; // switch to next node
            }
        }

        // End of knot multiplicity

        // pole weights --------------------------------------------------------
        std::vector<double> weights = spline->getWeights();

        if (rebuildinformationlayer) {

            for (size_t index = 0; index < weights.size(); ++index) {

                SoSwitch* sw = new SoSwitch();

                sw->whichChild = hGrpsk->GetBool("BSplinePoleWeightVisible", true) ? SO_SWITCH_ALL : SO_SWITCH_NONE;

                SoSeparator* sep = new SoSeparator();
                sep->ref();
                // no caching for frequently-changing data structures
                sep->renderCaching = SoSeparator::OFF;

                // every information visual node gets its own material for to-be-implemented preselection and selection
                SoMaterial* mat = new SoMaterial;
                mat->ref();
                mat->diffuseColor = drawingParameters.InformationColor;

                SoTranslation* translate = new SoTranslation;

                Base::Vector3d poleposition = poles[index];

                SoFont* font = new SoFont;
                font->name.setValue("Helvetica");
                font->size.setValue(edit->coinFontSize);

                translate->translation.setValue(poleposition.x, poleposition.y, drawingParameters.zInfo);

                // set up string with weight value and the user-defined number of decimals
                QString WeightString  = QString::fromLatin1("%1").arg(weights[index], 0, 'f', Base::UnitsApi::getDecimals());

                SoText2* WeightText = new SoText2;
                // since the first and last control point of a spline is also treated as knot and thus
                // can also have a displayed multiplicity, we must assure the multiplicity is not visibly overwritten
                // therefore be output the weight in a second line
                SoMFString label;
                label.set1Value(0, SbString(""));
                label.set1Value(1, SbString("[") + SbString(WeightString.toStdString().c_str()) + SbString("]"));
                WeightText->string = label;

                sep->addChild(translate);
                sep->addChild(mat);
                sep->addChild(font);
                sep->addChild(WeightText);

                sw->addChild(sep);

                edit->infoGroup->addChild(sw);
                sep->unref();
                mat->unref();

                currentInfoNode++; // switch to next node
            }
        }
        else {
            for (size_t index = 0; index < weights.size(); ++index) {
                SoSwitch* sw = static_cast<SoSwitch*>(edit->infoGroup->getChild(currentInfoNode));

                if (visualisationControlParameters.visibleInformationChanged)
                    sw->whichChild = hGrpsk->GetBool("BSplinePoleWeightVisible", true) ? SO_SWITCH_ALL : SO_SWITCH_NONE;

                SoSeparator* sep = static_cast<SoSeparator*>(sw->getChild(0));

                Base::Vector3d poleposition = poles[index];

                static_cast<SoTranslation*>(sep->getChild(GEOINFO_BSPLINE_DEGREE_POS))
                    ->translation.setValue(poleposition.x, poleposition.y, drawingParameters.zInfo);

                // set up string with weight value and the user-defined number of decimals
                QString WeightString = QString::fromLatin1("%1").arg(weights[index], 0, 'f', Base::UnitsApi::getDecimals());

                // since the first and last control point of a spline is also treated as knot and thus
                // can also have a displayed multiplicity, we must assure the multiplicity is not visibly overwritten
                // therefore be output the weight in a second line
                SoMFString label;
                label.set1Value(0, SbString(""));
                label.set1Value(1, SbString("[") + SbString(WeightString.toStdString().c_str()) + SbString("]"));

                static_cast<SoText2*>(sep->getChild(GEOINFO_BSPLINE_DEGREE_TEXT))
                                        ->string = label;

                currentInfoNode++; // switch to next node
            }
        }

        // End of pole weights
    }


    visualisationControlParameters.visibleInformationChanged = false; // just updated
}

void CoinManager::processGeometryAndInformationLayer(const GeoList & geolist, bool rebuildinformationlayer)
{
    processGeometry(geolist);

    processGeometryInformationLayer(geolist, rebuildinformationlayer);
}

void CoinManager::drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel)
{
    assert(edit);

    // determine marker size
    int augmentedmarkersize = edit->MarkerSize;

    auto supportedsizes = Gui::Inventor::MarkerBitmaps::getSupportedSizes("CIRCLE_LINE");

    auto defaultmarker = std::find(supportedsizes.begin(), supportedsizes.end(), edit->MarkerSize);

    if(defaultmarker != supportedsizes.end()) {
        auto validAugmentationLevels = std::distance(defaultmarker,supportedsizes.end());

        if(augmentationlevel >= validAugmentationLevels)
            augmentationlevel = validAugmentationLevels - 1;

        augmentedmarkersize = *std::next(defaultmarker, augmentationlevel);
    }

    edit->EditMarkerSet->markerIndex.startEditing();
    edit->EditMarkerSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", augmentedmarkersize);

    // add the points to set
    edit->EditMarkersCoordinate->point.setNum(EditMarkers.size());
    edit->EditMarkersMaterials->diffuseColor.setNum(EditMarkers.size());
    SbVec3f *verts = edit->EditMarkersCoordinate->point.startEditing();
    SbColor *color = edit->EditMarkersMaterials->diffuseColor.startEditing();

    int i=0; // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditMarkers.begin(); it != EditMarkers.end(); ++it,i++) {
        verts[i].setValue(it->x, it->y, drawingParameters.zEdit);
        color[i] = drawingParameters.InformationColor;
    }

    edit->EditMarkersCoordinate->point.finishEditing();
    edit->EditMarkersMaterials->diffuseColor.finishEditing();
    edit->EditMarkerSet->markerIndex.finishEditing();
}

void CoinManager::drawEdit(const std::vector<Base::Vector2d> &EditCurve)
{
    assert(edit);

    edit->EditCurveSet->numVertices.setNum(1);
    edit->EditCurvesCoordinate->point.setNum(EditCurve.size());
    edit->EditCurvesMaterials->diffuseColor.setNum(EditCurve.size());
    SbVec3f *verts = edit->EditCurvesCoordinate->point.startEditing();
    int32_t *index = edit->EditCurveSet->numVertices.startEditing();
    SbColor *color = edit->EditCurvesMaterials->diffuseColor.startEditing();

    int i=0; // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditCurve.begin(); it != EditCurve.end(); ++it,i++) {
        verts[i].setValue(it->x,it->y, drawingParameters.zEdit);
        color[i] = drawingParameters.CreateCurveColor;
    }

    index[0] = EditCurve.size();
    edit->EditCurvesCoordinate->point.finishEditing();
    edit->EditCurveSet->numVertices.finishEditing();
    edit->EditCurvesMaterials->diffuseColor.finishEditing();
}

void CoinManager::updateCoinManagerColors()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    auto updateColor = [&hGrp](SbColor & sbcolor, const char * parametername){
        float transparency = 0.f;
        unsigned long color = (unsigned long)(sbcolor.getPackedValue());
        color = hGrp->GetUnsigned(parametername, color);
        sbcolor.setPackedValue((uint32_t)color, transparency);
    };

    updateColor(drawingParameters.CreateCurveColor, "CreateLineColor");
}
