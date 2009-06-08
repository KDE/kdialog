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


#ifndef KLISTBOXDIALOG_H
#define KLISTBOXDIALOG_H

#include <kdialog.h>

#include <QtGui/QListWidget>

class QLabel;

class KListBoxDialog : public KDialog
{
  Q_OBJECT

public:

  explicit KListBoxDialog(const QString &text, QWidget *parent=0);
  ~KListBoxDialog() {}

  QListWidget &getTable() { return *table; }

  void insertItem( const QString& text );
  void setCurrentItem ( const QString& text );
  int currentItem() const;

protected:

  QListWidget *table;
  QLabel *label;

};

#endif
