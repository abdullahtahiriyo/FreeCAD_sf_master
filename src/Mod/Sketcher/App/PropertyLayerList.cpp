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

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Tools.h>

#include "PropertyLayerList.h"
#include "LayerPy.h"

using namespace App;
using namespace Base;
using namespace std;
using namespace Sketcher;


//**************************************************************************
// PropertyLayerList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(Sketcher::PropertyLayerList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyLayerList::~PropertyLayerList()
{
    for (std::vector<Layer*>::iterator it = _lValueList.begin(); it != _lValueList.end(); ++it)
        if (*it) delete *it;
}

void PropertyLayerList::setSize(int newSize)
{
    for (unsigned int i = newSize; i < _lValueList.size(); i++)
        delete _lValueList[i];

    /* Resize array to new size */
    _lValueList.resize(newSize);
}

int PropertyLayerList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyLayerList::set1Value(const int idx, const Layer* lValue)
{
    if (lValue) {
        aboutToSetValue();
        Layer* oldVal = _lValueList[idx];
        Layer* newVal = lValue->clone();

        _lValueList[idx] = newVal;

        delete oldVal;
        hasSetValue();
    }
}

void PropertyLayerList::setValue(const Layer* lValue)
{
    if (lValue) {
        aboutToSetValue();
        Layer* newVal = lValue->clone();

        // Cleanup
        for (unsigned int i = 0; i < _lValueList.size(); i++)
            delete _lValueList[i];

        /* Set new data */
        _lValueList.resize(1);
        _lValueList[0] = newVal;
        hasSetValue();
    }
}

void PropertyLayerList::setValues(const std::vector<Layer*>& lValue)
{
    auto copy = lValue;
    for(auto &cstr : copy)
        cstr = cstr->clone();

    setValues(std::move(copy));
}

void PropertyLayerList::setValues(std::vector<Layer*>&& lValue) {
    aboutToSetValue();
    applyValues(std::move(lValue));
    hasSetValue();
}

void PropertyLayerList::applyValues(std::vector<Layer*>&& lValue)
{
    std::set<Layer*> oldVals(_lValueList.begin(),_lValueList.end());

    _lValueList = std::move(lValue);

    /* Clean-up */
    for(auto &v : oldVals)
        delete v;
}

PyObject *PropertyLayerList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyLayerList::setPyObject(PyObject *value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<Layer*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PyList_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(LayerPy::Type))) {
                std::string error = std::string("types in list must be 'Layer', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<LayerPy*>(item)->getLayerPtr();
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(LayerPy::Type))) {
        LayerPy *pcObject = static_cast<LayerPy*>(value);
        setValue(pcObject->getLayerPtr());
    }
    else {
        std::string error = std::string("type must be 'Layer' or list of 'Layer', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyLayerList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<LayerList count=\"" << getSize() <<"\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++)
        _lValueList[i]->Save(writer);
    writer.decInd();
    writer.Stream() << writer.ind() << "</LayerList>" << endl ;
}

void PropertyLayerList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LayerList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<Layer*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        Layer *newC = new Layer();
        newC->Restore(reader);

        values.push_back(newC);
    }

    reader.readEndElement("LayerList");

    // assignment
    setValues(std::move(values));
}

Property *PropertyLayerList::Copy(void) const
{
    PropertyLayerList *p = new PropertyLayerList();
    p->setValues(_lValueList);
    return p;
}

void PropertyLayerList::Paste(const Property &from)
{
    const PropertyLayerList& FromList = dynamic_cast<const PropertyLayerList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyLayerList::getMemSize(void) const
{
    int size = sizeof(PropertyLayerList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
