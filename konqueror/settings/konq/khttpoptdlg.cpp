// File khttpoptdlg.cpp by Jacek Konieczny <jajcus@zeus.posl.gliwice.pl>
// Port to KControl by David Faure <faure@kde.org>

#include <qlayout.h> //CT

#include <klocale.h>
#include <kglobal.h>
#include <kapp.h>
#include "khttpoptdlg.h"
#include <kconfig.h>


extern KConfig *g_pConfig;
extern QString g_groupname;


KHTTPOptions::KHTTPOptions(KConfig *config, QString group, QWidget *parent, const char *name)
  : KCModule( parent, name ), g_pConfig(config), groupname(group)
{
  QGridLayout *lay = new QGridLayout(this,9,3,10,5);
  lay->addRowSpacing(0,10);
  lay->addRowSpacing(3,10);
  lay->addRowSpacing(6,10);
  lay->addRowSpacing(8,10);
  lay->addColSpacing(0,10);
  lay->addColSpacing(2,10);
  
  lay->setRowStretch(0,0);
  lay->setRowStretch(1,0);
  lay->setRowStretch(2,0);
  lay->setRowStretch(3,1);
  lay->setRowStretch(4,0);
  lay->setRowStretch(5,0);
  lay->setRowStretch(6,1);
  lay->setRowStretch(7,0);
  lay->setRowStretch(8,20);
  
  lay->setColStretch(0,0);
  lay->setColStretch(1,1);
  lay->setColStretch(2,0);
  
  lb_languages = new QLabel(i18n("Accept languages:"), this);
  lay->addWidget(lb_languages,1,1);

  le_languages = new QLineEdit(this);
  lay->addWidget(le_languages,2,1);
  connect(le_languages, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  
  lb_charsets = new QLabel(i18n("Accept character sets:"), this);
  lay->addWidget(lb_charsets,4,1);
  
  le_charsets = new QLineEdit(this);
  lay->addWidget(le_charsets,5,1);
  connect(le_charsets, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  cb_assumeHTML = new QCheckBox( i18n("Assume HTML"), this );
  lay->addWidget(cb_assumeHTML,7,1);
  connect(cb_assumeHTML, SIGNAL(clicked()), this, SLOT(changed()));

  lay->activate();

  // defaultCharsets = QString("utf-8 ")+klocale->charset()+" iso-8859-1";
  defaultCharsets = QString("utf-8 ")+" iso-8859-1"; // TODO

  // finaly read the options
  load();
}


void KHTTPOptions::load()
{
  QString tmp;
  g_pConfig->setGroup( "Browser Settings/HTTP" );	
  tmp = g_pConfig->readEntry( "AcceptLanguages",KGlobal::locale()->languages());
  le_languages->setText( tmp );
  tmp = g_pConfig->readEntry( "AcceptCharsets",defaultCharsets);
  le_charsets->setText( tmp );

  cb_assumeHTML->setChecked(g_pConfig->readBoolEntry( "AssumeHTML", false ));
}

void KHTTPOptions::save()
{
  g_pConfig->setGroup( "Browser Settings/HTTP" );	
  g_pConfig->writeEntry( "AcceptLanguages", le_languages->text());
  g_pConfig->writeEntry( "AcceptCharsets", le_charsets->text());
  g_pConfig->writeEntry( "AssumeHTML",cb_assumeHTML->isChecked());
  g_pConfig->sync();
}

void KHTTPOptions::defaults()
{
  le_languages->setText( KGlobal::locale()->languages() );
  le_charsets->setText( defaultCharsets );
  cb_assumeHTML->setChecked( false );
}


void KHTTPOptions::changed()
{
  emit KCModule::changed(true);
}


#include "khttpoptdlg.moc"
