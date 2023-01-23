/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef _PreComp_
# include <memory>
# include <mutex>
# include <QApplication>
# include <QAction>
# include <QActionEvent>
# include <QEvent>
# include <QHBoxLayout>
# include <QHeaderView>
# include <QMenu>
# include <QMessageBox>
# include <QStringList>
# include <QTreeWidget>
# include <QTimer>
# include <QWidgetAction>
#endif

#include <App/Application.h>
#include <Base/Console.h>

#include "BitmapFactory.h"
#include "NotificationBox.h"
#include "Application.h"
#include "MainWindow.h"
#include "MDIView.h"

#include "NotificationArea.h"

using namespace Gui;

using Connection = boost::signals2::connection;

namespace bp = boost::placeholders;

namespace Gui {
class NotificationItem : public QTreeWidgetItem
{
public:
    NotificationItem(Base::LogStyle notificationtype, QString notifiername, QString message):
        notificationType(notificationtype), notifierName(std::move(notifiername)), msg(std::move(message)) {}

    QVariant data(int column, int role) const override {
        // property name
        if( role == Qt::DisplayRole ) {
            switch(column) {
                case 1:
                    return notifierName;
                    break;
                case 2:
                    return msg;
                    break;
            }
        }
        else
        if(column == 0 && role == Qt::DecorationRole) {
            if(notificationType == Base::LogStyle::Error) {
                return QVariant::fromValue(BitmapFactory().pixmapFromSvg(":/icons/edit_Cancel.svg",QSize(16, 16)));
            }
            else
            if(notificationType == Base::LogStyle::Warning) {
                return QVariant::fromValue(BitmapFactory().pixmapFromSvg(":/icons/Warning.svg",QSize(16, 16)));
            }
            else
            if(notificationType == Base::LogStyle::CriticalMessage) {
                return QVariant::fromValue(BitmapFactory().pixmapFromSvg(":/icons/critical-info.svg",QSize(16, 16)));
            }
            else {
                return QVariant::fromValue(BitmapFactory().pixmapFromSvg(":/icons/info.svg",QSize(16, 16)));
            }
        }

        return QVariant();
    }

    Base::LogStyle notificationType;
    QString notifierName;
    QString msg;
};

class NotificationAreaObserver: public Base::ILogger
{
public:
    NotificationAreaObserver(NotificationArea * notificationarea);
    ~NotificationAreaObserver() override;


    void SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level) override;

    /// name of the observer
    const char *Name() override {return "NotificationAreaObserver";}

private:
    NotificationArea * notificationArea;
};

struct NotificationAreaP
{
    // Non-intrusive notifications
    int currentlyNotifyingIndex = 0;
    int maxOpenNotifications = 15; // Parameter controlled
    unsigned int notificationExpirationTime = 10000; // Parameter controlled
    unsigned int minimumOnScreenTime = 5000; // minimum time that the notification will remain unclosed
    bool notificationsDisabled = false; // Parameter controlled

    // Notification rate controller.
    // After a notification, further notifications within this time
    // are inhibited. If notifications reach inbetween they are shown
    // after this timeout.
    const unsigned int inhibitNotificationTime = 1000;
    bool notificationsDuringInhibitTimer = false; // did notifications arrive during inhibit timer
    bool inhibiting = false; // are we currently inhibiting?

    // Control of confirmation mechanism
    bool requireConfirmationCriticalMessageDuringRestoring = true;

    // NotificationArea control
    unsigned int unread = 0;

    // Widget messages
    int maxWidgetMessages = 1000; // Parameter controlled - maximum number of message allowed in the notification area widget (0 means no limit)

    // Access control
    std::mutex mutexNotification;

    // Pointers to widgets
    QMenu * menu;
    QTreeWidget * table;

    // Message observer
    std::unique_ptr<NotificationAreaObserver> observer;
    Connection finishRestoreDocumentConnection;

    // Parameter observer
    std::unique_ptr<NotificationArea::ParameterObserver> parameterObserver;
};



} // namespace Gui

/***************************************** Console Messages Observer **************************************/

NotificationAreaObserver::NotificationAreaObserver(NotificationArea * notificationarea): notificationArea(notificationarea)
{
    Base::Console().AttachObserver(this);
    bLog = false;                       // ignore log messages
    bMsg = false;                       // ignore messages
    bNotification = true;               // activate user notifications
    bTranslatedNotification = true;     // activate translated user notifications
}

NotificationAreaObserver::~NotificationAreaObserver()
{
    Base::Console().DetachObserver(this);
}

void NotificationAreaObserver::SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level)
{
    // 1. As notification system is shared with report view and others, the convention is that any individual message
    // shall end in "\n".
    // 2. Any QT_TRANSLATE_NOOT string does not comprise newlines.

    auto simplifiedstring = QString::fromStdString(msg).trimmed(); // remove any leading and trailing whitespace character ('\n')

    if(level == Base::LogStyle::TranslatedNotification) {
        notificationArea->pushNotification(QString::fromStdString(notifiername),
                                           simplifiedstring,
                                           level);
    }
    else {
        notificationArea->pushNotification(QString::fromStdString(notifiername),
                                           QCoreApplication::translate("Notifications", simplifiedstring.toUtf8()),
                                           level);
    }
}

/***************************************** Drop menu Action **************************************/

class NotificationsAction : public QWidgetAction
{
public:
    NotificationsAction(QWidget* parent) : QWidgetAction(parent) {}

    auto getTable(){return tableWidget;}
protected:

    QWidget* createWidget(QWidget* parent) override
    {
        QWidget* notificationsWidget = new QWidget(parent);

        QHBoxLayout* layout = new QHBoxLayout(notificationsWidget);
        notificationsWidget->setLayout(layout);

        tableWidget = new QTreeWidget(parent);
        tableWidget->setColumnCount(3);

        QStringList headers;
        headers << QObject::tr("Type") << QObject::tr("Notifier") << QObject::tr("Message");
        tableWidget->setHeaderLabels(headers);

        layout->addWidget(tableWidget);

        tableWidget->setMaximumSize(1200,600);
        tableWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        tableWidget->header()->setStretchLastSection(false);
        tableWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

        tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

        // context menu
        QObject::connect(tableWidget, &QTreeWidget::customContextMenuRequested,
                         [&](const QPoint & pos) {

                            //auto item = tableWidget->itemAt(pos);
                            auto selectedItems = tableWidget->selectedItems();

                            QMenu menu;

                            QAction* del = menu.addAction(tr("Delete"), this, [&]() {
                                for(auto it : selectedItems) {
                                    delete it;
                                }
                            });

                            del->setEnabled(!selectedItems.isEmpty());

                            menu.addSeparator();

                            QAction* delnotifications = menu.addAction(tr("Delete user notifications"), this, [&]() {
                                for(int i = tableWidget->topLevelItemCount()-1; i>=0; i--) {
                                    auto * item = static_cast<NotificationItem *>(tableWidget->topLevelItem(i));
                                    if( item->notificationType == Base::LogStyle::Notification ||
                                        item->notificationType == Base::LogStyle::TranslatedNotification) {
                                        delete item;
                                    }
                                }
                            });

                            delnotifications->setEnabled(tableWidget->topLevelItemCount() > 0);

                            QAction* delall = menu.addAction(tr("Delete All"), this, [&]() {
                                tableWidget->clear();
                            });

                            delall->setEnabled(tableWidget->topLevelItemCount() > 0);

                            menu.setDefaultAction(del);

                            menu.exec(tableWidget->mapToGlobal(pos));
                         });

        return notificationsWidget;
    }

private:
    QTreeWidget * tableWidget;
};

/***************************************** Parameter Observer **************************************/

NotificationArea::ParameterObserver::ParameterObserver(NotificationArea * notificationarea): notificationArea(notificationarea)
{
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/NotificationArea");

    parameterMap = {
        {"NotificationAreaEnabled", [this](const std::string & string){
            auto enabled = hGrp->GetBool(string.c_str(), true);
            if(!enabled)
                notificationArea->deleteLater();}},
        {"NonIntrusiveNotificationsEnabled", [this](const std::string & string){
            auto enabled = hGrp->GetBool(string.c_str(), true);
            notificationArea->d->notificationsDisabled = !enabled;}},
        {"NotificationTime", [this](const std::string & string){
            auto time = hGrp->GetInt(string.c_str(), 20)*1000;
            if(time < 0)
                time = 0;
            notificationArea->d->notificationExpirationTime = static_cast<unsigned int>(time);}},
        {"MinimumOnScreenTime", [this](const std::string & string){
            auto time = hGrp->GetInt(string.c_str(), 5)*1000;
            if(time < 0)
                time = 0;
            notificationArea->d->minimumOnScreenTime = static_cast<unsigned int>(time);}},
        {"MaxOpenNotifications", [this](const std::string & string){
            auto limit = hGrp->GetInt(string.c_str(), 15);
            if(limit < 0)
                limit = 0;
            notificationArea->d->maxOpenNotifications = static_cast<unsigned int>(limit);}},
        {"MaxWidgetMessages", [this](const std::string & string){
            auto limit = hGrp->GetInt(string.c_str(), 1000);
            if(limit < 0)
                limit = 0;
            notificationArea->d->maxWidgetMessages = static_cast<unsigned int>(limit);}},
    };

    for( auto & val : parameterMap ){
        auto string     = val.first;
        auto update     = val.second;

        update(string);
    }

    hGrp->Attach(this);
}

NotificationArea::ParameterObserver::~ParameterObserver()
{
    hGrp->Detach(this);
}

void NotificationArea::ParameterObserver::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    (void) rCaller;

    auto key = parameterMap.find(sReason);

    if( key != parameterMap.end()) {
        auto string     = key->first;
        auto update     = key->second;

        update(string);
    }
}

/***************************************** NotificationArea **************************************/

NotificationArea::NotificationArea(QWidget *parent):QPushButton(parent)
{
    setText(0);
    setFlat(true);

    d = std::make_unique<NotificationAreaP>();

    d->observer = std::make_unique<NotificationAreaObserver>(this);
    d->parameterObserver = std::make_unique<NotificationArea::ParameterObserver>(this);

    d->menu = new QMenu(parent);
    setMenu(d->menu);

    auto na = new NotificationsAction(d->menu);

    d->menu->addAction(na);

    d->table = na->getTable();

    QObject::connect(d->menu, &QMenu::aboutToHide,
                     [&]() {
                         std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip
                         d->unread = 0;
                         d->table->clearSelection();
                         setText(QString::number(d->unread));
                     });

    QObject::connect(d->menu, &QMenu::aboutToShow,
                     [&]() {
                         std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip

                         for (unsigned int i=0; i < d->unread; i++) {
                             d->table->topLevelItem(i)->setSelected(true);
                         }
                     });

    d->finishRestoreDocumentConnection = App::GetApplication().signalFinishRestoreDocument.connect(
        boost::bind(&Gui::NotificationArea::slotRestoreFinished, this, bp::_1)
    );
}

NotificationArea::~NotificationArea()
{
    d->finishRestoreDocumentConnection.disconnect();
}

void NotificationArea::pushNotification(const QString & notifiername, const QString & message, Base::LogStyle level)
{
    auto * item = new NotificationItem(level, notifiername, message);

    bool confirmation = confirmationRequired(level);

    if(confirmation) {
        showConfirmationDialog(notifiername, message);
    }

    std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip

    // Limit the maximum number of messages stored in the widget
    if(d->maxWidgetMessages != 0 && d->table->topLevelItemCount() > d->maxWidgetMessages) {
        delete d->table->topLevelItem(d->table->topLevelItemCount()-1);
    }

    d->table->insertTopLevelItem(0,item);
    d->unread++;

    if(!d->inhibiting) { // rate control (deferred update of UI)
        setText(QString::number(d->unread));
    }

    // If the non-intrusive notifications are disabled then stop here
    if(d->notificationsDisabled)
        return;

    // This controls the maximum number of messages to be displayed (to restrict to a certain area of the screen)
    if(d->currentlyNotifyingIndex > d->maxOpenNotifications) {
        return;
    }

    d->currentlyNotifyingIndex++;

    // rate control (flag controlled by timer)
    // showing the tooltip is relatively expensive
    // and it is useless to update it on each of
    // a cluster of messages
    if(!d->inhibiting) {
        showInNotificationArea();
    }
    else { // mark that we had notifications during the
        d->notificationsDuringInhibitTimer = true;
    }

    QTimer::singleShot(d->notificationExpirationTime, [this](){
        std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification start index while creating the tooltip
        if(d->currentlyNotifyingIndex > 0)
            d->currentlyNotifyingIndex--;
    });

}

bool NotificationArea::confirmationRequired(Base::LogStyle level)
{
    auto userInitiatedRestore = Application::Instance->testStatus(Gui::Application::UserInitiatedOpenDocument);

    return (level == Base::LogStyle::CriticalMessage && userInitiatedRestore && d->requireConfirmationCriticalMessageDuringRestoring);
}

void NotificationArea::showConfirmationDialog(const QString & notifiername, const QString & message)
{
    auto confirmMsg = QObject::tr("Notifier: ") + notifiername + QStringLiteral("\n\n") + message + QStringLiteral("\n\n") + QObject::tr("Do you want to skip confirmation of further critical message notifications while loading the file?");

    auto button = QMessageBox::critical(getMainWindow()->activeWindow(), QObject::tr("Critical Message"), confirmMsg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

    if(button == QMessageBox::Yes)
        d->requireConfirmationCriticalMessageDuringRestoring = false;
}

void NotificationArea::showInNotificationArea()
{
    QString msgw = QString::fromLatin1("<style>p { margin: 0 0 0 0 } td { padding: 0 15px }</style>                     \
    <p style='white-space:nowrap'>                                                                                      \
    <table>                                                                                                             \
     <tr>                                                                                                               \
      <th><small>%1</small></th>                                                                                        \
      <th><small>%2</small></th>                                                                                        \
      <th><small>%3</small></th>                                                                                        \
     </tr>")
        .arg(QObject::tr("Type"))
        .arg(QObject::tr("Notifier"))
        .arg(QObject::tr("Message"));

    if(d->currentlyNotifyingIndex == d->maxOpenNotifications) {
        msgw += QString::fromLatin1("                                                                                   \
        <tr>                                                                                                            \
        <td align='left'><img width=\"16\" height=\"16\" src=':/icons/Warning.svg'></td>                                \
        <td align='left'>FreeCAD</td>                                                                                   \
        <td align='left'>%1</td>                                                                                        \
        </tr>")
        .arg(QObject::tr("Too many opened non-intrusive notifications. Notifications are being omitted!"));
    }

    for(int i = 0 ; i < d->currentlyNotifyingIndex; i++) {
        NotificationItem* item = static_cast<NotificationItem*>(d->table->topLevelItem(i));

        QString iconstr;
        if(item->notificationType == Base::LogStyle::Error) {
            iconstr = QStringLiteral(":/icons/edit_Cancel.svg");
        }
        else
        if(item->notificationType == Base::LogStyle::Warning) {
            iconstr = QStringLiteral(":/icons/Warning.svg");
        }
        else
        if(item->notificationType == Base::LogStyle::CriticalMessage) {
            iconstr = QStringLiteral(":/icons/critical-info.svg");
        }
        else {
            iconstr = QStringLiteral(":/icons/info.svg");
        }

        msgw += QString::fromLatin1("                                                                                   \
        <tr>                                                                                                            \
        <td align='left'><img width=\"16\" height=\"16\" src='%1'></td>                                                 \
        <td align='left'>%2</td>                                                                                        \
        <td align='left'>%3</td>                                                                                        \
        </tr>")
            .arg(iconstr)
            .arg(item->notifierName)
            .arg(item->msg);
    }

    msgw += QString::fromLatin1("</table></p>");


    NotificationBox::showText( this->mapToGlobal( QPoint( ) ), msgw, d->notificationExpirationTime, d->minimumOnScreenTime);

    // This performs rate control, new notifications will be inhibited until the inhibitNotificationTime lapses
    d->inhibiting = true;

    QTimer::singleShot(d->inhibitNotificationTime, [this](){
        if(d->notificationsDuringInhibitTimer) {
            showInNotificationArea();
            setText(QString::number(d->unread));
        }

        d->notificationsDuringInhibitTimer = false;
        d->inhibiting = false;
    });
}

void NotificationArea::slotRestoreFinished(const App::Document&)
{
    // Re-arm on restore critical message modal notifications if another document is loaded
    std::lock_guard<std::mutex> g(d->mutexNotification);
    d->requireConfirmationCriticalMessageDuringRestoring = true;
}
