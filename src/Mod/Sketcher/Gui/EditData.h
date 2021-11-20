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

#ifndef SKETCHERGUI_EditData_H
#define SKETCHERGUI_EditData_H

#include <set>
#include <vector>
#include <map>

#include <qrect.h>

namespace Sketcher {
    enum ConstraintType : int;
    enum PointPos : int;
}

namespace SketcherGui {

    class DrawSketchHandler;
    class ViewProviderSketch;


//**************************************************************************
// Edit data structure

/// Data structure while editing the sketch
struct EditData {
    EditData();
    // pointer to the active handler for new sketch objects





    int PreselectPoint;
    int PreselectCurve;
    int PreselectCross;
    std::set<int> PreselectConstraintSet;
    bool blockedPreselection;

    // container to track our own selected parts
    std::set<int> SelPointSet;
    std::set<int> SelCurvSet; // also holds cross axes at -1 and -2
    std::set<int> SelConstraintSet;

    std::vector<int> CurvIdToGeoId; // conversion of SoLineSet index to GeoId
    std::vector<int> PointIdToGeoId; // conversion of SoCoordinate3 index to GeoId
    std::map<std::pair<int, Sketcher::PointPos>, int> GeoIdPointPosToPointId; // conversion of [GeoId,Pos] to PointId

    // helper data structures for the constraint rendering
    std::vector<Sketcher::ConstraintType> vConstrType;

    // For each of the combined constraint icons drawn, also create a vector
    // of bounding boxes and associated constraint IDs, to go from the icon's
    // pixel coordinates to the relevant constraint IDs.
    //
    // The outside map goes from a string representation of a set of constraint
    // icons (like the one used by the constraint IDs we insert into the Coin
    // rendering tree) to a vector of those bounding boxes paired with relevant
    // constraint IDs.

    using ConstrIconBB = std::pair<QRect, std::set<int> >;
    using ConstrIconBBVec = std::vector<ConstrIconBB>;

    std::map<QString, ConstrIconBBVec> combinedConstrBoxes;

    // nodes for the visuals

};

} // namespace SketcherGui


#endif // SKETCHERGUI_EditData_H

