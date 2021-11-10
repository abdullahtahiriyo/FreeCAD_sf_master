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


#ifndef SKETCHERGUI_CoinManager_H
#define SKETCHERGUI_CoinManager_H

#include <vector>
#include <functional>

#include <Base/Parameter.h>
#include <App/Application.h>

#include "CoinManagerParameters.h"

class SbVec3f;
class SoRayPickAction;

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

struct EditData;

template < typename T >
class GeoListModel;

class ViewProviderSketch;


/** @brief      Attorney class for limiting access to viewprovider
 *  @details
 *  ViewProviderSketch delegates a substantial part of coin related visualisation to
 *  CoinManager during edit mode.
 *
 *  Sometimes CoinManager needs to access selected functionalities only available to ViewProviderSketch.
 *
 *  This attorney class regulates which specific functionalities CoinManager is able to access in
 *  ViewProviderSketch.
 *
 *  The objective is:
 *  - to preserve as much as possible ViewProviderSketch encapsulation
 *  - to promote as much loose coupling as possible.
 *  - to keep control over the interactions between these classes and easily identify the cooperation interface.
 */
class ViewProviderSketchCoinAttorney {
private:
    static inline bool constraintHasExpression(ViewProviderSketch &vp, int constrid);
    static inline const std::vector<Sketcher::Constraint *> getConstraints(ViewProviderSketch & vp);
    static inline const GeoList getGeoList(ViewProviderSketch & vp);
    static inline Base::Placement getEditingPlacement(ViewProviderSketch & vp);
    static inline void updateGridExtent(ViewProviderSketch & vp, float minx, float maxx, float miny, float maxy);
    static inline bool isShownVirtualSpace(ViewProviderSketch & vp);
    static inline std::unique_ptr<SoRayPickAction> getRayPickAction(ViewProviderSketch & vp);

    static float getScaleFactor(ViewProviderSketch & vp);

    friend class CoinManager;
};




/** @brief      Class for managing the Coin nodes of ViewProviderSketch.
 *  @details    To be documented.
 *
 */
class SketcherGuiExport CoinManager
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
        ParameterObserver(CoinManager & client);
        ~ParameterObserver();

        void subscribeToParameters();

        void unsubscribeToParameters();

        /** Observer for parameter group. */
        void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;

    private:
        void initParameters();
        void updateCurvedEdgeCountSegmentsParameter();
        void updateLineRenderingOrderParameters();

        template<OverlayVisibilityParameter visibilityparameter>
        void updateOverlayVisibilityParameter();

    private:
        CoinManager &Client;
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

private:
    // TODO: This should probably go in ConstraintCoinConverter, but then updateColor should go there TODO
    enum class ConstraintNodePosition {
        MaterialIndex = 0,
        DatumLabelIndex = 0,
        FirstTranslationIndex = 1,
        FirstIconIndex = 2,
        FirstConstraintIdIndex = 3,
        SecondTranslationIndex = 4,
        SecondIconIndex = 5,
        SecondConstraintIdIndex = 6
    };
public:
    explicit CoinManager(ViewProviderSketch &vp, EditData * editdata);
    ~CoinManager();

     /** @name Temporary edit curves and markers */
    //@{
    void drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel);
    void drawEdit(const std::vector<Base::Vector2d> &EditCurve);
    //@}

    /** @name handle preselection and selection of points */
    //@{
    void setPreselectPoint(int PreselectPoint);
    void resetPreselectPoint(void);
    void addSelectPoint(int SelectPoint);
    void removeSelectPoint(int SelectPoint);
    void clearSelectPoints(void);
    //@}

    /** @name update coin nodes*/
    void processGeometryConstraintsInformationOverlay(const GeoList & geolist, bool rebuildinformationlayer);

    void updateVirtualSpace();
    //@}

    /** @name coin nodes creation*/
    void createEditModeInventorNodes();
    void rebuildConstraintNodes(void);
    //@}

    /** @name update coin colors*/
    //@{
    void updateGeometryColor(const GeoListFacade & geolistfacade, bool issketchinvalid);
    void updateConstraintColor(std::vector<Sketcher::Constraint *> constraints);
    //@}


    /** @name change coin visualisation and behaviour*/
    //@{
    void setAxisPickStyle(bool on);
    void updateGridExtent();
    //@}

    /** @name Configuration of the visualisation */
    //@{
    void updateCoinManagerColors();
    //@}

    /** @name Analysis Results */
    //@{
    float getboundingBoxMagnitudeOrder() { return analysisResults.boundingBoxMagnitudeOrder;}
    //@}

private:
    // This function populates the coin nodes with the information of the current geometry
    void processGeometry(const GeoList & geolist);

    // geometry list to be used for constraints, which may be a temporal geometry
    void processConstraints(const GeoList & geolist);

    // This function populates the geometry information layer of coin. It requires the analysis information
    // gathered during the processGeometry step, so it is not possible to run both in parallel.
    void processGeometryInformationOverlay(const GeoList & geolist);

    // updates the Axes length to extend beyond the calculated bounding box magnitude
    void updateAxesLength();

    // updates the parameters to be used for the Overlay information layer
    void updateOverlayParameters();

    // causes the ViewProvider to draw
    void redrawViewProvider();

    void rebuildConstraintNodes(const GeoList & geolist, const std::vector<Sketcher::Constraint *> constrlist, SbVec3f norm);

    /// finds a free position for placing a constraint icon
    Base::Vector3d seekConstraintPosition(const Base::Vector3d &origPos,
                                          const Base::Vector3d &norm,
                                          const Base::Vector3d &dir, float step,
                                          const SoNode *constraint);

    /// Return display string for constraint including hiding units if
    //requested.
    QString getPresentationString(const Sketcher::Constraint *constraint);

private:
    ViewProviderSketch & viewProvider;
    std::unique_ptr<CoinManager::ParameterObserver> pObserver;

    EditData * edit;
    DrawingParameters drawingParameters;
    AnalysisResults analysisResults;
    OverlayParameters overlayParameters;

};


} // namespace SketcherGui


#endif // SKETCHERGUI_CoinManager_H

