//  SPDX-FileCopyrightText: 2017 David Faure <faure@kde.org>
//  SPDX-License-Identifier: GPL-2.0-or-later

#ifndef UTILS_P_H
#define UTILS_P_H

#include <QString>
class QWidget;

namespace Utils {
void setGeometry(const QString &geometry);
void handleXGeometry(QWidget *dlg);
QString parseString(const QString &str);
}

#endif
