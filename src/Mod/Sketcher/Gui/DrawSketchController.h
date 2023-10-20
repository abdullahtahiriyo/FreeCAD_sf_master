/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHERGUI_DrawSketchController_H
#define SKETCHERGUI_DrawSketchController_H

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <Base/Tools2D.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/EditableDatumLabel.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "DrawSketchDefaultHandler.h"
#include "SketcherToolDefaultWidget.h"

namespace SketcherGui
{

/** Class to decide which control is responsible of handling an key event
 *using timers, type of entered event, ...
 */
class KeyboardManager: public QObject
{
    Q_OBJECT

public:
    KeyboardManager()
        : QObject(nullptr)
        , keyMode(KeyboardEventHandlingMode::Widget)
    {
        // get the active viewer, so that we can send it key events
        auto doc = Gui::Application::Instance->activeDocument();

        if (doc) {
            auto temp = dynamic_cast<Gui::View3DInventor*>(doc->getActiveView());
            if (temp) {
                vpViewer = temp->getViewer();
                keyMode = KeyboardEventHandlingMode::ViewProvider;
            }
        }

        timer.setSingleShot(true);

        QObject::connect(&timer, &QTimer::timeout, [this]() {
            onTimeOut();
        });
    }

    /// Indicates whether the widget should handle keyboard input or should signal it via boost
    enum class KeyboardEventHandlingMode
    {
        Widget,
        ViewProvider
    };

    bool isMode(KeyboardEventHandlingMode mode)
    {
        return mode == keyMode;
    }

    KeyboardEventHandlingMode getMode()
    {
        return keyMode;
    }

    bool eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
            /*If a key shortcut is required to work on sketcher when a tool using Tool Setting
            widget is being used, then you have to add this key to the below section such that the
            spinbox doesn't keep the keypress event for itself. Note if you want the event to be
            handled by the spinbox too, you can return false.*/

            auto keyEvent = static_cast<QKeyEvent*>(event);

            detectKeyboardEventHandlingMode(keyEvent);  // determine the handler

            if (vpViewer && isMode(KeyboardEventHandlingMode::ViewProvider)) {
                return QApplication::sendEvent(vpViewer, keyEvent);
            }

            return false;  // do not intercept the event and feed it to the widget
        }

        return false;
    }

private:
    /// This function decides whether events should be send to the ViewProvider
    /// or to the UI control of the Default widget.
    void detectKeyboardEventHandlingMode(QKeyEvent* keyEvent)
    {
        QRegularExpression rx(QStringLiteral("^[0-9]$"));
        auto match = rx.match(keyEvent->text());
        if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return
            || keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab
            || keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete
            || keyEvent->key() == Qt::Key_Minus || keyEvent->key() == Qt::Key_Period
            || keyEvent->key() == Qt::Key_Comma || match.hasMatch()) {
            keyMode = KeyboardEventHandlingMode::Widget;
            timer.start(timeOut);
        }
    }

    void onTimeOut()
    {
        keyMode = KeyboardEventHandlingMode::ViewProvider;
    }

private:
    /// Viewer responsible for the active document
    Gui::View3DInventorViewer* vpViewer = nullptr;
    KeyboardEventHandlingMode keyMode;

    QTimer timer;

    const int timeOut = 1000;
};

/** @brief template class for creating a type encapsulating an int value associated to each of
    the possible construction modes supported by the tool.

    @details Different construction modes of a DSH may use different types of controls. This class
    allows to instantiate a handler template class to provide such construction mode specific
    controls.

    Each different type of control is a template class deriving from this.
    */
template<int... sizes>  // Initial sizes for each mode
class ControlAmount
{
public:
    template<typename constructionT>
    static constexpr int size(constructionT constructionmethod)
    {
        auto modeint = static_cast<int>(constructionmethod);

        return constructionMethodParameters[modeint];
    }

    static constexpr int defaultMethodSize()
    {
        return size(0);
    }

private:
    static constexpr std::array<int, sizeof...(sizes)> constructionMethodParameters = {{sizes...}};
};

/** @brief Type encapsulating the number of On view parameters*/
template<int... sizes>  // Initial sizes for each mode
class OnViewParameters: public ControlAmount<sizes...>
{
};

/** @brief Type encapsulating the number of parameters in the widget*/
template<int... sizes>  // Initial sizes for each mode
class WidgetParameters: public ControlAmount<sizes...>
{
};

/** @brief Type encapsulating the number of checkboxes in the widget*/
template<int... sizes>  // Initial sizes for each mode
class WidgetCheckboxes: public ControlAmount<sizes...>
{
};

/** @brief Type encapsulating the number of comboboxes in the widget*/
template<int... sizes>  // Initial sizes for each mode
class WidgetComboboxes: public ControlAmount<sizes...>
{
};


namespace sp = std::placeholders;

/** @brief Class defining a generic handler controller operable with a DrawSketchControllableHandler
 *
 * @details
 * This class is intended as a parent for controller classes, which control a handler based on
 * additional input provided by the user (beyond the typical mouse-click driven behaviour).
 */
template<typename HandlerT,
         typename SelectModeT,
         int PAutoConstraintSize,     // The initial size of the AutoConstraint vector
         typename OnViewParametersT,  // The number of parameter spinboxes in the 3D view
         typename WidgetParametersT,  // The number of parameter spinboxes in the default widget
         typename WidgetCheckboxesT,  // The number of checkboxes in the default widget
         typename WidgetComboboxesT,  // The number of comboboxes in the default widget
         typename ConstructionMethodT = ConstructionMethods::DefaultConstructionMethod,
         bool PFirstComboboxIsConstructionMethod =
             false>  // The handler template or class having this as inner class
class DrawSketchController
{
public:
    using HandlerType = HandlerT;
    using SelectModeType = SelectModeT;
    using ContructionMethodType = ConstructionMethodT;
    static constexpr const int AutoConstraintInitialSize = PAutoConstraintSize;

    using DSDefaultHandler =
        DrawSketchDefaultHandler<HandlerT, SelectModeT, PAutoConstraintSize, ConstructionMethodT>;
    using ConstructionMachine = ConstructionMethodMachine<ConstructionMethodT>;

    using ConstructionMethod = ConstructionMethodT;

protected:
    HandlerT* handler;           // real derived type
    bool init = false;           // returns true if the widget has been configured
    bool firstMoveInit = false;  // track the first mouse movement

    Base::Vector2d prevCursorPosition;
    Base::Vector2d lastControlEnforcedPosition;

    int onViewIndexWithFocus = 0;  // track the index of the on-view parameter having the focus

    int nOnViewParameter = OnViewParametersT::defaultMethodSize();
    int nParameter = WidgetParametersT::defaultMethodSize();
    int nCheckbox = WidgetCheckboxesT::defaultMethodSize();
    int nCombobox = WidgetComboboxesT::defaultMethodSize();

    SketcherToolDefaultWidget* toolWidget;

    using Connection = boost::signals2::connection;

    Connection connectionParameterValueChanged;
    Connection connectionCheckboxCheckedChanged;
    Connection connectionComboboxSelectionChanged;

    /** @name Named indices for controls of the default widget (SketcherToolDefaultWidget) */
    //@{
    using WParameter = SketcherToolDefaultWidget::Parameter;
    using WCheckbox = SketcherToolDefaultWidget::Checkbox;
    using WCombobox = SketcherToolDefaultWidget::Combobox;

    using SelectMode = SelectModeT;
    //@}

    /** @name Named indices for controls of the on-view controls (SketcherToolDefaultWidget) */
    //@{
    enum OnViewParameter
    {
        First,
        Second,
        Third,
        Fourth,
        Fifth,
        Sixth,
        Seventh,
        Eighth,
        Ninth,
        Tenth,
        nOnViewParameters  // Must Always be the last one
    };
    //@}

    std::vector<std::unique_ptr<Gui::EditableDatumLabel>> onViewParameters;

    SbColor dimConstrColor, dimConstrDeactivatedColor;

public:
    DrawSketchController(HandlerT* dshandler)
        : handler(dshandler)
        , keymanager(new KeyboardManager())
    {}

    ~DrawSketchController()
    {
        delete keymanager;

        connectionParameterValueChanged.disconnect();
        connectionCheckboxCheckedChanged.disconnect();
        connectionComboboxSelectionChanged.disconnect();
    }

    /** @name functions NOT intended for specialisation */
    //@{

    /** @brief Initialises controls, such as the widget and the on-view parameters via NVI. */
    void initControls(QWidget* widget)
    {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");

        dimConstrColor = SbColor(1.0f, 0.149f, 0.0f);
        dimConstrDeactivatedColor = SbColor(0.8f, 0.8f, 0.8f);

        float transparency = 0.f;
        unsigned long color = (unsigned long)(dimConstrColor.getPackedValue());
        color = hGrp->GetUnsigned("ConstrainedDimColor", color);
        dimConstrColor.setPackedValue((uint32_t)color, transparency);

        color = (unsigned long)(dimConstrDeactivatedColor.getPackedValue());
        color = hGrp->GetUnsigned("DeactivatedConstrDimColor", color);
        dimConstrDeactivatedColor.setPackedValue((uint32_t)color, transparency);


        doInitControls(widget);  // NVI

        resetControls();
        init = true;
    }

    /** @brief Resets the controls, such as the widget and the on-view parameters */
    void resetControls()
    {
        doResetControls();  // NVI

        firstMoveInit = false;
    }

    /** @brief Initialises on-screen parameters */
    void initNOnViewParameters(int n)
    {
        Gui::View3DInventorViewer* viewer = handler->getViewer();
        Base::Placement placement = handler->sketchgui->getSketchObject()->Placement.getValue();

        onViewParameters.clear();

        for (int i = 0; i < n; i++) {

            // the returned is a naked pointer
            auto parameter = onViewParameters
                                 .emplace_back(std::make_unique<Gui::EditableDatumLabel>(
                                     viewer,
                                     placement,
                                     dimConstrDeactivatedColor,
                                     /*autoDistance = */ true))
                                 .get();

            QObject::connect(parameter, &Gui::EditableDatumLabel::valueChanged, [=](double value) {
                parameter->setColor(dimConstrColor);
                onViewValueChanged(i, value);
            });
        }
    }

    void unsetOnViewParameter(std::unique_ptr<Gui::EditableDatumLabel>& onViewParameter)
    {
        onViewParameter->isSet = false;
        onViewParameter->setColor(dimConstrDeactivatedColor);
    }

    /** @brief function triggered by the handler when the mouse has been moved */
    void mouseMoved(Base::Vector2d originalSketchPosition)
    {
        onMouseMoved(originalSketchPosition);  // NVI

        if (!firstMoveInit) {
            firstMoveInit = true;
        }
    }

    /** @brief function triggered by the handler to ensure its operating position takes into
     * account widget mandated parameters */
    void enforceControlParameters(Base::Vector2d& onSketchPos)
    {
        prevCursorPosition = onSketchPos;

        doEnforceControlParameters(onSketchPos);

        lastControlEnforcedPosition = onSketchPos;  // store enforced cursor position.

        afterEnforceControlParameters();  // NVI
    }

    /** slot triggering when a on view parameter has changed
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void onViewValueChanged(int onviewparameterindex, double value)
    {
        if (isOnViewParameterOfCurrentMode(onviewparameterindex + 1)) {
            setFocusToOnViewParameter(onviewparameterindex + 1);
        }

        /* That is not supported with on-view parameters.
        // -> A machine does not forward to a next state when adapting the parameter (though it
        // may forward to
        //    a next state if all the parameters are fulfilled, see
        //    doChangeDrawSketchHandlerMode). This ensures that the geometry has been defined
        //    (either by mouse clicking or by widget). Autoconstraints on point should be picked
        //    when the state is reached upon machine state advancement.
        //
        // -> A machine goes back to a previous state if a parameter of a previous state is
        // modified. This ensures
        //    that appropriate autoconstraints are picked.
        if (isOnViewParameterOfPreviousMode(onviewparameterindex)) {
            // change to previous state
            handler->setState(getState(onviewparameterindex));
        }*/

        adaptDrawingToOnViewParameterChange(onviewparameterindex, value);

        finishWidgetChanged();
    }

    /** boost slot triggering when a parameter has changed in the widget
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void parameterValueChanged(int parameterindex, double value)
    {
        adaptDrawingToParameterChange(parameterindex, value);

        finishWidgetChanged();
    }

    /** boost slot triggering when a checkbox has changed in the widget
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void checkboxCheckedChanged(int checkboxindex, bool value)
    {
        adaptDrawingToCheckboxChange(checkboxindex, value);

        finishWidgetChanged();
    }

    /** boost slot triggering when a combobox has changed in the widget
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void comboboxSelectionChanged(int comboboxindex, int value)
    {
        adaptDrawingToComboboxChange(comboboxindex, value);

        finishWidgetChanged();
    }

    void adaptParameters()
    {
        adaptParameters(lastControlEnforcedPosition);
    }

    //@}

    /** @name functions which MUST be specialised */
    //@{
    /// Change DSH to reflect a value entered in the view
    void adaptDrawingToOnViewParameterChange(int onviewparameterindex, double value)
    {
        Q_UNUSED(onviewparameterindex);
        Q_UNUSED(value);
    }

    /// Change DSH to reflect a value entered in the widget
    void adaptDrawingToParameterChange(int parameterindex, double value)
    {
        Q_UNUSED(parameterindex);
        Q_UNUSED(value);
    }

    /// Change DSH to reflect a checkbox changed in the widget
    void adaptDrawingToCheckboxChange(int checkboxindex, bool value)
    {
        Q_UNUSED(checkboxindex);
        Q_UNUSED(value);
    }

    /// Change DSH to reflect a comboBox changed in the widget
    void adaptDrawingToComboboxChange(int comboboxindex, int value)
    {
        Q_UNUSED(comboboxindex);

        if constexpr (PFirstComboboxIsConstructionMethod == true) {

            if (comboboxindex == WCombobox::FirstCombo && handler->ConstructionMethodsCount() > 1) {
                handler->setConstructionMethod(static_cast<ConstructionMethodT>(value));
            }
        }
    }

    /** Returns the state to which the widget parameter corresponds in the current construction
     * method
     */
    auto getState(int parameterindex) const
    {
        Q_UNUSED(parameterindex);
        return handler->getFirstState();
    }

    /// function to create constraints based on widget information.
    void addConstraints()
    {}
    //@}

    /** @name functions which need to be specialised */
    //@{
    /// Function to specialise to set the correct widget strings and commands
    void configureToolWidget()
    {}

    void configureOnViewParameters()
    {}

    /** Change DSH to reflect the SelectMode it should be in based on values entered in the
     * widget
     *
     * This is just a default implementation for common stateMachines, that may
     * or may not do what you expect. It assumes two parameters per seek state.
     *
     * It MUST be specialised otherwise
     */
    void doChangeDrawSketchHandlerMode()
    {}

    /** function that is called by the handler when the selection mode changed
     *
     * It can be specialised if needed
     */
    void onHandlerModeChanged()
    {
        setModeOnViewParameters();
    }

    void afterHandlerModeChanged()
    {
        if (!handler->isState(SelectModeT::End) || handler->continuousMode) {
            handler->mouseMove(prevCursorPosition);
        }
    }

    /* Activates the correct set of on-view parameters corresponding to current
     * mode. It may be specialized if necessary.*/
    void setModeOnViewParameters()
    {
        bool firstOfMode = true;
        for (size_t i = 0; i < onViewParameters.size(); i++) {
            if (!isOnViewParameterOfCurrentMode(i)) {
                onViewParameters[i]->stopEdit();
                if (!onViewParameters[i]->isSet || handler->state() == SelectMode::End) {
                    onViewParameters[i]->deactivate();
                }
            }
            else {
                if (firstOfMode) {
                    onViewIndexWithFocus = i;
                    firstOfMode = false;
                }

                onViewParameters[i]->activate();

                // points/value will be overriden by the mouseMove triggered by the mode change.
                onViewParameters[i]->setPoints(Base::Vector3d(), Base::Vector3d());
                onViewParameters[i]->startEdit(0.0, keymanager);
            }
        }
    }

    /** function that is called by the handler when the construction mode changed
     *
     * This is just a default implementation for common stateMachines, that may
     * or may not do what you expect. It assumes two parameters per seek state.
     *
     * It MUST be specialised otherwise
     */
    void onConstructionMethodChanged()
    {

        nOnViewParameter = OnViewParametersT::size(handler->constructionMethod());
        nParameter = WidgetParametersT::size(handler->constructionMethod());
        nCheckbox = WidgetCheckboxesT::size(handler->constructionMethod());
        nCombobox = WidgetComboboxesT::size(handler->constructionMethod());

        // update the combobox only if necessary (if the change was not triggered by the
        // combobox)
        if constexpr (PFirstComboboxIsConstructionMethod == true) {
            auto currentindex = toolWidget->getComboboxIndex(WCombobox::FirstCombo);
            auto methodint = static_cast<int>(handler->constructionMethod());

            if (currentindex != methodint) {
                // avoid triggering of method change
                boost::signals2::shared_connection_block combobox_block(
                    connectionComboboxSelectionChanged);
                toolWidget->setComboboxIndex(WCombobox::FirstCombo, methodint);
            }
        }

        handler->updateCursor();

        handler->reset();  // reset of handler to restart.
    }

    /** function that is called by the handler with a Vector2d position to update the widget
     *
     * It MUST be specialised if you want the parameters to update on mouseMove
     */
    void adaptParameters(Base::Vector2d onSketchPos)
    {
        Q_UNUSED(onSketchPos)
    }

    /** function that is called by the handler with a mouse position, enabling the
     * widget to override it having regard to the widget information.
     *
     * It MUST be specialised if you want to overide mouse position based on parameters.
     */
    void doEnforceControlParameters(Base::Vector2d& onSketchPos)
    {
        Q_UNUSED(onSketchPos)
    }

    /** on first shortcut, it toggles the first checkbox if there is go. Must be specialised if
     * this is not intended */
    void firstKeyShortcut()
    {
        if (nCheckbox >= 1) {
            auto firstchecked = toolWidget->getCheckboxChecked(WCheckbox::FirstBox);
            toolWidget->setCheckboxChecked(WCheckbox::FirstBox, !firstchecked);
        }
    }

    void secondKeyShortcut()
    {
        if (nCheckbox >= 2) {
            auto secondchecked = toolWidget->getCheckboxChecked(WCheckbox::SecondBox);
            toolWidget->setCheckboxChecked(WCheckbox::SecondBox, !secondchecked);
        }
    }

    virtual void tabShortcut()
    {
        passFocusToNextOnViewParameter();
    }


    /* This function give the focus to a spinbox and register to which it gave it.*/
    void setFocusToOnViewParameter(unsigned int onviewparameterindex)
    {
        if (onviewparameterindex < onViewParameters.size()) {
            onViewParameters[onviewparameterindex]->setFocusToSpinbox();
            onViewIndexWithFocus = onviewparameterindex;
        }
    }
    //@}

    void passFocusToNextOnViewParameter()
    {
        unsigned int index = onViewIndexWithFocus + 1;

        if (index >= onViewParameters.size()) {
            index = 0;
        }
        while (index < onViewParameters.size()) {
            if (isOnViewParameterOfCurrentMode(index)) {
                setFocusToOnViewParameter(index);
                break;
            }
            index++;
        }
    }

protected:
    /** @name NVI */
    //@{
    virtual void doInitControls(QWidget* widget)
    {
        initDefaultWidget(widget);
    }

    virtual void doResetControls()
    {
        resetOnViewParameters();
        resetDefaultWidget();
    }
    virtual void onMouseMoved(Base::Vector2d originalSketchPosition)
    {
        Q_UNUSED(originalSketchPosition)

        if (!firstMoveInit) {
            setModeOnViewParameters();
        }
    }

    virtual void afterEnforceControlParameters()
    {
        // Give focus to current on-view parameter. In case user interacted outside of 3dview.
        setFocusToOnViewParameter(onViewIndexWithFocus);
    }
    //@}

private:
    void initDefaultWidget(QWidget* widget)
    {
        toolWidget = static_cast<SketcherToolDefaultWidget*>(widget);

        connectionParameterValueChanged = toolWidget->registerParameterValueChanged(
            std::bind(&DrawSketchController::parameterValueChanged, this, sp::_1, sp::_2));

        connectionCheckboxCheckedChanged = toolWidget->registerCheckboxCheckedChanged(
            std::bind(&DrawSketchController::checkboxCheckedChanged, this, sp::_1, sp::_2));

        connectionComboboxSelectionChanged = toolWidget->registerComboboxSelectionChanged(
            std::bind(&DrawSketchController::comboboxSelectionChanged, this, sp::_1, sp::_2));
    }

    void resetDefaultWidget()
    {
        boost::signals2::shared_connection_block parameter_block(connectionParameterValueChanged);
        boost::signals2::shared_connection_block checkbox_block(connectionCheckboxCheckedChanged);
        boost::signals2::shared_connection_block combobox_block(connectionComboboxSelectionChanged);

        toolWidget->initNParameters(nParameter);
        toolWidget->initNCheckboxes(nCheckbox);
        toolWidget->initNComboboxes(nCombobox);

        configureToolWidget();
    }

    void resetOnViewParameters()
    {
        initNOnViewParameters(nOnViewParameter);
        onViewIndexWithFocus = 0;

        configureOnViewParameters();
    }

private:
    /** @name helper functions */
    //@{
    /// function to assist in adaptDrawingToComboboxChange specialisation
    /// assigns the modevalue to the modeenum
    /// it also triggers an update of the cursor
    template<typename T>
    void setMode(T& modeenum, int modevalue)
    {
        auto mode = static_cast<T>(modevalue);

        modeenum = mode;

        handler->updateCursor();

        handler->resetControls();  // resetControls of handler to restart.
    }

    /// function to redraw before and after any eventual mode change in reaction to a widget
    /// change
    void finishWidgetChanged()
    {

        // handler->moveCursorToSketchPoint(lastControlEnforcedPosition);

        handler->mouseMove(prevCursorPosition);

        auto currentstate = handler->state();
        // ensure that object at point is preselected, so that autoconstraints are generated
        handler->preselectAtPoint(lastControlEnforcedPosition);

        doChangeDrawSketchHandlerMode();

        // if the state changed and is not the last state (End). And is init (ie tool has not
        // reset)
        if (!handler->isLastState() && handler->state() != currentstate && firstMoveInit) {
            // mode has changed, so reprocess the previous position to the new widget state
            handler->mouseMove(prevCursorPosition);
        }
    }

    /// returns the status to which the handler was updated
    bool syncHandlerToCheckbox(int checkboxindex, bool& handlerboolean)
    {
        bool status = toolWidget->getCheckboxChecked(checkboxindex);
        handlerboolean = status;

        return status;
    }

    /// returns true if checkbox was changed, and false if no sync was necessary
    bool syncCheckboxToHandler(int checkboxindex, bool handlerboolean)
    {
        bool status = toolWidget->getCheckboxChecked(checkboxindex);
        if (handlerboolean != status) {
            toolWidget->setCheckboxChecked(checkboxindex, handlerboolean);
            return true;
        }

        return false;
    }

    void syncHandlerToConstructionMethodCombobox()
    {

        if constexpr (PFirstComboboxIsConstructionMethod == true) {
            auto constructionmethod = toolWidget->getComboboxIndex(WCombobox::FirstCombo);

            handler->initConstructionMethod(static_cast<ConstructionMethodT>(constructionmethod));
        }
    }
    void syncConstructionMethodComboboxToHandler()
    {

        if constexpr (PFirstComboboxIsConstructionMethod == true) {
            auto constructionmethod = toolWidget->getComboboxIndex(WCombobox::FirstCombo);

            auto actualconstructionmethod = static_cast<int>(handler->constructionMethod());

            if (constructionmethod != actualconstructionmethod) {
                toolWidget->setComboboxIndex(WCombobox::FirstCombo, actualconstructionmethod);
            }
        }
    }

    bool isOnViewParameterOfCurrentMode(unsigned int onviewparameterindex) const
    {
        return onviewparameterindex < onViewParameters.size()
            && getState(onviewparameterindex) == handler->state();
    }

    bool isOnViewParameterOfPreviousMode(unsigned int onviewparameterindex) const
    {
        return onviewparameterindex < onViewParameters.size()
            && getState(onviewparameterindex) < handler->state();
    }
    //@}

    KeyboardManager* keymanager;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchController_H
