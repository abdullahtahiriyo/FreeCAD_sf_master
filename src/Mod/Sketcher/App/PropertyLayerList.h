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


#ifndef APP_PropertyLayerList_H
#define APP_PropertyLayerList_H

// Std. configurations


#include <vector>
#include <string>
#include <App/Property.h>
#include "Layer.h"

namespace Base {
class Writer;
}

namespace Sketcher
{
class Layer;

class SketcherExport PropertyLayerList : public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyLayerList() = default;

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyLayerList();

    virtual void setSize(int newSize) override;
    virtual int getSize(void) const override;

    /*!
      Sets a single layer to the property at a certain
      position. The value is cloned internally so it's in the
      responsibility of the caller to free the memory.
    */
    void set1Value(const int idx, const Layer*);
    /*!
      Sets a single layer to the property.
      The value is cloned internally so it's in the
      responsibility of the caller to free the memory.
    */
    void setValue(const Layer*);
    /*!
      Sets a vector of layer to the property.
      The values of the array are cloned internally so it's
      in the responsibility of the caller to free the memory.
    */
    void setValues(const std::vector<Layer*>&);

    /*!
      Sets a vector of layer to the property.
      The values of the array are moved, and the ownership of layers
      inside are taken by this property
    */
    void setValues(std::vector<Layer*>&&);

    /*!
     Index operator
    */
    const Layer *operator[] (const int idx) const {
        return _lValueList[idx];
    }

    const std::vector<Layer*> &getValues(void) const {
        return _lValueList;
    }

    virtual PyObject *getPyObject(void) override;
    virtual void setPyObject(PyObject *) override;

    virtual void Save(Base::Writer &writer) const override;
    virtual void Restore(Base::XMLReader &reader) override;

    virtual Property *Copy(void) const override;
    virtual void Paste(const App::Property &from) override;

    virtual unsigned int getMemSize(void) const override;

private:
    void applyValues(std::vector<Layer *>&&);

private:
    std::vector<Layer *> _lValueList;
};


} // namespace Sketcher


#endif // APP_PropertyLayerList_H
