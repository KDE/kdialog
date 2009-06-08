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


// Own
#include "widgets.h"
#include <kdatepicker.h>

// Qt
#include <QtCore/QFile>
#include <QtGui/QDesktopWidget>
#include <QtCore/QTextStream>
#include <QtGui/QTextCursor>
#include <QtGui/QLabel>

// KDE
#include <klocale.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <kpassworddialog.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <ktextedit.h>
#include <kvbox.h>

// Local
#include "klistboxdialog.h"
#include "progressdialog.h"


#if defined Q_WS_X11 && ! defined K_WS_QTONLY
#include <netwm.h>
#endif

void Widgets::handleXGeometry(QWidget * dlg)
{
#ifdef Q_WS_X11
	QString geometry;
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs("kde");
	if (args && args->isSet("geometry"))
		geometry = args->getOption("geometry");
    if ( ! geometry.isEmpty()) {
	int x, y;
	int w, h;
	int m = XParseGeometry( geometry.toLatin1(), &x, &y, (unsigned int*)&w, (unsigned int*)&h);
	if ( (m & XNegative) )
	    x = KApplication::desktop()->width()  + x - w;
	if ( (m & YNegative) )
	    y = KApplication::desktop()->height() + y - h;
	dlg->setGeometry(x, y, w, h);
	// kDebug() << "x: " << x << "  y: " << y << "  w: " << w << "  h: " << h;
    }
#endif
}

bool Widgets::inputBox(QWidget *parent, const QString& title, const QString& text, const QString& init, QString &result)
{
  bool ok;
  const QString str = KInputDialog::getText( title, text, init, &ok, parent, 0, 0, QString() );
  if ( ok )
    result = str;
  return ok;
}

bool Widgets::passwordBox(QWidget *parent, const QString& title, const QString& text, QString &result)
{
  KPasswordDialog dlg( parent );
  kapp->setTopWidget( &dlg );
  dlg.setCaption(title);
  dlg.setPrompt(text);

  handleXGeometry(&dlg);

  bool retcode = (dlg.exec() == QDialog::Accepted);
  if ( retcode )
    result = dlg.password();
  return retcode;
}

int Widgets::textBox(QWidget *parent, int width, int height, const QString& title, const QString& file)
{
//  KTextBox dlg(parent, 0, true, width, height, file);
  KDialog dlg( parent );
  dlg.setCaption( title );
  dlg.setButtons( KDialog::Ok );
  dlg.setModal( true );

  kapp->setTopWidget( &dlg );
  KVBox* vbox = new KVBox(&dlg);
  dlg.setMainWidget(vbox);

  KTextEdit *edit = new KTextEdit( vbox );
  edit->setReadOnly(true);

  QFile f(file);
  if (!f.open(QIODevice::ReadOnly))
  {
    kError() << i18n("kdialog: could not open file %1", file) << endl;
    return -1;
  }
  QTextStream s(&f);

  while (!s.atEnd())
    edit->append(s.readLine());

  edit->setTextCursor(QTextCursor(edit->document()));

  f.close();

  if ( width > 0 && height > 0 )
      dlg.setInitialSize( QSize( width, height ) );

  handleXGeometry(&dlg);
  dlg.setCaption(title);
  return (dlg.exec() == KDialog::Accepted) ? 0 : 1;
}

int Widgets::textInputBox(QWidget *parent, int width, int height, const QString& title, const QStringList& args, QString &result)
{
//  KTextBox dlg(parent, 0, true, width, height, file);
  KDialog dlg( parent );
  dlg.setCaption( title );
  dlg.setButtons( KDialog::Ok );
  dlg.setModal( true );

  kapp->setTopWidget( &dlg );
  KVBox* vbox = new KVBox(&dlg);

  dlg.setMainWidget(vbox);

  if( args.count() > 0 )
  {
    QLabel *label = new QLabel(vbox);
    label->setText(args[0]);
  }

  KTextEdit *edit = new KTextEdit( vbox );
  edit->setReadOnly(false);
  edit->setFocus();

  if( args.count() > 1 )
    edit->insertPlainText( args[1] );

  if ( width > 0 && height > 0 )
    dlg.setInitialSize( QSize( width, height ) );

  handleXGeometry(&dlg);
  dlg.setCaption(title);
  const int returnDialogCode = dlg.exec();
  result = edit->toPlainText();
  return (returnDialogCode == KDialog::Accepted ? 0 : 1);
}

bool Widgets::comboBox(QWidget *parent, const QString& title, const QString& text, const QStringList& args,
		       const QString& defaultEntry, QString &result)
{
  KDialog dlg( parent );
  kapp->setTopWidget( &dlg );
  dlg.setCaption( title );
  dlg.setButtons( KDialog::Ok|KDialog::Cancel );
  dlg.setModal( true );
  dlg.setDefaultButton( KDialog::Ok );

  KVBox* vbox = new KVBox( &dlg );
  dlg.setMainWidget( vbox );

  QLabel label (vbox);
  label.setText (text);
  KComboBox combo (vbox);
  combo.insertItems (0, args);
  combo.setCurrentIndex( combo.findText( defaultEntry ) );

  handleXGeometry(&dlg);

  bool retcode = (dlg.exec() == QDialog::Accepted);

  if (retcode)
    result = combo.currentText();

  return retcode;
}

bool Widgets::listBox(QWidget *parent, const QString& title, const QString& text, const QStringList& args,
		      const QString& defaultEntry, QString &result)
{
  KListBoxDialog box(text,parent);

  kapp->setTopWidget( &box );
  box.setCaption(title);

  for (int i = 0; i+1<args.count(); i += 2) {
    box.insertItem(args[i+1]);
  }
  box.setCurrentItem( defaultEntry );

  handleXGeometry(&box);

  const bool retcode = (box.exec() == QDialog::Accepted);
  if ( retcode )
    result = args[ box.currentItem()*2 ];
  return retcode;
}


bool Widgets::checkList(QWidget *parent, const QString& title, const QString& text, const QStringList& args, bool separateOutput, QStringList &result)
{
  QStringList entries, tags;
  QString rs;

  result.clear();

  KListBoxDialog box(text,parent);

  QListWidget &table = box.getTable();

  kapp->setTopWidget( &box );
  box.setCaption(title);

  for (int i=0; i+2<args.count(); i += 3) {
    tags.append(args[i]);
    entries.append(args[i+1]);
  }

  table.addItems(entries);
  table.setSelectionMode(QListWidget::MultiSelection);
  table.setCurrentItem(0); // This is to circumvent a Qt bug

  for (int i=0; i+2<args.count(); i += 3) {
    table.item( i/3 )->setSelected( args[i+2] == QLatin1String("on") );
  }

  handleXGeometry(&box);

  const bool retcode = (box.exec() == QDialog::Accepted);

  if ( retcode ) {
    if (separateOutput) {
      for (int i=0; i<table.count(); i++)
        if (table.item(i)->isSelected())
          result.append(tags[i]);
    } else {
      for (int i=0; i<table.count(); i++)
        if (table.item(i)->isSelected())
          rs += QLatin1String("\"") + tags[i] + QLatin1String("\" ");
      result.append(rs);
    }
  }
  return retcode;
}


bool Widgets::radioBox(QWidget *parent, const QString& title, const QString& text, const QStringList& args, QString &result)
{
  QStringList entries, tags;

  KListBoxDialog box(text,parent);

  QListWidget &table = box.getTable();

  kapp->setTopWidget( &box );
  box.setCaption(title);

  for (int i=0; i+2<args.count(); i += 3) {
    tags.append(args[i]);
    entries.append(args[i+1]);
  }

  table.addItems(entries);

  for (int i=0; i+2<args.count(); i += 3) {
    if (args[i+2] == QLatin1String("on")) {
      table.setCurrentRow(i/3);
    }
  }

  handleXGeometry(&box);

  const bool retcode = (box.exec() == QDialog::Accepted);
  if ( retcode )
    result = tags[ table.currentRow() ];
  return retcode;
}

bool Widgets::progressBar(QWidget *parent, const QString& title, const QString& text, int totalSteps)
{
  ProgressDialog dlg( parent, title, text, totalSteps );
  kapp->setTopWidget( &dlg );
  dlg.setCaption( title );
  handleXGeometry(&dlg);
  dlg.exec();
  return dlg.wasCancelled();
}


bool Widgets::slider( QWidget *parent, const QString& title, const QString& text, int minValue, int maxValue, int step, int &result )
{
    KDialog dlg( parent );
    kapp->setTopWidget( &dlg );
    dlg.setCaption( title );
    dlg.setButtons( KDialog::Ok|KDialog::Cancel );
    dlg.setModal( true );
    dlg.setDefaultButton( KDialog::Ok );

    KVBox* vbox = new KVBox( &dlg );
    dlg.setMainWidget( vbox );

    QLabel label (vbox);
    label.setText (text);
    QSlider slider (vbox);
    slider.setMinimum( minValue );
    slider.setMaximum( maxValue );
    slider.setSingleStep( step );
    slider.setTickPosition ( QSlider::TicksAbove );
    slider.setOrientation( Qt::Horizontal );
    handleXGeometry(&dlg);

    const bool retcode = (dlg.exec() == QDialog::Accepted);

    if (retcode)
        result = slider.value();

    return retcode;
}

bool Widgets::calendar( QWidget *parent, const QString &title, const QString &text, QDate & result )
{
    KDialog dlg( parent );
    kapp->setTopWidget( &dlg );
    dlg.setCaption( title );
    dlg.setButtons( KDialog::Ok|KDialog::Cancel );
    dlg.setModal( true );
    dlg.setDefaultButton( KDialog::Ok );

    KVBox* vbox = new KVBox( &dlg );
    dlg.setMainWidget( vbox );

    QLabel label (vbox);
    label.setText (text);
    KDatePicker dateWidget( vbox );
    handleXGeometry(&dlg);

    const bool retcode = (dlg.exec() == QDialog::Accepted);

    if (retcode)
        result = dateWidget.date();

    return retcode;
}

