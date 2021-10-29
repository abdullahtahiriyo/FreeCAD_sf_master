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

namespace SketcherGui {

struct EditData;

struct GeoList {
    const std::vector<Part::Geometry *> & geomlist;
    int intGeoCount;
    int extGeoCount;
};

class SketcherGuiExport CoinManager
{
    // Delegate Pattern (Attorney - Client)
    // Monitor changes in parameters affecting drawing
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
        ParameterObserver(CoinManager * pclient);
        ~ParameterObserver();

        void subscribeToParameters();

        void unsubscribeToParameters();

        /** Observer for parameter group. */
        void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;

    private:
        void initParameters();
        void updateCurvedEdgeCountSegmentsParameter();

        template<OverlayVisibilityParameter visibilityparameter>
        void updateOverlayVisibilityParameter();

    private:
        CoinManager *pClient;
    };

    struct AnalysisResuls { // TODO: This needs to be refactored
        double combRepresentationScale = 0;
        float boundingBoxMagnitudeOrder = 0;
        std::vector<int> bsplineGeoIds;

    };

public:
    explicit CoinManager(EditData * editdata);
    ~CoinManager();

    using Vector3d = Base::Vector3<double>;

    void processGeometryAndInformationOverlay(const GeoList & geolist, bool rebuildinformationlayer);

    void drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel);
    void drawEdit(const std::vector<Base::Vector2d> &EditCurve);

    void updateCoinManagerColors();

    float getboundingBoxMagnitudeOrder() { return analysisResults.boundingBoxMagnitudeOrder;}

private:
    // This function populates the coin nodes with the information of the current geometry
    void processGeometry(const GeoList & geolist);

    // This function populates the geometry information layer of coin. It requires the analysis information
    // gathered during the processGeometry step, so it is not possible to run both in parallel.
    void processGeometryInformationOverlay(const GeoList & geolist);

    // updates the Axes length to extend beyond the calculated bounding box magnitude
    void updateAxesLength();

    // updates the parameters to be used for the Overlay information layer
    void updateOverlayParameters();

private:
    EditData * edit;
    DrawingParameters drawingParameters;
    AnalysisResuls analysisResults;
    OverlayParameters overlayParameters;
    std::unique_ptr<CoinManager::ParameterObserver> pObserver;
};


} // namespace SketcherGui


#endif // SKETCHERGUI_CoinManager_H

