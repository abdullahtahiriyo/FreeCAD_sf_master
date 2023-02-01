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

#ifndef GUI_NOTIFICATIONS_H
#define GUI_NOTIFICATIONS_H

#include <QMessageBox>

#include <Base/Console.h>
#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Gui/MainWindow.h>

namespace Gui {

/** Methods to seamlessly provide intrusive or non-intrusive notifications of error, warning,
 * messages, translated notifications, or untranslated notifications originating in a given
 * document object.
 *
 * The notifier field is obtained from DocumentObject.
 *
 * An attempt is made by NotificationArea to translate the message using the "Notifications" context,
 * except for TranslatedNotification.
 * For the former, this may be marked using QT_TRANSLATE_NOOP("Notifications","My message")
 */

// generic function to send any message provided by Base::LogStyle
template <Base::LogStyle type, typename TCaption, typename TMessage>
void Notify(const App::DocumentObject *, TCaption && caption, TMessage && message);

/** Convenience function to notify warnings
 *  The NotificationArea will attempt to find a translation in the "Notifications" context.
 *  This may be marked using QT_TRANSLATE_NOOP("Notifications","My message")
 */
template <typename TCaption, typename TMessage>
inline void NotifyWarning(const App::DocumentObject *, TCaption && caption, TMessage && message);

/** Convenience function to notify errors
 *  The NotificationArea will attempt to find a translation in the "Notifications" context.
 *  This may be marked using QT_TRANSLATE_NOOP("Notifications","My message")
 */
template <typename TCaption, typename TMessage>
inline void NotifyError(const App::DocumentObject *, TCaption && caption, TMessage && message);

/** Convenience function to notify messages
 *  The NotificationArea will attempt to find a translation in the "Notifications" context.
 *  This may be marked using QT_TRANSLATE_NOOP("Notifications","My message")
 */
template <typename TCaption, typename TMessage>
inline void NotifyMessage(const App::DocumentObject *, TCaption && caption, TMessage && message);

/** Convenience function to send already translated user notifications.
 *  No attempt will be made by the NotificationArea to translate them.
 */
template <typename TCaption, typename TMessage>
void TranslatedNotification(const App::DocumentObject *, TCaption && caption, TMessage && message);

/** Convenience function to send untranslated user notifications.
 *  The NotificationArea will attempt to find a translation in the "Notifications" context.
 *  This may be marked using QT_TRANSLATE_NOOP("Notifications","My message")
 */
template <typename TCaption, typename TMessage>
void Notification(const App::DocumentObject * obj, TCaption && caption, TMessage && message);
} //namespace Gui


template <Base::LogStyle type, typename TCaption, typename TMessage>
void Gui::Notify(const App::DocumentObject * obj, TCaption && caption, TMessage && message)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->
    GetGroup("NotificationArea");

    bool nonIntrusive = hGrp->GetBool("NonIntrusiveNotificationsEnabled", true);

    if(!nonIntrusive) {
        if constexpr( type == Base::LogStyle::Warning) {
            QMessageBox::warning(Gui::getMainWindow(),
                                QCoreApplication::translate("Notifications", caption),
                                QCoreApplication::translate("Notifications", message));
        }
        else
        if constexpr( type == Base::LogStyle::Error) {
            QMessageBox::critical(Gui::getMainWindow(),
                                 QCoreApplication::translate("Notifications", caption),
                                 QCoreApplication::translate("Notifications", message));
        }
        else
        if constexpr( type == Base::LogStyle::TranslatedNotification) {
            QMessageBox::information(Gui::getMainWindow(),
                                     caption,
                                     message);
        }
        else {
            QMessageBox::information(Gui::getMainWindow(),
                                QCoreApplication::translate("Notifications", caption),
                                QCoreApplication::translate("Notifications", message));
        }
    }
    else {
        if constexpr( type == Base::LogStyle::TranslatedNotification) {
            // trailing newline is necessary as this may be shown too in a console require them (depending on the configuration).
            auto msg = message.append(QStringLiteral("\n")); // QString

            Base::Console().Send<type>(obj->getFullLabel(), msg.toUtf8());
        }
        else {
            // trailing newline is necessary as this may be shown too in a console require them (depending on the configuration).
            auto msg = std::string(message).append("\n");

            Base::Console().Send<type>(obj->getFullLabel(), msg.c_str());
        }
    }
}

template <typename TCaption, typename TMessage>
void Gui::NotifyWarning(const App::DocumentObject * obj, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Warning>(obj,
                                    std::forward<TCaption &&>(caption),
                                    std::forward<TMessage &&>(message));
}

template <typename TCaption, typename TMessage>
void Gui::NotifyError(const App::DocumentObject * obj, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Error>(obj,
                                    std::forward<TCaption &&>(caption),
                                    std::forward<TMessage &&>(message));
}

template <typename TCaption, typename TMessage>
void Gui::NotifyMessage(const App::DocumentObject * obj, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Message>(obj,
                                    std::forward<TCaption &&>(caption),
                                    std::forward<TMessage &&>(message));
}

template <typename TCaption, typename TMessage>
void Gui::TranslatedNotification(const App::DocumentObject * obj, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::TranslatedNotification>(obj,
                                                   std::forward<TCaption &&>(caption),
                                                   std::forward<TMessage &&>(message));
}

template <typename TCaption, typename TMessage>
void Gui::Notification(const App::DocumentObject * obj, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Notification>(obj,
                                         std::forward<TCaption &&>(caption),
                                         std::forward<TMessage &&>(message));
}

#endif // GUI_NOTIFICATIONS_H
