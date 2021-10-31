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
 * N.B.: Note that the index of the geomlist (all layers) and the GeoId can be converted
 * from each other at needed using the member fuctions (and sometimes the statics).
 */
class GeoList {

public:
    /**
    * Constructs the object from a list of geometry in geomlist format and the number of internal
    * geometries (non external) present in the list.
    *
    * @param geometrylist: the geometry in geomlist format (external after internal in a single vector).
    */
    explicit GeoList( const std::vector<Part::Geometry *> & geometrylist, int intgeocount);

    /**
    * returns the geometry given by the GeoId
    */
    const Part::Geometry* getGeometryFromGeoId(int geoId) const;

    /**
    * returns the GeoId index from the index in the geometry in geomlist format with which it was constructed.
    *
    * @param index: the index of the list of geometry in geomlist format.
    */
    int getGeoIdFromGeomListIndex(int index) const;

    /**
    * returns the geometry given by the GeoId in the geometrylist in geomlist format provided as a parameter.
    *
    * @param geometrylist: the geometry in geomlist format (external after internal in a single vector).
    *
    * @param index: the index of the list of geometry in geomlist format.
    */
    static const Part::Geometry* getGeometryFromGeoId(const std::vector<Part::Geometry*> geometrylist, int geoId);

    /**
    * returns the amount of internal geometry objects.
    */
    int getInternalCount() const { return intGeoCount;}

    /**
    * returns the amount of external geometry objects.
    */
    int getExternalCount() const { return int(geomlist.size()) - intGeoCount;}

public:
    const std::vector<Part::Geometry *> & geomlist;
private:
    int intGeoCount;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_GeoList_H

