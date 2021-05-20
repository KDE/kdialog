//  SPDX-FileCopyrightText: 2004-2005 Stephan Binner <binner@kde.org>
//  SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QProgressDialog>

class ProgressDialog : public QProgressDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = Q_NULLPTR, const QString &caption = QString(), const QString &text = QString(), int totalSteps = 100);

    void showCancelButton(bool show);
    bool wasCancelled() const
    {
        return wasCanceled();
    }
};

#endif // PROGRESSDIALOG_H
