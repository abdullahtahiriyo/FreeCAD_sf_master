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

#include "EditModeCoinManager.h"

using namespace SketcherGui;
using namespace Sketcher;

//**************************** ParameterObserver nested class ******************************
EditModeCoinManager::ParameterObserver::ParameterObserver(EditModeCoinManager &client): Client(client)
{
    initParameters();
    subscribeToParameters();
}

EditModeCoinManager::ParameterObserver::~ParameterObserver()
{
    unsubscribeToParameters();
}

void EditModeCoinManager::ParameterObserver::initParameters()
{
        // static map to avoid substantial if/else branching
    //
    // key->first               => String of parameter,
    // key->second              => Update function to be called for the parameter,
    str2updatefunction = {
        {"SegmentsPerGeometry",
            [this](const std::string & param){updateCurvedEdgeCountSegmentsParameter(param);}},
        {"BSplineDegreeVisible",
            [this](const std::string & param){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineDegree>(param);}},
        {"BSplineControlPolygonVisible",
            [this](const std::string & param){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineControlPolygonVisible>(param);}},
        {"BSplineCombVisible",
            [this](const std::string & param){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineCombVisible>(param);}},
        {"BSplineKnotMultiplicityVisible",
            [this](const std::string & param){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplineKnotMultiplicityVisible>(param);}},
        {"BSplinePoleWeightVisible",
            [this](const std::string & param){updateOverlayVisibilityParameter<OverlayVisibilityParameter::BSplinePoleWeightVisible>(param);}},
        {"TopRenderGeometryId",
            [this](const std::string & param){updateLineRenderingOrderParameters(param);}},
        {"MidRenderGeometryId",
            [this](const std::string & param){updateLineRenderingOrderParameters(param);}},
        {"HideUnits",
            [this](const std::string & param){updateConstraintPresentationParameters(param);}},
        {"ShowDimensionalName",
            [this](const std::string & param){updateConstraintPresentationParameters(param);}},
        {"DimensionalStringFormat",
            [this](const std::string & param){updateConstraintPresentationParameters(param);}},
        {"ViewScalingFactor",
            [this](const std::string & param){updateElementSizeParameters(param);}},
        {"MarkerSize",
            [this](const std::string & param){updateElementSizeParameters(param);}},
        {"EditSketcherFontSize",
            [this](const std::string & param){updateElementSizeParameters(param);}},
        {"CreateLineColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.CreateCurveColor, param);}},
        {"EditedVertexColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.VertexColor, param);}},
        {"EditedEdgeColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.CurveColor, param);}},
        {"ConstructionColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.CurveDraftColor, param);}},
        {"InternalAlignedGeoColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.InternalAlignedGeoColor, param);}},
        {"FullyConstraintElementColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.FullyConstraintElementColor, param);}},
        {"FullyConstraintConstructionElementColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.FullyConstraintConstructionElementColor, param);}},
        {"FullyConstraintInternalAlignmentColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.FullyConstraintInternalAlignmentColor, param);}},
        {"FullyConstraintConstructionPointColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.FullyConstraintConstructionPointColor, param);}},
        {"FullyConstraintElementColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.FullyConstraintElementColor, param);}},
        {"InvalidSketchColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.InvalidSketchColor, param);}},
        {"FullyConstrainedColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.FullyConstrainedColor, param);}},
        {"ConstrainedDimColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.ConstrDimColor, param);}},
        {"ConstrainedIcoColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.ConstrIcoColor, param);}},
        {"NonDrivingConstrDimColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.NonDrivingConstrDimColor, param);}},
        {"ExprBasedConstrDimColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.ExprBasedConstrDimColor, param);}},
        {"DeactivatedConstrDimColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.DeactivatedConstrDimColor, param);}},
        {"ExternalColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.CurveExternalColor, param);}},
        {"HighlightColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.PreselectColor, param);}},
        {"SelectionColor",
            [this, drawingParameters = Client.drawingParameters](const std::string & param){updateColor(drawingParameters.SelectColor, param);}},
    };

    for( auto & val : str2updatefunction){
        auto string     = val.first;
        auto function   = val.second;

        function(string);
    }
}

void EditModeCoinManager::ParameterObserver::updateCurvedEdgeCountSegmentsParameter(const std::string & parametername)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    int stdcountsegments = hGrp->GetInt(parametername.c_str(), 50);
    // value cannot be smaller than 6
    if (stdcountsegments < 6)
        stdcountsegments = 6;

    Client.drawingParameters.curvedEdgeCountSegments = stdcountsegments;
}

void EditModeCoinManager::ParameterObserver::updateLineRenderingOrderParameters(const std::string & parametername)
{
    (void) parametername;

    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    Client.drawingParameters.topRenderingGeometry = DrawingParameters::GeometryRendering (hGrpp->GetInt("TopRenderGeometryId",1));
    Client.drawingParameters.midRenderingGeometry = DrawingParameters::GeometryRendering (hGrpp->GetInt("MidRenderGeometryId",2));
}

void EditModeCoinManager::ParameterObserver::updateConstraintPresentationParameters(const std::string & parametername)
{
    (void) parametername;

    ParameterGrp::handle hGrpskg = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");

    Client.constraintParameters.bHideUnits = hGrpskg->GetBool("HideUnits", false);
    Client.constraintParameters.bShowDimensionalName = hGrpskg->GetBool("ShowDimensionalName", false);
    Client.constraintParameters.sDimensionalStringFormat = QString::fromStdString(hGrpskg->GetASCII("DimensionalStringFormat", "%N = %V"));
}

template<EditModeCoinManager::ParameterObserver::OverlayVisibilityParameter visibilityparameter>
void EditModeCoinManager::ParameterObserver::updateOverlayVisibilityParameter(const std::string & parametername)
{
    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineDegree)
        Client.overlayParameters.bSplineDegreeVisible = hGrpsk->GetBool(parametername.c_str(), true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineControlPolygonVisible)
        Client.overlayParameters.bSplineControlPolygonVisible = hGrpsk->GetBool(parametername.c_str(), true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineCombVisible)
        Client.overlayParameters.bSplineCombVisible = hGrpsk->GetBool(parametername.c_str(), true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplineKnotMultiplicityVisible)
        Client.overlayParameters.bSplineKnotMultiplicityVisible = hGrpsk->GetBool(parametername.c_str(), true);
    else if constexpr (visibilityparameter == OverlayVisibilityParameter::BSplinePoleWeightVisible)
        Client.overlayParameters.bSplinePoleWeightVisible = hGrpsk->GetBool(parametername.c_str(), true);

    Client.overlayParameters.visibleInformationChanged = true;
}

void EditModeCoinManager::ParameterObserver::updateElementSizeParameters(const std::string & parametername)
{
    (void) parametername;

    //Add scaling to Constraint icons
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    double viewScalingFactor = hGrp->GetFloat("ViewScalingFactor", 1.0);
    viewScalingFactor = Base::clamp<double>(viewScalingFactor, 0.5, 5.0);

    int markersize = hGrp->GetInt("MarkerSize", 7);

    int defaultFontSizePixels = Client.defaultApplicationFontSizePixels(); // returns height in pixels, not points

    int sketcherfontSize = hGrp->GetInt("EditSketcherFontSize", defaultFontSizePixels);

    int dpi = Client.getApplicationLogicalDPIX();

    // simple scaling factor for hardcoded pixel values in the Sketcher
    Client.drawingParameters.pixelScalingFactor = viewScalingFactor * dpi / 96; // 96 ppi is the standard pixel density for which pixel quantities were calculated

    // Coin documentation indicates the size of a font is:
    // SoSFFloat SoFont::size        Size of font. Defaults to 10.0.
    //
    // For 2D rendered bitmap fonts (like for SoText2), this value is the height of a character in screen pixels. For 3D text, this value is the world-space coordinates height of a character in the current units setting (see documentation for SoUnits node).
    //
    // However, with hdpi monitors, the coin font labels do not respect the size passed in pixels:
    // https://forum.freecadweb.org/viewtopic.php?f=3&t=54347&p=467610#p467610
    // https://forum.freecadweb.org/viewtopic.php?f=10&t=49972&start=40#p467471
    //
    // Because I (abdullah) have  96 dpi logical, 82 dpi physical, and I see a 35px font setting for a "1" in a datum label as 34px,
    // and I see kilsore and Elyas screenshots showing 41px and 61px in higher resolution monitors for the same configuration, I think
    // that coin pixel size has to be corrected by the logical dpi of the monitor. The rationale is that: a) it obviously needs dpi
    // correction, b) with physical dpi, the ratio of representation between kilsore and me is too far away.
    //
    // This means that the following correction does not have a documented basis, but appears necessary so that the Sketcher is usable in
    // HDPI monitors.

    Client.drawingParameters.coinFontSize = std::lround(sketcherfontSize * 96.0f / dpi);
    Client.drawingParameters.constraintIconSize = std::lround(0.8 * sketcherfontSize);

    // For marker size the global default is used.
    //
    // Rationale:
    // -> Other WBs use the default value as is
    // -> If a user has a HDPI, he will eventually change the value for the other WBs
    // -> If we correct the value here in addition, we would get two times a resize
    Client.drawingParameters.markerSize = markersize;

    Client.updateInventorNodeSizes();
}

void EditModeCoinManager::ParameterObserver::updateColor(SbColor &sbcolor, const std::string &parametername)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    float transparency = 0.f;
    unsigned long color = (unsigned long)(sbcolor.getPackedValue());
    color = hGrp->GetUnsigned(parametername.c_str(), color);
    sbcolor.setPackedValue((uint32_t)color, transparency);
}

void EditModeCoinManager::ParameterObserver::subscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Attach(this);

    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpsk->Attach(this);

    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpp->Attach(this);

    ParameterGrp::handle hGrpskg = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    hGrpskg->Attach(this);

}

void EditModeCoinManager::ParameterObserver::unsubscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Detach(this);

    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpsk->Detach(this);

    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrpp->Detach(this);

    ParameterGrp::handle hGrpskg = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    hGrpskg->Detach(this);
}

void EditModeCoinManager::ParameterObserver::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    (void) rCaller;

    auto key = str2updatefunction.find(sReason);
    if( key != str2updatefunction.end() ) {
        auto string     = key->first;
        auto function   = key->second;

        function(string);

        Client.redrawViewProvider(); // redraw with non-temporal geometry
    }
}

//**************************** EditModeCoinManager class ******************************

EditModeCoinManager::EditModeCoinManager(ViewProviderSketch &vp):viewProvider(vp) {

    pEditModeConstraintCoinManager = std::make_unique<EditModeConstraintCoinManager>(viewProvider,
                                                                                    drawingParameters,
                                                                                    constraintParameters,
                                                                                    editModeScenegraphNodes,
                                                                                    coinMapping);
    // Create Edit Mode Scenograph
    createEditModeInventorNodes();

    // Create parameter observer and initialise watched parameters
    pObserver = std::make_unique<EditModeCoinManager::ParameterObserver>(*this);
}

EditModeCoinManager::~EditModeCoinManager()
{
    Gui::coinRemoveAllChildren(editModeScenegraphNodes.EditRoot);
    ViewProviderSketchCoinAttorney::removeNodeFromRoot(viewProvider, editModeScenegraphNodes.EditRoot);
    editModeScenegraphNodes.EditRoot->unref();
}

/***** Temporary edit curves and markers *****/

void EditModeCoinManager::drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel)
{
    // determine marker size
    int augmentedmarkersize = drawingParameters.markerSize;

    auto supportedsizes = Gui::Inventor::MarkerBitmaps::getSupportedSizes("CIRCLE_LINE");

    auto defaultmarker = std::find(supportedsizes.begin(), supportedsizes.end(), drawingParameters.markerSize);

    if(defaultmarker != supportedsizes.end()) {
        auto validAugmentationLevels = std::distance(defaultmarker,supportedsizes.end());

        if(augmentationlevel >= validAugmentationLevels)
            augmentationlevel = validAugmentationLevels - 1;

        augmentedmarkersize = *std::next(defaultmarker, augmentationlevel);
    }

    editModeScenegraphNodes.EditMarkerSet->markerIndex.startEditing();
    editModeScenegraphNodes.EditMarkerSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", augmentedmarkersize);

    // add the points to set
    editModeScenegraphNodes.EditMarkersCoordinate->point.setNum(EditMarkers.size());
    editModeScenegraphNodes.EditMarkersMaterials->diffuseColor.setNum(EditMarkers.size());
    SbVec3f *verts = editModeScenegraphNodes.EditMarkersCoordinate->point.startEditing();
    SbColor *color = editModeScenegraphNodes.EditMarkersMaterials->diffuseColor.startEditing();

    int i=0; // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditMarkers.begin(); it != EditMarkers.end(); ++it,i++) {
        verts[i].setValue(it->x, it->y, drawingParameters.zEdit);
        color[i] = drawingParameters.InformationColor;
    }

    editModeScenegraphNodes.EditMarkersCoordinate->point.finishEditing();
    editModeScenegraphNodes.EditMarkersMaterials->diffuseColor.finishEditing();
    editModeScenegraphNodes.EditMarkerSet->markerIndex.finishEditing();
}

void EditModeCoinManager::drawEdit(const std::vector<Base::Vector2d> &EditCurve)
{
    editModeScenegraphNodes.EditCurveSet->numVertices.setNum(1);
    editModeScenegraphNodes.EditCurvesCoordinate->point.setNum(EditCurve.size());
    editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.setNum(EditCurve.size());
    SbVec3f *verts = editModeScenegraphNodes.EditCurvesCoordinate->point.startEditing();
    int32_t *index = editModeScenegraphNodes.EditCurveSet->numVertices.startEditing();
    SbColor *color = editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.startEditing();

    int i=0; // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditCurve.begin(); it != EditCurve.end(); ++it,i++) {
        verts[i].setValue(it->x,it->y, drawingParameters.zEdit);
        color[i] = drawingParameters.CreateCurveColor;
    }

    index[0] = EditCurve.size();
    editModeScenegraphNodes.EditCurvesCoordinate->point.finishEditing();
    editModeScenegraphNodes.EditCurveSet->numVertices.finishEditing();
    editModeScenegraphNodes.EditCurvesMaterials->diffuseColor.finishEditing();
}

void EditModeCoinManager::setPositionText(const Base::Vector2d &Pos, const SbString &text)
{
    editModeScenegraphNodes.textX->string = text;
    editModeScenegraphNodes.textPos->translation = SbVec3f(Pos.x, Pos.y, drawingParameters.zText);
}

void EditModeCoinManager::setPositionText(const Base::Vector2d &Pos)
{
    SbString text;
    text.sprintf(" (%.1f,%.1f)", Pos.x, Pos.y);
    setPositionText(Pos,text);
}

void EditModeCoinManager::resetPositionText(void)
{
    editModeScenegraphNodes.textX->string = "";
}

void EditModeCoinManager::setAxisPickStyle(bool on)
{
    if (on)
        editModeScenegraphNodes.pickStyleAxes->style = SoPickStyle::SHAPE;
    else
        editModeScenegraphNodes.pickStyleAxes->style = SoPickStyle::UNPICKABLE;
}

/***** Selection and Preselection *****/

void EditModeCoinManager::drawPreselectPoint(int PreselectPoint)
{
    int oldPtId = -1;
    auto preselectpoint = ViewProviderSketchCoinAttorney::getPreselectPoint(viewProvider);
    if (preselectpoint != -1)
        oldPtId = preselectpoint + 1;
    else if (ViewProviderSketchCoinAttorney::getPreselectCross(viewProvider) == 0)
        oldPtId = 0;
    int newPtId = PreselectPoint + 1;
    SbVec3f *pverts = editModeScenegraphNodes.PointsCoordinate->point.startEditing();
    float x,y,z;
    if (oldPtId != -1 && !ViewProviderSketchCoinAttorney::isPointSelected(viewProvider, oldPtId)) {
        // send to background
        pverts[oldPtId].getValue(x,y,z);
        pverts[oldPtId].setValue(x,y,drawingParameters.zLowPoints);
    }
    // bring to foreground
    pverts[newPtId].getValue(x,y,z);
    pverts[newPtId].setValue(x,y,drawingParameters.zHighlight);

    editModeScenegraphNodes.PointsCoordinate->point.finishEditing();
}

void EditModeCoinManager::clearPointPreselection(void)
{
    int oldPtId = -1;
     auto preselectpoint = ViewProviderSketchCoinAttorney::getPreselectPoint(viewProvider);
    if (preselectpoint != -1)
        oldPtId = preselectpoint + 1;
    else if (ViewProviderSketchCoinAttorney::getPreselectCross(viewProvider) == 0)
        oldPtId = 0;
    if (oldPtId != -1 && !ViewProviderSketchCoinAttorney::isPointSelected(viewProvider, oldPtId)) {
        // send to background
        SbVec3f *pverts = editModeScenegraphNodes.PointsCoordinate->point.startEditing();
        float x,y,z;
        pverts[oldPtId].getValue(x,y,z);
        pverts[oldPtId].setValue(x,y,drawingParameters.zLowPoints);
        editModeScenegraphNodes.PointsCoordinate->point.finishEditing();
    }
}

void EditModeCoinManager::drawPreselectRootPoint()
{
    drawPreselectPoint(-1);

    //TODO: This is both a hack and a placeholder. It is a hack because -1 in ViewProviderSketch means 'not used'. It works because it assumes a root point at index 0. This hack was present in ViewProviderSketch. I have move it here to show intent in ViewProviderSketch, knowing that changes to the indexing will be undertaken here when geometry layers are added. So the code is functionally the same as before.
}

void EditModeCoinManager::drawPointAsSelected(int selectpointId)
{
    int PtId = selectpointId + 1;
    SbVec3f *pverts = editModeScenegraphNodes.PointsCoordinate->point.startEditing();
    // bring to foreground
    float x,y,z;
    pverts[PtId].getValue(x,y,z);
    pverts[PtId].setValue(x,y,drawingParameters.zHighlight);
    editModeScenegraphNodes.PointsCoordinate->point.finishEditing();
}

void EditModeCoinManager::clearPointSelection(int selectpointId)
{
    int PtId = selectpointId + 1;
    SbVec3f *pverts = editModeScenegraphNodes.PointsCoordinate->point.startEditing();
    // send to background
    float x,y,z;
    pverts[PtId].getValue(x,y,z);
    pverts[PtId].setValue(x,y,drawingParameters.zLowPoints);
    editModeScenegraphNodes.PointsCoordinate->point.finishEditing();
}

void EditModeCoinManager::clearPointSelection(void)
{
    SbVec3f *pverts = editModeScenegraphNodes.PointsCoordinate->point.startEditing();
    // send to background
    ViewProviderSketchCoinAttorney::executeOnSelectionPointSet(viewProvider,
        [pverts, drawingParameters = this->drawingParameters](const int i) {
            float x,y,z;
            pverts[i].getValue(x,y,z);
            pverts[i].setValue(x,y,drawingParameters.zLowPoints);
        });
    editModeScenegraphNodes.PointsCoordinate->point.finishEditing();
}

EditModeCoinManager::PreselectionResult EditModeCoinManager::detectPreselection(SoPickedPoint * Point, const SbVec2s &cursorPos)
{
    EditModeCoinManager::PreselectionResult result;

    if(!Point)
        return result;

    //Base::Console().Log("Point pick\n");
    SoPath *path = Point->getPath();
    SoNode *tail = path->getTail(); // Tail is directly the node containing points and curves

    // checking for a hit in the points
    if (tail == editModeScenegraphNodes.PointSet) {
        const SoDetail *point_detail = Point->getDetail(editModeScenegraphNodes.PointSet);
        if (point_detail && point_detail->getTypeId() == SoPointDetail::getClassTypeId()) {
            // get the index
            result.ptIndex = static_cast<const SoPointDetail *>(point_detail)->getCoordinateIndex();
            result.ptIndex -= 1; // shift corresponding to RootPoint
            if (result.ptIndex == Sketcher::GeoEnum::RtPnt)
                result.axes = PreselectionResult::Axes::RootPoint;
        }
    } else {
        // checking for a hit in the curves
        if (tail == editModeScenegraphNodes.CurveSet) {
            const SoDetail *curve_detail = Point->getDetail(editModeScenegraphNodes.CurveSet);
            if (curve_detail && curve_detail->getTypeId() == SoLineDetail::getClassTypeId()) {
                // get the index
                int curveIndex = static_cast<const SoLineDetail *>(curve_detail)->getLineIndex();
                result.geoIndex = coinMapping.CurvIdToGeoId[curveIndex];
            }
        // checking for a hit in the axes
        } else if (tail == editModeScenegraphNodes.RootCrossSet) {
            const SoDetail *cross_detail = Point->getDetail(editModeScenegraphNodes.RootCrossSet);
            if (cross_detail && cross_detail->getTypeId() == SoLineDetail::getClassTypeId()) {
                // get the index (reserve index 0 for root point)
                int CrossIndex = static_cast<const SoLineDetail *>(cross_detail)->getLineIndex();

                if(CrossIndex == 0)
                    result.axes = PreselectionResult::Axes::HorizontalAxis;
                else if(CrossIndex == 1)
                    result.axes = PreselectionResult::Axes::VerticalAxis;
            }
        } else {
            // checking if a constraint is hit
            result.constrIndices = pEditModeConstraintCoinManager->detectPreselectionConstr(Point, cursorPos);
        }
    }

    return result;
}

SoGroup* EditModeCoinManager::getSelectedConstraints()
{
    SoGroup* group = new SoGroup();
    group->ref();

    for (int i=0; i < editModeScenegraphNodes.constrGroup->getNumChildren(); i++) {
        if (ViewProviderSketchCoinAttorney::isConstraintSelected(viewProvider, i)) {
            SoSeparator *sep = pEditModeConstraintCoinManager->getConstraintIdSeparator(i);
            if (sep)
                group->addChild(sep);
        }
    }

    return group;
}

/***** update coin nodes *****/

void EditModeCoinManager::processGeometryConstraintsInformationOverlay(const GeoList & geolist, bool rebuildinformationlayer)
{
    overlayParameters.rebuildInformationLayer = rebuildinformationlayer;

    processGeometry(geolist);

    updateOverlayParameters();

    processGeometryInformationOverlay(geolist);

    updateAxesLength();

    updateGridExtent();

    pEditModeConstraintCoinManager->processConstraints(geolist);
}

void EditModeCoinManager::processGeometry(const GeoList & geolist)
{
    // Define a single layer processing to be converted to coin (currently it is the whole geometry)
    GeometryLayer geolayer { geolist };

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

    gcconv.convert(geolayer);

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

void EditModeCoinManager::updateOverlayParameters()
{
    if ( (analysisResults.combRepresentationScale > (2 * overlayParameters.currentBSplineCombRepresentationScale)) ||
        (analysisResults.combRepresentationScale < (overlayParameters.currentBSplineCombRepresentationScale / 2)))
        overlayParameters.currentBSplineCombRepresentationScale = analysisResults.combRepresentationScale ;
}

void EditModeCoinManager::processGeometryInformationOverlay(const GeoList & geolist)
{
    if(overlayParameters.rebuildInformationLayer) {
        // every time we start with empty information overlay
        Gui::coinRemoveAllChildren(editModeScenegraphNodes.infoGroup);
    }

    auto ioconv = InformationOverlayCoinConverter(editModeScenegraphNodes.infoGroup, overlayParameters, drawingParameters);

    // geometry information layer for bsplines, as they need a second round now that max curvature is known
    for (auto geoid : analysisResults.bsplineGeoIds) {
        const Part::Geometry *geo = geolist.getGeometryFromGeoId(geoid);

        ioconv.convert(geo);
    }

    overlayParameters.visibleInformationChanged = false; // just updated
}

void EditModeCoinManager::updateAxesLength()
{
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(0,SbVec3f(-analysisResults.boundingBoxMagnitudeOrder, 0.0f, drawingParameters.zCross));
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(1,SbVec3f(analysisResults.boundingBoxMagnitudeOrder, 0.0f, drawingParameters.zCross));
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(2,SbVec3f(0.0f, -analysisResults.boundingBoxMagnitudeOrder, drawingParameters.zCross));
    editModeScenegraphNodes.RootCrossCoordinate->point.set1Value(3,SbVec3f(0.0f, analysisResults.boundingBoxMagnitudeOrder, drawingParameters.zCross));
}

void EditModeCoinManager::updateGridExtent()
{
    float dMagF = analysisResults.boundingBoxMagnitudeOrder;

    ViewProviderSketchCoinAttorney::updateGridExtent(viewProvider,-dMagF, dMagF, -dMagF, dMagF);
}

void EditModeCoinManager::updateVirtualSpace()
{
    pEditModeConstraintCoinManager->updateVirtualSpace();
}


void EditModeCoinManager::updateColor()
{
    auto geolistfacade = ViewProviderSketchCoinAttorney::getGeoListFacade(viewProvider);

    bool sketchinvalid = ViewProviderSketchCoinAttorney::isSketchInvalid(viewProvider);

    updateGeometryColor(geolistfacade, sketchinvalid);

    // update constraint color

    auto constraints = ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    if(ViewProviderSketchCoinAttorney::haveConstraintsInvalidGeometry(viewProvider))
        return;

    pEditModeConstraintCoinManager->updateConstraintColor(constraints);
}

void EditModeCoinManager::updateColor(const GeoList & geolist)
{
    auto geolistfacade = Sketcher::getGeoListFacade(geolist);

    bool sketchinvalid = ViewProviderSketchCoinAttorney::isSketchInvalid(viewProvider);

    updateGeometryColor(geolistfacade, sketchinvalid);

    // update constraint color

    auto constraints = ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    if(ViewProviderSketchCoinAttorney::haveConstraintsInvalidGeometry(viewProvider))
        return;

    pEditModeConstraintCoinManager->updateConstraintColor(constraints);
}


void EditModeCoinManager::updateGeometryColor(const GeoListFacade & geolistfacade, bool issketchinvalid)
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
        auto geom = geolistfacade.getGeometryFromGeoId(coinMapping.PointIdToGeoId[i]);
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


void EditModeCoinManager::createEditModeInventorNodes()
{
    // 1 - Create the edit root node
    editModeScenegraphNodes.EditRoot = new SoSeparator;
    editModeScenegraphNodes.EditRoot->ref();
    editModeScenegraphNodes.EditRoot->setName("Sketch_EditRoot");
    ViewProviderSketchCoinAttorney::addNodeToRoot(viewProvider, editModeScenegraphNodes.EditRoot);
    editModeScenegraphNodes.EditRoot->renderCaching = SoSeparator::OFF ;


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

    // stuff for the EditCurves +++++++++++++++++++++++++++++++++++++++
    SoSeparator* editCurvesRoot = new SoSeparator;
    editModeScenegraphNodes.EditRoot->addChild(editCurvesRoot);
    editModeScenegraphNodes.EditCurvesMaterials = new SoMaterial;
    editModeScenegraphNodes.EditCurvesMaterials->setName("EditCurvesMaterials");
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurvesMaterials);

    editModeScenegraphNodes.EditCurvesCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.EditCurvesCoordinate->setName("EditCurvesCoordinate");
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurvesCoordinate);

    editModeScenegraphNodes.EditCurvesDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.EditCurvesDrawStyle->setName("EditCurvesDrawStyle");
    editModeScenegraphNodes.EditCurvesDrawStyle->lineWidth = 3 * drawingParameters.pixelScalingFactor;
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurvesDrawStyle);

    editModeScenegraphNodes.EditCurveSet = new SoLineSet;
    editModeScenegraphNodes.EditCurveSet->setName("EditCurveLineSet");
    editCurvesRoot->addChild(editModeScenegraphNodes.EditCurveSet);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    float transparency;
    SbColor cursorTextColor(0,0,1);
    cursorTextColor.setPackedValue((uint32_t)hGrp->GetUnsigned("CursorTextColor", cursorTextColor.getPackedValue()), transparency);

    // stuff for the EditMarkers +++++++++++++++++++++++++++++++++++++++
    SoSeparator* editMarkersRoot = new SoSeparator;
    editModeScenegraphNodes.EditRoot->addChild(editMarkersRoot);
    editModeScenegraphNodes.EditMarkersMaterials = new SoMaterial;
    editModeScenegraphNodes.EditMarkersMaterials->setName("EditMarkersMaterials");
    editMarkersRoot->addChild(editModeScenegraphNodes.EditMarkersMaterials);

    editModeScenegraphNodes.EditMarkersCoordinate = new SoCoordinate3;
    editModeScenegraphNodes.EditMarkersCoordinate->setName("EditMarkersCoordinate");
    editMarkersRoot->addChild(editModeScenegraphNodes.EditMarkersCoordinate);

    editModeScenegraphNodes.EditMarkersDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.EditMarkersDrawStyle->setName("EditMarkersDrawStyle");
    editModeScenegraphNodes.EditMarkersDrawStyle->pointSize = 8 * drawingParameters.pixelScalingFactor;
    editMarkersRoot->addChild(editModeScenegraphNodes.EditMarkersDrawStyle);

    editModeScenegraphNodes.EditMarkerSet = new SoMarkerSet;
    editModeScenegraphNodes.EditMarkerSet->setName("EditMarkerSet");
    editModeScenegraphNodes.EditMarkerSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", drawingParameters.markerSize);
    editMarkersRoot->addChild(editModeScenegraphNodes.EditMarkerSet);

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
    font->size.setValue(drawingParameters.coinFontSize);

    Coordsep->addChild(font);

    editModeScenegraphNodes.textPos = new SoTranslation();
    Coordsep->addChild(editModeScenegraphNodes.textPos);

    editModeScenegraphNodes.textX = new SoText2();
    editModeScenegraphNodes.textX->justification = SoText2::LEFT;
    editModeScenegraphNodes.textX->string = "";
    Coordsep->addChild(editModeScenegraphNodes.textX);
    editModeScenegraphNodes.EditRoot->addChild(Coordsep);

    // group node for the Constraint visual +++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("ConstraintMaterialBinding");
    MtlBind->value = SoMaterialBinding::OVERALL ;
    editModeScenegraphNodes.EditRoot->addChild(MtlBind);

    // use small line width for the Constraints
    editModeScenegraphNodes.ConstraintDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.ConstraintDrawStyle->setName("ConstraintDrawStyle");
    editModeScenegraphNodes.ConstraintDrawStyle->lineWidth = 1 * drawingParameters.pixelScalingFactor;
   editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.ConstraintDrawStyle);

    // add the group where all the constraints has its SoSeparator
    editModeScenegraphNodes.constrGroup = new SmSwitchboard();
    editModeScenegraphNodes.constrGroup->setName("ConstraintGroup");
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.constrGroup);

    // group node for the Geometry information visual +++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("InformationMaterialBinding");
    MtlBind->value = SoMaterialBinding::OVERALL ;
    editModeScenegraphNodes.EditRoot->addChild(MtlBind);

    // use small line width for the information visual
    editModeScenegraphNodes.InformationDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.InformationDrawStyle->setName("InformationDrawStyle");
    editModeScenegraphNodes.InformationDrawStyle->lineWidth = 1 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.InformationDrawStyle);

    // add the group where all the information entity has its SoSeparator
    editModeScenegraphNodes.infoGroup = new SoGroup();
    editModeScenegraphNodes.infoGroup->setName("InformationGroup");
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.infoGroup);
}



void EditModeCoinManager::redrawViewProvider()
{
    viewProvider.draw(false,false);
}


// public function that triggers drawing of most constraint icons
void EditModeCoinManager::drawConstraintIcons()
{
    pEditModeConstraintCoinManager->drawConstraintIcons();
}

void EditModeCoinManager::drawConstraintIcons(const GeoList & geolist)
{
    pEditModeConstraintCoinManager->drawConstraintIcons(geolist);
}


int EditModeCoinManager::defaultApplicationFontSizePixels() const {
    return ViewProviderSketchCoinAttorney::defaultApplicationFontSizePixels(viewProvider);
}

int EditModeCoinManager::getApplicationLogicalDPIX() const {
    return ViewProviderSketchCoinAttorney::getApplicationLogicalDPIX(viewProvider);
}

void EditModeCoinManager::updateInventorNodeSizes()
{
    editModeScenegraphNodes.PointsDrawStyle->pointSize = 8 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.PointSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", drawingParameters.markerSize);
    editModeScenegraphNodes.CurvesDrawStyle->lineWidth = 3 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.RootCrossDrawStyle->lineWidth = 2 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditCurvesDrawStyle->lineWidth = 3 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditMarkersDrawStyle->pointSize = 8 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditMarkerSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", drawingParameters.markerSize);
    editModeScenegraphNodes.ConstraintDrawStyle->lineWidth = 1 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.InformationDrawStyle->lineWidth = 1 * drawingParameters.pixelScalingFactor;

    pEditModeConstraintCoinManager->rebuildConstraintNodes();
}

SoSeparator* EditModeCoinManager::getRootEditNode()
{
    return editModeScenegraphNodes.EditRoot;
}
