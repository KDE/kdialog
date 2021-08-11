//  SPDX-FileCopyrightText: 1998-2005 Matthias Hoelzer <hoelzer@physik.uni-wuerzburg.de>
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "klistboxdialog.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QDialogButtonBox>

KListBoxDialog::KListBoxDialog(const QString &text, QWidget *parent)
    : QDialog(parent)
{
    setModal(true);
    auto *vLayout = new QVBoxLayout(this);

    label = new QLabel(text, this);
    vLayout->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    table = new QListWidget(this);
    vLayout->addWidget(table);
    table->setFocus();

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void KListBoxDialog::insertItem(const QString &item)
{
    table->addItem(item);
    table->setCurrentItem(nullptr);
}

void KListBoxDialog::setCurrentItem(const QString &item)
{
    for (int i = 0; i < table->count(); ++i) {
        if (table->item(i)->text() == item) {
            table->setCurrentItem(table->item(i));
            break;
        }
    }
}

int KListBoxDialog::currentItem() const
{
    return table->currentRow();
}
