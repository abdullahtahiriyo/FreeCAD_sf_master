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


#ifndef SKETCHERGUI_EditModeCoinManagerParameters_H
#define SKETCHERGUI_EditModeCoinManagerParameters_H

#ifndef _PreComp_
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/SbColor.h>
#endif  // #ifndef _PreComp_

#include <Gui/Inventor/SmSwitchboard.h>

#include <qstring.h>
#include <qcolor.h>

#include <Mod/Sketcher/App/GeoList.h>

#include <vector>
#include <map>


namespace Part {
    class Geometry;
}

namespace SketcherGui {

/** @brief      Struct for storing local drawing parameters
 *
 * Parameters based on user preferenced are auto loaded by EditCoinManager observer nested class.
 */
struct DrawingParameters {
    int curvedEdgeCountSegments;
    // Rendering Heights
    const float zEdit       = 0.001f;
    const float zCross      = 0.001f;
    const float zInfo       = 0.004f;
    const float zLowLines   = 0.005f;    //TODO: Fix zLowLines
    const float zMidLines   = 0.006f;
    const float zHighLines  = 0.007f;  // Lines that are somehow selected to be in the high position (higher than other line categories)
    const float zHighLine   = 0.008f;   // highlighted rendering height for lines
    const float zConstr     = 0.009f; // constraint not construction
    const float zLowPoints  = 0.010f;    // TODO: Fix zLowPoints
    const float zHighPoints = 0.011f;
    const float zHighlight  = 0.012f;   // Highlighted rendering height for points
    const float zText       = 0.012f;


    // Rendering Order
    enum class GeometryRendering {
        NormalGeometry = 1,
        Construction = 2,
        ExternalGeometry = 3
    };

    GeometryRendering topRenderingGeometry = GeometryRendering::NormalGeometry;
    GeometryRendering midRenderingGeometry = GeometryRendering::Construction;

    // Rendering Colors
    static SbColor InformationColor;
    static SbColor CreateCurveColor;
    static SbColor CrossColorH;
    static SbColor CrossColorV;
    static SbColor InvalidSketchColor;
    static SbColor FullyConstrainedColor;
    static SbColor FullyConstraintInternalAlignmentColor;
    static SbColor InternalAlignedGeoColor;
    static SbColor FullyConstraintConstructionPointColor;
    static SbColor VertexColor;
    static SbColor FullyConstraintElementColor;
    static SbColor CurveColor;
    static SbColor PreselectColor;
    static SbColor PreselectSelectedColor;
    static SbColor SelectColor;
    static SbColor CurveExternalColor;
    static SbColor CurveDraftColor;
    static SbColor FullyConstraintConstructionElementColor;
    static SbColor ConstrDimColor;
    static SbColor ConstrIcoColor;
    static SbColor NonDrivingConstrDimColor;
    static SbColor ExprBasedConstrDimColor;
    static SbColor DeactivatedConstrDimColor;

    // Icon colors
    static QColor constrIcoColor;
    static QColor nonDrivingConstrIcoColor;
    static QColor constrIconSelColor;
    static QColor constrIconPreselColor;
    static QColor constrIconDisabledColor;

    // Rendering sizes (also to support HDPI monitors)
    double pixelScalingFactor = 1.0;
    int coinFontSize = 17;
    int constraintIconSize = 15;
    int markerSize = 7;

};

/** @brief      Struct for storing the nodes that need to be edited to represent a geometry layer
 */
struct GeometryLayerNodes {
    std::vector<SoMaterial *> &     PointsMaterials;
    std::vector<SoCoordinate3 *>&   PointsCoordinate;

    std::vector<SoMaterial *> &     CurvesMaterials;
    std::vector<SoCoordinate3 *> &  CurvesCoordinate;
    std::vector<SoLineSet *> &      CurveSet;
};

/** @brief
 * Helper class to store together a field index of a coin multifield object and the geometry layer to
 * which it belongs.
 *
 * Overloaded operators and specialisation of std::less enable it to be used in containers including ordered
 * containers.
 */
class MultiFieldId {
public:
    explicit constexpr MultiFieldId(int fieldindex = -1, int layerid = 0):  fieldIndex(fieldindex),
                                                                            layerId(layerid){}

    MultiFieldId(const MultiFieldId & ) = default;
    MultiFieldId & operator=(const MultiFieldId &) = default;

    inline bool operator==(const MultiFieldId& obj) const
    {
        return this->fieldIndex == obj.fieldIndex && this->layerId == obj.layerId;
    }

    inline bool operator!=(const MultiFieldId& obj) const
    {
        return this->fieldIndex != obj.fieldIndex || this->layerId != obj.layerId;
    }

    int fieldIndex = -1;
    int layerId = 0;

    static const MultiFieldId Invalid;
};


} // namespace SketcherGui

namespace std
{
    template<> struct less<SketcherGui::MultiFieldId>
    {
       bool operator() (const SketcherGui::MultiFieldId& lhs, const SketcherGui::MultiFieldId& rhs) const
       {
           return (lhs.layerId != rhs.layerId)?(lhs.layerId < rhs.layerId):(static_cast<int>(lhs.fieldIndex) < static_cast<int>(rhs.fieldIndex));
       }
    };
} // namespace std


namespace SketcherGui {

/** @brief
 * Helper class to store geometry layers configuration
 */
struct GeometryLayerParameters {
    int Layers = 1; // defaults to a single Coin Geometry Layer.
};

/** @brief     Struct to hold the results of analysis performed on geometry
*/
struct AnalysisResults { // TODO: This needs to be refactored
    double combRepresentationScale = 0;     // used for information overlay (BSpline comb)
    float boundingBoxMagnitudeOrder = 0;    // used for grid extension
    std::vector<int> bsplineGeoIds;         // used for information overlay
};

/** @brief      Struct adapted to store the parameters necessary to create and update
 *  the information overlay layer.
 */
struct OverlayParameters {
    bool rebuildInformationLayer = false;
    bool visibleInformationChanged = true;
    double currentBSplineCombRepresentationScale = 0;

    // Parameters (auto loaded by EditCoinManager observer nested class)
    bool bSplineDegreeVisible;
    bool bSplineControlPolygonVisible;
    bool bSplineCombVisible;
    bool bSplineKnotMultiplicityVisible;
    bool bSplinePoleWeightVisible;
};

/** @brief      Struct adapted to store the parameters necessary to create and update
 *  constraints.
 */
struct ConstraintParameters {
    bool bHideUnits;
    bool bShowDimensionalName;
    QString sDimensionalStringFormat;
};

/** @brief      Helper struct adapted to store the pointer to edit mode scenegraph objects.
 */
struct EditModeScenegraphNodes {
    SoSeparator *                   EditRoot;
    SmSwitchboard *                 PointsGroup;
    std::vector<SoMaterial *>       PointsMaterials;
    std::vector<SoCoordinate3 *>    PointsCoordinate;
    std::vector<SoDrawStyle *>      PointsDrawStyle;
    std::vector<SoMarkerSet *>      PointSet;

    SmSwitchboard *                 CurvesGroup;
    std::vector<SoMaterial *>       CurvesMaterials;
    std::vector<SoCoordinate3 *>    CurvesCoordinate;
    std::vector<SoDrawStyle *>      CurvesDrawStyle;
    std::vector<SoLineSet *>        CurveSet;

    SoMaterial    *RootCrossMaterials;
    SoMaterial    *EditCurvesMaterials;
    SoMaterial    *EditMarkersMaterials;

    SoCoordinate3 *RootCrossCoordinate;
    SoCoordinate3 *EditCurvesCoordinate;
    SoCoordinate3 *EditMarkersCoordinate;

    SoLineSet     *RootCrossSet;
    SoLineSet     *EditCurveSet;
    SoMarkerSet   *EditMarkerSet;

    SoText2       *textX;
    SoTranslation *textPos;

    SmSwitchboard *constrGroup;
    SoGroup       *infoGroup;
    SoPickStyle   *pickStyleAxes;

    SoDrawStyle * RootCrossDrawStyle;
    SoDrawStyle * EditCurvesDrawStyle;
    SoDrawStyle * EditMarkersDrawStyle;
    SoDrawStyle * ConstraintDrawStyle;
    SoDrawStyle * InformationDrawStyle;
};

/** @brief      Helper struct adapted to map
 */
struct CoinMapping {

    void clear() {
        CurvIdToGeoId.clear();
        PointIdToGeoId.clear();
        GeoElementId2SetId.clear();
        PointIdToVertexId.clear();
    };

    int getCurveGeoId(int curveindex, int layerindex) {return CurvIdToGeoId[layerindex][curveindex];}
    int getPointGeoId(int pointindex, int layerindex) {return PointIdToGeoId[layerindex][pointindex];}
    int getPointVertexId(int pointindex, int layerindex) {return PointIdToVertexId[layerindex][pointindex];}


    MultiFieldId getIndexLayer(int geoid, Sketcher::PointPos pos) {
        auto indexit = GeoElementId2SetId.find(Sketcher::GeoElementId(geoid, pos));

        if (indexit != GeoElementId2SetId.end()) {
            return indexit->second;
        }

        return MultiFieldId::Invalid;
    }

    MultiFieldId getIndexLayer(int vertexId) {

        for(size_t l=0; l<PointIdToVertexId.size(); l++) {
            auto indexit = std::find(PointIdToVertexId[l].begin(), PointIdToVertexId[l].end(), vertexId);

            if(indexit != PointIdToVertexId[l].end())
                return MultiFieldId(std::distance(PointIdToVertexId[l].begin(),indexit),l);
        }

        return MultiFieldId::Invalid;
    }

    //* These map an index within layer for points or curves to a GeoId */
    std::vector<std::vector<int>> CurvIdToGeoId; // conversion of SoLineSet index to GeoId
    std::vector<std::vector<int>> PointIdToGeoId; // conversion of SoCoordinate3 index to GeoId

    //* This maps an index within layer for points to a global VertexId */
    std::vector<std::vector<int>> PointIdToVertexId;

    /// This maps GeoElementId index {GeoId, PointPos} to a {layer and index} of a curves or points.
    std::map<Sketcher::GeoElementId,MultiFieldId> GeoElementId2SetId;
};


} // namespace SketcherGui

#endif // SKETCHERGUI_EditModeCoinManagerParameters_H

