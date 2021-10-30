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


#ifndef SKETCHERGUI_CoinManagerParameters_H
#define SKETCHERGUI_CoinManagerParameters_H

#ifndef _PreComp_
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/SbColor.h>
#endif  // #ifndef _PreComp_

#include <vector>
#include <map>


namespace Part {
    class Geometry;
}

namespace SketcherGui {

struct DrawingParameters {
    int curvedEdgeCountSegments;
    // Rendering Heights
    const float zLowLines   = 0.005f;    //TODO: Fix zLowLines
    const float zLowPoints  = 0.010f;    // TODO: Fix zLowPoints
    const float zInfo       = 0.004f;
    const float zEdit       = 0.001f;
    const float zCross      = 0.001f;
    // Rendering Colors
    static SbColor InformationColor;
    static SbColor CreateCurveColor;
    // Rendering font information
    int coinFontSize;
};

struct GeometryLayerNodes {
    SoMaterial    *PointsMaterials;
    SoMaterial    *CurvesMaterials;
    SoCoordinate3 *PointsCoordinate;
    SoCoordinate3 *CurvesCoordinate;
    SoLineSet     *CurveSet;
};

// geomlist is the list of all sketcher geometry (including all layers)
// geoId2geomlist has two responsibilities:
//  - The keys [GeoId] are the GeoIds of this layer (so the ones that will be processed)
//  - The mapping allows to get from the GeoId the
struct GeometryLayer {
    std::map<int /*GeoId*/, int /* geomlist index */> geoId2geomlist;
    const std::vector<Part::Geometry *> & geomlist;
};

struct OverlayParameters {
    bool rebuildInformationLayer;
    bool visibleInformationChanged = true;
    double currentBSplineCombRepresentationScale = 0;

    // Parameters
    bool bSplineDegreeVisible;
    bool bSplineControlPolygonVisible;
    bool bSplineCombVisible;
    bool bSplineKnotMultiplicityVisible;
    bool bSplinePoleWeightVisible;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_CoinManagerParameters_H

