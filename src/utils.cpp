//  SPDX-FileCopyrightText: 2017 David Faure <faure@kde.org>
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "utils.h"

#include <QApplication>
#include <QDebug>
#include <QScreen>
#include <QWidget>

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
        const QRect screenGeom = dlg->screen()->availableGeometry();
        int x;
        int y;
        int w;
        int h;
        const int m = XParseGeometry(xGeometry.toLatin1().constData(), &x, &y, (unsigned int *)&w, (unsigned int *)&h);
        if ((m & XNegative)) {
            x = screenGeom.width() + x - w;
        }
        if ((m & YNegative)) {
            y = screenGeom.height() + y - h;
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
