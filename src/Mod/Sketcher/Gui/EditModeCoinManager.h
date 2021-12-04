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


#ifndef SKETCHERGUI_EditModeCoinManager_H
#define SKETCHERGUI_EditModeCoinManager_H

#include <vector>
#include <functional>

#include <Base/Parameter.h>
#include <App/Application.h>

#include "EditModeCoinManagerParameters.h"

#include <Mod/Sketcher/App/GeoList.h>

class SbVec3f;
class SoRayPickAction;
class SoPickedPoint;
class SbVec3s;

namespace Base {
    template< typename T >
    class Vector3;

    class Vector2d;

    class Placement;
}

namespace Part {
    class Geometry;
}

namespace Sketcher {
    class Constraint;
    class PropertyConstraintList;
};

namespace SketcherGui {

class ViewProviderSketch;
class EditModeConstraintCoinManager;

using GeoList = Sketcher::GeoList;
using GeoListFacade = Sketcher::GeoListFacade;

/** @brief      Class for managing the Coin nodes of ViewProviderSketch.
 *  @details    To be documented.
 *
 */
class SketcherGuiExport EditModeCoinManager
{
    /** @brief      Class for monitoring changes in parameters affecting drawing and coin node generation
    *  @details    To be documented.
    *
    */
    class ParameterObserver : public ParameterGrp::ObserverType
    {
    private:
        enum class OverlayVisibilityParameter {
            BSplineDegree,
            BSplineControlPolygonVisible,
            BSplineCombVisible,
            BSplineKnotMultiplicityVisible,
            BSplinePoleWeightVisible
        };

    public:
        ParameterObserver(EditModeCoinManager & client);
        ~ParameterObserver();

        void subscribeToParameters();

        void unsubscribeToParameters();

        /** Observer for parameter group. */
        void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;

    private:
        void initParameters();
        void updateCurvedEdgeCountSegmentsParameter(const std::string & parametername);
        void updateLineRenderingOrderParameters(const std::string & parametername);
        void updateConstraintPresentationParameters(const std::string & parametername);
        void updateElementSizeParameters(const std::string & parametername);
        void updateColor(SbColor &sbcolor, const std::string &parametername);

        template<OverlayVisibilityParameter visibilityparameter>
        void updateOverlayVisibilityParameter(const std::string & parametername);

    private:
        std::map<std::string, std::function<void(const std::string &)>> str2updatefunction;
        EditModeCoinManager &Client;
    };

    /** @brief     Struct to hold the results of analysis
    *  @details    To be documented.
    *
    */
    struct AnalysisResults { // TODO: This needs to be refactored
        double combRepresentationScale = 0;
        float boundingBoxMagnitudeOrder = 0;
        std::vector<int> bsplineGeoIds;

    };

public:
    struct PreselectionResult {
        enum class Axes {
            None = -1,
            RootPoint = 0,
            HorizontalAxis = 1,
            VerticalAxis = 2
        };

        int ptIndex = -1;
        int geoIndex = -1; // valid values are 0,1,2,... for normal geometry and -3,-4,-5,... for external geometry
        Axes axes = Axes::None;
        std::set<int> constrIndices;

        inline void clear() {
            ptIndex = -1;
            geoIndex = -1;
            axes = Axes::None;
            constrIndices.clear();
        }
    };

public:
    explicit EditModeCoinManager(ViewProviderSketch &vp);
    ~EditModeCoinManager();

     /** @name Temporary edit curves and markers */
    //@{
    void drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel);
    void drawEdit(const std::vector<Base::Vector2d> &EditCurve);
    void setPositionText(const Base::Vector2d &Pos, const SbString &txt);
    void setPositionText(const Base::Vector2d &Pos);
    void resetPositionText(void);
    void setAxisPickStyle(bool on);
    //@}

    /** @name handle preselection and selection of points */
    //@{
    PreselectionResult detectPreselection(SoPickedPoint * Point, const SbVec2s &cursorPos);
    void drawPreselectPoint(int PreselectPoint);
    void drawPreselectRootPoint();
    void clearPointPreselection(void);
    void drawPointAsSelected(int selectpointId);
    void clearPointSelection(int selectpointId);
    void clearPointSelection(void);
    /// The client is responsible for unref-ing the SoGroup to release the memory.
    SoGroup* getSelectedConstraints();
    //@}

    /** @name update coin nodes*/
    void processGeometryConstraintsInformationOverlay(const GeoListFacade & geolistfacade, bool rebuildinformationlayer);

    void updateVirtualSpace();

    /// Draw all constraint icons
    /*! Except maybe the radius and lock ones? */
    void drawConstraintIcons();

    // This specific overload is to use a specific geometry list, which may be a temporal one
    void drawConstraintIcons(const GeoList & geolist);
    //@}

    /** @name coin node access*/
    SoSeparator* getRootEditNode();
    //@}

    /** @name update coin colors*/
    //@{
    void updateColor();
    void updateColor(const GeoList & geolist); // overload to be used with temporal geometry.
    //@}


    /** @name change coin visualisation and behaviour*/
    //@{
    void updateGridExtent();
    //@}

private:
    // This function populates the coin nodes with the information of the current geometry
    void processGeometry(const GeoListFacade & geolistfacade);

    // This function populates the geometry information layer of coin. It requires the analysis information
    // gathered during the processGeometry step, so it is not possible to run both in parallel.
    void processGeometryInformationOverlay(const GeoListFacade & geolistfacade);

    // updates the Axes length to extend beyond the calculated bounding box magnitude
    void updateAxesLength();

    // updates the parameters to be used for the Overlay information layer
    void updateOverlayParameters();

    void updateGeometryColor(const GeoListFacade & geolistfacade, bool issketchinvalid);

    // causes the ViewProvider to draw
    void redrawViewProvider();

    int defaultApplicationFontSizePixels() const;

    int getApplicationLogicalDPIX() const;

    void updateInventorNodeSizes();

    /** @name coin nodes creation*/
    void createEditModeInventorNodes();
    //@}

private:
    ViewProviderSketch & viewProvider;
    std::unique_ptr<EditModeCoinManager::ParameterObserver> pObserver;

    DrawingParameters drawingParameters;
    AnalysisResults analysisResults;
    OverlayParameters overlayParameters;
    ConstraintParameters constraintParameters;

    EditModeScenegraphNodes editModeScenegraphNodes;

    CoinMapping coinMapping;

    // Coin Helpers
    std::unique_ptr<EditModeConstraintCoinManager> pEditModeConstraintCoinManager;

};


} // namespace SketcherGui


#endif // SKETCHERGUI_EditModeCoinManager_H

