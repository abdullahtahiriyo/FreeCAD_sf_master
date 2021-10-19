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


namespace Base {
    template< typename T >
    class Vector3;
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

public:
    explicit CoinManager(EditData * editdata);
    ~CoinManager();

    using Vector3d = Base::Vector3<double>;

    std::tuple<std::vector<Vector3d>/* Coords*/, std::vector<Vector3d> /*Points;*/, std::vector<unsigned int> /* Index */>
    processGeometry(const GeoList & geolist);



    EditData * edit;
};


} // namespace SketcherGui


#endif // SKETCHERGUI_CoinManager_H

