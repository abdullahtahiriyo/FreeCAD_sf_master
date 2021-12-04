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

# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoPointDetail.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/details/SoLineDetail.h>

# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoImage.h>
# include <Inventor/nodes/SoInfo.h>

# include <Inventor/actions/SoRayPickAction.h>

# include <Inventor/SbVec3f.h>
# include <Inventor/SbImage.h>

# include <memory>
#endif  // #ifndef _PreComp_

#include <Gui/Inventor/SmSwitchboard.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeoList.h>

#include <Base/Exception.h>
#include <Base/Tools2D.h>
#include <Base/UnitsApi.h>
#include <Gui/Utilities.h>
#include <Base/Converter.h>
#include <Base/Tools.h>

#include <Base/Vector3D.h>

#include <App/ObjectIdentifier.h>

#include <Gui/SoFCBoundingBox.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/Tools.h>

#include <qpainter.h>

#include "SoZoomTranslation.h"
#include "SoDatumLabel.h"

#include "InformationOverlayCoinConverter.h"

#include "GeometryCoinConverter.h"

#include "ViewProviderSketch.h"

#include "ViewProviderSketchCoinAttorney.h"

#include "EditModeConstraintCoinManager.h"

#include "EditModeGeometryCoinManager.h"

using namespace SketcherGui;
using namespace Sketcher;

//**************************** EditModeGeometryCoinManager class ******************************

EditModeGeometryCoinManager::EditModeGeometryCoinManager(   ViewProviderSketch &vp,
                                                            DrawingParameters & drawingParams,
                                                            GeometryLayerParameters & geometryLayerParams,
                                                            AnalysisResults & analysisResultStruct,
                                                            EditModeScenegraphNodes & editModeScenegraph,
                                                            CoinMapping & coinMap):
    viewProvider(vp),
    drawingParameters(drawingParams),
    geometryLayerParameters(geometryLayerParams),
    analysisResults(analysisResultStruct),
    editModeScenegraphNodes(editModeScenegraph),
    coinMapping(coinMap)
{}

EditModeGeometryCoinManager::~EditModeGeometryCoinManager()
{}

void EditModeGeometryCoinManager::processGeometry(const GeoListFacade & geolistfacade)
{
    // Define the coin nodes that will be filled in with the single layer
    GeometryLayerNodes geometryLayerNodes {
        editModeScenegraphNodes.PointsMaterials,
        editModeScenegraphNodes.CurvesMaterials,
        editModeScenegraphNodes.PointsCoordinate,
        editModeScenegraphNodes.CurvesCoordinate,
        editModeScenegraphNodes.CurveSet
    };

    // process geometry layer
    // TODO: Root is set by GeometryCoinConverter which is ok for one layer only.
    GeometryCoinConverter gcconv(geometryLayerNodes, drawingParameters);

    gcconv.convert(geolistfacade);

    // set cross coordinates
    editModeScenegraphNodes.RootCrossSet->numVertices.set1Value(0,2);
    editModeScenegraphNodes.RootCrossSet->numVertices.set1Value(1,2);

    coinMapping.CurvIdToGeoId = gcconv.getCurveMap();
    coinMapping.PointIdToGeoId = gcconv.getPointMap();
    coinMapping.GeoIdPointPosToPointId = gcconv.getReversePointMap();


    // TODO: THIS NEEDS REFACTORING
    analysisResults.combRepresentationScale = gcconv.getCombRepresentationScale();
    analysisResults.boundingBoxMagnitudeOrder = exp(ceil(log(std::abs(gcconv.getBoundingBoxMaxMagnitude()))));
    analysisResults.bsplineGeoIds = gcconv.getBSplineGeoIds();
}

void EditModeGeometryCoinManager::updateGeometryColor(const GeoListFacade & geolistfacade, bool issketchinvalid)
{
    // Lambdas for convenience retrieval of geometry information
    auto isConstructionGeom = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        if (geom)
            return geom->getConstruction();
        return false;
    };

    auto isDefinedGeomPoint = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        if (geom)
            return geom->isGeoType(Part::GeomPoint::getClassTypeId()) && !geom->getConstruction();
        return false;
    };

    auto isInternalAlignedGeom = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);
        if (geom) {
            return geom->isInternalAligned();
        }
        return false;
    };

    auto isFullyConstraintElement = [&geolistfacade](int GeoId) {
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(GeoId);

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

    int PtNum = editModeScenegraphNodes.PointsMaterials->diffuseColor.getNum();
    SbColor *pcolor = editModeScenegraphNodes.PointsMaterials->diffuseColor.startEditing();
    int CurvNum = editModeScenegraphNodes.CurvesMaterials->diffuseColor.getNum();
    SbColor *color = editModeScenegraphNodes.CurvesMaterials->diffuseColor.startEditing();
    SbColor *crosscolor = editModeScenegraphNodes.RootCrossMaterials->diffuseColor.startEditing();

    SbVec3f *verts = editModeScenegraphNodes.CurvesCoordinate->point.startEditing();
    SbVec3f *pverts = editModeScenegraphNodes.PointsCoordinate->point.startEditing();

    float x,y,z;

    // colors of the point set
    if( issketchinvalid ) {
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = drawingParameters.InvalidSketchColor;
    }
    else if (ViewProviderSketchCoinAttorney::isSketchFullyConstrained(viewProvider)) {
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = drawingParameters.FullyConstrainedColor;
    }
    else {
        for (int  i=0; i < PtNum; i++) {
            int GeoId = coinMapping.PointIdToGeoId[i];

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
        auto geom = geolistfacade.getGeometryFacadeFromGeoId(coinMapping.PointIdToGeoId[i]);
        if(geom && z < drawingParameters.zHighlight) {
            if(geom->getConstruction())
                pverts[i].setValue(x,y,zConstrPoint);
            else
                pverts[i].setValue(x,y,zNormPoint);
        }
    }

    auto preselectpoint = ViewProviderSketchCoinAttorney::getPreselectPoint(viewProvider);
    auto preselectcross = ViewProviderSketchCoinAttorney::getPreselectCross(viewProvider);
    auto preselectcurve = ViewProviderSketchCoinAttorney::getPreselectCurve(viewProvider);

    if (preselectcross == 0) {
        pcolor[0] = drawingParameters.PreselectColor;
    }
    else if (preselectpoint != -1) {
        if (preselectpoint + 1 < PtNum)
            pcolor[preselectpoint + 1] = drawingParameters.PreselectColor;
    }

    ViewProviderSketchCoinAttorney::executeOnSelectionPointSet(viewProvider,
        [pcolor, PtNum, preselectpoint, drawingParameters = this->drawingParameters](const int i) {
            if (i < PtNum) {
                pcolor[i] = (i==(preselectpoint + 1) && (preselectpoint != -1))
                    ? drawingParameters.PreselectSelectedColor : drawingParameters.SelectColor;
            }
        });

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
        int GeoId = coinMapping.CurvIdToGeoId[i];
        // CurvId has several vertices associated to 1 material
        //edit->CurveSet->numVertices => [i] indicates number of vertex for line i.
        int indexes = (editModeScenegraphNodes.CurveSet->numVertices[i]);

        bool selected = ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, GeoId);
        bool preselected = (preselectcurve == GeoId);

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
        else if (ViewProviderSketchCoinAttorney::isSketchFullyConstrained(viewProvider)) {
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
    if (ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, Sketcher::GeoEnum::HAxis))
        crosscolor[0] = drawingParameters.SelectColor;
    else if (preselectcross == 1)
        crosscolor[0] = drawingParameters.PreselectColor;
    else
        crosscolor[0] = drawingParameters.CrossColorH;

    if (ViewProviderSketchCoinAttorney::isCurveSelected(viewProvider, Sketcher::GeoEnum::VAxis))
        crosscolor[1] = drawingParameters.SelectColor;
    else if (preselectcross == 2)
        crosscolor[1] = drawingParameters.PreselectColor;
    else
        crosscolor[1] = drawingParameters.CrossColorV;

    // end editing
    editModeScenegraphNodes.CurvesMaterials->diffuseColor.finishEditing();
    editModeScenegraphNodes.PointsMaterials->diffuseColor.finishEditing();
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.finishEditing();
    editModeScenegraphNodes.CurvesCoordinate->point.finishEditing();
    editModeScenegraphNodes.CurveSet->numVertices.finishEditing();
}


void EditModeGeometryCoinManager::createEditModeInventorNodes()
{
    // stuff for the points ++++++++++++++++++++++++++++++++++++++
    SoSeparator* pointsRoot = new SoSeparator;
    editModeScenegraphNodes.EditRoot->addChild(pointsRoot);
    editModeScenegraphNodes.PointsMaterials = new SoMaterial;
    editModeScenegraphNodes.PointsMaterials->setName("PointsMaterials");
    pointsRoot->addChild(editModeScenegraphNodes.PointsMaterials);

    SoMaterialBinding *MtlBind = new SoMaterialBinding;
    MtlBind->setName("PointsMaterialBinding");
    MtlBind->value = SoMaterialBinding::PER_VERTEX;
    pointsRoot->addChild(MtlBind);

    editModeScenegraphNodes.PointsCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.PointsCoordinate->setName("PointsCoordinate");
    pointsRoot->addChild(editModeScenegraphNodes.PointsCoordinate);

    editModeScenegraphNodes.PointsDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.PointsDrawStyle->setName("PointsDrawStyle");
    editModeScenegraphNodes.PointsDrawStyle->pointSize = 8 * drawingParameters.pixelScalingFactor;
    pointsRoot->addChild(editModeScenegraphNodes.PointsDrawStyle);

    editModeScenegraphNodes.PointSet = new SoMarkerSet;
    editModeScenegraphNodes.PointSet->setName("PointSet");
    editModeScenegraphNodes.PointSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", drawingParameters.markerSize);
    pointsRoot->addChild(editModeScenegraphNodes.PointSet);

    // stuff for the Curves +++++++++++++++++++++++++++++++++++++++
    SoSeparator* curvesRoot = new SoSeparator;
    editModeScenegraphNodes.EditRoot->addChild(curvesRoot);
    editModeScenegraphNodes.CurvesMaterials = new SoMaterial;
    editModeScenegraphNodes.CurvesMaterials->setName("CurvesMaterials");
    curvesRoot->addChild(editModeScenegraphNodes.CurvesMaterials);

    MtlBind = new SoMaterialBinding;
    MtlBind->setName("CurvesMaterialsBinding");
    MtlBind->value = SoMaterialBinding::PER_FACE;
    curvesRoot->addChild(MtlBind);

    editModeScenegraphNodes.CurvesCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.CurvesCoordinate->setName("CurvesCoordinate");
    curvesRoot->addChild(editModeScenegraphNodes.CurvesCoordinate);

    editModeScenegraphNodes.CurvesDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.CurvesDrawStyle->setName("CurvesDrawStyle");
    editModeScenegraphNodes.CurvesDrawStyle->lineWidth = 3 * drawingParameters.pixelScalingFactor;
    curvesRoot->addChild(editModeScenegraphNodes.CurvesDrawStyle);

    editModeScenegraphNodes.CurveSet = new SoLineSet;
    editModeScenegraphNodes.CurveSet->setName("CurvesLineSet");
    curvesRoot->addChild(editModeScenegraphNodes.CurveSet);

    // stuff for the RootCross lines +++++++++++++++++++++++++++++++++++++++
    SoGroup* crossRoot = new Gui::SoSkipBoundingGroup;
    editModeScenegraphNodes.pickStyleAxes = new SoPickStyle();
    editModeScenegraphNodes.pickStyleAxes->style = SoPickStyle::SHAPE;
    crossRoot->addChild(editModeScenegraphNodes.pickStyleAxes);
    editModeScenegraphNodes.EditRoot->addChild(crossRoot);
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("RootCrossMaterialBinding");
    MtlBind->value = SoMaterialBinding::PER_FACE;
    crossRoot->addChild(MtlBind);

    editModeScenegraphNodes.RootCrossDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.RootCrossDrawStyle->setName("RootCrossDrawStyle");
    editModeScenegraphNodes.RootCrossDrawStyle->lineWidth = 2 * drawingParameters.pixelScalingFactor;
    crossRoot->addChild(editModeScenegraphNodes.RootCrossDrawStyle);

    editModeScenegraphNodes.RootCrossMaterials = new SoMaterial;
    editModeScenegraphNodes.RootCrossMaterials->setName("RootCrossMaterials");
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(0, drawingParameters.CrossColorH);
    editModeScenegraphNodes.RootCrossMaterials->diffuseColor.set1Value(1, drawingParameters.CrossColorV);
    crossRoot->addChild(editModeScenegraphNodes.RootCrossMaterials);

    editModeScenegraphNodes.RootCrossCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.RootCrossCoordinate->setName("RootCrossCoordinate");
    crossRoot->addChild(editModeScenegraphNodes.RootCrossCoordinate);

    editModeScenegraphNodes.RootCrossSet = new SoLineSet;
    editModeScenegraphNodes.RootCrossSet->setName("RootCrossLineSet");
    crossRoot->addChild(editModeScenegraphNodes.RootCrossSet);

}
