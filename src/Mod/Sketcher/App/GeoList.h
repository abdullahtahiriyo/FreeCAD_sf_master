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


#ifndef SKETCHER_GeoList_H
#define SKETCHER_GeoList_H

#ifndef _PreComp_

#endif  // #ifndef _PreComp_

#include <vector>
#include <memory>

#include <Mod/Sketcher/App/GeometryFacade.h>

namespace Base {
    template< typename T >
    class Vector3;
}

namespace Part {
    class Geometry;
}

namespace Sketcher {
    enum PointPos : int;

    class GeometryFacade;
}

namespace Sketcher {

// TODO: This class is half-cooked and needs to be reviewed. Specially the const/non-const aspect
// as well as the ability to take ownership of deepcopied vectors.

/** @brief      Class for managing internal and external geometry as a single object
 *  @details
 *  Internal and external geometries are present in a single geometry vector one after the other.
 *
 * N.B.: Note that the index of the geomlist (all layers) and the GeoId can be converted
 * from each other at needed using the member fuctions (and sometimes the statics).
 */
template <typename T>
class GeoListModel {
    using Vector3d = Base::Vector3<double>;


protected:
    /**
    * Constructs the object from a list of geometry in geomlist format and the number of internal
    * geometries (non external) present in the list.
    *
    * @param geometrylist: the geometry in geomlist format (external after internal in a single vector).
    * @param intgeocount: the number of internal geometries (non external) in the list.
    * @param ownerT: indicates whether the GeoListModel takes ownership of the elements of the std::vector<T> (for pointers)
    */
    inline explicit GeoListModel(std::vector<T> && geometrylist, int intgeocount, bool ownerT = false);

    inline explicit GeoListModel(const std::vector<T> & geometrylist, int intgeocount, bool ownerT = false);

public:
    ~GeoListModel();

    GeoListModel(GeoListModel &&) = default;

    /**
     * GeoListModel manages the lifetime of its internal std::vector. This means that while the actual ownership
     * of the T parameter needs to be specified or separately handled, a new vector will be created and the T elements
     * shallow copied to the internal vector.
     *
     * The constness of the GeoListModel is tied to the constness of the std::vector from which it is constructed,
     * except when the vector is not const, but the user uses the factory method to create a const model.
     */
    static GeoListModel<T> getGeoListModel(std::vector<T> && geometrylist, int intgeocount, bool ownerT = false);
    static const GeoListModel<T> getGeoListModel(const std::vector<T> & geometrylist, int intgeocount, bool ownerT = false);


    /**
    * returns the geometry given by the GeoId
    */
    const T getGeometryFromGeoId(int geoId) const;

    /**
    * returns the GeoId index from the index in the geometry in geomlist format with which it was constructed.
    *
    * @param index: the index of the list of geometry in geomlist format.
    */
    int getGeoIdFromGeomListIndex(int index) const;

    /**
    * returns the geometry given by the GeoId in the geometrylist in geomlist format provided as a parameter.
    *
    * @param geometrylist: the geometry in geomlist format (external after internal in a single vector).
    *
    * @param index: the index of the list of geometry in geomlist format.
    */
    static const T getGeometryFromGeoId(const std::vector<T> & geometrylist, int geoId);


    Vector3d getPoint(int geoId, Sketcher::PointPos pos) const;

    /**
    * returns the amount of internal geometry objects.
    */
    int getInternalCount() const { return intGeoCount;}

    /**
    * returns the amount of external geometry objects.
    */
    int getExternalCount() const { return int(geomlist.size()) - intGeoCount;}

    /**
     * return a reference to the internal geometry list vector.
     *
     * @warning { It returns a reference to the internal list vector. The validity of the
     * reference depends on the lifetime of the GeoListModel object.}
     */
     std::vector<T> & geometryList() { return const_cast<std::vector<T> &>(geomlist);}

public:
    std::vector<T> geomlist;

private:
    Vector3d getPoint(const Part::Geometry * geo, Sketcher::PointPos pos) const;

private:
    int intGeoCount;
    bool OwnerT;
};


// Vector is moved
template <typename T>
inline GeoListModel<T>::GeoListModel(  std::vector<T> && geometrylist,
                                int intgeocount,
                                bool ownerT): geomlist(std::move(geometrylist)),
                                              intGeoCount(intgeocount),
                                              OwnerT(ownerT)
{

}

// Vector is shallow copied (copy constructed)
template <typename T>
inline GeoListModel<T>::GeoListModel(  const std::vector<T> & geometrylist,
                                int intgeocount,
                                bool ownerT): geomlist(geometrylist), // copy constructed here
                                              intGeoCount(intgeocount),
                                              OwnerT(ownerT)
{

}


template <>
inline GeoListModel<std::unique_ptr< const Sketcher::GeometryFacade>>::GeoListModel(
                                std::vector<std::unique_ptr<const Sketcher::GeometryFacade>> && geometrylist,
                                int intgeocount,
                                bool ownerT) :  geomlist(std::move(geometrylist)),
                                                intGeoCount(intgeocount),
                                                OwnerT(false)
{
    // GeometryFacades hold the responsibility for releasing the resources.
    //
    // This means that each GeometryFacade that is passed to the GeoListModel,
    // must set the ownership (GF->setOwner(true)), if it should be the owner.
    // Otherwise, it follows the default behaviour that some other class, which
    // created the pointer, is responsible for freeing it.
    //
    // Under the Single Responsibility Principle GeoListModel cannot be made
    // responsible for releasing those pointers.
    assert(ownerT == false);
}

template <>
inline GeoListModel<std::unique_ptr< const Sketcher::GeometryFacade>>::GeoListModel(
                                const std::vector<std::unique_ptr< const Sketcher::GeometryFacade>> & geometrylist,
                                int intgeocount,
                                bool ownerT):   intGeoCount(intgeocount),
                                                OwnerT(false)
{
    // GeometryFacades are movable, but not copiable, so they need to be reconstructed (shallow copy of vector)
    // Under the Single Responsibility Principle, these will not take over a responsibility that shall be enforced
    // on the original GeometryFacade. Use the move version of getGeoListModel if moving the responsibility is intended.
    assert(ownerT == false);

    geomlist.reserve(geometrylist.size());

    for(auto & v : geometrylist) {
        geomlist.push_back(GeometryFacade::getFacade(v->getGeometry()));
    }
}

using GeoList = GeoListModel<Part::Geometry *>;
using GeoListFacade = GeoListModel<std::unique_ptr<const Sketcher::GeometryFacade>>;

} // namespace Sketcher


#endif // SKETCHER_GeoList_H

