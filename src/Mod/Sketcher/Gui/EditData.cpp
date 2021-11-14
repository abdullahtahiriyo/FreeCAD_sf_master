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

# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoGroup.h>
# include <Gui/Inventor/SmSwitchboard.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>

# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoDrawStyle.h>

# include <QCursor>
# include <QPixmap>

#endif

#include "DrawSketchHandler.h"

#include "EditData.h"

#include <Mod/Part/App/Geometry.h>


using namespace SketcherGui;

//**************************************************************************
// Construction/Destruction

EditData::EditData():
    sketchHandler(0),
    buttonPress(false),
    handleEscapeButton(false),
    DragPoint(-1),
    DragCurve(-1),
    PreselectPoint(-1),
    PreselectCurve(-1),
    PreselectCross(-1),
    blockedPreselection(false),
    FullyConstrained(false),
    //ActSketch(0), // if you are wondering, it went to SketchObject, accessible via getSolvedSketch() and via SketchObject interface as appropriate
    EditRoot(0),
    PointsMaterials(0),
    CurvesMaterials(0),
    RootCrossMaterials(0),
    EditCurvesMaterials(0),
    EditMarkersMaterials(0),
    PointsCoordinate(0),
    CurvesCoordinate(0),
    RootCrossCoordinate(0),
    EditCurvesCoordinate(0),
    EditMarkersCoordinate(0),
    CurveSet(0),
    RootCrossSet(0),
    EditCurveSet(0),
    EditMarkerSet(0),
    PointSet(0),
    textX(0),
    textPos(0),
    constrGroup(0),
    infoGroup(0),
    pickStyleAxes(0),
    PointsDrawStyle(0),
    CurvesDrawStyle(0),
    RootCrossDrawStyle(0),
    EditCurvesDrawStyle(0),
    EditMarkersDrawStyle(0),
    ConstraintDrawStyle(0),
    InformationDrawStyle(0)
    {}
