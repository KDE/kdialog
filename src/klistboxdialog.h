//  SPDX-FileCopyrightText: 1998-2005 Matthias Hoelzer <oelzer@physik.uni-wuerzburg.de>
//
//  SPDX-License-Identifier: GPL-2.0-or-later

#ifndef KLISTBOXDIALOG_H
#define KLISTBOXDIALOG_H

#include <QDialog>

#include <QListWidget>

class QLabel;

class KListBoxDialog : public QDialog
{
    Q_OBJECT

public:

    explicit KListBoxDialog(const QString &text, QWidget *parent = nullptr);
    ~KListBoxDialog() override
    {
    }

    QListWidget &getTable()
    {
        return *table;
    }

    void insertItem(const QString &text);
    void setCurrentItem(const QString &text);
    int currentItem() const;

protected:

    QListWidget *table;
    QLabel *label;
};

#endif
