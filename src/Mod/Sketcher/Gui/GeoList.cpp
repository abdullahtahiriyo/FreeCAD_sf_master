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


#include "PreCompiled.h"

#ifndef _PreComp_

#endif  // #ifndef _PreComp_

#include <assert.h>

#include "GeoList.h"

using namespace SketcherGui;

GeoList::GeoList(   const std::vector<Part::Geometry *> & geometrylist,
                    int intgeocount ):  geomlist(geometrylist),
                                        intGeoCount(intgeocount){

}

int GeoList::getGeoIdFromGeomListIndex(int index) const
{
    assert(index < int(geomlist.size()));

    if(index < intGeoCount)
        return index;
    else
        return -( index - intGeoCount);
}

// this function is used to simulate cyclic periodic negative geometry indices (for external geometry)
const Part::Geometry* GeoList::getGeometryFromGeoId(int geoId) const
{
    return GeoList::getGeometryFromGeoId (geomlist,geoId);
}

const Part::Geometry* GeoList::getGeometryFromGeoId(const std::vector<Part::Geometry*> geometrylist, int geoId)
{
    if (geoId >= 0)
        return geometrylist[geoId];
    else
        return geometrylist[geometrylist.size()+geoId];
}
