//  Copyright (C) 2017 David Faure <faure@kde.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the7 implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "utils.h"
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <config-kdialog.h>

#if defined HAVE_X11 && !defined K_WS_QTONLY
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

static QString xGeometry;

void Utils::setGeometry(const QString &geometry)
{
    xGeometry = geometry;
}

void Utils::handleXGeometry(QWidget *dlg)
{
#ifdef HAVE_X11
    if (!xGeometry.isEmpty()) {
        int x, y;
        int w, h;
        int m = XParseGeometry(xGeometry.toLatin1().constData(), &x, &y, (unsigned int *)&w, (unsigned int *)&h);
        if ((m & XNegative)) {
            x = QApplication::desktop()->width() + x - w;
        }
        if ((m & YNegative)) {
            y = QApplication::desktop()->height() + y - h;
        }
        dlg->setGeometry(x, y, w, h);
        qDebug() << "x: " << x << "  y: " << y << "  w: " << w << "  h: " << h;
    }
#endif
}

QString Utils::parseString(const QString &str)
{
    QString ret;
    ret.reserve(str.size());
    bool escaped = false;
    for (int i = 0; i < str.size(); i++) {
        QChar c = str.at(i);
        if (escaped) {
            escaped = false;
            if (c == QLatin1Char('\\')) {
                ret += c;
            } else if (c == QLatin1Char('n')) {
                ret += QLatin1Char('\n');
            } else {
                qWarning() << qPrintable(QString::fromLatin1("Unrecognized escape sequence \\%1").arg(c));
                ret += QLatin1Char('\\');
                ret += c;
            }
        } else {
            if (c == QLatin1Char('\\')) {
                escaped = true;
            } else {
                ret += c;
            }
        }
    }
    if (escaped) {
        qWarning() << "Unterminated escape sequence";
        ret += QLatin1Char('\\');
    }
    return ret;
}
