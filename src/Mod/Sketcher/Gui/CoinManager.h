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


namespace Base {
    template< typename T >
    class Vector3;

    class Vector2d;
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


class ViewProviderSketchCoinAttorney {
private:
    static inline bool constraintHasExpression(ViewProviderSketch &vp, int constrid);

    friend class CoinManager;
};




/** @brief      Class for managing the Coin nodes of ViewProviderSketch.
 *  @details    To be documented.
 *
 */
class SketcherGuiExport CoinManager
{
    // Monitor changes in parameters affecting drawing and coin node generation
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

    using Vector3d = Base::Vector3<double>;

    void processGeometryAndInformationOverlay(const GeoList & geolist, bool rebuildinformationlayer);

    void drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel);
    void drawEdit(const std::vector<Base::Vector2d> &EditCurve);

    /** @name handle preselection and selection of points */
    //@{
    //
    void setPreselectPoint(int PreselectPoint);
    void resetPreselectPoint(void);
    void addSelectPoint(int SelectPoint);
    void removeSelectPoint(int SelectPoint);
    void clearSelectPoints(void);
    //@}

    void updateCoinManagerColors();

    float getboundingBoxMagnitudeOrder() { return analysisResults.boundingBoxMagnitudeOrder;}

    void createEditModeInventorNodes();

    void updateGeometryColor(const GeoListFacade & geolistfacade, bool issketchinvalid);

    void updateConstraintColor(std::vector<Sketcher::Constraint *> constraints);

private:
    // This function populates the coin nodes with the information of the current geometry
    void processGeometry(const GeoList & geolist);

    void processConstraints();

    // This function populates the geometry information layer of coin. It requires the analysis information
    // gathered during the processGeometry step, so it is not possible to run both in parallel.
    void processGeometryInformationOverlay(const GeoList & geolist);

    // updates the Axes length to extend beyond the calculated bounding box magnitude
    void updateAxesLength();

    // updates the parameters to be used for the Overlay information layer
    void updateOverlayParameters();

    // causes the ViewProvider to draw
    void redrawViewProvider();

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

