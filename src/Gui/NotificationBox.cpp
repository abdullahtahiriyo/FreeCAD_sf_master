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

#ifndef _PreComp_
# include <memory>
# include <mutex>
# include <QApplication>
# include <QAction>
# include <QActionEvent>
# include <QDesktopWidget>
# include <QEvent>
# include <QHBoxLayout>
# include <QHeaderView>
# include <QLabel>
# include <QMenu>
# include <QPointer>
# include <QScreen>
# include <QStyleOption>
# include <QStylePainter>
# include <QStringList>
# include <QTextDocument>
# include <QTimer>
#endif

#include "NotificationBox.h"

using namespace Gui;

class NotificationProxy : public QWidget
{

public:
    NotificationProxy():QWidget()
    {
    }

    void disableTracking() {

        for ( auto * c : this->findChildren<QWidget*>()){
            c->setMouseTracking(false);
            c->installEventFilter(this);
        }
    }

    bool eventFilter(QObject *o, QEvent *e)
    {
        bool ischild = this->findChildren<QObject*>().contains(o);

        if(ischild) {
            switch (e->type()) {
                case QEvent::KeyPress:
                case QEvent::KeyRelease:
                case QEvent::Leave:
                case QEvent::WindowActivate:
                case QEvent::WindowDeactivate:
                case QEvent::FocusIn:
                case QEvent::FocusOut:
                case QEvent::Close:
                case QEvent::MouseButtonPress:
                case QEvent::MouseButtonRelease:
                case QEvent::MouseButtonDblClick:
                case QEvent::Wheel:
                case QEvent::MouseMove:
                    return true;
                default:
                    break;
            }
        }
        return false;
    }
};

/***************************** NotificationBox **********************************/

void NotificationBox::showText(const QPoint &pos, const QString &text, int msecDisplayTime)
{
    static NotificationProxy proxy;

    QToolTip::showText(pos, text, &proxy, QRect(), msecDisplayTime);
    proxy.disableTracking();
}

bool NotificationBox::isVisible()
{
    return QToolTip::isVisible();
}

QString NotificationBox::text()
{
    return QToolTip::text();
}

QPalette NotificationBox::palette()
{
    return QToolTip::palette();
}

QFont NotificationBox::font()
{
    return QToolTip::font();
}

void NotificationBox::setPalette(const QPalette &palette)
{
    return QToolTip::setPalette(palette);
}

void NotificationBox::setFont(const QFont &font)
{
   return QToolTip::setFont(font);
}

#include "NotificationBox.moc"

