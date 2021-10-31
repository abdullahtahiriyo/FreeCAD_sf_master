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


#ifndef SKETCHERGUI_GeoList_H
#define SKETCHERGUI_GeoList_H

#ifndef _PreComp_

#endif  // #ifndef _PreComp_

#include <vector>


namespace Part {
    class Geometry;
}

namespace SketcherGui {

/** @brief      Class for managing internal and external geometry as a single object
 *  @details
 *  Internal and external geometries are present in a single geometry vector one after the other.
 *
 */
class GeoList {

public:
    explicit GeoList( const std::vector<Part::Geometry *> & geometrylist, int intgeocount);

    /// returns the geometry given by the GeoId
    const Part::Geometry* getGeometryFromGeoId(int geoId) const;

    int getGeoIdFromGeomListIndex(int index) const;

    /// returns the geometry given by the GeoId from the given geometrylist vector containing internal and
    /// external geometry in a geomlist type of structure
    static const Part::Geometry* getGeometryFromGeoId(const std::vector<Part::Geometry*> geometrylist, int geoId);

    int getInternalCount() const { return intGeoCount;}

    int getExternalCount() const { return int(geomlist.size()) - intGeoCount;}

public:
    const std::vector<Part::Geometry *> & geomlist;
private:
    int intGeoCount;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_GeoList_H

