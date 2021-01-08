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

#include "TextGeometryExtensionPy.h"
#include "TextGeometryExtensionPy.cpp"

using namespace Sketcher;

// returns a string which represents the object e.g. when printed in python
std::string TextGeometryExtensionPy::representation(void) const
{
    std::stringstream str;
    bool override = getTextGeometryExtensionPtr()->isShapeEnabled();
    str << "<TextGeometryExtension (" ;

    if(getTextGeometryExtensionPtr()->getName().size()>0)
        str << "\'" << getTextGeometryExtensionPtr()->getName() << "\', ";

    str << (override?"TextEnabled":"TextDisabled") << ") >";

    return str.str();
}

PyObject *TextGeometryExtensionPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of the python object and the Twin object
    return new TextGeometryExtensionPy(new TextGeometryExtension);
}

// constructor method
int TextGeometryExtensionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default extension
        return 0;
    }

    PyErr_Clear();
    PyObject * val;
    if (PyArg_ParseTuple(args, "O!", &PyBool_Type, &val)) {
        this->getTextGeometryExtensionPtr()->setShapeEnabled( PyObject_IsTrue(val) ? true : false);
        return 0;
    }

    PyErr_Clear();
    char * pystr;
    if (PyArg_ParseTuple(args, "O!s",  &PyBool_Type, &val, &pystr)) {
        this->getTextGeometryExtensionPtr()->setShapeEnabled( PyObject_IsTrue(val) ? true : false);
        this->getTextGeometryExtensionPtr()->setName(pystr);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "TextGeometryExtension constructor accepts:\n"
    "-- empty parameter list\n"
    "-- boolean\n"
    "-- boolean, string\n");
    return -1;
}

Py::Boolean TextGeometryExtensionPy::getShapeOverrideEnabled(void) const
{
    return Py::Boolean(this->getTextGeometryExtensionPtr()->isShapeEnabled());
}

void TextGeometryExtensionPy::setShapeOverrideEnabled(Py::Boolean value)
{
    this->getTextGeometryExtensionPtr()->setShapeEnabled(value);
}

PyObject *TextGeometryExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TextGeometryExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
