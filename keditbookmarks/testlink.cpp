/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qtimer.h>
#include <qpainter.h>

#include <kdebug.h>
#include <klocale.h>

#include <krfcdate.h>
#include <kcharsets.h>
#include <kbookmarkmanager.h>

#include "toplevel.h"
#include "listview.h"
#include "testlink.h"
#include "bookmarkiterator.h"

#include <kaction.h>

TestLinkItrHolder *TestLinkItrHolder::s_self = 0;

TestLinkItrHolder::TestLinkItrHolder() 
   : BookmarkIteratorHolder() {
   // do stuff
}

void TestLinkItrHolder::doItrListChanged() {
   KEBApp::self()->setCancelTestsEnabled(m_itrs.count() > 0);
}

/* -------------------------- */

TestLinkItr::TestLinkItr(QValueList<KBookmark> bks)
   : BookmarkIterator(bks) {

   m_job = 0;
}

TestLinkItr::~TestLinkItr() {
   if (m_job) {
      kdDebug() << "JOB kill\n";
      curItem()->restoreStatus();
      m_job->disconnect();
      m_job->kill(false);
   }
}

bool TestLinkItr::isApplicable(const KBookmark &bk) const {
   return (!bk.isGroup() && !bk.isSeparator());
}

void TestLinkItr::doAction() {
   m_errSet = false;
   m_job = KIO::get(m_book.url(), true, false);
   connect(m_job, SIGNAL( result( KIO::Job *)),
           this, SLOT( slotJobResult(KIO::Job *)));
   connect(m_job, SIGNAL( data( KIO::Job *,  const QByteArray &)),
           this, SLOT( slotJobData(KIO::Job *, const QByteArray &)));
   m_job->addMetaData("errorPage", "true");
   curItem()->setTmpStatus(i18n("Checking..."));
}

void TestLinkItr::slotJobData(KIO::Job *job, const QByteArray &data) {
   KIO::TransferJob *transfer = (KIO::TransferJob *)job;

   if (transfer->isErrorPage()) {
      QStringList lines = QStringList::split('\n', data);
      for (QStringList::Iterator it = lines.begin(); it != lines.end(); ++it) {
         int open_pos = (*it).find("<title>", 0, false);
         if (open_pos >= 0) {
            QString leftover = (*it).mid(open_pos + 7);
            int close_pos = leftover.findRev("</title>", -1, false);
            if (close_pos >= 0) {
               // if no end tag found then just 
               // print the first line of the <title>
               leftover = leftover.left(close_pos);
            }
            curItem()->nsPut(KCharsets::resolveEntities(leftover));
            m_errSet = true;
            break;
         }
      }

   } else {
      QString modDate = transfer->queryMetaData("modified");
      if (!modDate.isEmpty()) {
         curItem()->nsPut(QString::number(KRFCDate::parseDate(modDate)));
      }
   }

   transfer->kill(false);
}

void TestLinkItr::slotJobResult(KIO::Job *job) {
   m_job = 0;

   KIO::TransferJob *transfer = (KIO::TransferJob *)job;
   QString modDate = transfer->queryMetaData("modified");

   bool chkErr = true;
   if (transfer->error()) {
      // can we assume that errorString will contain no entities?
      QString jerr = job->errorString();
      if (!jerr.isEmpty()) {
         jerr.replace("\n", " ");
         curItem()->nsPut(jerr);
         chkErr = false;
      }
   }
   
   if (chkErr) {
      if (!modDate.isEmpty()) {
         curItem()->nsPut(QString::number(KRFCDate::parseDate(modDate)));
      } else if (!m_errSet) {
         curItem()->nsPut(QString::number(KRFCDate::parseDate("0")));
      }
   }

   curItem()->modUpdate();
   delayedEmitNextOne();
}

void TestLinkItr::paintCellHelper(QPainter *p, QColorGroup &cg, KEBListViewItem::PaintStyle style) {
   switch (style) {
      case KEBListViewItem::TempStyle: 
      {
         int h, s, v;
         cg.background().hsv(&h,&s,&v);
         QColor color = (v > 180 && v < 220) ? (Qt::darkGray) : (Qt::gray);
         cg.setColor(QColorGroup::Text, color);
         break;
      }
      case KEBListViewItem::BoldStyle:
      {
         QFont font = p->font();
         font.setBold(true);
         p->setFont(font);
         break;
      }
      case KEBListViewItem::DefaultStyle:
         break;
   }
}

/* -------------------------- */

static QString mkTimeStr(int b) {
   QDateTime dt;
   dt.setTime_t(b);
   return (dt.daysTo(QDateTime::currentDateTime()) > 31)
        ? KGlobal::locale()->formatDate(dt.date(), false)
        : KGlobal::locale()->formatDateTime(dt, false);
}

/* -------------------------- */

void TestLinkItrHolder::resetToValue(const QString &url, const QString &oldValue) {
   if (!oldValue.isEmpty()) {
      self()->m_modify[url] = oldValue;
   } else {
      self()->m_modify.remove(url);
   }
}

QString TestLinkItrHolder::getMod(const QString &url) /*const*/ {
   return self()->m_modify.contains(url) 
        ? self()->m_modify[url] 
        : QString::null;
}

QString TestLinkItrHolder::getOldMod(const QString &url) /*const*/ {
   return self()->m_oldModify.contains(url) 
        ? self()->m_oldModify[url] 
        : QString::null;
}

void TestLinkItrHolder::setMod(const QString &url, const QString &val) {
   self()->m_modify[url] = val;
}

void TestLinkItrHolder::setOldMod(const QString &url, const QString &val) {
   self()->m_oldModify[url] = val;
}

/* -------------------------- */

QString TestLinkItrHolder::calcPaintStyle(const QString &url, KEBListViewItem::PaintStyle &_style, const QString &nsinfo) {
   bool newModValid = false;
   int newMod = 0;

   // get new mod date if there is one
   QString newModStr = getMod(url);
   if (!newModStr.isNull()) {
      newMod = newModStr.toInt(&newModValid);
   }

   QString oldModStr;

   if (getOldMod(url).isNull()) {
      // first time
      oldModStr = nsinfo;
      setOldMod(url, oldModStr);

   } else if (!newModStr.isNull()) {
      // umm... nsGet not called here, missing optimisation?
      oldModStr = getOldMod(url);

   } else { 
      // may be reading a second bookmark with same url
      QString oom = nsinfo;
      oldModStr = getOldMod(url);
      if (oom.toInt() > oldModStr.toInt()) {
         setOldMod(url, oom);
         oldModStr = oom;
      }
   }

   int oldMod = oldModStr.toInt(); // TODO - check validity?

   QString statusStr;
   KEBListViewItem::PaintStyle style = KEBListViewItem::DefaultStyle;

   if (!newModStr.isNull() && !newModValid) { 
      // error in current check
      statusStr = newModStr;
      style = (oldMod == 1) 
            ? KEBListViewItem::DefaultStyle : KEBListViewItem::BoldStyle;

   } else if (!newModStr.isNull() && (newMod == 0)) { 
      // no modify time returned
      statusStr = i18n("Ok");

   } else if (!newModStr.isNull() && (newMod >= oldMod)) { 
      // info from current check
      statusStr = mkTimeStr(newMod);
      style = (newMod == oldMod) 
            ? KEBListViewItem::DefaultStyle : KEBListViewItem::BoldStyle;

   } else if (oldMod == 1) { 
      // error in previous check
      statusStr = i18n("Error");

   } else if (oldMod != 0) { 
      // info from previous check
      statusStr = mkTimeStr(oldMod);

   } else {
      return QString::null;
   }

   _style = style;
   return statusStr;
}

/* -------------------------- */

static void parseNsInfo(const QString &nsinfo, QString &nCreate, QString &nAccess, QString &nModify) {
   QStringList sl = QStringList::split(' ', nsinfo);

   for (QStringList::Iterator it = sl.begin(); it != sl.end(); ++it) {
      QStringList spl = QStringList::split('"', (*it));

      if (spl[0] == "LAST_MODIFIED=") {
         nModify = spl[1];
      } else if (spl[0] == "ADD_DATE=") {
         nCreate = spl[1];
      } else if (spl[0] == "LAST_VISIT=") {
         nAccess = spl[1];
      }
   }
}

/* -------------------------- */

const QString KEBListViewItem::nsGet() const {
   QString nCreate, nAccess, nModify;
   QString nsinfo = m_bookmark.internalElement().attribute("netscapeinfo");
   parseNsInfo(nsinfo, nCreate, nAccess, nModify);
   return nModify;
}

/* -------------------------- */

static QString updateNsInfoMod(const QString &_nsinfo, const QString &nm) {
   QString nCreate, nAccess, nModify;
   parseNsInfo(_nsinfo, nCreate, nAccess, nModify);

   bool numValid = false;
   nm.toInt(&numValid);

   QString tmp;
   tmp  =  "ADD_DATE=\"" + ((nCreate.isEmpty()) ? QString::number(time(0)) : nCreate) + "\"";
   tmp += " LAST_VISIT=\"" + ((nAccess.isEmpty()) ? "0" : nAccess) + "\"";
   tmp += " LAST_MODIFIED=\"" + ((numValid) ? nm : "1") + "\"";

   return tmp;
}

void KEBListViewItem::nsPut(const QString &nm) {
   QString tmp = updateNsInfoMod(m_bookmark.internalElement().attribute("netscapeinfo"), nm);
   m_bookmark.internalElement().setAttribute("netscapeinfo", tmp);

   KEBApp::self()->setModifiedFlag(true);
   setText(KEBListView::StatusColumn, nm);
   TestLinkItrHolder::setMod(url(), nm);
}

void KEBListViewItem::modUpdate() {
   QString statusLine;
   // DESIGN - should return paint style?
   statusLine = TestLinkItrHolder::calcPaintStyle(url(), m_paintStyle, nsGet());
   // ARGGGGGGGGGGGGGGGGHHHHHHHHHHHH!!!!!!!!!!!!
   if (statusLine != "Error") {
      setText(KEBListView::StatusColumn, statusLine);
   }
}

const QString KEBListViewItem::url() const {
   return m_bookmark.url().url();
}

void KEBListViewItem::setTmpStatus(const QString &status) {
   kdDebug() << "KEBListViewItem::setTmpStatus" << endl;
   m_paintStyle = KEBListViewItem::BoldStyle;
   m_oldStatus = TestLinkItrHolder::getMod(url());
   setText(KEBListView::StatusColumn, status);
   TestLinkItrHolder::setMod(url(), status);
}

void KEBListViewItem::restoreStatus() {
   if (!m_oldStatus.isNull()) {
      kdDebug() << "KEBListViewItem::restoreStatus" << endl;
      TestLinkItrHolder::resetToValue(url(), m_oldStatus);
      modUpdate();
   }
}

#include "testlink.moc"
