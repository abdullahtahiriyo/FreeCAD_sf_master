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


#include "PreCompiled.h"
#ifndef _PreComp_

#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <App/Property.h>


#include "Layer.h"
#include "LayerPy.h"


using namespace Sketcher;
using namespace Base;


TYPESYSTEM_SOURCE(Sketcher::Layer, Base::Persistence)


Layer::Layer()
: Id(0),
  Name("")
{

}

Layer *Layer::clone(void) const
{
    return new Layer(*this);
}

PyObject *Layer::getPyObject(void)
{
    return new LayerPy(new Layer(*this));
}

unsigned int Layer::getMemSize (void) const
{
    return 0;
}

void Layer::Save (Writer &writer) const
{
    std::string encodeName = encodeAttribute(Name);
    writer.Stream() << writer.ind()     << "<Layer "
    << "Id=\""                          <<  Id                      << "\" "
    << "Name=\""                        <<  Name                    << "\" />"

    << std::endl;
}

void Layer::Restore(XMLReader &reader)
{
    reader.readElement("Layer");
    Id      = reader.getAttributeAsInteger("Id");
    Name      = reader.getAttribute("Name");
}
