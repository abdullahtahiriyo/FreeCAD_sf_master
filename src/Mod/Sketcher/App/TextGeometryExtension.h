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

#ifndef SKETCHER_TEXTGEOMETRYEXTENSION_H
#define SKETCHER_TEXTGEOMETRYEXTENSION_H

#include <Mod/Part/App/Geometry.h>

namespace Sketcher
{

class SketcherExport TextGeometryExtension : public Part::GeometryPersistenceExtension
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:

    TextGeometryExtension() = default;
    virtual ~TextGeometryExtension() override = default;

    virtual std::unique_ptr<Part::GeometryExtension> copy(void) const override;

    virtual PyObject *getPyObject(void) override;

    // Shape interface
    virtual TopoDS_Shape toShape() const;

    inline bool isShapeEnabled() const {return shapeEnabled;}
    inline void setShapeEnabled (bool on ) {shapeEnabled = on;}

    virtual void notifyAttachment(Part::Geometry * geo) override {geometry = geo;}

protected:
    virtual void copyAttributes(Part::GeometryExtension * cpy) const override;
    virtual void restoreAttributes(Base::XMLReader &reader) override;
    virtual void saveAttributes(Base::Writer &writer) const override;

private:
    TextGeometryExtension(const TextGeometryExtension&) = default;

private:
    bool shapeEnabled;
    Part::Geometry * geometry;
};

} //namespace Sketcher


#endif // SKETCHER_TEXTGEOMETRYEXTENSION_H
