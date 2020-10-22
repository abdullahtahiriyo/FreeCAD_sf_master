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
# include <sstream>
#endif

#include "Layer.h"
#include "LayerPy.h"
#include "LayerPy.cpp"

using namespace Sketcher;

PyObject *LayerPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of LayerPy and the Twin object
    return new LayerPy(new Layer);
}

// constructor method
int LayerPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();

    char *LayerName;
    int  Identifier = 0;

    if (PyArg_ParseTuple(args, "si", &LayerName, &Identifier)) {
        this->getLayerPtr()->setId(Identifier);
        this->getLayerPtr()->setName(LayerName);
        return 0;
    }

    std::stringstream str;
    str << "Invalid parameters: ";
    Py::Tuple tuple(args);
    str << tuple.as_string() << std::endl;
    str << "Layer constructor accepts:" << std::endl
        << "-- empty parameter list" << std::endl
        << "-- Layer identifier and name" << std::endl;

    PyErr_SetString(PyExc_TypeError, str.str().c_str());
    return -1;
}

// returns a string which represents the object e.g. when printed in python
std::string LayerPy::representation(void) const
{
    std::stringstream result;
    result << "<Layer Id=" << getLayerPtr()->getId() << " Name=" << getLayerPtr()->getName() << ">";

    return result.str();
}

Py::String LayerPy::getName(void) const
{
    return Py::String(getLayerPtr()->getName());
}

void LayerPy::setName(Py::String arg)
{
    this->getLayerPtr()->setName(arg);
}

Py::Long LayerPy::getId(void) const
{
    return Py::Long(this->getLayerPtr()->getId());
}

void LayerPy::setId(Py::Long arg)
{
    this->getLayerPtr()->setId(arg);
}

PyObject *LayerPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int LayerPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
