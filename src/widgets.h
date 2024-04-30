//  SPDX-FileCopyrightText: 1998 Matthias Hoelzer <hoelzer@kde.org>
//  SPDX-FileCopyrightText: 2002-2005 David Faure <faure@kde.org>
//  SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WIDGETS_H
#define WIDGETS_H

#include <QDate>
#include <QWidget>
#include <QStringList>

namespace Widgets {
bool inputBox(QWidget *parent, const QString &title, const QString &text, const QString &init, QString &result);
bool passwordBox(QWidget *parent, const QString &title, const QString &text, QString &result);
bool newPasswordBox(QWidget *parent, const QString &title, const QString &text, QString &result);
bool textBox(QWidget *parent, int width, int height, const QString &title, const QString &file);
bool imgBox(QWidget *parent, const QString &title, const QString &file);
bool imgInputBox(QWidget *parent, const QString &title, const QString &file, const QString &init, QString &result);
bool textInputBox(QWidget *parent, int width, int height, const QString &title, const QString &text, const QString &init, QString &result);
bool listBox(QWidget *parent, const QString &title, const QString &text, const QStringList &args, const QString &defaultEntry, QString &result);
bool checkList(QWidget *parent, const QString &title, const QString &text, const QStringList &args, bool separateOutput, QStringList &result);
bool radioBox(QWidget *parent, const QString &title, const QString &text, const QStringList &args, QString &result);
bool comboBox(QWidget *parent, const QString &title, const QString &text, const QStringList &args, const QString &defaultEntry, QString &result);
bool slider(QWidget *parent, const QString &title, const QString &test, int minValue, int maxValue, int step, int &result);
bool calendar(QWidget *parent, const QString &title, const QString &text, QDate &result, QDate defaultEntry);
}

#endif
