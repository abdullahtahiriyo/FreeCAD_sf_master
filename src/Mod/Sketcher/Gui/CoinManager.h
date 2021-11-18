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

struct EditData;
class ViewProviderSketch;

using GeoList = Sketcher::GeoList;
using GeoListFacade = Sketcher::GeoListFacade;

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
    static inline bool constraintHasExpression(const ViewProviderSketch &vp, int constrid);
    static inline const std::vector<Sketcher::Constraint *> getConstraints(const ViewProviderSketch & vp);
    static inline const GeoList getGeoList(const ViewProviderSketch & vp);
    static inline const GeoListFacade getGeoListFacade(const ViewProviderSketch & vp);
    static inline Base::Placement getEditingPlacement(const ViewProviderSketch & vp);
    static inline void updateGridExtent(ViewProviderSketch & vp, float minx, float maxx, float miny, float maxy);
    static inline bool isShownVirtualSpace(const ViewProviderSketch & vp);
    static inline std::unique_ptr<SoRayPickAction> getRayPickAction(const ViewProviderSketch & vp);

    static inline float getScaleFactor(const ViewProviderSketch & vp);
    static inline SbVec2f getScreenCoordinates(const ViewProviderSketch & vp, SbVec2f sketchcoordinates);
    static inline QFont getApplicationFont(const ViewProviderSketch & vp);
    static inline double getRotation(const ViewProviderSketch & vp, SbVec3f pos0, SbVec3f pos1);
    static inline int defaultApplicationFontSizePixels(const ViewProviderSketch & vp);
    static inline int getApplicationLogicalDPIX(const ViewProviderSketch & vp);

    static inline bool isSketchInvalid(const ViewProviderSketch & vp);
    static inline bool haveConstraintsInvalidGeometry(const ViewProviderSketch & vp);

    static inline void addNodeToRoot(ViewProviderSketch & vp, SoSeparator * node);

    static inline void removeNodeFromRoot(ViewProviderSketch & vp, SoSeparator * node);

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
        void updateConstraintPresentationParameters();
        void updateElementSizeParameters();

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
    void updateColor();
    void updateColor(const GeoList & geolist); // overload to be used with temporal geometry.
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

    PreselectionResult detectPreselection(SoPickedPoint * Point, const SbVec2s &cursorPos);

    /// Draw all constraint icons
    /*! Except maybe the radius and lock ones? */
    void drawConstraintIcons();

    // This specific overload is to use a specific geometry list, which may be a temporal one
    void drawConstraintIcons(const GeoList & geolist);

    void setPositionText(const Base::Vector2d &Pos, const SbString &txt);
    void setPositionText(const Base::Vector2d &Pos);
    void resetPositionText(void);

    /// The client is responsible for unref-ing the SoGroup to release the memory.
    SoGroup* getSelectedConstraints();

    SoSeparator* getRootEditNode();

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

    void updateGeometryColor(const GeoListFacade & geolistfacade, bool issketchinvalid);
    void updateConstraintColor(const std::vector<Sketcher::Constraint *> & constraints);

    // causes the ViewProvider to draw
    void redrawViewProvider();

    void rebuildConstraintNodes(const GeoList & geolist); // with specific geometry

    void rebuildConstraintNodes(const GeoList & geolist, const std::vector<Sketcher::Constraint *> constrlist, SbVec3f norm);

    /// finds a free position for placing a constraint icon
    Base::Vector3d seekConstraintPosition(const Base::Vector3d &origPos,
                                          const Base::Vector3d &norm,
                                          const Base::Vector3d &dir, float step,
                                          const SoNode *constraint);

    /// Return display string for constraint including hiding units if
    //requested.
    QString getPresentationString(const Sketcher::Constraint *constraint);

    std::set<int> detectPreselectionConstr( const SoPickedPoint *Point,
                                            const SbVec2s &cursorPos);

    /// Returns the size that Coin should display the indicated image at
    SbVec3s getDisplayedSize(const SoImage *) const;

    /** @name Protected helpers for drawing constraint icons*/
    //@{
    QString iconTypeFromConstraint(Sketcher::Constraint *constraint);

    /// Returns a QColor object appropriate for constraint with given id
    /*! In the case of combined icons, the icon color is chosen based on
     *  the constraint with the highest priority from constrColorPriority()
     */
    QColor constrColor(int constraintId);
    /// Used by drawMergedConstraintIcons to decide what color to make icons
    /*! See constrColor() */
    int constrColorPriority(int constraintId);

    /// Internal type used for drawing constraint icons
    struct constrIconQueueItem {
        /// Type of constraint the icon represents.  Eg: "small/Constraint_PointOnObject_sm"
        QString type;

        /// Internal constraint ID number
        /// These map to results of getSketchObject()->Constraints.getValues()
        int constraintId;

        /// Label to be rendered with this icon, if any
        QString label;

        /// Absolute coordinates of the constraint icon
        SbVec3f position;

        /// Pointer to the SoImage object where the icon should be written
        SoImage *destination;

        /// Pointer to SoInfo object where we store the constraint IDs that the icon refers to
        SoInfo *infoPtr;

        /// Angle to rotate an icon
        double iconRotation;

        bool visible;
    };

    /// Internal type used for drawing constraint icons
    typedef std::vector<constrIconQueueItem> IconQueue;
    /// For constraint icon bounding boxes
    typedef std::pair<QRect, std::set<int> > ConstrIconBB;
    /// For constraint icon bounding boxes
    typedef std::vector<ConstrIconBB> ConstrIconBBVec;

    void combineConstraintIcons(IconQueue iconQueue);

    /// Renders an icon for a single constraint and sends it to Coin
    void drawTypicalConstraintIcon(const constrIconQueueItem &i);

    /// Combines multiple constraint icons and sends them to Coin
    void drawMergedConstraintIcons(IconQueue iconQueue);

    /// Helper for drawMergedConstraintIcons and drawTypicalConstraintIcon
    QImage renderConstrIcon(const QString &type,
                            const QColor &iconColor,
                            const QStringList &labels,
                            const QList<QColor> &labelColors,
                            double iconRotation,
                            //! Gets populated with bounding boxes (in icon
                            //! image coordinates) for the icon at left, then
                            //! labels for different constraints.
                            std::vector<QRect> *boundingBoxes = NULL,
                            //! If not NULL, gets set to the number of pixels
                            //! that the text extends below the icon base.
                            int *vPad = NULL);

    /// Copies a QImage constraint icon into a SoImage*
    /*! Used by drawTypicalConstraintIcon() and drawMergedConstraintIcons() */
    void sendConstraintIconToCoin(const QImage &icon, SoImage *soImagePtr);

    /// Essentially a version of sendConstraintIconToCoin, with a blank icon
    void clearCoinImage(SoImage *soImagePtr);
    //@}

    int defaultApplicationFontSizePixels() const;

    int getApplicationLogicalDPIX() const;

    void updateInventorNodeSizes();

    SoSeparator * getConstraintIdSeparator(int i);

private:
    ViewProviderSketch & viewProvider;
    std::unique_ptr<CoinManager::ParameterObserver> pObserver;

    EditData * edit;
    DrawingParameters drawingParameters;
    AnalysisResults analysisResults;
    OverlayParameters overlayParameters;
    ConstraintParameters constraintParameters;

    EditModeScenegraphNodes editModeScenegraphNodes;

};


} // namespace SketcherGui


#endif // SKETCHERGUI_CoinManager_H

