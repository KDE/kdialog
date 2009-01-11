//
//  Copyright (C) 1998 Matthias Hoelzer <hoelzer@kde.org>
//  Copyright (C) 2002-2005 David Faure <faure@kde.org>
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


#ifndef WIDGETS_H
#define WIDGETS_H

#include <QDate>
#include <QtCore/QByteRef>
#include <QtGui/QWidget>
#include <QtCore/QStringList>

namespace Widgets
{
    bool inputBox(QWidget *parent, const QString& title, const QString& text, const QString& init, QString &result);
    bool passwordBox(QWidget *parent, const QString& title, const QString& text, QString &result);
    int textBox(QWidget *parent, int width, int height, const QString& title, const QString& file);
    int textInputBox(QWidget *parent, int width, int height, const QString& title, const QStringList& args, QString &result);
    bool listBox(QWidget *parent, const QString& title, const QString& text, const QStringList& args, const QString &defaultEntry, QString &result);
    bool checkList(QWidget *parent, const QString& title, const QString& text, const QStringList& args, bool separateOutput, QStringList &result);
    bool radioBox(QWidget *parent, const QString& title, const QString& text, const QStringList& args, QString &result);
    bool comboBox(QWidget *parent, const QString& title, const QString& text, const QStringList& args, const QString& defaultEntry, QString &result);
    bool progressBar(QWidget *parent, const QString& title, const QString& text, int totalSteps);
    bool slider( QWidget *parent, const QString& title, const QString& test, int minValue, int maxValue, int step, int &result );
    bool calendar( QWidget *parent, const QString &title, const QString &text, QDate & result );

    void handleXGeometry(QWidget * dlg);

}

#endif
