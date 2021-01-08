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
# include <TopoDS.hxx>
# include <TopoDS_Wire.hxx>
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>

#include "TextGeometryExtension.h"

#include "TextGeometryExtensionPy.h"

using namespace Sketcher;

TYPESYSTEM_SOURCE(Sketcher::TextGeometryExtension,Part::GeometryPersistenceExtension)

std::unique_ptr<Part::GeometryExtension> TextGeometryExtension::copy(void) const
{
    auto cpy = std::make_unique<TextGeometryExtension>();

    copyAttributes(cpy.get());

#if defined (__GNUC__) && (__GNUC__ <=4)
    return std::move(cpy);
#else
    return cpy;
#endif
}

void TextGeometryExtension::copyAttributes(Part::GeometryExtension * cpy) const
{
    Part::GeometryPersistenceExtension::copyAttributes(cpy);
    static_cast<TextGeometryExtension *>(cpy)->shapeEnabled = this->shapeEnabled;
}

void TextGeometryExtension::restoreAttributes(Base::XMLReader &reader)
{
    Part::GeometryPersistenceExtension::restoreAttributes(reader);
    shapeEnabled = reader.getAttributeAsInteger("shapeOverride");
}

void TextGeometryExtension::saveAttributes(Base::Writer &writer) const
{
    Part::GeometryPersistenceExtension::saveAttributes(writer);
    writer.Stream() << "\" shapeOverride=\"" << shapeEnabled;
}

PyObject * TextGeometryExtension::getPyObject(void)
{
    return new TextGeometryExtensionPy(new TextGeometryExtension(*this));
}

TopoDS_Shape TextGeometryExtension::toShape() const
{
    if( shapeEnabled )
        return TopoDS_Wire(); // Placeholder. It can and most generally will be a TopoDS_Compound see commit text.
    else
        return TopoDS_Shape();

}
