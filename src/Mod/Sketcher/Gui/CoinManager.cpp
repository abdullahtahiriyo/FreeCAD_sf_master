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
# include <Inventor/SoPickedPoint.h>

# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoImage.h>
# include <Inventor/nodes/SoInfo.h>

# include <Inventor/actions/SoRayPickAction.h>

# include <Inventor/SbVec3f.h>

# include <memory>
#endif  // #ifndef _PreComp_

#include "EditData.h"

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Base/Exception.h>
#include <Base/Tools2D.h>
#include <Base/UnitsApi.h>
#include <Gui/Utilities.h>
#include <Base/Converter.h>

#include <Base/Vector3D.h>

#include <App/ObjectIdentifier.h>

#include <Gui/SoFCBoundingBox.h>

#include <Gui/Inventor/MarkerBitmaps.h>

#include "SoZoomTranslation.h"
#include "SoDatumLabel.h"

#include "InformationOverlayCoinConverter.h"

#include "GeometryCoinConverter.h"

#include "GeoList.h"

#include "ViewProviderSketch.h"

#include "CoinManager.h"

using namespace SketcherGui;
using namespace Sketcher;


//***** ViewProviderSketchCoinAttorney - Attorney to limit coupling and encapsulation to viewprovider ******************************

inline bool ViewProviderSketchCoinAttorney::constraintHasExpression(ViewProviderSketch & vp, int constrid)
{
    return vp.constraintHasExpression(constrid);
};

inline const std::vector<Sketcher::Constraint *> ViewProviderSketchCoinAttorney::getConstraints(ViewProviderSketch & vp)
{
    return vp.getConstraints();
}

inline const GeoList ViewProviderSketchCoinAttorney::getGeoList(ViewProviderSketch & vp)
{
    return vp.getGeoList();
}

inline Base::Placement ViewProviderSketchCoinAttorney::getEditingPlacement(ViewProviderSketch & vp)
{
    return vp.getEditingPlacement();
}

inline void ViewProviderSketchCoinAttorney::updateGridExtent(ViewProviderSketch & vp, float minx, float maxx, float miny, float maxy)
{
    vp.updateGridExtent(minx, maxx, miny, maxy);
}

inline bool ViewProviderSketchCoinAttorney::isShownVirtualSpace(ViewProviderSketch & vp)
{
    return vp.isShownVirtualSpace;
}

inline std::unique_ptr<SoRayPickAction> ViewProviderSketchCoinAttorney::getRayPickAction(ViewProviderSketch & vp)
{
    return vp.getRayPickAction();
}

float ViewProviderSketchCoinAttorney::getScaleFactor(ViewProviderSketch & vp)
{
    return vp.getScaleFactor();
}


//**************************** ParameterObserver nested class ******************************
CoinManager::ParameterObserver::ParameterObserver(CoinManager &client): Client(client)
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

    updateLineRenderingOrderParameters();

    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineDegree>();
    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineControlPolygonVisible>();
    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineCombVisible>();
    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineKnotMultiplicityVisible>();
    updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplinePoleWeightVisible>();
}

void CoinManager::ParameterObserver::updateCurvedEdgeCountSegmentsParameter()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    int stdcountsegments = hGrp->GetInt("SegmentsPerGeometry", 50);
    // value cannot be smaller than 6
    if (stdcountsegments < 6)
        stdcountsegments = 6;

    Client.drawingParameters.curvedEdgeCountSegments = stdcountsegments;
}

void CoinManager::ParameterObserver::updateLineRenderingOrderParameters()
{
    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    Client.drawingParameters.topRenderingGeometry = DrawingParameters::GeometryRendering (hGrpp->GetInt("TopRenderGeometryId",1));
    Client.drawingParameters.midRenderingGeometry = DrawingParameters::GeometryRendering (hGrpp->GetInt("MidRenderGeometryId",2));
}

template<CoinManager::ParameterObserver::OverlayVisibilityParameter visibilityparameter>
void CoinManager::ParameterObserver::updateOverlayVisibilityParameter()
{
    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineDegree)
        Client.overlayParameters.bSplineDegreeVisible = hGrpsk->GetBool("BSplineDegreeVisible", true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineControlPolygonVisible)
        Client.overlayParameters.bSplineControlPolygonVisible = hGrpsk->GetBool("BSplineControlPolygonVisible", true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineCombVisible)
        Client.overlayParameters.bSplineCombVisible = hGrpsk->GetBool("BSplineCombVisible", true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineKnotMultiplicityVisible)
        Client.overlayParameters.bSplineKnotMultiplicityVisible = hGrpsk->GetBool("BSplineKnotMultiplicityVisible", true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplinePoleWeightVisible)
        Client.overlayParameters.bSplinePoleWeightVisible = hGrpsk->GetBool("BSplinePoleWeightVisible", true);

    Client.overlayParameters.visibleInformationChanged = true;
}

void CoinManager::ParameterObserver::subscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Attach(this);

    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpsk->Attach(this);

    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpp->Attach(this);
}

void CoinManager::ParameterObserver::unsubscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Detach(this);

    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpsk->Detach(this);

    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpp->Detach(this);
}

void CoinManager::ParameterObserver::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    (void) rCaller;

    // static map to avoid substantial if/else branching
    //
    // key->first               => String of parameter,
    // key->second              => Update function to be called for the parameter,
    static std::map<std::string, std::function<void()>> str2updatefunction {
        {"SegmentsPerGeometry",
            [this](){updateCurvedEdgeCountSegmentsParameter();}},
        {"BSplineDegreeVisible",
            [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineDegree>();}},
        {"BSplineControlPolygonVisible",
            [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineControlPolygonVisible>();}},
        {"BSplineCombVisible",
            [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineCombVisible>();}},
        {"BSplineKnotMultiplicityVisible",
            [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineKnotMultiplicityVisible>();}},
        {"BSplinePoleWeightVisible",
            [this](){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplinePoleWeightVisible>();}},
        {"TopRenderGeometryId",
            [this](){updateLineRenderingOrderParameters();}},
        {"MidRenderGeometryId",
            [this](){updateLineRenderingOrderParameters();}}
    };

    auto key = str2updatefunction.find(sReason);
    if( key != str2updatefunction.end() ) {
        key->second();

        Client.redrawViewProvider(); // redraw with non-temporal geometry
    }


}

//**************************** CoinManager class ******************************

CoinManager::CoinManager(ViewProviderSketch &vp, EditData * editdata):viewProvider(vp),edit(editdata) {
    // Create parameter observer and initialise watched parameters
    pObserver = std::make_unique<CoinManager::ParameterObserver>(*this);

}

CoinManager::~CoinManager() {}


void CoinManager::processGeometry(const GeoList & geolist)
{
    // Define a single layer processing to be converted to coin (currently it is the whole geometry)
    GeometryLayer geolayer { geolist };

    // Define the coin nodes that will be filled in with the single layer
    GeometryLayerNodes geometryLayerNodes {
        edit->PointsMaterials,
        edit->CurvesMaterials,
        edit->PointsCoordinate,
        edit->CurvesCoordinate,
        edit->CurveSet
    };

    // process geometry layer
    // TODO: Root is set by GeometryCoinConverter which is ok for one layer only.
    GeometryCoinConverter gcconv(geometryLayerNodes, drawingParameters);

    gcconv.convert(geolayer);

    // set cross coordinates
    edit->RootCrossSet->numVertices.set1Value(0,2);
    edit->RootCrossSet->numVertices.set1Value(1,2);

    edit->CurvIdToGeoId = gcconv.getCurveMap();
    edit->PointIdToGeoId = gcconv.getPointMap();
    edit->GeoIdPointPosToPointId = gcconv.getReversePointMap();


    // TODO: THIS NEEDS REFACTORING
    analysisResults.combRepresentationScale = gcconv.getCombRepresentationScale();
    analysisResults.boundingBoxMagnitudeOrder = exp(ceil(log(std::abs(gcconv.getBoundingBoxMaxMagnitude()))));
    analysisResults.bsplineGeoIds = gcconv.getBSplineGeoIds();
}

void CoinManager::updateAxesLength()
{
    edit->RootCrossCoordinate->point.set1Value(0,SbVec3f(-analysisResults.boundingBoxMagnitudeOrder, 0.0f, drawingParameters.zCross));
    edit->RootCrossCoordinate->point.set1Value(1,SbVec3f(analysisResults.boundingBoxMagnitudeOrder, 0.0f, drawingParameters.zCross));
    edit->RootCrossCoordinate->point.set1Value(2,SbVec3f(0.0f, -analysisResults.boundingBoxMagnitudeOrder, drawingParameters.zCross));
    edit->RootCrossCoordinate->point.set1Value(3,SbVec3f(0.0f, analysisResults.boundingBoxMagnitudeOrder, drawingParameters.zCross));
}

void CoinManager::processGeometryInformationOverlay(const GeoList & geolist)
{
    if(overlayParameters.rebuildInformationLayer) {
        // every time we start with empty information overlay
        Gui::coinRemoveAllChildren(edit->infoGroup);
    }

    auto ioconv = InformationOverlayCoinConverter(edit->infoGroup, overlayParameters, drawingParameters);

    // geometry information layer for bsplines, as they need a second round now that max curvature is known
    for (auto geoid : analysisResults.bsplineGeoIds) {
        const Part::Geometry *geo = geolist.getGeometryFromGeoId(geoid);

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

void CoinManager::updateVirtualSpace()
{
    const std::vector<Sketcher::Constraint *> &constrlist = ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    bool isshownvirtualspace = ViewProviderSketchCoinAttorney::isShownVirtualSpace(viewProvider);

    if(constrlist.size() == edit->vConstrType.size()) {

        edit->constrGroup->enable.setNum(constrlist.size());

        SbBool *sws = edit->constrGroup->enable.startEditing();

        for (size_t i = 0; i < constrlist.size(); i++)
            sws[i] = !(constrlist[i]->isInVirtualSpace != isshownvirtualspace); // XOR of constraint mode and VP mode


        edit->constrGroup->enable.finishEditing();
    }
}

void CoinManager::processConstraints(const GeoList & geolist)
{
    const auto &constrlist = ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    // After an undo/redo it can happen that we have an empty geometry list but a non-empty constraint list
    // In this case just ignore the constraints. (See bug #0000421)
    if (geolist.geomlist.size() <= 2 && !constrlist.empty()) {
        rebuildConstraintNodes();
        return;
    }

    int extGeoCount = geolist.getExternalCount();
    int intGeoCount = geolist.getInternalCount();

    // reset point if the constraint type has changed
Restart:
    // check if a new constraint arrived
    if (constrlist.size() != edit->vConstrType.size())
        rebuildConstraintNodes();

    assert(int(constrlist.size()) == edit->constrGroup->getNumChildren());
    assert(int(edit->vConstrType.size()) == edit->constrGroup->getNumChildren());

    // update the virtual space
    updateVirtualSpace();

    auto getNormal = [] (const GeoList & geolist, const int geoid, const Base::Vector3d & pointoncurve) {
        auto geom = geolist.getGeometryFromGeoId(geoid);
        auto curve = dynamic_cast<const Part::GeomCurve *>(geom);

        Base::Vector3d normal;
        if(!(curve && curve->normalAt(pointoncurve, normal))) {
                normal = Base::Vector3d(1,0,0);
        }

        return normal;
    };

    // go through the constraints and update the position
    int i = 0;
    for (std::vector<Sketcher::Constraint *>::const_iterator it=constrlist.begin();
         it != constrlist.end(); ++it, i++) {
        // check if the type has changed
        if ((*it)->Type != edit->vConstrType[i]) {
            // clearing the type vector will force a rebuild of the visual nodes
            edit->vConstrType.clear();
            //TODO: The 'goto' here is unsafe as it can happen that we cause an endless loop (see bug #0001956).
            goto Restart;
        }
        try{//because calculateNormalAtPoint, used in there, can throw
            // root separator for this constraint
            SoSeparator *sep = static_cast<SoSeparator *>(edit->constrGroup->getChild(i));
            const Constraint *Constr = *it;

            if(Constr->First < -extGeoCount || Constr->First >= intGeoCount
                    || (Constr->Second!=Constraint::GeoUndef
                        && (Constr->Second < -extGeoCount || Constr->Second >= intGeoCount))
                    || (Constr->Third!=Constraint::GeoUndef
                        && (Constr->Third < -extGeoCount || Constr->Third >= intGeoCount)))
            {
                // Constraint can refer to non-existent geometry during undo/redo
                continue;
            }

            // distinguish different constraint types to build up
            switch (Constr->Type) {
                case Block:
                case Horizontal: // write the new position of the Horizontal constraint Same as vertical position.
                case Vertical: // write the new position of the Vertical constraint
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        bool alignment = Constr->Type!=Block && Constr->Second != Constraint::GeoUndef;

                        // get the geometry
                        const Part::Geometry *geo = geolist.getGeometryFromGeoId(Constr->First);

                        if (!alignment) {
                            // Vertical & Horiz can only be a GeomLineSegment, but Blocked can be anything.
                            Base::Vector3d midpos;
                            Base::Vector3d dir;
                            Base::Vector3d norm;

                            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);

                                // calculate the half distance between the start and endpoint
                                midpos = ((lineSeg->getEndPoint()+lineSeg->getStartPoint())/2);

                                //Get a set of vectors perpendicular and tangential to these
                                dir = (lineSeg->getEndPoint()-lineSeg->getStartPoint()).Normalize();

                                norm = Base::Vector3d(-dir.y,dir.x,0);
                            }
                            else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                                const Part::GeomBSplineCurve *bsp = static_cast<const Part::GeomBSplineCurve *>(geo);
                                midpos = Base::Vector3d(0,0,0);

                                std::vector<Base::Vector3d> poles = bsp->getPoles();

                                // Move center of gravity towards start not to collide with bspline degree information.
                                double ws = 1.0 / poles.size();
                                double w = 1.0;

                                for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it) {
                                    midpos += w*(*it);
                                    w -= ws;
                                }

                                midpos /= poles.size();

                                dir = (bsp->getEndPoint() - bsp->getStartPoint()).Normalize();
                                norm = Base::Vector3d(-dir.y,dir.x,0);
                            }
                            else {
                                double ra=0,rb=0;
                                double angle,angleplus=0.;//angle = rotation of object as a whole; angleplus = arc angle (t parameter for ellipses).
                                if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                    const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo);
                                    ra = circle->getRadius();
                                    angle = M_PI/4;
                                    midpos = circle->getCenter();
                                } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                    const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                                    ra = arc->getRadius();
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle = (startangle + endangle)/2;
                                    midpos = arc->getCenter();
                                } else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo);
                                    ra = ellipse->getMajorRadius();
                                    rb = ellipse->getMinorRadius();
                                    Base::Vector3d majdir = ellipse->getMajorAxisDir();
                                    angle = atan2(majdir.y, majdir.x);
                                    angleplus = M_PI/4;
                                    midpos = ellipse->getCenter();
                                } else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                                    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo);
                                    ra = aoe->getMajorRadius();
                                    rb = aoe->getMinorRadius();
                                    double startangle, endangle;
                                    aoe->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoe->getMajorAxisDir();
                                    angle = atan2(majdir.y, majdir.x);
                                    angleplus = (startangle + endangle)/2;
                                    midpos = aoe->getCenter();
                                } else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                                    const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(geo);
                                    ra = aoh->getMajorRadius();
                                    rb = aoh->getMinorRadius();
                                    double startangle, endangle;
                                    aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoh->getMajorAxisDir();
                                    angle = atan2(majdir.y, majdir.x);
                                    angleplus = (startangle + endangle)/2;
                                    midpos = aoh->getCenter();
                                } else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                                    const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(geo);
                                    ra = aop->getFocal();
                                    double startangle, endangle;
                                    aop->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = - aop->getXAxisDir();
                                    angle = atan2(majdir.y, majdir.x);
                                    angleplus = (startangle + endangle)/2;
                                    midpos = aop->getFocus();
                                } else
                                    break;

                                if( geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                                    geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                                    geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ){

                                    Base::Vector3d majDir, minDir, rvec;
                                    majDir = Base::Vector3d(cos(angle),sin(angle),0);//direction of major axis of ellipse
                                    minDir = Base::Vector3d(-majDir.y,majDir.x,0);//direction of minor axis of ellipse
                                    rvec = (ra*cos(angleplus)) * majDir   +   (rb*sin(angleplus)) * minDir;
                                    midpos += rvec;
                                    rvec.Normalize();
                                    norm = rvec;
                                    dir = Base::Vector3d(-rvec.y,rvec.x,0);//DeepSOIC: I'm not sure what dir is supposed to mean.
                                }
                                else {
                                    norm = Base::Vector3d(cos(angle),sin(angle),0);
                                    dir = Base::Vector3d(-norm.y,norm.x,0);
                                    midpos += ra*norm;
                                }
                            }

                            Base::Vector3d relpos = seekConstraintPosition(midpos, norm, dir, 2.5, edit->constrGroup->getChild(i));

                            auto translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                            translation->abPos = SbVec3f(midpos.x, midpos.y, drawingParameters.zConstr); //Absolute Reference

                            //Reference Position that is scaled according to zoom
                            translation->translation = SbVec3f(relpos.x, relpos.y, 0);
                        }
                        else {
                            assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);
                            assert(Constr->FirstPos != Sketcher::none && Constr->SecondPos != Sketcher::none);

                            Base::Vector3d midpos1, dir1, norm1;
                            Base::Vector3d midpos2, dir2, norm2;

                            midpos1 = geolist.getPoint(Constr->First, Constr->FirstPos);
                            midpos2 = geolist.getPoint(Constr->Second, Constr->SecondPos);

                            dir1 = (midpos2-midpos1).Normalize();
                            dir2 = -dir1;
                            norm1 = Base::Vector3d(-dir1.y,dir1.x,0.);
                            norm2 = norm1;

                            Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 4.0, edit->constrGroup->getChild(i));

                            auto translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                            translation->abPos = SbVec3f(midpos1.x, midpos1.y, drawingParameters.zConstr);
                            translation->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                            Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 4.0, edit->constrGroup->getChild(i));

                            Base::Vector3d secondPos = midpos2 - midpos1;

                            translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::SecondTranslationIndex)));

                            translation->abPos = SbVec3f(secondPos.x, secondPos.y, drawingParameters.zConstr);
                            translation->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);
                        }
                    }
                    break;
                case Perpendicular:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);
                        // get the geometry
                        const Part::Geometry *geo1 = geolist.getGeometryFromGeoId(Constr->First);
                        const Part::Geometry *geo2 = geolist.getGeometryFromGeoId(Constr->Second);

                        Base::Vector3d midpos1, dir1, norm1;
                        Base::Vector3d midpos2, dir2, norm2;
                        bool twoIcons = false;//a very local flag. It's set to true to indicate that the second dir+norm are valid and should be used


                        if (Constr->Third != Constraint::GeoUndef || //perpty via point
                                Constr->FirstPos != Sketcher::none) { //endpoint-to-curve or endpoint-to-endpoint perpty

                            int ptGeoId;
                            Sketcher::PointPos ptPosId;
                            do {//dummy loop to use break =) Maybe goto?
                                ptGeoId = Constr->First;
                                ptPosId = Constr->FirstPos;
                                if (ptPosId != Sketcher::none) break;
                                ptGeoId = Constr->Second;
                                ptPosId = Constr->SecondPos;
                                if (ptPosId != Sketcher::none) break;
                                ptGeoId = Constr->Third;
                                ptPosId = Constr->ThirdPos;
                                if (ptPosId != Sketcher::none) break;
                                assert(0);//no point found!
                            } while (false);

                            midpos1 = geolist.getPoint(ptGeoId, ptPosId);

                            norm1 = getNormal(geolist, Constr->Second, midpos1);

                            // TODO: Check the method above. This was the old one making use of the solver.
                            //norm1 = getSolvedSketch().calculateNormalAtPoint(Constr->Second, midpos1.x, midpos1.y);

                            norm1.Normalize();
                            dir1 = norm1; dir1.RotateZ(-M_PI/2.0);

                        } else if (Constr->FirstPos == Sketcher::none) {

                            if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment *>(geo1);
                                midpos1 = ((lineSeg1->getEndPoint()+lineSeg1->getStartPoint())/2);
                                dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint()).Normalize();
                                norm1 = Base::Vector3d(-dir1.y,dir1.x,0.);
                            } else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo1);
                                double startangle, endangle, midangle;
                                arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                midangle = (startangle + endangle)/2;
                                norm1 = Base::Vector3d(cos(midangle),sin(midangle),0);
                                dir1 = Base::Vector3d(-norm1.y,norm1.x,0);
                                midpos1 = arc->getCenter() + arc->getRadius() * norm1;
                            } else if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo1);
                                norm1 = Base::Vector3d(cos(M_PI/4),sin(M_PI/4),0);
                                dir1 = Base::Vector3d(-norm1.y,norm1.x,0);
                                midpos1 = circle->getCenter() + circle->getRadius() * norm1;
                            } else
                                break;

                            if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment *>(geo2);
                                midpos2 = ((lineSeg2->getEndPoint()+lineSeg2->getStartPoint())/2);
                                dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint()).Normalize();
                                norm2 = Base::Vector3d(-dir2.y,dir2.x,0.);
                            } else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                double startangle, endangle, midangle;
                                arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                midangle = (startangle + endangle)/2;
                                norm2 = Base::Vector3d(cos(midangle),sin(midangle),0);
                                dir2 = Base::Vector3d(-norm2.y,norm2.x,0);
                                midpos2 = arc->getCenter() + arc->getRadius() * norm2;
                            } else if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo2);
                                norm2 = Base::Vector3d(cos(M_PI/4),sin(M_PI/4),0);
                                dir2 = Base::Vector3d(-norm2.y,norm2.x,0);
                                midpos2 = circle->getCenter() + circle->getRadius() * norm2;
                            } else
                                break;
                            twoIcons = true;
                        }

                        Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 4.0, edit->constrGroup->getChild(i));

                        auto translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                        translation->abPos = SbVec3f(midpos1.x, midpos1.y, drawingParameters.zConstr);
                        translation->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                        if (twoIcons) {
                            Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 4.0, edit->constrGroup->getChild(i));

                            Base::Vector3d secondPos = midpos2 - midpos1;
                            auto translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::SecondTranslationIndex)));
                            translation->abPos = SbVec3f(secondPos.x, secondPos.y, drawingParameters.zConstr);
                            translation->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);
                        }

                    }
                    break;
                case Parallel:
                case Equal:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);
                        // get the geometry
                        const Part::Geometry *geo1 = geolist.getGeometryFromGeoId(Constr->First);
                        const Part::Geometry *geo2 = geolist.getGeometryFromGeoId(Constr->Second);

                        Base::Vector3d midpos1, dir1, norm1;
                        Base::Vector3d midpos2, dir2, norm2;
                        if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                            geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                            if (Constr->Type == Equal) {
                                double r1a=0,r1b=0,r2a=0,r2b=0;
                                double angle1,angle1plus=0.,  angle2, angle2plus=0.;//angle1 = rotation of object as a whole; angle1plus = arc angle (t parameter for ellipses).
                                if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                    const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo1);
                                    r1a = circle->getRadius();
                                    angle1 = M_PI/4;
                                    midpos1 = circle->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                    const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo1);
                                    r1a = arc->getRadius();
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle1 = (startangle + endangle)/2;
                                    midpos1 = arc->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo1);
                                    r1a = ellipse->getMajorRadius();
                                    r1b = ellipse->getMinorRadius();
                                    Base::Vector3d majdir = ellipse->getMajorAxisDir();
                                    angle1 = atan2(majdir.y, majdir.x);
                                    angle1plus = M_PI/4;
                                    midpos1 = ellipse->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                                    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo1);
                                    r1a = aoe->getMajorRadius();
                                    r1b = aoe->getMinorRadius();
                                    double startangle, endangle;
                                    aoe->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoe->getMajorAxisDir();
                                    angle1 = atan2(majdir.y, majdir.x);
                                    angle1plus = (startangle + endangle)/2;
                                    midpos1 = aoe->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                                    const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(geo1);
                                    r1a = aoh->getMajorRadius();
                                    r1b = aoh->getMinorRadius();
                                    double startangle, endangle;
                                    aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoh->getMajorAxisDir();
                                    angle1 = atan2(majdir.y, majdir.x);
                                    angle1plus = (startangle + endangle)/2;
                                    midpos1 = aoh->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                                    const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(geo1);
                                    r1a = aop->getFocal();
                                    double startangle, endangle;
                                    aop->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = - aop->getXAxisDir();
                                    angle1 = atan2(majdir.y, majdir.x);
                                    angle1plus = (startangle + endangle)/2;
                                    midpos1 = aop->getFocus();
                                } else
                                    break;

                                if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                    const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo2);
                                    r2a = circle->getRadius();
                                    angle2 = M_PI/4;
                                    midpos2 = circle->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                    const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                    r2a = arc->getRadius();
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle2 = (startangle + endangle)/2;
                                    midpos2 = arc->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo2);
                                    r2a = ellipse->getMajorRadius();
                                    r2b = ellipse->getMinorRadius();
                                    Base::Vector3d majdir = ellipse->getMajorAxisDir();
                                    angle2 = atan2(majdir.y, majdir.x);
                                    angle2plus = M_PI/4;
                                    midpos2 = ellipse->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                                    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo2);
                                    r2a = aoe->getMajorRadius();
                                    r2b = aoe->getMinorRadius();
                                    double startangle, endangle;
                                    aoe->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoe->getMajorAxisDir();
                                    angle2 = atan2(majdir.y, majdir.x);
                                    angle2plus = (startangle + endangle)/2;
                                    midpos2 = aoe->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                                    const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(geo2);
                                    r2a = aoh->getMajorRadius();
                                    r2b = aoh->getMinorRadius();
                                    double startangle, endangle;
                                    aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoh->getMajorAxisDir();
                                    angle2 = atan2(majdir.y, majdir.x);
                                    angle2plus = (startangle + endangle)/2;
                                    midpos2 = aoh->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                                    const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(geo2);
                                    r2a = aop->getFocal();
                                    double startangle, endangle;
                                    aop->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = -aop->getXAxisDir();
                                    angle2 = atan2(majdir.y, majdir.x);
                                    angle2plus = (startangle + endangle)/2;
                                    midpos2 = aop->getFocus();
                                } else
                                    break;

                                if( geo1->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                                    geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                                    geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ){

                                    Base::Vector3d majDir, minDir, rvec;
                                    majDir = Base::Vector3d(cos(angle1),sin(angle1),0);//direction of major axis of ellipse
                                    minDir = Base::Vector3d(-majDir.y,majDir.x,0);//direction of minor axis of ellipse
                                    rvec = (r1a*cos(angle1plus)) * majDir   +   (r1b*sin(angle1plus)) * minDir;
                                    midpos1 += rvec;
                                    rvec.Normalize();
                                    norm1 = rvec;
                                    dir1 = Base::Vector3d(-rvec.y,rvec.x,0);//DeepSOIC: I'm not sure what dir is supposed to mean.
                                }
                                else {
                                    norm1 = Base::Vector3d(cos(angle1),sin(angle1),0);
                                    dir1 = Base::Vector3d(-norm1.y,norm1.x,0);
                                    midpos1 += r1a*norm1;
                                }


                                if( geo2->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                                    geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                                    geo2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {

                                    Base::Vector3d majDir, minDir, rvec;
                                    majDir = Base::Vector3d(cos(angle2),sin(angle2),0);//direction of major axis of ellipse
                                    minDir = Base::Vector3d(-majDir.y,majDir.x,0);//direction of minor axis of ellipse
                                    rvec = (r2a*cos(angle2plus)) * majDir   +   (r2b*sin(angle2plus)) * minDir;
                                    midpos2 += rvec;
                                    rvec.Normalize();
                                    norm2 = rvec;
                                    dir2 = Base::Vector3d(-rvec.y,rvec.x,0);
                                }
                                else {
                                    norm2 = Base::Vector3d(cos(angle2),sin(angle2),0);
                                    dir2 = Base::Vector3d(-norm2.y,norm2.x,0);
                                    midpos2 += r2a*norm2;
                                }

                            } else // Parallel can only apply to a GeomLineSegment
                                break;
                        } else {
                            const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment *>(geo1);
                            const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment *>(geo2);

                            // calculate the half distance between the start and endpoint
                            midpos1 = ((lineSeg1->getEndPoint()+lineSeg1->getStartPoint())/2);
                            midpos2 = ((lineSeg2->getEndPoint()+lineSeg2->getStartPoint())/2);
                            //Get a set of vectors perpendicular and tangential to these
                            dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint()).Normalize();
                            dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint()).Normalize();
                            norm1 = Base::Vector3d(-dir1.y,dir1.x,0.);
                            norm2 = Base::Vector3d(-dir2.y,dir2.x,0.);
                        }

                        Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 4.0, edit->constrGroup->getChild(i));
                        Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 4.0, edit->constrGroup->getChild(i));

                        auto translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                        translation->abPos = SbVec3f(midpos1.x, midpos1.y, drawingParameters.zConstr); //Absolute Reference

                        //Reference Position that is scaled according to zoom
                        translation->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                        Base::Vector3d secondPos = midpos2 - midpos1;

                        translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::SecondTranslationIndex)));

                        translation->abPos = SbVec3f(secondPos.x, secondPos.y, drawingParameters.zConstr); //Absolute Reference

                        //Reference Position that is scaled according to zoom
                        translation->translation = SbVec3f(relpos2.x - relpos1.x, relpos2.y -relpos1.y, 0);

                    }
                    break;
                case Distance:
                case DistanceX:
                case DistanceY:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                        Base::Vector3d pnt1(0.,0.,0.), pnt2(0.,0.,0.);
                        if (Constr->SecondPos != Sketcher::none) { // point to point distance
                            pnt1 = geolist.getPoint(Constr->First, Constr->FirstPos);
                            pnt2 = geolist.getPoint(Constr->Second, Constr->SecondPos);
                        } else if (Constr->Second != Constraint::GeoUndef) { // point to line distance
                            pnt1 = geolist.getPoint(Constr->First, Constr->FirstPos);

                            const Part::Geometry *geo = geolist.getGeometryFromGeoId(Constr->Second);
                            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);
                                Base::Vector3d l2p1 = lineSeg->getStartPoint();
                                Base::Vector3d l2p2 = lineSeg->getEndPoint();
                                // calculate the projection of p1 onto line2
                                pnt2.ProjectToLine(pnt1-l2p1, l2p2-l2p1);
                                pnt2 += pnt1;
                            } else
                                break;
                        } else if (Constr->FirstPos != Sketcher::none) {
                            pnt2 = geolist.getPoint(Constr->First, Constr->FirstPos);
                        } else if (Constr->First != Constraint::GeoUndef) {
                            const Part::Geometry *geo = geolist.getGeometryFromGeoId(Constr->First);
                            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);
                                pnt1 = lineSeg->getStartPoint();
                                pnt2 = lineSeg->getEndPoint();
                            } else
                                break;
                        } else
                            break;

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));

                        // Get presentation string (w/o units if option is set)
                        asciiText->string = SbString( getPresentationString(Constr).toUtf8().constData() );

                        if (Constr->Type == Distance)
                            asciiText->datumtype = SoDatumLabel::DISTANCE;
                        else if (Constr->Type == DistanceX)
                            asciiText->datumtype = SoDatumLabel::DISTANCEX;
                        else if (Constr->Type == DistanceY)
                             asciiText->datumtype = SoDatumLabel::DISTANCEY;

                        // Assign the Datum Points
                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = SbVec3f (pnt1.x, pnt1.y, drawingParameters.zConstr);
                        verts[1] = SbVec3f (pnt2.x, pnt2.y, drawingParameters.zConstr);

                        asciiText->pnts.finishEditing();

                        //Assign the Label Distance
                        asciiText->param1 = Constr->LabelDistance;
                        asciiText->param2 = Constr->LabelPosition;
                    }
                    break;
                case PointOnObject:
                case Tangent:
                case SnellsLaw:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);

                        Base::Vector3d pos, relPos;
                        if (  Constr->Type == PointOnObject ||
                              Constr->Type == SnellsLaw ||
                              (Constr->Type == Tangent && Constr->Third != Constraint::GeoUndef) || //Tangency via point
                              (Constr->Type == Tangent && Constr->FirstPos != Sketcher::none) //endpoint-to-curve or endpoint-to-endpoint tangency
                                ) {

                            //find the point of tangency/point that is on object
                            //just any point among first/second/third should be OK
                            int ptGeoId;
                            Sketcher::PointPos ptPosId;
                            do {//dummy loop to use break =) Maybe goto?
                                ptGeoId = Constr->First;
                                ptPosId = Constr->FirstPos;
                                if (ptPosId != Sketcher::none) break;
                                ptGeoId = Constr->Second;
                                ptPosId = Constr->SecondPos;
                                if (ptPosId != Sketcher::none) break;
                                ptGeoId = Constr->Third;
                                ptPosId = Constr->ThirdPos;
                                if (ptPosId != Sketcher::none) break;
                                assert(0);//no point found!
                            } while (false);

                            pos = geolist.getPoint(ptGeoId, ptPosId);
                            auto norm = getNormal(geolist, Constr->Second, pos);

                            // TODO: Check substitution
                            // Base::Vector3d norm = getSolvedSketch().calculateNormalAtPoint(Constr->Second, pos.x, pos.y);
                            norm.Normalize();
                            Base::Vector3d dir = norm; dir.RotateZ(-M_PI/2.0);

                            relPos = seekConstraintPosition(pos, norm, dir, 2.5, edit->constrGroup->getChild(i));

                            auto translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                            translation->abPos = SbVec3f(pos.x, pos.y, drawingParameters.zConstr); //Absolute Reference
                            translation->translation = SbVec3f(relPos.x, relPos.y, 0);
                        }
                        else if (Constr->Type == Tangent) {
                            // get the geometry
                            const Part::Geometry *geo1 = geolist.getGeometryFromGeoId(Constr->First);
                            const Part::Geometry *geo2 = geolist.getGeometryFromGeoId(Constr->Second);

                            if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                                geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment *>(geo1);
                                const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment *>(geo2);
                                // tangency between two lines
                                Base::Vector3d midpos1 = ((lineSeg1->getEndPoint()+lineSeg1->getStartPoint())/2);
                                Base::Vector3d midpos2 = ((lineSeg2->getEndPoint()+lineSeg2->getStartPoint())/2);
                                Base::Vector3d dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint()).Normalize();
                                Base::Vector3d dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint()).Normalize();
                                Base::Vector3d norm1 = Base::Vector3d(-dir1.y,dir1.x,0.f);
                                Base::Vector3d norm2 = Base::Vector3d(-dir2.y,dir2.x,0.f);

                                Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 4.0, edit->constrGroup->getChild(i));
                                Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 4.0, edit->constrGroup->getChild(i));

                                auto translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                                translation->abPos = SbVec3f(midpos1.x, midpos1.y, drawingParameters.zConstr); //Absolute Reference
                                translation->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                                Base::Vector3d secondPos = midpos2 - midpos1;

                                translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::SecondTranslationIndex)));

                                translation->abPos = SbVec3f(secondPos.x, secondPos.y, drawingParameters.zConstr); //Absolute Reference
                                translation->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);

                                break;
                            }
                            else if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                std::swap(geo1,geo2);
                            }

                            if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo1);
                                Base::Vector3d dir = (lineSeg->getEndPoint() - lineSeg->getStartPoint()).Normalize();
                                Base::Vector3d norm(-dir.y, dir.x, 0);
                                if (geo2->getTypeId()== Part::GeomCircle::getClassTypeId()) {
                                    const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo2);
                                    // tangency between a line and a circle
                                    float length = (circle->getCenter() - lineSeg->getStartPoint())*dir;

                                    pos = lineSeg->getStartPoint() + dir * length;
                                    relPos = norm * 1;  //TODO Huh?
                                }
                                else if (geo2->getTypeId()== Part::GeomEllipse::getClassTypeId() ||
                                         geo2->getTypeId()== Part::GeomArcOfEllipse::getClassTypeId()) {

                                    Base::Vector3d center;
                                    if(geo2->getTypeId()== Part::GeomEllipse::getClassTypeId()){
                                        const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo2);
                                        center=ellipse->getCenter();
                                    } else {
                                        const Part::GeomArcOfEllipse *aoc = static_cast<const Part::GeomArcOfEllipse *>(geo2);
                                        center=aoc->getCenter();
                                    }

                                    // tangency between a line and an ellipse
                                    float length = (center - lineSeg->getStartPoint())*dir;

                                    pos = lineSeg->getStartPoint() + dir * length;
                                    relPos = norm * 1;
                                }
                                else if (geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                                    const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                    // tangency between a line and an arc
                                    float length = (arc->getCenter() - lineSeg->getStartPoint())*dir;

                                    pos = lineSeg->getStartPoint() + dir * length;
                                    relPos = norm * 1;  //TODO Huh?
                                }
                            }

                            if (geo1->getTypeId()== Part::GeomCircle::getClassTypeId() &&
                                geo2->getTypeId()== Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle1 = static_cast<const Part::GeomCircle *>(geo1);
                                const Part::GeomCircle *circle2 = static_cast<const Part::GeomCircle *>(geo2);
                                // tangency between two cicles
                                Base::Vector3d dir = (circle2->getCenter() - circle1->getCenter()).Normalize();
                                pos =  circle1->getCenter() + dir *  circle1->getRadius();
                                relPos = dir * 1;
                            }
                            else if (geo2->getTypeId()== Part::GeomCircle::getClassTypeId()) {
                                std::swap(geo1,geo2);
                            }

                            if (geo1->getTypeId()== Part::GeomCircle::getClassTypeId() &&
                                geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo1);
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                // tangency between a circle and an arc
                                Base::Vector3d dir = (arc->getCenter() - circle->getCenter()).Normalize();
                                pos =  circle->getCenter() + dir *  circle->getRadius();
                                relPos = dir * 1;
                            }
                            else if (geo1->getTypeId()== Part::GeomArcOfCircle::getClassTypeId() &&
                                     geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc1 = static_cast<const Part::GeomArcOfCircle *>(geo1);
                                const Part::GeomArcOfCircle *arc2 = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                // tangency between two arcs
                                Base::Vector3d dir = (arc2->getCenter() - arc1->getCenter()).Normalize();
                                pos =  arc1->getCenter() + dir *  arc1->getRadius();
                                relPos = dir * 1;
                            }
                            auto translation = static_cast<SoZoomTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                            translation->abPos = SbVec3f(pos.x, pos.y, drawingParameters.zConstr); //Absolute Reference
                            translation->translation = SbVec3f(relPos.x, relPos.y, 0);
                        }
                    }
                    break;
                case Symmetric:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);

                        Base::Vector3d pnt1 = geolist.getPoint(Constr->First, Constr->FirstPos);
                        Base::Vector3d pnt2 = geolist.getPoint(Constr->Second, Constr->SecondPos);

                        SbVec3f p1(pnt1.x, pnt1.y, drawingParameters.zConstr);
                        SbVec3f p2(pnt2.x, pnt2.y, drawingParameters.zConstr);
                        SbVec3f dir = (p2-p1);
                        dir.normalize();
                        SbVec3f norm (-dir[1],dir[0],0);

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));
                        asciiText->datumtype    = SoDatumLabel::SYMMETRIC;

                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = p1;
                        verts[1] = p2;

                        asciiText->pnts.finishEditing();

                        auto translation = static_cast<SoTranslation *>(sep->getChild(static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                        translation->translation = (p1 + p2)/2;
                    }
                    break;
                case Angle:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert((Constr->Second >= -extGeoCount && Constr->Second < intGeoCount) ||
                               Constr->Second == Constraint::GeoUndef);

                        SbVec3f p0;
                        double startangle,range,endangle;
                        if (Constr->Second != Constraint::GeoUndef) {
                            Base::Vector3d dir1, dir2;
                            if(Constr->Third == Constraint::GeoUndef) { //angle between two lines
                                const Part::Geometry *geo1 = geolist.getGeometryFromGeoId(Constr->First);
                                const Part::Geometry *geo2 = geolist.getGeometryFromGeoId(Constr->Second);
                                if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                                    geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                                    break;
                                const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment *>(geo1);
                                const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment *>(geo2);

                                bool flip1 = (Constr->FirstPos == end);
                                bool flip2 = (Constr->SecondPos == end);
                                dir1 = (flip1 ? -1. : 1.) * (lineSeg1->getEndPoint()-lineSeg1->getStartPoint());
                                dir2 = (flip2 ? -1. : 1.) * (lineSeg2->getEndPoint()-lineSeg2->getStartPoint());
                                Base::Vector3d pnt1 = flip1 ? lineSeg1->getEndPoint() : lineSeg1->getStartPoint();
                                Base::Vector3d pnt2 = flip2 ? lineSeg2->getEndPoint() : lineSeg2->getStartPoint();

                                // line-line intersection
                                {
                                    double det = dir1.x*dir2.y - dir1.y*dir2.x;
                                    if ((det > 0 ? det : -det) < 1e-10) {
                                        // lines are coincident (or parallel) and in this case the center
                                        // of the point pairs with the shortest distance is used
                                        Base::Vector3d p1[2], p2[2];
                                        p1[0] = lineSeg1->getStartPoint();
                                        p1[1] = lineSeg1->getEndPoint();
                                        p2[0] = lineSeg2->getStartPoint();
                                        p2[1] = lineSeg2->getEndPoint();
                                        double length = DBL_MAX;
                                        for (int i=0; i <= 1; i++) {
                                            for (int j=0; j <= 1; j++) {
                                                double tmp = (p2[j]-p1[i]).Length();
                                                if (tmp < length) {
                                                    length = tmp;
                                                    p0.setValue((p2[j].x+p1[i].x)/2,(p2[j].y+p1[i].y)/2,0);
                                                }
                                            }
                                        }
                                    }
                                    else {
                                        double c1 = dir1.y*pnt1.x - dir1.x*pnt1.y;
                                        double c2 = dir2.y*pnt2.x - dir2.x*pnt2.y;
                                        double x = (dir1.x*c2 - dir2.x*c1)/det;
                                        double y = (dir1.y*c2 - dir2.y*c1)/det;
                                        p0 = SbVec3f(x,y,0);
                                    }
                                }

                                range = Constr->getValue(); // WYSIWYG
                                startangle = atan2(dir1.y,dir1.x);
                            }
                            else {//angle-via-point
                                Base::Vector3d p = geolist.getPoint(Constr->Third, Constr->ThirdPos);
                                p0 = SbVec3f(p.x, p.y, 0);
                                dir1 = getNormal(geolist, Constr->First, p);
                                // TODO: Check
                                // dir1 = getSolvedSketch().calculateNormalAtPoint(Constr->First, p.x, p.y);
                                dir1.RotateZ(-M_PI/2);//convert to vector of tangency by rotating
                                dir2 = getNormal(geolist, Constr->Second, p);
                                // TODO: Check
                                // dir2 = getSolvedSketch().calculateNormalAtPoint(Constr->Second, p.x, p.y);
                                dir2.RotateZ(-M_PI/2);

                                startangle = atan2(dir1.y,dir1.x);
                                range = atan2(dir1.x*dir2.y-dir1.y*dir2.x,
                                          dir1.x*dir2.x+dir1.y*dir2.y);
                            }

                            endangle = startangle + range;

                        } else if (Constr->First != Constraint::GeoUndef) {
                            const Part::Geometry *geo = geolist.getGeometryFromGeoId(Constr->First);
                            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);
                                p0 = Base::convertTo<SbVec3f>((lineSeg->getEndPoint()+lineSeg->getStartPoint())/2);

                                Base::Vector3d dir = lineSeg->getEndPoint()-lineSeg->getStartPoint();
                                startangle = 0.;
                                range = atan2(dir.y,dir.x);
                                endangle = startangle + range;
                            }
                            else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                                p0 = Base::convertTo<SbVec3f>(arc->getCenter());

                                arc->getRange(startangle, endangle,/*emulateCCWXY=*/true);
                                range = endangle - startangle;
                            }
                            else {
                                break;
                            }
                        } else
                            break;

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));
                        asciiText->string    = SbString(Constr->getPresentationValue().getUserString().toUtf8().constData());
                        asciiText->datumtype = SoDatumLabel::ANGLE;
                        asciiText->param1    = Constr->LabelDistance;
                        asciiText->param2    = startangle;
                        asciiText->param3    = range;

                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = p0;

                        asciiText->pnts.finishEditing();

                    }
                    break;
                case Diameter:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                        Base::Vector3d pnt1(0.,0.,0.), pnt2(0.,0.,0.);
                        if (Constr->First != Constraint::GeoUndef) {
                            const Part::Geometry *geo = geolist.getGeometryFromGeoId(Constr->First);

                            if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                                double radius = arc->getRadius();
                                double angle = (double) Constr->LabelPosition;
                                if (angle == 10) {
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle = (startangle + endangle)/2;
                                }
                                Base::Vector3d center = arc->getCenter();
                                pnt1 = center - radius * Base::Vector3d(cos(angle),sin(angle),0.);
                                pnt2 = center + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                            }
                            else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo);
                                double radius = circle->getRadius();
                                double angle = (double) Constr->LabelPosition;
                                if (angle == 10) {
                                    angle = 0;
                                }
                                Base::Vector3d center = circle->getCenter();
                                pnt1 = center - radius * Base::Vector3d(cos(angle),sin(angle),0.);
                                pnt2 = center + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                            }
                            else
                                break;
                        } else
                            break;

                        SbVec3f p1(pnt1.x, pnt1.y, drawingParameters.zConstr);
                        SbVec3f p2(pnt2.x, pnt2.y, drawingParameters.zConstr);

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));

                        // Get display string with units hidden if so requested
                        asciiText->string = SbString( getPresentationString(Constr).toUtf8().constData() );

                        asciiText->datumtype    = SoDatumLabel::DIAMETER;
                        asciiText->param1       = Constr->LabelDistance;
                        asciiText->param2       = Constr->LabelPosition;

                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = p1;
                        verts[1] = p2;

                        asciiText->pnts.finishEditing();
                    }
                    break;
                    case Weight:
                    case Radius:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                        Base::Vector3d pnt1(0.,0.,0.), pnt2(0.,0.,0.);

                        if (Constr->First != Constraint::GeoUndef) {
                            const Part::Geometry *geo = geolist.getGeometryFromGeoId(Constr->First);

                            if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                                double radius = arc->getRadius();
                                double angle = (double) Constr->LabelPosition;
                                if (angle == 10) {
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle = (startangle + endangle)/2;
                                }
                                pnt1 = arc->getCenter();
                                pnt2 = pnt1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                            }
                            else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo);
                                auto gf = GeometryFacade::getFacade(geo);

                                double radius;

                                radius = circle->getRadius();

                                double angle = (double) Constr->LabelPosition;
                                if (angle == 10) {
                                    angle = 0;
                                }
                                pnt1 = circle->getCenter();
                                pnt2 = pnt1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                            }
                            else
                                break;
                        } else
                            break;

                        SbVec3f p1(pnt1.x, pnt1.y, drawingParameters.zConstr);
                        SbVec3f p2(pnt2.x, pnt2.y, drawingParameters.zConstr);

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));

                        // Get display string with units hidden if so requested
                        if(Constr->Type == Weight)
                            asciiText->string = SbString( QString::number(Constr->getValue()).toStdString().c_str());
                        else
                            asciiText->string = SbString( getPresentationString(Constr).toUtf8().constData() );

                        asciiText->datumtype    = SoDatumLabel::RADIUS;
                        asciiText->param1       = Constr->LabelDistance;
                        asciiText->param2       = Constr->LabelPosition;

                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = p1;
                        verts[1] = p2;

                        asciiText->pnts.finishEditing();
                    }
                    break;
                case Coincident: // nothing to do for coincident
                case None:
                case InternalAlignment:
                case NumConstraintTypes:
                    break;
            }

        } catch (Base::Exception &e) {
            Base::Console().Error("Exception during draw: %s\n", e.what());
        } catch (...){
            Base::Console().Error("Exception during draw: unknown\n");
        }

    }
}

Base::Vector3d CoinManager::seekConstraintPosition(const Base::Vector3d &origPos,
                                                          const Base::Vector3d &norm,
                                                          const Base::Vector3d &dir, float step,
                                                          const SoNode *constraint)
{

    auto rp = ViewProviderSketchCoinAttorney::getRayPickAction(viewProvider);

    float scaled_step = step * ViewProviderSketchCoinAttorney::getScaleFactor(viewProvider);

    int multiplier = 0;
    Base::Vector3d relPos, freePos;
    bool isConstraintAtPosition = true;
    while (isConstraintAtPosition && multiplier < 10) {
        // Calculate new position of constraint
        relPos = norm * 0.5f + dir * multiplier;
        freePos = origPos + relPos * scaled_step;

        rp->setRadius(0.1f);
        rp->setPickAll(true);
        rp->setRay(SbVec3f(freePos.x, freePos.y, -1.f), SbVec3f(0, 0, 1) );
        //problem
        rp->apply(edit->constrGroup); // We could narrow it down to just the SoGroup containing the constraints

        // returns a copy of the point
        SoPickedPoint *pp = rp->getPickedPoint();
        const SoPickedPointList ppl = rp->getPickedPointList();

        if (ppl.getLength() <= 1 && pp) {
            SoPath *path = pp->getPath();
            int length = path->getLength();
            SoNode *tailFather1 = path->getNode(length-2);
            SoNode *tailFather2 = path->getNode(length-3);

            // checking if a constraint is the same as the one selected
            if (tailFather1 == constraint || tailFather2 == constraint)
                isConstraintAtPosition = false;
        }
        else {
            isConstraintAtPosition = false;
        }

        multiplier *= -1; // search in both sides
        if (multiplier >= 0)
            multiplier++; // Increment the multiplier
    }
    if (multiplier == 10)
        relPos = norm * 0.5f; // no free position found
    return relPos * step;
}


void CoinManager::processGeometryConstraintsInformationOverlay(const GeoList & geolist, bool rebuildinformationlayer)
{
    drawingParameters.coinFontSize = edit->coinFontSize; // TODO: Evaluate refactoring this after constraints are migrated.
    overlayParameters.rebuildInformationLayer = rebuildinformationlayer;

    processGeometry(geolist);

    updateOverlayParameters();

    processGeometryInformationOverlay(geolist);

    updateAxesLength();

    updateGridExtent();

    processConstraints(geolist);
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

void CoinManager::setPreselectPoint(int PreselectPoint)
{
    if (edit) {
        int oldPtId = -1;
        if (edit->PreselectPoint != -1)
            oldPtId = edit->PreselectPoint + 1;
        else if (edit->PreselectCross == 0)
            oldPtId = 0;
        int newPtId = PreselectPoint + 1;
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        float x,y,z;
        if (oldPtId != -1 &&
            edit->SelPointSet.find(oldPtId) == edit->SelPointSet.end()) {
            // send to background
            pverts[oldPtId].getValue(x,y,z);
            pverts[oldPtId].setValue(x,y,drawingParameters.zLowPoints);
        }
        // bring to foreground
        pverts[newPtId].getValue(x,y,z);
        pverts[newPtId].setValue(x,y,drawingParameters.zHighlight);
        edit->PreselectPoint = PreselectPoint;
        edit->PointsCoordinate->point.finishEditing();
    }
}

void CoinManager::resetPreselectPoint(void)
{
    if (edit) {
        int oldPtId = -1;
        if (edit->PreselectPoint != -1)
            oldPtId = edit->PreselectPoint + 1;
        else if (edit->PreselectCross == 0)
            oldPtId = 0;
        if (oldPtId != -1 &&
            edit->SelPointSet.find(oldPtId) == edit->SelPointSet.end()) {
            // send to background
            SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
            float x,y,z;
            pverts[oldPtId].getValue(x,y,z);
            pverts[oldPtId].setValue(x,y,drawingParameters.zLowPoints);
            edit->PointsCoordinate->point.finishEditing();
        }
        edit->PreselectPoint = -1;
    }
}

void CoinManager::addSelectPoint(int SelectPoint)
{
    if (edit) {
        int PtId = SelectPoint + 1;
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        // bring to foreground
        float x,y,z;
        pverts[PtId].getValue(x,y,z);
        pverts[PtId].setValue(x,y,drawingParameters.zHighlight);
        edit->SelPointSet.insert(PtId);
        edit->PointsCoordinate->point.finishEditing();
    }
}

void CoinManager::removeSelectPoint(int SelectPoint)
{
    if (edit) {
        int PtId = SelectPoint + 1;
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        // send to background
        float x,y,z;
        pverts[PtId].getValue(x,y,z);
        pverts[PtId].setValue(x,y,drawingParameters.zLowPoints);
        edit->SelPointSet.erase(PtId);
        edit->PointsCoordinate->point.finishEditing();
    }
}

void CoinManager::clearSelectPoints(void)
{
    if (edit) {
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        // send to background
        float x,y,z;
        for (std::set<int>::const_iterator it=edit->SelPointSet.begin();
             it != edit->SelPointSet.end(); ++it) {
            pverts[*it].getValue(x,y,z);
            pverts[*it].setValue(x,y,drawingParameters.zLowPoints);
        }
        edit->PointsCoordinate->point.finishEditing();
        edit->SelPointSet.clear();
    }
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

void CoinManager::updateGeometryColor(const GeoListFacade & geolistfacade, bool issketchinvalid)
{
    // Lambdas for convenience retrieval of geometry information
    auto isConstructionGeom = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFromGeoId(GeoId);
        if (geom)
            return geom->getConstruction();
        return false;
    };

    auto isDefinedGeomPoint = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFromGeoId(GeoId);
        if (geom)
            return geom->isGeoType(Part::GeomPoint::getClassTypeId()) && !geom->getConstruction();
        return false;
    };

    auto isInternalAlignedGeom = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFromGeoId(GeoId);
        if (geom) {
            return geom->isInternalAligned();
        }
        return false;
    };

    auto isFullyConstraintElement = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFromGeoId(GeoId);

        if(geom) {
            if(geom->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {

                auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                                    geom->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

                return (solvext->getGeometry() == Sketcher::SolverGeometryExtension::FullyConstraint);
            }
        }
        return false;
    };

    // Update Colors

    int PtNum = edit->PointsMaterials->diffuseColor.getNum();
    SbColor *pcolor = edit->PointsMaterials->diffuseColor.startEditing();
    int CurvNum = edit->CurvesMaterials->diffuseColor.getNum();
    SbColor *color = edit->CurvesMaterials->diffuseColor.startEditing();
    SbColor *crosscolor = edit->RootCrossMaterials->diffuseColor.startEditing();

    SbVec3f *verts = edit->CurvesCoordinate->point.startEditing();
    SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();

    float x,y,z;

    // colors of the point set
    if( issketchinvalid ) {
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = drawingParameters.InvalidSketchColor;
    }
    else if (edit->FullyConstrained) {
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = drawingParameters.FullyConstrainedColor;
    }
    else {
        for (int  i=0; i < PtNum; i++) {
            int GeoId = edit->PointIdToGeoId[i];

            bool constrainedElement = isFullyConstraintElement(GeoId);

            if(isInternalAlignedGeom(GeoId)) {
                if(constrainedElement)
                    pcolor[i] = drawingParameters.FullyConstraintInternalAlignmentColor;
                else
                    pcolor[i] = drawingParameters.InternalAlignedGeoColor;
            }
            else {
                if(!isDefinedGeomPoint(GeoId)) {

                    if(constrainedElement)
                        pcolor[i] = drawingParameters.FullyConstraintConstructionPointColor;
                    else
                        pcolor[i] = drawingParameters.VertexColor;
                }
                else { // this is a defined GeomPoint
                    if(constrainedElement)
                        pcolor[i] = drawingParameters.FullyConstraintElementColor;
                    else
                        pcolor[i] = drawingParameters.CurveColor;
                }
            }
        }
    }

    // update rendering height of points

    auto getRenderHeight = [this](DrawingParameters::GeometryRendering renderingtype, float toprendering, float midrendering, float lowrendering) {
        if(drawingParameters.topRenderingGeometry == renderingtype)
            return toprendering;
        else if(drawingParameters.midRenderingGeometry == renderingtype)
            return midrendering;
        else
            return lowrendering;
    };

    float zNormPoint = getRenderHeight(DrawingParameters::GeometryRendering::NormalGeometry,
                                       drawingParameters.zHighPoints,
                                       drawingParameters.zLowPoints,
                                       drawingParameters.zLowPoints);

    float zConstrPoint = getRenderHeight(DrawingParameters::GeometryRendering::Construction,
                                       drawingParameters.zHighPoints,
                                       drawingParameters.zLowPoints,
                                       drawingParameters.zLowPoints);


    for (int  i=0; i < PtNum; i++) { // 0 is the origin
        pverts[i].getValue(x,y,z);
        auto geom = geolistfacade.getGeometryFromGeoId(edit->PointIdToGeoId[i]);
        if(geom && z < drawingParameters.zHighlight) {
            if(geom->getConstruction())
                pverts[i].setValue(x,y,zConstrPoint);
            else
                pverts[i].setValue(x,y,zNormPoint);
        }
    }

    if (edit->PreselectCross == 0) {
        pcolor[0] = drawingParameters.PreselectColor;
    }
    else if (edit->PreselectPoint != -1) {
        if (edit->PreselectPoint + 1 < PtNum)
            pcolor[edit->PreselectPoint + 1] = drawingParameters.PreselectColor;
    }

    for (std::set<int>::iterator it = edit->SelPointSet.begin(); it != edit->SelPointSet.end(); ++it) {
        if (*it < PtNum) {
            pcolor[*it] = (*it==(edit->PreselectPoint + 1) && (edit->PreselectPoint != -1))
                ? drawingParameters.PreselectSelectedColor : drawingParameters.SelectColor;
        }
    }

    // update colors and rendering height of the curves

    float zNormLine = getRenderHeight(DrawingParameters::GeometryRendering::NormalGeometry,
                                       drawingParameters.zHighLines,
                                       drawingParameters.zMidLines,
                                       drawingParameters.zLowLines);

    float zConstrLine = getRenderHeight(DrawingParameters::GeometryRendering::Construction,
                                       drawingParameters.zHighLines,
                                       drawingParameters.zMidLines,
                                       drawingParameters.zLowLines);

    float zExtLine = getRenderHeight(DrawingParameters::GeometryRendering::ExternalGeometry,
                                       drawingParameters.zHighLines,
                                       drawingParameters.zMidLines,
                                       drawingParameters.zLowLines);

    int j=0; // vertexindex

    for (int  i=0; i < CurvNum; i++) {
        int GeoId = edit->CurvIdToGeoId[i];
        // CurvId has several vertices associated to 1 material
        //edit->CurveSet->numVertices => [i] indicates number of vertex for line i.
        int indexes = (edit->CurveSet->numVertices[i]);

        bool selected = (edit->SelCurvSet.find(GeoId) != edit->SelCurvSet.end());
        bool preselected = (edit->PreselectCurve == GeoId);

        bool constrainedElement = isFullyConstraintElement(GeoId);

        if (selected && preselected) {
            color[i] = drawingParameters.PreselectSelectedColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,drawingParameters.zHighLine);
            }
        }
        else if (selected){
            color[i] = drawingParameters.SelectColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,drawingParameters.zHighLine);
            }
        }
        else if (preselected){
            color[i] = drawingParameters.PreselectColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,drawingParameters.zHighLine);
            }
        }
        else if (GeoId <= Sketcher::GeoEnum::RefExt) {  // external Geometry
            color[i] = drawingParameters.CurveExternalColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zExtLine);
            }
        }
        else if ( issketchinvalid ) {
            color[i] = drawingParameters.InvalidSketchColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zNormLine);
            }
        }
        else if (isConstructionGeom(GeoId)) {
            if(isInternalAlignedGeom(GeoId)) {
                if(constrainedElement)
                    color[i] = drawingParameters.FullyConstraintInternalAlignmentColor;
                else
                    color[i] = drawingParameters.InternalAlignedGeoColor;
            }
            else {
                if(constrainedElement)
                    color[i] = drawingParameters.FullyConstraintConstructionElementColor;
                else
                    color[i] = drawingParameters.CurveDraftColor;
            }

            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zConstrLine);
            }
        }
        else if (edit->FullyConstrained) {
            color[i] = drawingParameters.FullyConstrainedColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zNormLine);
            }
        }
        else if (isFullyConstraintElement(GeoId)) {
            color[i] = drawingParameters.FullyConstraintElementColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zNormLine);
            }
        }
        else {
            color[i] = drawingParameters.CurveColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zNormLine);
            }
        }
    }

    // colors of the cross
    if (edit->SelCurvSet.find(-1) != edit->SelCurvSet.end())
        crosscolor[0] = drawingParameters.SelectColor;
    else if (edit->PreselectCross == 1)
        crosscolor[0] = drawingParameters.PreselectColor;
    else
        crosscolor[0] = drawingParameters.CrossColorH;

    if (edit->SelCurvSet.find(Sketcher::GeoEnum::VAxis) != edit->SelCurvSet.end())
        crosscolor[1] = drawingParameters.SelectColor;
    else if (edit->PreselectCross == 2)
        crosscolor[1] = drawingParameters.PreselectColor;
    else
        crosscolor[1] = drawingParameters.CrossColorV;

    // end editing
    edit->CurvesMaterials->diffuseColor.finishEditing();
    edit->PointsMaterials->diffuseColor.finishEditing();
    edit->RootCrossMaterials->diffuseColor.finishEditing();
    edit->CurvesCoordinate->point.finishEditing();
    edit->CurveSet->numVertices.finishEditing();
}

void CoinManager::updateConstraintColor(std::vector<Sketcher::Constraint *> constraints)
{
    // Because coincident constraints are selected using the point color, we need to edit the point materials.
    // TODO: Review this
    int PtNum = edit->PointsMaterials->diffuseColor.getNum();
    SbColor *pcolor = edit->PointsMaterials->diffuseColor.startEditing();

    int maxNumberOfConstraints = std::min(edit->constrGroup->getNumChildren(), static_cast<int>(constraints.size()));

    // colors of the constraints
    for (int i = 0; i < maxNumberOfConstraints; i++) {
        SoSeparator *s = static_cast<SoSeparator *>(edit->constrGroup->getChild(i));

        // Check Constraint Type
        Sketcher::Constraint* constraint = constraints[i];
        ConstraintType type = constraint->Type;
        bool hasDatumLabel  = (type == Sketcher::Angle ||
                               type == Sketcher::Radius ||
                               type == Sketcher::Diameter ||
                               type == Sketcher::Weight ||
                               type == Sketcher::Symmetric ||
                               type == Sketcher::Distance ||
                               type == Sketcher::DistanceX ||
                               type == Sketcher::DistanceY);

        // Non DatumLabel Nodes will have a material excluding coincident
        bool hasMaterial = false;

        SoMaterial *m = 0;
        if (!hasDatumLabel && type != Sketcher::Coincident && type != Sketcher::InternalAlignment) {
            hasMaterial = true;
            m = static_cast<SoMaterial *>(s->getChild(static_cast<int>(ConstraintNodePosition::MaterialIndex)));
        }

        auto selectpoint = [this, pcolor, PtNum](int geoid, Sketcher::PointPos pos){
            if(geoid >= 0) {
                auto indexit = edit->GeoIdPointPosToPointId.find(std::make_pair(geoid, pos));

                if (indexit != edit->GeoIdPointPosToPointId.end()) {
                    int index = indexit->second + 1;
                    if(index >= 0 && index < PtNum) {
                        pcolor[index] = drawingParameters.SelectColor;
                    }
                }
            }
        };

        if (edit->SelConstraintSet.find(i) != edit->SelConstraintSet.end()) {
            if (hasDatumLabel) {
                SoDatumLabel *l = static_cast<SoDatumLabel *>(s->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));
                l->textColor = drawingParameters.SelectColor;
            } else if (hasMaterial) {
                m->diffuseColor = drawingParameters.SelectColor;
            } else if (type == Sketcher::Coincident) {

                auto selectpoint = [this, pcolor, PtNum](int geoid, Sketcher::PointPos pos){
                    if(geoid >= 0) {
                        auto indexit = edit->GeoIdPointPosToPointId.find(std::make_pair(geoid, pos));

                        if (indexit != edit->GeoIdPointPosToPointId.end()) {
                            int index = indexit->second + 1;
                            if(index >= 0 && index < PtNum) {
                                pcolor[index] = drawingParameters.SelectColor;
                            }
                        }
                    }
                };

                selectpoint(constraint->First, constraint->FirstPos);
                selectpoint(constraint->Second, constraint->SecondPos);
            } else if (type == Sketcher::InternalAlignment) {
                switch(constraint->AlignmentType) {
                    case EllipseMajorDiameter:
                    case EllipseMinorDiameter:
                    {
                        // color line
                        int CurvNum = edit->CurvesMaterials->diffuseColor.getNum();
                        for (int  i=0; i < CurvNum; i++) {
                            int cGeoId = edit->CurvIdToGeoId[i];

                            if(cGeoId == constraint->First) {
                                pcolor[i] = drawingParameters.SelectColor;
                                break;
                            }
                        }
                    }
                    break;
                    case EllipseFocus1:
                    case EllipseFocus2:
                    {
                        selectpoint(constraint->First, constraint->FirstPos);
                    }
                    break;
                    default:
                    break;
                }
            }
        } else if (edit->PreselectConstraintSet.count(i)) {
            if (hasDatumLabel) {
                SoDatumLabel *l = static_cast<SoDatumLabel *>(s->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));
                l->textColor = drawingParameters.PreselectColor;
            } else if (hasMaterial) {
                m->diffuseColor = drawingParameters.PreselectColor;
            }
        }
        else {
            if (hasDatumLabel) {
                SoDatumLabel *l = static_cast<SoDatumLabel *>(s->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));

                l->textColor = constraint->isActive ?
                                    ViewProviderSketchCoinAttorney::constraintHasExpression(viewProvider, i) ?
                                        drawingParameters.ExprBasedConstrDimColor
                                        :(constraint->isDriving ?
                                            drawingParameters.ConstrDimColor
                                            : drawingParameters.NonDrivingConstrDimColor)
                                    :drawingParameters.DeactivatedConstrDimColor;

            } else if (hasMaterial) {
                m->diffuseColor = constraint->isActive ?
                                    (constraint->isDriving ?
                                        drawingParameters.ConstrDimColor
                                        :drawingParameters.NonDrivingConstrDimColor)
                                    :drawingParameters.DeactivatedConstrDimColor;
            }
        }
    }

    edit->PointsMaterials->diffuseColor.finishEditing();

}

void CoinManager::rebuildConstraintNodes(void)
{
    const std::vector<Sketcher::Constraint *> &constrlist = ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    auto geolist = ViewProviderSketchCoinAttorney::getGeoList(viewProvider);

    // clean up
    Gui::coinRemoveAllChildren(edit->constrGroup);

    edit->vConstrType.clear();

    // Get sketch normal
    Base::Vector3d RN(0,0,1);

    // move to position of Sketch
    Base::Placement Plz = ViewProviderSketchCoinAttorney::getEditingPlacement(viewProvider);
    Base::Rotation tmp(Plz.getRotation());
    tmp.multVec(RN,RN);
    Plz.setRotation(tmp);

    SbVec3f norm(RN.x, RN.y, RN.z);

    rebuildConstraintNodes(geolist, constrlist, norm);
}

void CoinManager::rebuildConstraintNodes(const GeoList & geolist, const std::vector<Sketcher::Constraint *> constrlist, SbVec3f norm)
{

    for (std::vector<Sketcher::Constraint *>::const_iterator it=constrlist.begin(); it != constrlist.end(); ++it) {
        // root separator for one constraint
        SoSeparator *sep = new SoSeparator();
        sep->ref();
        // no caching for frequently-changing data structures
        sep->renderCaching = SoSeparator::OFF;

        // every constrained visual node gets its own material for preselection and selection
        SoMaterial *mat = new SoMaterial;
        mat->ref();
        mat->diffuseColor = (*it)->isActive ?
                                ((*it)->isDriving ?
                                    drawingParameters.ConstrDimColor
                                    :drawingParameters.NonDrivingConstrDimColor)
                                :drawingParameters.DeactivatedConstrDimColor;


        // distinguish different constraint types to build up
        switch ((*it)->Type) {
            case Distance:
            case DistanceX:
            case DistanceY:
            case Radius:
            case Diameter:
            case Weight:
            case Angle:
            {
                SoDatumLabel *text = new SoDatumLabel();
                text->norm.setValue(norm);
                text->string = "";
                text->textColor = (*it)->isActive ?
                                        ((*it)->isDriving ?
                                            drawingParameters.ConstrDimColor
                                            :drawingParameters.NonDrivingConstrDimColor)
                                        :drawingParameters.DeactivatedConstrDimColor;
                text->size.setValue(edit->coinFontSize);
                text->lineWidth = 2 * edit->pixelScalingFactor;
                text->useAntialiasing = false;
                SoAnnotation *anno = new SoAnnotation();
                anno->renderCaching = SoSeparator::OFF;
                anno->addChild(text);
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(text);
                edit->constrGroup->addChild(anno);
                edit->vConstrType.push_back((*it)->Type);
                // nodes not needed
                sep->unref();
                mat->unref();
                continue; // jump to next constraint
            }
            break;
            case Horizontal:
            case Vertical:
            case Block:
            {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6
                sep->addChild(new SoInfo());

                // remember the type of this constraint node
                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            case Coincident: // no visual for coincident so far
                edit->vConstrType.push_back(Coincident);
                break;
            case Parallel:
            case Perpendicular:
            case Equal:
            {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6
                sep->addChild(new SoInfo());

                // remember the type of this constraint node
                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            case PointOnObject:
            case Tangent:
            case SnellsLaw:
            {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());

                if ((*it)->Type == Tangent) {
                    const Part::Geometry *geo1 = geolist.getGeometryFromGeoId((*it)->First);
                    const Part::Geometry *geo2 = geolist.getGeometryFromGeoId((*it)->Second);
                    if (!geo1 || !geo2) {
                        Base::Console().Warning("Tangent constraint references non-existing geometry\n");
                    }
                    else if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                             geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
                        sep->addChild(new SoZoomTranslation());
                        // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
                        sep->addChild(new SoImage());
                        // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6
                        sep->addChild(new SoInfo());
                    }
                }

                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            case Symmetric:
            {
                SoDatumLabel *arrows = new SoDatumLabel();
                arrows->norm.setValue(norm);
                arrows->string = "";
                arrows->textColor = drawingParameters.ConstrDimColor;
                arrows->lineWidth = 2 * edit->pixelScalingFactor;

                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(arrows);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());

                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            case InternalAlignment:
            {
                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            default:
                edit->vConstrType.push_back((*it)->Type);
        }

        edit->constrGroup->addChild(sep);
        // decrement ref counter again
        sep->unref();
        mat->unref();
    }
}

void CoinManager::createEditModeInventorNodes()
{
    assert(edit);

    // stuff for the points ++++++++++++++++++++++++++++++++++++++
    SoSeparator* pointsRoot = new SoSeparator;
    edit->EditRoot->addChild(pointsRoot);
    edit->PointsMaterials = new SoMaterial;
    edit->PointsMaterials->setName("PointsMaterials");
    pointsRoot->addChild(edit->PointsMaterials);

    SoMaterialBinding *MtlBind = new SoMaterialBinding;
    MtlBind->setName("PointsMaterialBinding");
    MtlBind->value = SoMaterialBinding::PER_VERTEX;
    pointsRoot->addChild(MtlBind);

    edit->PointsCoordinate = new SoCoordinate3;
    edit->PointsCoordinate->setName("PointsCoordinate");
    pointsRoot->addChild(edit->PointsCoordinate);

    edit->PointsDrawStyle = new SoDrawStyle;
    edit->PointsDrawStyle->setName("PointsDrawStyle");
    edit->PointsDrawStyle->pointSize = 8 * edit->pixelScalingFactor;
    pointsRoot->addChild(edit->PointsDrawStyle);

    edit->PointSet = new SoMarkerSet;
    edit->PointSet->setName("PointSet");
    edit->PointSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", edit->MarkerSize);
    pointsRoot->addChild(edit->PointSet);

    // stuff for the Curves +++++++++++++++++++++++++++++++++++++++
    SoSeparator* curvesRoot = new SoSeparator;
    edit->EditRoot->addChild(curvesRoot);
    edit->CurvesMaterials = new SoMaterial;
    edit->CurvesMaterials->setName("CurvesMaterials");
    curvesRoot->addChild(edit->CurvesMaterials);

    MtlBind = new SoMaterialBinding;
    MtlBind->setName("CurvesMaterialsBinding");
    MtlBind->value = SoMaterialBinding::PER_FACE;
    curvesRoot->addChild(MtlBind);

    edit->CurvesCoordinate = new SoCoordinate3;
    edit->CurvesCoordinate->setName("CurvesCoordinate");
    curvesRoot->addChild(edit->CurvesCoordinate);

    edit->CurvesDrawStyle = new SoDrawStyle;
    edit->CurvesDrawStyle->setName("CurvesDrawStyle");
    edit->CurvesDrawStyle->lineWidth = 3 * edit->pixelScalingFactor;
    curvesRoot->addChild(edit->CurvesDrawStyle);

    edit->CurveSet = new SoLineSet;
    edit->CurveSet->setName("CurvesLineSet");
    curvesRoot->addChild(edit->CurveSet);

    // stuff for the RootCross lines +++++++++++++++++++++++++++++++++++++++
    SoGroup* crossRoot = new Gui::SoSkipBoundingGroup;
    edit->pickStyleAxes = new SoPickStyle();
    edit->pickStyleAxes->style = SoPickStyle::SHAPE;
    crossRoot->addChild(edit->pickStyleAxes);
    edit->EditRoot->addChild(crossRoot);
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("RootCrossMaterialBinding");
    MtlBind->value = SoMaterialBinding::PER_FACE;
    crossRoot->addChild(MtlBind);

    edit->RootCrossDrawStyle = new SoDrawStyle;
    edit->RootCrossDrawStyle->setName("RootCrossDrawStyle");
    edit->RootCrossDrawStyle->lineWidth = 2 * edit->pixelScalingFactor;
    crossRoot->addChild(edit->RootCrossDrawStyle);

    edit->RootCrossMaterials = new SoMaterial;
    edit->RootCrossMaterials->setName("RootCrossMaterials");
    edit->RootCrossMaterials->diffuseColor.set1Value(0, drawingParameters.CrossColorH);
    edit->RootCrossMaterials->diffuseColor.set1Value(1, drawingParameters.CrossColorV);
    crossRoot->addChild(edit->RootCrossMaterials);

    edit->RootCrossCoordinate = new SoCoordinate3;
    edit->RootCrossCoordinate->setName("RootCrossCoordinate");
    crossRoot->addChild(edit->RootCrossCoordinate);

    edit->RootCrossSet = new SoLineSet;
    edit->RootCrossSet->setName("RootCrossLineSet");
    crossRoot->addChild(edit->RootCrossSet);

    // stuff for the EditCurves +++++++++++++++++++++++++++++++++++++++
    SoSeparator* editCurvesRoot = new SoSeparator;
    edit->EditRoot->addChild(editCurvesRoot);
    edit->EditCurvesMaterials = new SoMaterial;
    edit->EditCurvesMaterials->setName("EditCurvesMaterials");
    editCurvesRoot->addChild(edit->EditCurvesMaterials);

    edit->EditCurvesCoordinate = new SoCoordinate3;
    edit->EditCurvesCoordinate->setName("EditCurvesCoordinate");
    editCurvesRoot->addChild(edit->EditCurvesCoordinate);

    edit->EditCurvesDrawStyle = new SoDrawStyle;
    edit->EditCurvesDrawStyle->setName("EditCurvesDrawStyle");
    edit->EditCurvesDrawStyle->lineWidth = 3 * edit->pixelScalingFactor;
    editCurvesRoot->addChild(edit->EditCurvesDrawStyle);

    edit->EditCurveSet = new SoLineSet;
    edit->EditCurveSet->setName("EditCurveLineSet");
    editCurvesRoot->addChild(edit->EditCurveSet);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    float transparency;
    SbColor cursorTextColor(0,0,1);
    cursorTextColor.setPackedValue((uint32_t)hGrp->GetUnsigned("CursorTextColor", cursorTextColor.getPackedValue()), transparency);

    // stuff for the EditMarkers +++++++++++++++++++++++++++++++++++++++
    SoSeparator* editMarkersRoot = new SoSeparator;
    edit->EditRoot->addChild(editMarkersRoot);
    edit->EditMarkersMaterials = new SoMaterial;
    edit->EditMarkersMaterials->setName("EditMarkersMaterials");
    editMarkersRoot->addChild(edit->EditMarkersMaterials);

    edit->EditMarkersCoordinate = new SoCoordinate3;
    edit->EditMarkersCoordinate->setName("EditMarkersCoordinate");
    editMarkersRoot->addChild(edit->EditMarkersCoordinate);

    edit->EditMarkersDrawStyle = new SoDrawStyle;
    edit->EditMarkersDrawStyle->setName("EditMarkersDrawStyle");
    edit->EditMarkersDrawStyle->pointSize = 8 * edit->pixelScalingFactor;
    editMarkersRoot->addChild(edit->EditMarkersDrawStyle);

    edit->EditMarkerSet = new SoMarkerSet;
    edit->EditMarkerSet->setName("EditMarkerSet");
    edit->EditMarkerSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", edit->MarkerSize);
    editMarkersRoot->addChild(edit->EditMarkerSet);

    // stuff for the edit coordinates ++++++++++++++++++++++++++++++++++++++
    SoSeparator *Coordsep = new SoSeparator();
    SoPickStyle* ps = new SoPickStyle();
    ps->style.setValue(SoPickStyle::UNPICKABLE);
    Coordsep->addChild(ps);
    Coordsep->setName("CoordSeparator");
    // no caching for frequently-changing data structures
    Coordsep->renderCaching = SoSeparator::OFF;

    SoMaterial *CoordTextMaterials = new SoMaterial;
    CoordTextMaterials->setName("CoordTextMaterials");
    CoordTextMaterials->diffuseColor = cursorTextColor;
    Coordsep->addChild(CoordTextMaterials);

    SoFont *font = new SoFont();
    font->size.setValue(edit->coinFontSize);

    Coordsep->addChild(font);

    edit->textPos = new SoTranslation();
    Coordsep->addChild(edit->textPos);

    edit->textX = new SoText2();
    edit->textX->justification = SoText2::LEFT;
    edit->textX->string = "";
    Coordsep->addChild(edit->textX);
    edit->EditRoot->addChild(Coordsep);

    // group node for the Constraint visual +++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("ConstraintMaterialBinding");
    MtlBind->value = SoMaterialBinding::OVERALL ;
    edit->EditRoot->addChild(MtlBind);

    // use small line width for the Constraints
    edit->ConstraintDrawStyle = new SoDrawStyle;
    edit->ConstraintDrawStyle->setName("ConstraintDrawStyle");
    edit->ConstraintDrawStyle->lineWidth = 1 * edit->pixelScalingFactor;
    edit->EditRoot->addChild(edit->ConstraintDrawStyle);

    // add the group where all the constraints has its SoSeparator
    edit->constrGroup = new SmSwitchboard();
    edit->constrGroup->setName("ConstraintGroup");
    edit->EditRoot->addChild(edit->constrGroup);

    // group node for the Geometry information visual +++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("InformationMaterialBinding");
    MtlBind->value = SoMaterialBinding::OVERALL ;
    edit->EditRoot->addChild(MtlBind);

    // use small line width for the information visual
    edit->InformationDrawStyle = new SoDrawStyle;
    edit->InformationDrawStyle->setName("InformationDrawStyle");
    edit->InformationDrawStyle->lineWidth = 1 * edit->pixelScalingFactor;
    edit->EditRoot->addChild(edit->InformationDrawStyle);

    // add the group where all the information entity has its SoSeparator
    edit->infoGroup = new SoGroup();
    edit->infoGroup->setName("InformationGroup");
    edit->EditRoot->addChild(edit->infoGroup);
}

void CoinManager::setAxisPickStyle(bool on)
{
    assert(edit);
    if (on)
        edit->pickStyleAxes->style = SoPickStyle::SHAPE;
    else
        edit->pickStyleAxes->style = SoPickStyle::UNPICKABLE;
}

void CoinManager::redrawViewProvider()
{
    viewProvider.draw(false,false);
}

void CoinManager::updateGridExtent()
{
    float dMagF = analysisResults.boundingBoxMagnitudeOrder;

    ViewProviderSketchCoinAttorney::updateGridExtent(viewProvider,-dMagF, dMagF, -dMagF, dMagF);
}

QString CoinManager::getPresentationString(const Constraint *constraint)
{
    Base::Reference<ParameterGrp>   hGrpSketcher; // param group that includes HideUnits and ShowDimensionalName option
    bool                            iHideUnits; // internal HideUnits setting
    bool                            iShowDimName; // internal ShowDimensionalName setting
    QString                         nameStr; // name parameter string
    QString                         valueStr; // dimensional value string
    QString                         presentationStr; // final return string
    QString                         unitStr;  // the actual unit string
    QString                         baseUnitStr; // the expected base unit string
    QString                         formatStr; // the user defined format for the representation string
    double                          factor; // unit scaling factor, currently not used
    Base::UnitSystem                unitSys; // current unit system

    if(!constraint->isActive)
        return QString::fromLatin1(" ");

    // get parameter group for Sketcher display settings
    hGrpSketcher = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Sketcher");
    // Get value of HideUnits option. Default is false.
    iHideUnits = hGrpSketcher->GetBool("HideUnits", 0);
    // Get Value of ShowDimensionalName option. Default is true.
    iShowDimName = hGrpSketcher->GetBool("ShowDimensionalName", false);
    // Get the defined format string
    formatStr = QString::fromStdString(hGrpSketcher->GetASCII("DimensionalStringFormat", "%N = %V"));

    // Get the current name parameter string of the constraint
    nameStr = QString::fromStdString(constraint->Name);

    // Get the current value string including units
    valueStr = constraint->getPresentationValue().getUserString(factor, unitStr);

    // Hide units if user has requested it, is being displayed in the base
    // units, and the schema being used has a clear base unit in the first
    // place. Otherwise, display units.
    if( iHideUnits )
    {
        // Only hide the default length unit. Right now there is not an easy way
        // to get that from the Unit system so we have to manually add it here.
        // Hopefully this can be added in the future so this code won't have to
        // be updated if a new units schema is added.
        unitSys = Base::UnitsApi::getSchema();

        // If this is a supported unit system then define what the base unit is.
        switch (unitSys)
        {
            case Base::UnitSystem::SI1:
            case Base::UnitSystem::MmMin:
                baseUnitStr = QString::fromLatin1("mm");
                break;

            case Base::UnitSystem::SI2:
                baseUnitStr = QString::fromLatin1("m");
                break;

            case Base::UnitSystem::ImperialDecimal:
                baseUnitStr = QString::fromLatin1("in");
                break;

            case Base::UnitSystem::Centimeters:
                baseUnitStr = QString::fromLatin1("cm");
                break;

            default:
                // Nothing to do
                break;
        }

        if( !baseUnitStr.isEmpty() )
        {
            // expected unit string matches actual unit string. remove.
            if( QString::compare(baseUnitStr, unitStr)==0 )
            {
                // Example code from: Mod/TechDraw/App/DrawViewDimension.cpp:372
                QRegExp rxUnits(QString::fromUtf8(" \\D*$"));  //space + any non digits at end of string
                valueStr.remove(rxUnits);                      //getUserString(defaultDecimals) without units
            }
        }
    }

    if (constraint->Type == Sketcher::Diameter){
        valueStr.insert(0, QChar(8960)); // Diameter sign
    }
    else if (constraint->Type == Sketcher::Radius){
        valueStr.insert(0, QChar(82)); // Capital letter R
    }

    /**
    Create the representation string from the user defined format string
    Format options are:
    %N - the constraint name parameter
    %V - the value of the dimensional constraint, including any unit characters
    */
    if (iShowDimName && !nameStr.isEmpty())
    {
        if (formatStr.contains(QLatin1String("%V")) || formatStr.contains(QLatin1String("%N")))
        {
            presentationStr = formatStr;
            presentationStr.replace(QLatin1String("%N"), nameStr);
            presentationStr.replace(QLatin1String("%V"), valueStr);
        }
        else
        {
            // user defined format string does not contain any valid parameter, using default format "%N = %V"
            presentationStr = nameStr + QLatin1String(" = ") + valueStr;

            // TODO: Fix commented macro
            /*FC_WARN("When parsing dimensional format string \""
                    << QString(formatStr).toStdString()
                    << "\", no valid parameter found, using default format.");*/
        }

        return presentationStr;
    }

    return valueStr;
}
