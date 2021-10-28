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

    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineDegree>();
    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineControlPolygonVisible>();
    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineCombVisible>();
    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineKnotMultiplicityVisible>();
    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplinePoleWeightVisible>();

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

template<CoinManager::ParameterObserver::OverlayVisibilityParameter visibilityparameter>
void CoinManager::ParameterObserver::updateOverlayVisibilityParameter()
{
    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineDegree)
        pClient->overlayParameters.bSplineDegreeVisible = hGrpsk->GetBool("BSplineDegreeVisible", true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineControlPolygonVisible)
        pClient->overlayParameters.bSplineControlPolygonVisible = hGrpsk->GetBool("BSplineControlPolygonVisible", true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineCombVisible)
        pClient->overlayParameters.bSplineCombVisible = hGrpsk->GetBool("BSplineCombVisible", true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineKnotMultiplicityVisible)
        pClient->overlayParameters.bSplineKnotMultiplicityVisible = hGrpsk->GetBool("BSplineKnotMultiplicityVisible", true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplinePoleWeightVisible)
        pClient->overlayParameters.bSplinePoleWeightVisible = hGrpsk->GetBool("BSplinePoleWeightVisible", true);
}

void CoinManager::ParameterObserver::subscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Attach(this);

    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpsk->Attach(this);
}

void CoinManager::ParameterObserver::unsubscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Detach(this);

    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpsk->Detach(this);
}

void CoinManager::ParameterObserver::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    (void) rCaller;

    static std::map<const char *, std::function<void()>> str2updatefunction {
        {"SegmentsPerGeometry", [this](){updateCurvedEdgeCountSegmentsParameter();}},
        {"BSplineDegreeVisible", [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineDegree>();}},
        {"BSplineControlPolygonVisible", [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineControlPolygonVisible>();}},
        {"BSplineCombVisible", [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineCombVisible>();}},
        {"BSplineKnotMultiplicityVisible", [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineKnotMultiplicityVisible>();}},
        {"BSplinePoleWeightVisible", [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplinePoleWeightVisible>();}}

    };

    auto key = str2updatefunction.find(sReason);
    if( key != str2updatefunction.end() )
        key->second();
}

//*************************** GeometryCoinConverter ***************************

class GeometryCoinConverter {
public:
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
    GeometryCoinConverter(std::vector<Base::Vector3d> & points,
                          std::vector<Base::Vector3d> & coords,
                          std::vector<unsigned int> & index,
                          int curvedEdgeCountSegments
                         ): Points(points), Coords(coords), Index(index), CurvedEdgeCountSegments(curvedEdgeCountSegments){}

    template < typename GeoType, PointsMode pointmode, CurveMode curvemode, AnalyseMode analysemode >
    void convert(const Part::Geometry * geometry) {
        auto geo = static_cast<const GeoType *>(geometry);

        auto addPoint = [&dMg = boundingBoxMaxMagnitude] (auto & pushvector, Base::Vector3d point) {

            if constexpr (analysemode == AnalyseMode::BoundingBoxMagnitude || analysemode == AnalyseMode::BoundingBoxMagnitudeAndBSplineCurvature) {
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

    float getBoundingBoxMaxMagnitude() {return boundingBoxMaxMagnitude;}
    double getCombRepresentationScale() {return combrepscale;}

private:
    std::vector<Base::Vector3d> & Points;
    std::vector<Base::Vector3d> & Coords;
    std::vector<unsigned int> & Index;

    // drawing parameters
    int CurvedEdgeCountSegments;

    // measurements
    float boundingBoxMaxMagnitude = 100;
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
            gcconv.convert< Part::GeomPoint,
                            GeometryCoinConverter::PointsMode::InsertSingle,
                            GeometryCoinConverter::CurveMode::NoCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>((*it));
            pushToEdit(GeoId, 1, 0);
        }
        else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) { // add a line
            gcconv.convert< Part::GeomLineSegment,
                            GeometryCoinConverter::PointsMode::InsertStartEnd,
                            GeometryCoinConverter::CurveMode::StartEndPointsOnly,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>((*it));
            pushToEdit(GeoId, 2, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) { // add a circle
            gcconv.convert< Part::GeomCircle,
                            GeometryCoinConverter::PointsMode::InsertMidOnly,
                            GeometryCoinConverter::CurveMode::ClosedCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>((*it));
            pushToEdit(GeoId, 1, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) { // add an ellipse
            gcconv.convert< Part::GeomEllipse,
                            GeometryCoinConverter::PointsMode::InsertMidOnly,
                            GeometryCoinConverter::CurveMode::ClosedCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>((*it));
            pushToEdit(GeoId, 1, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) { // add an arc
            gcconv.convert< Part::GeomArcOfCircle,
                            GeometryCoinConverter::PointsMode::InsertStartEndMid,
                            GeometryCoinConverter::CurveMode::OpenCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>((*it));
            pushToEdit(GeoId, 3, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) { // add an arc
            gcconv.convert< Part::GeomArcOfEllipse,
                            GeometryCoinConverter::PointsMode::InsertStartEndMid,
                            GeometryCoinConverter::CurveMode::OpenCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>((*it));
            pushToEdit(GeoId, 3, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            gcconv.convert< Part::GeomArcOfHyperbola,
                            GeometryCoinConverter::PointsMode::InsertStartEndMid,
                            GeometryCoinConverter::CurveMode::OpenCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>((*it));
            pushToEdit(GeoId, 3, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            gcconv.convert< Part::GeomArcOfParabola,
                            GeometryCoinConverter::PointsMode::InsertStartEndMid,
                            GeometryCoinConverter::CurveMode::OpenCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitude>((*it));
            pushToEdit(GeoId, 3, 1);
        }
        else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) { // add a bspline
            gcconv.convert< Part::GeomBSplineCurve,
                            GeometryCoinConverter::PointsMode::InsertStartEnd,
                            GeometryCoinConverter::CurveMode::OpenCurve,
                            GeometryCoinConverter::AnalyseMode::BoundingBoxMagnitudeAndBSplineCurvature>((*it));
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
    analysisResults.boundingBoxMagnitudeOrder = exp(ceil(log(std::abs(gcconv.getBoundingBoxMaxMagnitude()))));
}

void CoinManager::updateAxesLength()
{
    edit->RootCrossCoordinate->point.set1Value(0,SbVec3f(-analysisResults.boundingBoxMagnitudeOrder, 0.0f, drawingParameters.zCross));
    edit->RootCrossCoordinate->point.set1Value(1,SbVec3f(analysisResults.boundingBoxMagnitudeOrder, 0.0f, drawingParameters.zCross));
    edit->RootCrossCoordinate->point.set1Value(2,SbVec3f(0.0f, -analysisResults.boundingBoxMagnitudeOrder, drawingParameters.zCross));
    edit->RootCrossCoordinate->point.set1Value(3,SbVec3f(0.0f, analysisResults.boundingBoxMagnitudeOrder, drawingParameters.zCross));
}


/** @brief      Class for creating the Overlay information layer
 *  @details
 *
 * Responsibility:
 * To create and update the SoGroup provided as a constructor parameter,
 * taking into account the drawing and overlay parameters provided as
 * constructor parameters.
 *
 * Interface:
 * A single entry point convert(), performing the following flow:
 *
 * [Geometry] => Calculate => addUpdateNode
 *
 * Calculate is responsible for generating information directly usable by Coin (but with standard types that
 * would enable portability) in a predetermined internal Node structure format (e.g. StringNode, PolygonNode)
 * that can generically be used by the addUpdateNode.
 *
 * addUpdateNode is responsible for creating or updating the node structure (depending on overlayParameters.rebuildInformationLayer)
 *
 * Supported:
 * Currently it only supports information of Part::Geometry objects and implements calculations only for GeomBSplineCurve.
 *
 * Caveats:
 * - This class relies on the order of creation to perform the update. Any parallel execution that does not deterministically
 * maintain the order will result in undefined behaviour. This provides a reasonable tradeoff between complexity and the fact that
 * currently the information layer is generally so small that no parallel execution would actually result in a performance gain.
 *
 */
class InformationOverlayCoinConverter {
private:

    enum class Calculation
    {
        BSplineDegree,
        BSplineControlPolygon,
        BSplineCurvatureComb,
        BSplineKnotMultiplicity,
        BSplinePoleWeight
    };
    enum class VisualisationType
    {
        Text,
        Polygon
    };

private:
    template< VisualisationType visualisationtype >
    struct Node {
        static constexpr VisualisationType type = visualisationtype;
    };

    struct NodeText : public Node<VisualisationType::Text> {
        std::vector<std::string> strings;
        std::vector<Base::Vector3d> positions;
    };

    struct NodePolygon: public Node<VisualisationType::Polygon> {
        std::vector<Base::Vector3d> coordinates;
        std::vector<int> indices;
    };

private:
    // Node Position in the Coin Scenograph for the different types of nodes
    enum class TextNodePosition {
        TextCoordinates = 0,
        TextInformation = 3
    };

    enum class PolygonNodePosition {
        PolygonCoordinates = 1,
        PolygonLineSet = 2
    };

public:
    InformationOverlayCoinConverter(SoGroup * infogroup,
                                    CoinManager::OverlayParameters & overlayparameters,
                                    CoinManager::DrawingParameters & drawingparameters):    infoGroup(infogroup),
                                                                                            overlayParameters(overlayparameters),
                                                                                            drawingParameters(drawingparameters),
                                                                                            nodeId(0){};

    /**
    * extracts information from the geometry and converts it into an information overlay in the
    * SoGroup provided in the constructor.
    *
    * @param geometry: the geometry to be processed
    */
    void convert(const Part::Geometry * geometry) {

        // at this point all calculations relate to BSplineCurves
        assert(geometry->getTypeId() == Part::GeomBSplineCurve::getClassTypeId());

        calculate<Calculation::BSplineDegree>(geometry);
        calculate<Calculation::BSplineControlPolygon>(geometry);
        calculate<Calculation::BSplineCurvatureComb>(geometry);
        calculate<Calculation::BSplineKnotMultiplicity>(geometry);
        calculate<Calculation::BSplinePoleWeight>(geometry);

        addUpdateNode<NodeText, Calculation::BSplineDegree>(degree);
        addUpdateNode<NodePolygon, Calculation::BSplineControlPolygon>(controlPolygon);
        addUpdateNode<NodePolygon, Calculation::BSplineCurvatureComb>(curvatureComb);
        addUpdateNode<NodeText, Calculation::BSplineKnotMultiplicity>(knotMultiplicity);
        addUpdateNode<NodeText, Calculation::BSplinePoleWeight>(poleWeights);

    };

private:
    template < Calculation calculation >
    void calculate(const Part::Geometry * geometry) {
        const Part::GeomBSplineCurve *spline = static_cast<const Part::GeomBSplineCurve *>(geometry);

        if constexpr (calculation == Calculation::BSplineDegree ) {
            clearCalculation<NodeText>(degree);

            std::vector<Base::Vector3d> poles = spline->getPoles();

            degree.strings.clear();
            degree.positions.clear();

            Base::Vector3d midp = Base::Vector3d(0,0,0);

            for (auto val : poles)
                midp += val;

            midp /= poles.size();

            degree.strings.emplace_back(std::to_string(spline->getDegree()));
            degree.positions.emplace_back(midp);
        }
        else if constexpr (calculation == Calculation::BSplineControlPolygon ) {

            clearCalculation<NodePolygon>(controlPolygon);

            std::vector<Base::Vector3d> poles = spline->getPoles();

            controlPolygon.coordinates.clear();
            controlPolygon.indices.clear();

            if (spline->isPeriodic())
                controlPolygon.coordinates.reserve(poles.size()+1);
            else
                controlPolygon.coordinates.reserve(poles.size());

            for (auto & v : poles)
                controlPolygon.coordinates.emplace_back(v);

            if (spline->isPeriodic())
                controlPolygon.coordinates.emplace_back(poles[0]);

            controlPolygon.indices.push_back(poles.size()); // single continuous poligon starting at index 0
        }
        else if constexpr (calculation == Calculation::BSplineCurvatureComb ) {

            clearCalculation<NodePolygon>(curvatureComb);
            // curvature graph --------------------------------------------------------

            // reimplementation of python source:
            // https://github.com/tomate44/CurvesWB/blob/master/ParametricComb.py
            // by FreeCAD user Chris_G

            std::vector<Base::Vector3d> poles = spline->getPoles();

            double firstparam = spline->getFirstParameter();
            double lastparam =  spline->getLastParameter();

            const int ndiv = poles.size()>4?poles.size()*16:64; // heuristic of number of division to fill in
            double step = (lastparam - firstparam) / (ndiv-1);

            std::vector<Base::Vector3d> pointatcurvelist;
            std::vector<double> curvaturelist;
            std::vector<Base::Vector3d> normallist;

            pointatcurvelist.reserve(ndiv);
            curvaturelist.reserve(ndiv);
            normallist.reserve(ndiv);

            for(int i = 0; i < ndiv; i++) {
                double param = firstparam + i * step;
                pointatcurvelist.emplace_back(spline->value(param));

                try {
                    curvaturelist.emplace_back(spline->curvatureAt(param));
                }
                catch(Base::CADKernelError &e) {
                    // it is "just" a visualisation matter OCC could not calculate the curvature
                    // terminating here would mean that the other shapes would not be drawn.
                    // Solution: Report the issue and set dummy curvature to 0
                    e.ReportException();
                    Base::Console().Error("Curvature graph for B-Spline with GeoId=%d could not be calculated.\n", 0); // TODO: Fix me
                    curvaturelist.emplace_back(0);
                }

                Base::Vector3d normal;
                try {
                    spline->normalAt(param, normal);
                    normallist.emplace_back(normal);
                }
                catch(Base::Exception&) {
                    normallist.emplace_back(0,0,0);
                }
            }

            std::vector<Base::Vector3d> pointatcomblist;
            pointatcomblist.reserve(ndiv);

            for(int i = 0; i < ndiv; i++) {
                pointatcomblist.emplace_back(pointatcurvelist[i] - overlayParameters.currentBSplineCombRepresentationScale * curvaturelist[i] * normallist[i]);
            }

            curvatureComb.coordinates.reserve(3*ndiv); // 2*ndiv +1 points of ndiv separate segments + ndiv points for last segment
            curvatureComb.indices.reserve(ndiv+1); // ndiv separate segments of radials + 1 segment connecting at comb end

            for(int i = 0; i < ndiv; i++) {
                // note emplace emplaces on the position BEFORE the iterator given.
                curvatureComb.coordinates.emplace_back(pointatcurvelist[i].x, pointatcurvelist[i].y, drawingParameters.zInfo); // radials
                curvatureComb.coordinates.emplace_back(pointatcomblist[i].x, pointatcomblist[i].y, drawingParameters.zInfo); // radials

                curvatureComb.indices.emplace_back(2); // line
            }

            for(int i = 0; i < ndiv; i++)
                curvatureComb.coordinates.emplace_back(pointatcomblist[i].x, pointatcomblist[i].y, drawingParameters.zInfo); // // comb endpoint closing segment

            curvatureComb.indices.emplace_back(ndiv); // Comb line
        }
        else if constexpr (calculation == Calculation::BSplineKnotMultiplicity ) {

            clearCalculation<NodeText>(knotMultiplicity);
            std::vector<double> knots = spline->getKnots();
            std::vector<int> mult = spline->getMultiplicities();

            for(size_t i=0; i<knots.size(); i++) {
                knotMultiplicity.positions.emplace_back(spline->pointAtParameter(knots[i]));

                std::ostringstream stringStream;
                stringStream << "(" << mult[i] << ")";

                knotMultiplicity.strings.emplace_back( stringStream.str());
            }
        }
        else if constexpr (calculation == Calculation::BSplinePoleWeight ) {

            clearCalculation<NodeText>(poleWeights);
            std::vector<Base::Vector3d> poles = spline->getPoles();
            auto weights = spline->getWeights();

            for(size_t i=0; i<poles.size(); i++) {
                poleWeights.positions.emplace_back(poles[i]);

                QString WeightString  = QString::fromLatin1("[%1]").arg(weights[i], 0, 'f', Base::UnitsApi::getDecimals());

                poleWeights.strings.emplace_back(WeightString.toStdString());
            }
        }

    }

    template < typename Result, Calculation calculation>
    void addUpdateNode(const Result & result) {

        if(overlayParameters.rebuildInformationLayer)
            addNode<Result, calculation>(result);
        else
            updateNode<Result, calculation>(result);
    }

    template < Calculation calculation >
    bool isVisible() {
        if constexpr ( calculation == Calculation::BSplineDegree ) {
            return overlayParameters.bSplineDegreeVisible;
        }
        else if constexpr ( calculation == Calculation::BSplineControlPolygon ) {
            return overlayParameters.bSplineControlPolygonVisible;
        }
        else if constexpr ( calculation == Calculation::BSplineCurvatureComb ) {
            return overlayParameters.bSplineCombVisible;
        }
        else if constexpr ( calculation == Calculation::BSplineKnotMultiplicity ) {
            return overlayParameters.bSplineKnotMultiplicityVisible;
        }
        else if constexpr ( calculation == Calculation::BSplinePoleWeight ) {
            return overlayParameters.bSplinePoleWeightVisible;
        }
    }

    template < typename Result >
    void setPolygon(const Result & result, SoLineSet *polygonlineset, SoCoordinate3 *polygoncoords) {

        polygoncoords->point.setNum(result.coordinates.size());
        polygonlineset->numVertices.setNum(result.indices.size());

        int32_t *index = polygonlineset->numVertices.startEditing();
        SbVec3f *vts = polygoncoords->point.startEditing();

        for(size_t i = 0; i < result.coordinates.size(); i++)
            vts[i].setValue(result.coordinates[i].x, result.coordinates[i].y, drawingParameters.zInfo);

        for(size_t i = 0; i < result.indices.size(); i++)
            index[i] = result.indices[i];

        polygoncoords->point.finishEditing();
        polygonlineset->numVertices.finishEditing();
    }

    template < int line = 1 >
    void setText(const std::string & string, SoText2 * text) {

        if constexpr (line == 1) {
            text->string = SbString(string.c_str());
        }
        else {
            assert(line > 1);
            SoMFString label;
            for ( int l = 0; l < (line - 1) ; l++)
                label.set1Value(l, SbString(""));

            label.set1Value(line-1, SbString(string.c_str()));
            text->string = label;
        }
    }

    void addToInfoGroup(SoSwitch * sw) {
        infoGroup->addChild(sw);
        nodeId++;
    }

    template < typename Result >
    void clearCalculation(Result & result) {
         if constexpr ( Result::type == VisualisationType::Text ) {
            result.positions.clear();
            result.strings.clear();
         }
         else if constexpr (Result::type == VisualisationType::Polygon) {
            result.coordinates.clear();
            result.indices.clear();
         }
    }

    template < typename Result, Calculation calculation >
    void addNode(const Result & result) {

        if constexpr ( Result::type == VisualisationType::Text ) {

            for(size_t i = 0; i < result.strings.size(); i++) {

                SoSwitch *sw = new SoSwitch();

                sw->whichChild = isVisible<calculation>()?SO_SWITCH_ALL:SO_SWITCH_NONE;

                SoSeparator *sep = new SoSeparator();
                sep->ref();
                // no caching for frequently-changing data structures
                sep->renderCaching = SoSeparator::OFF;

                // every information visual node gets its own material for to-be-implemented preselection and selection
                SoMaterial *mat = new SoMaterial;
                mat->ref();
                mat->diffuseColor = drawingParameters.InformationColor;

                SoTranslation *translate = new SoTranslation;

                translate->translation.setValue(result.positions[i].x, result.positions[i].y, drawingParameters.zInfo);

                SoFont *font = new SoFont;
                font->name.setValue("Helvetica");
                font->size.setValue(drawingParameters.coinFontSize);

                SoText2 *text = new SoText2;

                // since the first and last control point of a spline is also treated as knot and thus
                // can also have a displayed multiplicity, we must assure the multiplicity is not visibly overwritten
                // therefore be output the weight in a second line
                //
                // This could be made into a more generic form, but it is probably not worth the effort at this time.
                if constexpr ( calculation == Calculation::BSplinePoleWeight )
                    setText<2>(result.strings[i], text);
                else
                    setText(result.strings[i], text);

                sep->addChild(translate);
                sep->addChild(mat);
                sep->addChild(font);
                sep->addChild(text);

                sw->addChild(sep);

                addToInfoGroup(sw);
                sep->unref();
                mat->unref();
            }
        }
        else if constexpr (Result::type == VisualisationType::Polygon) {

            SoSwitch *sw = new SoSwitch();

            // hGrpsk->GetBool("BSplineControlPolygonVisible", true)
            sw->whichChild = isVisible<calculation>()?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = new SoSeparator();
            sep->ref();
            // no caching for frequently-changing data structures
            sep->renderCaching = SoSeparator::OFF;

            // every information visual node gets its own material for to-be-implemented preselection and selection
            SoMaterial *mat = new SoMaterial;
            mat->ref();
            mat->diffuseColor = drawingParameters.InformationColor;

            SoLineSet *polygonlineset = new SoLineSet;
            SoCoordinate3 *polygoncoords = new SoCoordinate3;

            setPolygon<Result>(result, polygonlineset, polygoncoords);

            sep->addChild(mat);
            sep->addChild(polygoncoords);
            sep->addChild(polygonlineset);

            sw->addChild(sep);

            addToInfoGroup(sw);
            sep->unref();
            mat->unref();
        }
    }

    template < typename Result, Calculation calculation >
    void updateNode(const Result & result) {

         if constexpr ( Result::type == VisualisationType::Text ) {

            for(size_t i = 0; i < result.strings.size(); i++) {
                SoSwitch *sw = static_cast<SoSwitch *>(infoGroup->getChild(nodeId));

                if (overlayParameters.visibleInformationChanged)
                    sw->whichChild = isVisible<calculation>()?SO_SWITCH_ALL:SO_SWITCH_NONE;

                SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

                static_cast<SoTranslation *>(sep->getChild(static_cast<int>(TextNodePosition::TextCoordinates)))->translation.setValue(result.positions[i].x, result.positions[i].y, drawingParameters.zInfo);

                // since the first and last control point of a spline is also treated as knot and thus
                // can also have a displayed multiplicity, we must assure the multiplicity is not visibly overwritten
                // therefore be output the weight in a second line
                //
                // This could be made into a more generic form, but it is probably not worth the effort at this time.
                if constexpr ( calculation == Calculation::BSplinePoleWeight )
                    setText<2>(result.strings[i], static_cast<SoText2 *>(sep->getChild(static_cast<int>(TextNodePosition::TextInformation))));
                else
                    setText(result.strings[i], static_cast<SoText2 *>(sep->getChild(static_cast<int>(TextNodePosition::TextInformation))));

                nodeId++;
            }

        }
        else if constexpr (Result::type == VisualisationType::Polygon) {

            SoSwitch *sw = static_cast<SoSwitch *>(infoGroup->getChild(nodeId));

            if(overlayParameters.visibleInformationChanged)
                sw->whichChild = isVisible<calculation>()?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

            SoCoordinate3 *polygoncoords = static_cast<SoCoordinate3 *>(sep->getChild(static_cast<int>(PolygonNodePosition::PolygonCoordinates)));

            SoLineSet *polygonlineset = static_cast<SoLineSet *>(sep->getChild(static_cast<int>(PolygonNodePosition::PolygonLineSet)));

            setPolygon(result, polygonlineset, polygoncoords);

            nodeId++;
        }
    }

private:
    SoGroup * infoGroup;
    CoinManager::OverlayParameters & overlayParameters;
    CoinManager::DrawingParameters & drawingParameters;

    // Calculations
    NodeText degree;
    NodeText knotMultiplicity;
    NodeText poleWeights;
    NodePolygon controlPolygon;
    NodePolygon curvatureComb;

    // Node Management
    int nodeId;
};

void CoinManager::processGeometryInformationOverlay(const GeoList & geolist)
{
    auto GeoById = [](const std::vector<Part::Geometry*> GeoList, int Id){
    {
        if (Id >= 0)
            return GeoList[Id];
        else
            return GeoList[GeoList.size()+Id];
        }
    };

    auto ioconv = InformationOverlayCoinConverter(edit->infoGroup, overlayParameters, drawingParameters);

    // geometry information layer for bsplines, as they need a second round now that max curvature is known
    for (auto geoid : analysisResults.bsplineGeoIds) {
        const Part::Geometry *geo = GeoById(geolist.geomlist, geoid);

        ioconv.convert(geo);
    }

    overlayParameters.visibleInformationChanged = false; // just updated
}

void CoinManager::updateOverlayParameters()
{
    if ( (analysisResults.combRepresentationScale > (2 * overlayParameters.currentBSplineCombRepresentationScale)) ||
        (analysisResults.combRepresentationScale < (overlayParameters.currentBSplineCombRepresentationScale / 2)))
        overlayParameters.currentBSplineCombRepresentationScale = analysisResults.combRepresentationScale ;
}

void CoinManager::processGeometryAndInformationOverlay(const GeoList & geolist, bool rebuildinformationlayer)
{
    drawingParameters.coinFontSize = edit->coinFontSize; // TODO: Evaluate refactoring this after constraints are migrated.
    overlayParameters.rebuildInformationLayer = rebuildinformationlayer;

    processGeometry(geolist);

    updateOverlayParameters();

    processGeometryInformationOverlay(geolist);

    updateAxesLength();
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
