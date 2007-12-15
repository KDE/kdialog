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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __toplevel_h
#define __toplevel_h

#include <kxmlguiwindow.h>
#include <k3command.h>
#include <kbookmark.h>
#include <QtGui/QMenu>
#include <kxmlguifactory.h>
#include "bookmarklistview.h"

class KBookmarkModel;
class KBookmarkManager;
class KToggleAction;
class KBookmarkEditorIface;
class BookmarkInfoWidget;
class BookmarkListView;

class CmdHistory : public QObject {
    Q_OBJECT
public:
    CmdHistory(KActionCollection *collection);
    virtual ~CmdHistory() {}

    void notifyDocSaved();

    void clearHistory();
    void addCommand(K3Command *);
    void didCommand(K3Command *);

    //For an explanation see bookmarkInfo::commitChanges()
    void addInFlightCommand(K3Command *);

    static CmdHistory *self();

protected Q_SLOTS:
    void slotCommandExecuted(K3Command *k);

private:
    K3CommandHistory m_commandHistory;
    static CmdHistory *s_self;
};

class KBookmark;
class KBookmarkManager;

class CurrentMgr : public QObject {
    Q_OBJECT
public:
    typedef enum {HTMLExport, OperaExport, IEExport, MozillaExport, NetscapeExport} ExportType;

    static CurrentMgr* self() { if (!s_mgr) { s_mgr = new CurrentMgr(); } return s_mgr; }
    ~CurrentMgr();
    KBookmarkGroup root();
    static KBookmark bookmarkAt(const QString & a);

    KBookmarkModel* model() const { return m_model; }
    KBookmarkManager* mgr() const { return m_mgr; }
    QString path() const;

    void createManager(const QString &filename, const QString &dbusObjectName);
    void notifyManagers(const KBookmarkGroup& grp);
    void notifyManagers();
    bool managerSave();
    void saveAs(const QString &fileName);
    void doExport(ExportType type, const QString & path = QString());
    void setUpdate(bool update);

    void reloadConfig();

    static QString makeTimeStr(const QString &);
    static QString makeTimeStr(int);

protected Q_SLOTS:
    void slotBookmarksChanged(const QString &, const QString &);

private:
    CurrentMgr();
    KBookmarkManager *m_mgr;
    KBookmarkModel *m_model;
    static CurrentMgr *s_mgr;
    uint ignorenext;
};

class KEBApp : public KXmlGuiWindow {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.keditbookmarks")
public:
    static KEBApp* self() { return s_topLevel; }

    KEBApp(const QString & bookmarksFile, bool readonly, const QString &address, bool browser, const QString &caption, const QString& dbusObjectName);
    virtual ~KEBApp();

    void reset(const QString & caption, const QString & bookmarksFileName);

    void updateActions();
    void updateStatus(const QString &url);
    void setActionsEnabled(SelcAbilities);

    void setCancelFavIconUpdatesEnabled(bool);
    void setCancelTestsEnabled(bool);

    void notifyCommandExecuted();
    void findURL(QString url);

    QMenu* popupMenuFactory(const char *type)
    {
        QWidget * menu = factory()->container(type, this);
        return dynamic_cast<QMenu *>(menu);
    }

    KToggleAction* getToggleAction(const char *) const;

    QString caption() const { return m_caption; }
    bool readonly() const { return m_readOnly; }
    bool browser() const { return m_browser; }
    bool nsShown() const;

    BookmarkInfoWidget *bkInfo() { return m_bkinfo; }

    void collapseAll();
    void expandAll();

    enum Column {
      NameColumn = 0,
      UrlColumn = 1,
      CommentColumn = 2,
      StatusColumn = 3
    };
    void startEdit( Column c );
    KBookmark firstSelected() const;
    QString insertAddress() const;
    KBookmark::List selectedBookmarks() const;
    KBookmark::List selectedBookmarksExpanded() const;
    KBookmark::List allBookmarks() const
        { return KBookmark::List();} //FIXME look up what it is suppposed to do, seems like only bookmarks but not folder are returned
public Q_SLOTS:
    Q_SCRIPTABLE QString bookmarkFilename();

public Q_SLOTS:
    void slotConfigureToolbars();

protected Q_SLOTS:
    void slotClipboardDataChanged();
    void slotNewToolbarConfig();
    void selectionChanged();

private:
    void selectedBookmarksExpandedHelper(const KBookmark& bk,
                                         KBookmark::List & bookmarks) const;
    void collapseAllHelper( QModelIndex index );
    void expandAllHelper(QTreeView * view, QModelIndex index);

public: //FIXME
    BookmarkListView * mBookmarkListView;
    BookmarkFolderView * mBookmarkFolderView;
private:

    void resetActions();
    void createActions();

    static KEBApp *s_topLevel;

    CmdHistory *m_cmdHistory;
    QString m_bookmarksFilename;
    QString m_caption;
    QString m_dbusObjectName;

    BookmarkInfoWidget *m_bkinfo;

    bool m_canPaste:1;
    bool m_readOnly:1;
    bool m_browser:1;
};

#endif
