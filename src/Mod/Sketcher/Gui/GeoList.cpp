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

#include <Mod/Sketcher/App/GeometryFacade.h>

#include "GeoList.h"

using namespace SketcherGui;

template <typename T>
GeoListModel<T>::GeoListModel(  const std::vector<T> & geometrylist,
                                    int intgeocount ):  geomlist(geometrylist),
                                                        intGeoCount(intgeocount){

}

template <typename T>
int GeoListModel<T>::getGeoIdFromGeomListIndex(int index) const
{
    assert(index < int(geomlist.size()));

    if(index < intGeoCount)
        return index;
    else
        return -( index - intGeoCount);
}

template <typename T>
const T GeoListModel<T>::getGeometryFromGeoId(const std::vector<T> & geometrylist, int geoId)
{
    if (geoId >= 0)
        return geometrylist[geoId];
    else
        return geometrylist[geometrylist.size()+geoId];
}

// this function is used to simulate cyclic periodic negative geometry indices (for external geometry)
template <typename T>
const T GeoListModel<T>::getGeometryFromGeoId(int geoId) const
{
    return GeoListModel<T>::getGeometryFromGeoId(geomlist, geoId);
}


namespace SketcherGui {

// Template specialisations

template < >
const std::unique_ptr<const Sketcher::GeometryFacade>
GeoListModel<std::unique_ptr<const Sketcher::GeometryFacade>>::getGeometryFromGeoId
    (const std::vector<std::unique_ptr<const Sketcher::GeometryFacade>> & geometrylist, int geoId)
{
    if (geoId >= 0)
        return Sketcher::GeometryFacade::getFacade(geometrylist[geoId]->getGeometry());
    else
        return Sketcher::GeometryFacade::getFacade(geometrylist[geometrylist.size()+geoId]->getGeometry());
}


// instantiate the types so that other translation units can access template constructors
template class GeoListModel<Part::Geometry *>;
template class GeoListModel<std::unique_ptr<const Sketcher::GeometryFacade>>;


} // namespace SketcherGui
