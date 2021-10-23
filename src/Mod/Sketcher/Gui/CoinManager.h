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

    private:
        CoinManager *pClient;
    };

    struct DrawingParameters {
        int curvedEdgeCountSegments;
        // Rendering Heights
        const float zLowLines   = 0.005f;       //TODO: Fix zLowLines
        const float zLowPoints  = 0.010f;    // TODO: Fix zLowPoints
        const float zInfo       = 0.004f;
        const float zEdit       = 0.001f;
        // Rendering Colors
        static SbColor InformationColor;
        static SbColor CreateCurveColor;
    };

    struct VisualisationControlParameters {
        double currentBSplineCombRepresentationScale = 0;
        bool visibleInformationChanged = true;
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

    // This function populates the coin nodes with the information of the current geometry
    void processGeometry(const GeoList & geolist);

    // This function populates the geometry information layer of coin. It requires the analysis information
    // gathered during the processGeometry step, so it is not possible to run both in parallel.
    void processGeometryInformationLayer(const GeoList & geolist, bool rebuildinformationlayer);

    void drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel);
    void drawEdit(const std::vector<Base::Vector2d> &EditCurve);

    inline void setVisibleInformationChanged() {visualisationControlParameters.visibleInformationChanged = true;}
    void updateCoinManagerColors();

private:
    EditData * edit;
    DrawingParameters drawingParameters;
    VisualisationControlParameters visualisationControlParameters;
    AnalysisResuls analysisResults;
    std::unique_ptr<CoinManager::ParameterObserver> pObserver;
};


} // namespace SketcherGui


#endif // SKETCHERGUI_CoinManager_H

