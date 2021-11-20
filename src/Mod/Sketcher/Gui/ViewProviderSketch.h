/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel <juergen.riegel@web.de>             *
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


#ifndef SKETCHERGUI_VIEWPROVIDERSKETCH_H
#define SKETCHERGUI_VIEWPROVIDERSKETCH_H

#include <memory>

#include <Mod/Part/Gui/ViewProvider2DObject.h>
#include <Mod/Part/Gui/ViewProviderAttachExtension.h>
#include <Mod/Part/App/BodyBase.h>
#include <Base/Tools2D.h>
#include <Base/Placement.h>
#include <Gui/Selection.h>
#include <Gui/GLPainter.h>
#include <App/Part.h>
#include <boost_signals2.hpp>
#include <QCoreApplication>
#include <Gui/Document.h>
#include "ShortcutListener.h"

#include <Mod/Sketcher/App/GeoList.h>

class TopoDS_Shape;
class TopoDS_Face;
class SoSeparator;
class SbLine;
class SbVec2f;
class SbVec3f;
class SoCoordinate3;
class SoInfo;
class SoPointSet;
class SoTransform;
class SoLineSet;
class SoMarkerSet;
class SoPickedPoint;
class SoRayPickAction;

class SoImage;
class QImage;
class QColor;

class SoText2;
class SoTranslation;
class SbString;
class SbTime;

namespace Part {
    class Geometry;
}

namespace Gui {
    class View3DInventorViewer;
}

namespace Sketcher {
    class Constraint;
    class Sketch;
    class SketchObject;
}

namespace SketcherGui {

struct EditData;
class CoinManager;
class DrawSketchHandler;

using GeoList = Sketcher::GeoList;
using GeoListFacade = Sketcher::GeoListFacade;


/** The Sketch ViewProvider
  * This class handles mainly the drawing and editing of the sketch.
  * It draws the geometry and the constraints applied to the sketch.
  * It uses the class DrawSketchHandler to facilitate the creation
  * of new geometry while editing.
  */
class SketcherGuiExport ViewProviderSketch : public PartGui::ViewProvider2DObjectGrid
                                            , public PartGui::ViewProviderAttachExtension
                                            , public Gui::SelectionObserver
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::ViewProviderSketch)
    /// generates a warning message about constraint conflicts and appends it to the given message
    static QString appendConflictMsg(const std::vector<int> &conflicting);
    /// generates a warning message about redundant constraints and appends it to the given message
    static QString appendRedundantMsg(const std::vector<int> &redundant);
    /// generates a warning message about partially redundant constraints and appends it to the given message
    static QString appendPartiallyRedundantMsg(const std::vector<int> &partiallyredundant);
    /// generates a warning message about redundant constraints and appends it to the given message
    static QString appendMalformedMsg(const std::vector<int> &redundant);

    PROPERTY_HEADER_WITH_OVERRIDE(SketcherGui::ViewProviderSketch);

private:
    class ParameterObserver : public ParameterGrp::ObserverType
    {
    public:
        ParameterObserver(ViewProviderSketch & client);
        ~ParameterObserver();

        void initParameters();

        void subscribeToParameters();

        void unsubscribeToParameters();

        /** Observer for parameter group. */
        void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;

    private:

        void updateBoolProperty(const std::string & string, App::Property * property, bool defaultvalue);
        void updateGridSize(const std::string & string, App::Property * property);

        // Only for colors outside of edit mode, edit mode colors are handled by CoinManager.
        void updateColorProperty(const std::string & string, App::Property * property, float r, float g, float b);

        void updateEscapeKeyBehaviour(const std::string & string, App::Property * property);

        void updateAutoRecompute(const std::string & string, App::Property * property);

        void updateRecalculateInitialSolutionWhileDragging(const std::string & string, App::Property * property);

    private:
        ViewProviderSketch &Client;
        std::map<std::string, std::tuple<std::function<void(const std::string & string, App::Property *)>, App::Property * >> parameterMap;
    };

    class Drag {
    public:
        Drag() {
            resetVector();
        }

        void resetVector() {
            xInit = 0;
            yInit = 0;
            relative = false;
        }

        void resetIds() {
            DragPoint = -1;
            DragCurve = -1;
            DragConstraintSet.clear();
        }

        double xInit, yInit;                // starting point of the dragging operation
        bool relative;                      // whether the dragging move vector is relative or absolute


        int DragPoint = -1;                 // dragged point id
        int DragCurve = -1;                 // dragged curve id
        std::set<int> DragConstraintSet;    // dragged constraints ids
    };

    struct DoubleClick {
        static SbTime prvClickTime;
        static SbVec2s prvClickPos; //used by double-click-detector
        static SbVec2s prvCursorPos;
        static SbVec2s newCursorPos;
    };

    struct ViewProviderParameters {
        bool handleEscapeButton = false;
        bool autoRecompute = false;
        bool recalculateInitialSolutionWhileDragging = false;

        bool isShownVirtualSpace = false; // indicates whether the present virtual space view is the Real Space or the Virtual Space (virtual space 1 or 2)
        bool buttonPress = false;
    };

public:
    /// constructor
    ViewProviderSketch();
    /// destructor
    virtual ~ViewProviderSketch();

    /** @name Properties */
    //@{
    App::PropertyBool Autoconstraints;
    App::PropertyBool AvoidRedundant;
    App::PropertyPythonObject TempoVis;
    App::PropertyBool HideDependent;
    App::PropertyBool ShowLinks;
    App::PropertyBool ShowSupport;
    App::PropertyBool RestoreCamera;
    App::PropertyBool ForceOrtho;
    App::PropertyBool SectionView;
    App::PropertyString EditingWorkbench;
    //@}

    /** @name handler control */
    //@{
    /// sets an DrawSketchHandler in control
    void activateHandler(DrawSketchHandler *newHandler);
    /// removes the active handler
    void purgeHandler(void);

    //@}


    /** @name modus handling */
    //@{
    /// mode table
    enum SketchMode{
        STATUS_NONE,              /**< enum value View provider is in neutral. */
        STATUS_SELECT_Point,      /**< enum value a point was selected. */
        STATUS_SELECT_Edge,       /**< enum value an edge was selected. */
        STATUS_SELECT_Constraint, /**< enum value a constraint was selected. */
        STATUS_SELECT_Cross,      /**< enum value the base coordinate system was selected. */
        STATUS_SKETCH_DragPoint,  /**< enum value while dragging a point. */
        STATUS_SKETCH_DragCurve,  /**< enum value while dragging a curve. */
        STATUS_SKETCH_DragConstraint,  /**< enum value while dragging a compatible constraint. */
        STATUS_SKETCH_UseHandler, /**< enum value a DrawSketchHandler is in control. */
        STATUS_SKETCH_StartRubberBand, /**< enum value for initiating a rubber band selection */
        STATUS_SKETCH_UseRubberBand /**< enum value when making a rubber band selection *//**< enum value a DrawSketchHandler is in control. */
    };

    /// is called by GuiCommands to set the drawing mode
    void setSketchMode(SketchMode mode) {Mode = mode;}
    /// get the sketch mode
    SketchMode getSketchMode(void) const {return Mode;}
    //@}

    /** @name Drawing functions */
    //@{
    /// draw the sketch in the inventor nodes
    /// temp => use temporary solver solution in SketchObject
    /// recreateinformationscenography => forces a rebuild of the information overlay scenography
    void draw(bool temp=false, bool rebuildinformationoverlay=true);

    /// helper change the color of the sketch according to selection and solver status
    void updateColor(void);
    //@}

    /** @name Selection functions */
    //@{
    /// Is the view provider selectable
    bool isSelectable(void) const override;

    /// Observer message from the Selection
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    /// box selection method
    void doBoxSelection(const SbVec2s &startPos, const SbVec2s &endPos,
                        const Gui::View3DInventorViewer *viewer);
    //@}

    /** @name preselection functions */
    //@{
    /// helper to detect preselection
    bool detectAndShowPreselection (SoPickedPoint * Point, const SbVec2s &cursorPos);
    int getPreselectPoint(void) const;
    int getPreselectCurve(void) const;
    int getPreselectCross(void) const;
    //@}

    /** @name Access to Sketch and Solver objects */
    //@{
    /// get the pointer to the sketch document object
    Sketcher::SketchObject *getSketchObject(void) const;

    /** returns a const reference to the last solved sketch object. It guarantees that
     *  the solver object does not lose synchronisation with the SketchObject properties.
     *
     * NOTE: Operations requiring write access to the solver must be done via SketchObject
     * interface. See for example functions:
     * -> inline void setRecalculateInitialSolutionWhileMovingPoint(bool recalculateInitialSolutionWhileMovingPoint)
     * -> inline int initTemporaryMove(int geoId, PointPos pos, bool fine=true)
     * -> inline int moveTemporaryPoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative=false)
     * -> inline void updateSolverExtension(int geoId, std::unique_ptr<Part::GeometryExtension> && ext)
     */
    const Sketcher::Sketch &getSolvedSketch(void) const;
    //@}

    /** @name miscelanea utilities */
    //@{
    /*! Look at the center of the bounding of all selected items */
    void centerSelection();

    /// snap points x,y (mouse coordinates) onto grid if enabled
    void snapToGrid(double &x, double &y);

    /// moves a selected constraint
    void moveConstraint(int constNum, const Base::Vector2d &toPos);

    float getScaleFactor() const;
    //@}


    /// updates the visibility of the virtual space
    /** @name constraint Virtual Space visibility management */
    //@{
    void updateVirtualSpace(void);
    void setIsShownVirtualSpace(bool isshownvirtualspace);
    bool getIsShownVirtualSpace(void) const;
    //@}


    /** @name base class implementer */
    //@{
    virtual void attach(App::DocumentObject *) override;
    virtual void updateData(const App::Property *) override;

    virtual void setupContextMenu(QMenu *menu, QObject *receiver, const char *member) override;
    /// is called when the Provider is in edit and a deletion request occurs
    virtual bool onDelete(const std::vector<std::string> &) override;
    /// Is called by the tree if the user double clicks on the object. It returns the string
    /// for the transaction that will be shown in the undo/redo dialog.
    /// If null is returned then no transaction will be opened.
    virtual const char* getTransactionText() const override { return nullptr; }
    /// is called by the tree if the user double clicks on the object
    virtual bool doubleClicked(void) override;
    /// is called when the Provider is in edit and the mouse is moved
    virtual bool mouseMove(const SbVec2s &pos, Gui::View3DInventorViewer *viewer) override;
    /// is called when the Provider is in edit and a key event ocours. Only ESC ends edit.
    virtual bool keyPressed(bool pressed, int key) override;
    /// is called when the Provider is in edit and the mouse is clicked
    virtual bool mouseButtonPressed(int Button, bool pressed, const SbVec2s& cursorPos, const Gui::View3DInventorViewer* viewer) override;
    //@}

    /// Icons and Icon overlays
    virtual QIcon mergeColorfulOverlayIcons (const QIcon & orig) const override;

    /// signals if the constraints list has changed
    boost::signals2::signal<void ()> signalConstraintsChanged;
    /// signals if the sketch has been set up
    boost::signals2::signal<void (const QString &state, const QString &msg, const QString &url, const QString &linkText)> signalSetUp;
    /// signals if the elements list has changed
    boost::signals2::signal<void ()> signalElementsChanged;

    friend class ViewProviderSketchDrawSketchHandlerAttorney;
    friend class ViewProviderSketchCoinAttorney;
    friend class ViewProviderSketchShortcutListenerAttorney;

protected:
    /** @name enter/exit edit mode */
    //@{
    virtual bool setEdit(int ModNum) override;
    virtual void unsetEdit(int ModNum) override;
    virtual void setEditViewer(Gui::View3DInventorViewer*, int ModNum) override;
    virtual void unsetEditViewer(Gui::View3DInventorViewer*) override;
    //@}

    /** @name miscelanea editing functions */
    //@{

    void deactivateHandler();

    /// get called if a subelement is double clicked while editing
    void editDoubleClicked(void);
    //@}

    /** @name Solver Information */
    //@{
    /// update solver information based on last solving at SketchObject
    void UpdateSolverInformation(void);

    /// Auxiliary function to generate messages about conflicting, redundant and malformed constraints
    static QString appendConstraintMsg( const QString & singularmsg,
                                        const QString & pluralmsg,
                                        const std::vector<int> &vector);
    //@}

    /** @name manage updates during undo/redo operations */
    //@{
    void slotUndoDocument(const Gui::Document&);
    void slotRedoDocument(const Gui::Document&);
    void forceUpdateData();
    //@}

    /** @name base class implementer */
    //@{
    /// get called by the container whenever a property has been changed
    virtual void onChanged(const App::Property *prop) override;
    //@}

private:
    /// function to handle OCCT BSpline weight calculation singularities and representation
    void scaleBSplinePoleCirclesAndUpdateSolverAndSketchObjectGeometry(
                        GeoList & geolist,
                        bool geometrywithmemoryallocation,
                        std::vector<std::unique_ptr<Part::Geometry>> &deepCopiesToDelete);

    /** @name geometry and coordinates auxiliary functions */
    //@{
    /// give the coordinates of a line on the sketch plane in sketcher (2D) coordinates
    void getCoordsOnSketchPlane(const SbVec3f &point, const SbVec3f &normal, double &u, double &v) const;

    /// give projecting line of position
    void getProjectingLine(const SbVec2s&,
                           const Gui::View3DInventorViewer *viewer,
                           SbLine&) const;
    //@}


    /** @name Attorney functions*/
    //@{
    /* private functions to decouple Attorneys and Clients from the internal implementation of
    the ViewProvider and its members, such as sketchObject (see friend attorney classes) and
    improve encapsulation.
    */

    //********* ViewProviderSketchCoinAttorney ***********************

    bool constraintHasExpression(int constrid) const;

    const std::vector<Sketcher::Constraint *> getConstraints() const;

    // gets the list of geometry of the sketchobject or of the solver instance
    const GeoList getGeoList() const;

    GeoListFacade getGeoListFacade() const;

    Base::Placement getEditingPlacement() const;

    std::unique_ptr<SoRayPickAction> getRayPickAction() const;

    SbVec2f getScreenCoordinates(SbVec2f sketchcoordinates) const;

    QFont getApplicationFont() const;

    int defaultFontSizePixels() const;

    int getApplicationLogicalDPIX() const;

    double getRotation(SbVec3f pos0, SbVec3f pos1) const;

    bool isSketchInvalid() const;

    bool isSketchFullyConstrained() const;

    bool haveConstraintsInvalidGeometry() const;

    void addNodeToRoot(SoSeparator * node);

    void removeNodeFromRoot(SoSeparator * node);

    //********* ViewProviderSketchShortcutListenerAttorney ***********//
    void deleteSelected();

    //********* ViewProviderSketchDrawSketchHandlerAttorney **********//
    void setPositionText(const Base::Vector2d &Pos, const SbString &txt);
    void setPositionText(const Base::Vector2d &Pos);
    void resetPositionText(void);

    /// draw the edit curve
    void drawEdit(const std::vector<Base::Vector2d> &EditCurve);
    /// draw the edit markers
    void drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel = 0);
    /// set the pick style of the sketch coordinate axes
    void setAxisPickStyle(bool on);
    //@}

private:
    boost::signals2::connection connectUndoDocument;
    boost::signals2::connection connectRedoDocument;

    /// pointer to the edit data structure if the ViewProvider is in edit.
    EditData *edit;

    // modes while sketching
    SketchMode Mode;

    // reference coordinates for relative operations
    Drag drag;

    std::unique_ptr<Gui::Rubberband> rubberband;

    std::string editDocName;
    std::string editObjName;
    std::string editSubName;

    ShortcutListener* listener;

    std::unique_ptr<CoinManager> coinManager;

    std::unique_ptr<ViewProviderSketch::ParameterObserver> pObserver;

    std::unique_ptr<DrawSketchHandler> sketchHandler;

    ViewProviderParameters viewProviderParameters;
};

} // namespace PartGui


#endif // SKETCHERGUI_VIEWPROVIDERSKETCH_H

