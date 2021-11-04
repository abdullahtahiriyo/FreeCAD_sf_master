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

#include "EditData.h"

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Base/Exception.h>
#include <Base/Tools2D.h>
#include <Base/UnitsApi.h>

#include <App/ObjectIdentifier.h>

#include <Gui/SoFCBoundingBox.h>

#include <Gui/Inventor/MarkerBitmaps.h>

#include "SoDatumLabel.h"

#include "InformationOverlayCoinConverter.h"

#include "GeometryCoinConverter.h"

#include "GeoList.h"

#include "ViewProviderSketch.h"

#include "CoinManager.h"

using namespace SketcherGui;
using namespace Sketcher;


inline bool ViewProviderSketchCoinAttorney::constraintHasExpression(ViewProviderSketch & vp, int constrid) {
    return vp.constraintHasExpression(constrid);
};


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

void CoinManager::processConstraints()
{

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

void CoinManager::redrawViewProvider()
{
    viewProvider.draw(false,false);
}
