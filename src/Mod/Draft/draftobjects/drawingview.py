# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""This module provides the object code for the Draft DrawingView object.
This module is obsolete, since Drawing was substituted by TechDraw.
"""
## @package drawingview
# \ingroup DRAFT
# \brief This module provides the object code for the Draft DrawingView object.

import math

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App

import DraftVecUtils

from getSVG import getSVG

import draftutils.utils as utils

from draftobjects.base import DraftObject


class DrawingView(DraftObject):
    """The Draft DrawingView object
    
    OBSOLETE: this class is obsolete, since Drawing was substituted by TechDraw.
    """

    def __init__(self, obj):
        super(DrawingView, self).__init__(obj, "DrawingView")
        
        _tip = "The linked object"
        obj.addProperty("App::PropertyLink", "Source",
                        "Base", QT_TRANSLATE_NOOP("App::Property", _tip))
        
        _tip ="Projection direction"
        obj.addProperty("App::PropertyVector", "Direction",
                        "Shape View", QT_TRANSLATE_NOOP("App::Property", _tip))
        
        _tip = "The width of the lines inside this object"
        obj.addProperty("App::PropertyFloat", "LineWidth",
                        "View Style", QT_TRANSLATE_NOOP("App::Property", _tip))
                
        _tip = "The size of the texts inside this object"
        obj.addProperty("App::PropertyLength", "FontSize",
                        "View Style", QT_TRANSLATE_NOOP("App::Property", _tip))
                
        _tip = "The spacing between lines of text"
        obj.addProperty("App::PropertyLength", "LineSpacing",
                        "View Style", QT_TRANSLATE_NOOP("App::Property", _tip))
                
        _tip = "The color of the projected objects"
        obj.addProperty("App::PropertyColor", "LineColor",
                        "View Style", QT_TRANSLATE_NOOP("App::Property", _tip))
                
        _tip = "Shape Fill Style"
        obj.addProperty("App::PropertyEnumeration", "FillStyle",
                        "View Style", QT_TRANSLATE_NOOP("App::Property", _tip))
                
        _tip = "Line Style"
        obj.addProperty("App::PropertyEnumeration", "LineStyle",
                        "View Style", QT_TRANSLATE_NOOP("App::Property", _tip))
                
        _tip = "If checked, source objects are displayed regardless of being \
                visible in the 3D model"
        obj.addProperty("App::PropertyBool", "AlwaysOn",
                        "View Style", QT_TRANSLATE_NOOP("App::Property", _tip))

        obj.FillStyle = ['shape color'] + list(utils.svgpatterns().keys())
        obj.LineStyle = ['Solid','Dashed','Dotted','Dashdot']
        obj.LineWidth = 0.35
        obj.FontSize = 12

    def execute(self, obj):
        result = ""
        if hasattr(obj,"Source"):
            if obj.Source:
                if hasattr(obj,"LineStyle"):
                    ls = obj.LineStyle
                else:
                    ls = None
                if hasattr(obj,"LineColor"):
                    lc = obj.LineColor
                else:
                    lc = None
                if hasattr(obj,"LineSpacing"):
                    lp = obj.LineSpacing
                else:
                    lp = None
                if obj.Source.isDerivedFrom("App::DocumentObjectGroup"):
                    svg = ""
                    shapes = []
                    others = []
                    objs = utils.getGroupContents([obj.Source])
                    for o in objs:
                        v = o.ViewObject.isVisible()
                        if hasattr(obj,"AlwaysOn"):
                            if obj.AlwaysOn:
                                v = True
                        if v:
                            svg += getSVG(o,obj.Scale,obj.LineWidth,obj.FontSize.Value,obj.FillStyle,obj.Direction,ls,lc,lp)
                else:
                    svg = getSVG(obj.Source,obj.Scale,obj.LineWidth,obj.FontSize.Value,obj.FillStyle,obj.Direction,ls,lc,lp)
                result += '<g id="' + obj.Name + '"'
                result += ' transform="'
                result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
                result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
                result += 'scale('+str(obj.Scale)+','+str(-obj.Scale)+')'
                result += '">'
                result += svg
                result += '</g>'
        obj.ViewResult = result

    def getDXF(self,obj):
        "returns a DXF fragment"
        return utils.getDXF(obj)


_DrawingView = DrawingView
