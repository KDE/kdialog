//
//  Copyright (C) 1998-2005 Matthias Hoelzer
//  email:  hoelzer@physik.uni-wuerzburg.de
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
#include "klistboxdialog.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QDialogButtonBox>

KListBoxDialog::KListBoxDialog(const QString &text, QWidget *parent)
    : QDialog(parent)
{
    setModal(true);
    QVBoxLayout *vLayout = new QVBoxLayout(this);

    label = new QLabel(text, this);
    vLayout->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    table = new QListWidget(this);
    vLayout->addWidget(table);
    table->setFocus();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

}

void KListBoxDialog::insertItem(const QString& item)
{
    table->addItem(item);
    table->setCurrentItem(nullptr);
}

void KListBoxDialog::setCurrentItem(const QString& item)
{
    for (int i=0; i < table->count(); ++i) {
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
