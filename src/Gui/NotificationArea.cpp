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

class NotificationAreaObserver;

namespace Gui {
/** PImpl idiom structure having all data necessary for the notification area */
struct NotificationAreaP
{
    // Non-intrusive notifications parameters
    int maxOpenNotifications = 15; // Parameter controlled
    unsigned int notificationExpirationTime = 10000; // Parameter controlled
    unsigned int minimumOnScreenTime = 5000; // minimum time that the notification will remain unclosed
    bool notificationsDisabled = false; // Parameter controlled

    // Control of confirmation mechanism
    bool requireConfirmationCriticalMessageDuringRestoring = true;

    // Widget parameters
    int maxWidgetMessages = 1000; // Parameter controlled - maximum number of message allowed in the notification area widget (0 means no limit)

    // Access control
    std::mutex mutexNotification;

    // Control rate of updates of non-intrusive messages
    QTimer inhibitTimer; // Timer to delay notification until a minimum time between two consecutive messages have lapsed
    const unsigned int inhibitNotificationTime = 250; // The time between two consecutive messages forced by the inhibitTimer

    // Pointers to widgets
    QMenu * menu;
    QWidgetAction * notificationaction;

    // Message observer
    std::unique_ptr<NotificationAreaObserver> observer;
    Connection finishRestoreDocumentConnection;

    // Parameter observer
    std::unique_ptr<NotificationArea::ParameterObserver> parameterObserver;
};

} // namespace Gui


/********************************* Resource Management **********************************************************/

/** Simple class to manage Notification Area Resources*/
class ResourceManager {

private:
    ResourceManager() {
        error = BitmapFactory().pixmapFromSvg(":/icons/edit_Cancel.svg",QSize(16, 16));
        warning = BitmapFactory().pixmapFromSvg(":/icons/Warning.svg",QSize(16, 16));
        critical = BitmapFactory().pixmapFromSvg(":/icons/critical-info.svg",QSize(16, 16));
        info = BitmapFactory().pixmapFromSvg(":/icons/info.svg",QSize(16, 16));
    }

    inline static const auto & getResourceManager() {
        static ResourceManager manager;

        return manager;
    }

public:
    inline static auto ErrorPixmap(){
        auto rm = getResourceManager();
        return rm.error;
    }

    inline static auto WarningPixmap(){
        auto rm = getResourceManager();
        return rm.warning;
    }

    inline static auto CriticalPixmap(){
        auto rm = getResourceManager();
        return rm.critical;
    }

    inline static auto InfoPixmap(){
        auto rm = getResourceManager();
        return rm.info;
    }

private:
    QPixmap error;
    QPixmap warning;
    QPixmap critical;
    QPixmap info;
};

/***************************************** Console Messages Observer (Console Interface) ************************/

/** This class listens to all messages sent via the console interface and
   feeds the non-intrusive notification system and the notifications widget */
class NotificationAreaObserver: public Base::ILogger
{
public:
    NotificationAreaObserver(NotificationArea * notificationarea);
    ~NotificationAreaObserver() override;

    /// Function that is called by the console interface for this observer with the message information
    void SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level) override;

    /// Name of the observer
    const char *Name() override {return "NotificationAreaObserver";}

private:
    NotificationArea * notificationArea;
};

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
    // 1. As notification system is shared with report view and others, the expectation is that any individual message
    // will end in "\n". This means the string must be stripped of this character.
    // 2. However, any message marked with the QT_TRANSLATE_NOOT macro with the "Notifications" context, shall not include
    // "\n", as this generates problems with the translation system. Then the string must be stripped of "\n" before translation.

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


/********************************* Notification Widget ***********************************************************/

/** Specialised Item class for the Widget messages/notifications/errors/warnings
   It holds all item specific data, including visualisation data and controls how
   the item should appear in the widget.
*/
class NotificationItem : public QTreeWidgetItem
{
public:
    NotificationItem(Base::LogStyle notificationtype, QString notifiername, QString message):
        notificationType(notificationtype), notifierName(std::move(notifiername)), msg(std::move(message)) {}

    QVariant data(int column, int role) const override {
        // strings that will be displayed for each column of the widget
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
        else // Icons to be displayed for the first row
        if(column == 0 && role == Qt::DecorationRole) {
            if(notificationType == Base::LogStyle::Error) {
                return std::move(ResourceManager::ErrorPixmap());
            }
            else
            if(notificationType == Base::LogStyle::Warning) {
                return std::move(ResourceManager::WarningPixmap());
            }
            else
            if(notificationType == Base::LogStyle::CriticalMessage) {
                return std::move(ResourceManager::CriticalPixmap());
            }
            else {
                return std::move(ResourceManager::InfoPixmap());
            }
        }
        else // Visualisation control of unread messages
        if (role == Qt::FontRole) {
            static QFont font;
            static QFont boldFont(font.family(),font.pointSize(), QFont::Bold);

            if(unread) {
                return boldFont;
            }

            return font;
        }

        return QVariant();
    }

    Base::LogStyle notificationType;
    QString notifierName;
    QString msg;

    bool unread = true;     // item is unread in the Notification Area Widget
    bool notifying = true;  // item is to be notified or being notified as non-intrusive message
    bool shown = false;     // item is already being notified (it is onScreen)
};

/**  Drop menu Action containing the notifications widget */
class NotificationsAction : public QWidgetAction
{
public:
    NotificationsAction(QWidget* parent) : QWidgetAction(parent) {}

    ~NotificationsAction() {
        for(auto * item : pushedItems) {
            if(item) {
                delete item;
            }
        }
    }

public:
    void deleteNotifications() {
        if(tableWidget) {
            for(int i = tableWidget->topLevelItemCount()-1; i>=0; i--) {
                auto * item = static_cast<NotificationItem *>(tableWidget->topLevelItem(i));
                if( item->notificationType == Base::LogStyle::Notification ||
                    item->notificationType == Base::LogStyle::TranslatedNotification) {
                    delete item;
                }
            }
        }
        for (int i = pushedItems.size()-1; i >=0 ; i--) {
            auto * item = static_cast<NotificationItem*>(pushedItems.at(i));
            if( item->notificationType == Base::LogStyle::Notification ||
                item->notificationType == Base::LogStyle::TranslatedNotification) {
                delete pushedItems.takeAt(i);
            }
        }
    }

    void deleteAll() {
        if(tableWidget) {
            tableWidget->clear(); // the parent will delete the items.
        }
        while (!pushedItems.isEmpty())
            delete pushedItems.takeFirst();
    }

    inline int getUnreadCount() const {
        return getCurrently([](auto * item){return item->unread;});
    }

    inline int getCurrentlyNotifyingCount() const {
        return getCurrently([](auto * item){return item->notifying;});
    }

    inline int getShownCount() const {
        return getCurrently([](auto * item){return item->shown;});
    }


    void clearUnreadFlag() {
        for(auto i = 0; i < tableWidget->topLevelItemCount(); i++) { // all messages were read, so clear the unread flag
            auto * item = static_cast<NotificationItem *>(tableWidget->topLevelItem(i));
            item->unread = false;
        }
    }

    // pushes all Notification Items to the Widget, so that they can be shown
    void synchroniseWidget() {
        tableWidget->insertTopLevelItems(0, pushedItems);
        pushedItems.clear();
    }

    // pushes all Notification Items to the fast cache (this also prevents all unnecessary signaling from parent)
    void shiftToCache() {
        tableWidget->blockSignals(true);
        tableWidget->clearSelection();
        while(tableWidget->topLevelItemCount() > 0) {
            auto * item = tableWidget->takeTopLevelItem(0);
            pushedItems.push_back(item);
        }
        tableWidget->blockSignals(false);
    }

    bool isEmpty() const {
        return tableWidget->topLevelItemCount() == 0 && pushedItems.isEmpty();
    }

    auto count() const {
        return tableWidget->topLevelItemCount() + pushedItems.count();
    }

    auto getItem(int index) const {
        if(index < pushedItems.count()) {
            return pushedItems.at(index);
        }
        else {
            return tableWidget->topLevelItem(index-pushedItems.count());
        }
    }

    void deleteItem(int index) {
        if(index < pushedItems.count()) {
            delete pushedItems.takeAt(index);
        }
        else {
            delete tableWidget->topLevelItem(index-pushedItems.count());
        }
    }

    void deleteLastItem() {
        deleteItem(count()-1);
    }

    void push_front(NotificationItem * item) {
        pushedItems.push_front(item);
    }

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

        // context menu on any item (row) of the widget
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

                            QAction* delnotifications = menu.addAction(tr("Delete user notifications"), this, &NotificationsAction::deleteNotifications);

                            delnotifications->setEnabled(tableWidget->topLevelItemCount() > 0);

                            QAction* delall = menu.addAction(tr("Delete All"), this, &NotificationsAction::deleteAll);

                            delall->setEnabled(tableWidget->topLevelItemCount() > 0);

                            menu.setDefaultAction(del);

                            menu.exec(tableWidget->mapToGlobal(pos));
                         });

        return notificationsWidget;
    }

private:
    int getCurrently(std::function<bool(const NotificationItem *)> F) const {
        int instate = 0;
        for(auto i = 0; i < tableWidget->topLevelItemCount(); i++) {
            auto * item = static_cast<NotificationItem *>(tableWidget->topLevelItem(i));
            if(F(item)) {
                instate++;
            }
        }
        for(auto i = 0; i < pushedItems.count(); i++) {
            auto * item = static_cast<NotificationItem *>(pushedItems.at(i));
            if(F(item)) {
                instate++;
            }
        }
        return instate;
    }

private:
    QTreeWidget * tableWidget;
    // Intermediate storage
    // Note: QTreeWidget is helplessly slow to single insertions, QTreeWidget is actually only necessary when showing the widget. A single QList insertion
    // into a QTreeWidget is actually not that slow. The use of this intermediate storage substantially accelerates non-intrusive notifications.
    QList<QTreeWidgetItem *> pushedItems;
};

/***************************************** Parameter Observer (preferences) **************************************/

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

/***************************************** Notification Area *****************************************************/

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

    d->notificationaction = na;

    QObject::connect(d->menu, &QMenu::aboutToHide,
                     [&]() {
                         std::lock_guard<std::mutex> g(d->mutexNotification);
                         static_cast<NotificationsAction *>(d->notificationaction)->clearUnreadFlag();
                         static_cast<NotificationsAction *>(d->notificationaction)->shiftToCache();
                     });

    QObject::connect(d->menu, &QMenu::aboutToShow,
                     [this]() {
                        std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip
                        setText(QString::number(0)); // no unread notifications
                        static_cast<NotificationsAction *>(d->notificationaction)->synchroniseWidget();

                        // the position of the action has already been calculated (for a non-synchronised widget), the size could be recalculated here, but not the
                        // position according to the previous size. This resize event forces this recalculation.
                        QResizeEvent re(d->menu->size(), d->menu->size());
                        qApp->sendEvent(d->menu, &re);
                     });


    d->finishRestoreDocumentConnection = App::GetApplication().signalFinishRestoreDocument.connect(
        boost::bind(&Gui::NotificationArea::slotRestoreFinished, this, bp::_1)
    );

    d->inhibitTimer.setSingleShot(true);

    d->inhibitTimer.callOnTimeout([this, na](){
        d->inhibitTimer.stop();
        setText(QString::number(na->getUnreadCount()));
        showInNotificationArea();
    });
}

NotificationArea::~NotificationArea()
{
    d->finishRestoreDocumentConnection.disconnect();
}

void NotificationArea::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::RightButton && hitButton(e->pos()))
    {
        QMenu menu;

        NotificationsAction * na = static_cast<NotificationsAction *>(d->notificationaction);

        QAction* delnotifications = menu.addAction(tr("Delete user notifications"), [&]() {
            std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip
            na->deleteNotifications();
            setText(QString::number(na->getUnreadCount()));
        });

        delnotifications->setEnabled(!na->isEmpty());

        QAction* delall = menu.addAction(tr("Delete All"), [&]() {
            std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip
            na->deleteAll();
            setText(QString::number(0));
        });

        delall->setEnabled(!na->isEmpty());

        menu.setDefaultAction(delall);

        menu.exec(this->mapToGlobal(e->pos()));
    }
    QPushButton::mousePressEvent(e);
}

void NotificationArea::pushNotification(const QString & notifiername, const QString & message, Base::LogStyle level)
{
    auto * item = new NotificationItem(level, notifiername, message);

    bool confirmation = confirmationRequired(level);

    if(confirmation) {
        showConfirmationDialog(notifiername, message);
    }

    std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip

    NotificationsAction * na = static_cast<NotificationsAction *>(d->notificationaction);

    // Limit the maximum number of messages stored in the widget
    if(d->maxWidgetMessages != 0 && na->count() > d->maxWidgetMessages) {
        na->deleteLastItem();
    }

    na->push_front(item);

    // If the non-intrusive notifications are disabled then stop here
    if(d->notificationsDisabled) {
        item->notifying = false; // avoid mass of old notifications if feature is activated afterwards
        setText(QString::number(static_cast<NotificationsAction *>(d->notificationaction)->getUnreadCount()));
        return;
    }

    // start or restart rate control
    d->inhibitTimer.start(d->inhibitNotificationTime);
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
    std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip

    NotificationsAction * na = static_cast<NotificationsAction *>(d->notificationaction);

    if(!NotificationBox::isVisible()) {
        // The Notification Box may have been closed (by the user by popping it out or by left mouse button) ensure
        // that old notifications are not shown again, even if the timer has not lapsed
        int i = 0;
        while(i < na->count() && static_cast<NotificationItem *>(na->getItem(i))->notifying) {
            NotificationItem* item = static_cast<NotificationItem*>(na->getItem(i));

            if(item->shown) {
                item->notifying = false;
                item->shown = false;
            }

            i++;
        }
    }

    auto currentlyshown = na->getShownCount();

    // If we cannot show more messages, we do no need to update the non-intrusive notification
    if(currentlyshown < d->maxOpenNotifications) {
        // There is space for at least one more notification
        // We update the message with the most recent up to maxOpenNotifications

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

        auto currentlynotifying = na->getCurrentlyNotifyingCount();

        if(currentlynotifying > d->maxOpenNotifications) {
            msgw += QString::fromLatin1("                                                                                   \
            <tr>                                                                                                            \
            <td align='left'><img width=\"16\" height=\"16\" src=':/icons/Warning.svg'></td>                                \
            <td align='left'>FreeCAD</td>                                                                                   \
            <td align='left'>%1</td>                                                                                        \
            </tr>")
            .arg(QObject::tr("Too many opened non-intrusive notifications. Notifications are being omitted!"));
        }

        int i = 0;

        while(i < na->count() && static_cast<NotificationItem *>(na->getItem(i))->notifying) {

            if(i < d->maxOpenNotifications) { // show the first up to maxOpenNotifications
                NotificationItem* item = static_cast<NotificationItem*>(na->getItem(i));

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

                // start a timer for each of these notifications that was not previously shown
                if(!item->shown) {
                    QTimer::singleShot(d->notificationExpirationTime, [this, item](){
                        std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification start index while creating the tooltip

                        if(item) {
                            item->shown = false;
                            item->notifying = false;
                        }
                    });
                }

                // We update the status to shown
                item->shown = true;
            }
            else { // We do not have more space and older notifications will be too old
                static_cast<NotificationItem *>(na->getItem(i))->notifying = false;
                static_cast<NotificationItem *>(na->getItem(i))->shown = false;
            }

            i++;
        }

        msgw += QString::fromLatin1("</table></p>");


        NotificationBox::showText( this->mapToGlobal( QPoint( ) ), msgw, d->notificationExpirationTime, d->minimumOnScreenTime);
    }
}

void NotificationArea::slotRestoreFinished(const App::Document&)
{
    // Re-arm on restore critical message modal notifications if another document is loaded
    d->requireConfirmationCriticalMessageDuringRestoring = true;
}
