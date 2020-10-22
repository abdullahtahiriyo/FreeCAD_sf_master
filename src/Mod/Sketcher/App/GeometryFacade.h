/***************************************************************************
 *   Copyright (c) 2020 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef SKETCHER_GEOMETRYFACADE_H
#define SKETCHER_GEOMETRYFACADE_H

#include <Base/Console.h> // Only for Debug - To be removed
#include <boost/uuid/uuid_io.hpp>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchGeometryExtension.h>

namespace Sketcher
{

// This class is a facade/wrapper to handle geometry and sketcher geometry extensions with a single sketcher specific interface
class SketcherExport GeometryFacade : ISketchGeometryExtension
{
public:
    GeometryFacade(Part::Geometry * geometry);

    // Identification information
    inline virtual long getId() const override {return SketchGeoExtension->getId();};
    virtual void setId(long id) override {SketchGeoExtension->setId(id);};

    // Layer information
    virtual int getLayerId() const override {return SketchGeoExtension->getLayerId();};
    virtual void setLayerId(int layerid) override {SketchGeoExtension->setLayerId(layerid);};

    template <  typename GeometryT = Part::Geometry,
                typename = typename std::enable_if<
                    std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value
             >::type
    >
    GeometryT * getGeometry() {return dynamic_cast<GeometryT>(Geo);}

private:
    Part::Geometry * Geo;
    std::shared_ptr<SketchGeometryExtension> SketchGeoExtension;
};





} //namespace Sketcher


#endif // SKETCHER_GEOMETRYFACADE_H
