/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002, 2003 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_MDI_H__
#define __KATE_MDI_H__

#include <kparts/mainwindow.h>

#include <KMultiTabBar>
#include <KXMLGUIClient>
#include <KToggleAction>
#include <KVBox>

#include <q3dict.h>
#include <q3intdict.h>
#include <QMap>
#include <QSplitter>
#include <QPixmap>
#include <QList>
#include <QEvent>
#include <QChildEvent>

class KActionCollection;
class KActionMenu;
class QAction;

namespace KateMDI
{

  class ToolView;

  class ToggleToolViewAction : public KToggleAction
  {
      Q_OBJECT

    public:
      ToggleToolViewAction ( const QString& text, const KShortcut& cut,
                             class ToolView *tv, QObject *parent );

      virtual ~ToggleToolViewAction();

    protected Q_SLOTS:
      void slotToggled(bool);
      void toolVisibleChanged(bool);

    private:
      ToolView *m_tv;
  };

  class GUIClient : public QObject, public KXMLGUIClient
  {
      Q_OBJECT

    public:
      GUIClient ( class MainWindow *mw );
      virtual ~GUIClient();

      void registerToolView (ToolView *tv);
      void unregisterToolView (ToolView *tv);
      void updateSidebarsVisibleAction();

    private Q_SLOTS:
      void clientAdded( KXMLGUIClient *client );
      void updateActions();

    private:
      MainWindow *m_mw;
      KToggleAction *m_showSidebarsAction;
      QList<KAction*> m_toolViewActions;
      QMap<ToolView*, KAction*> m_toolToAction;
      KActionMenu *m_toolMenu;
  };

  class ToolView : public KVBox
  {
      Q_OBJECT

      friend class Sidebar;
      friend class MainWindow;
      friend class GUIClient;
      friend class ToggleToolViewAction;

    protected:
      /**
       * ToolView
       * Objects of this clas represent a toolview in the mainwindow
       * you should only add one widget as child to this toolview, it will
       * be automatically set to be the focus proxy of the toolview
       * @param mainwin main window for this toolview
       * @param sidebar sidebar of this toolview
       * @param parent parent widget, e.g. the splitter of one of the sidebars
       */
      ToolView (class MainWindow *mainwin, class Sidebar *sidebar, QWidget *parent);

    public:
      /**
       * destuct me, this is allowed for all, will care itself that the toolview is removed
       * from the mainwindow and sidebar
       */
      virtual ~ToolView ();

    Q_SIGNALS:
      /**
       * toolview hidden or shown
       * @param visible is this toolview made visible?
       */
      void toolVisibleChanged (bool visible);

      /**
       * some internal methodes needed by the main window and the sidebars
       */
    protected:
      MainWindow *mainWindow ()
      {
        return m_mainWin;
      }

      Sidebar *sidebar ()
      {
        return m_sidebar;
      }

      void setToolVisible (bool vis);

    public:
      bool toolVisible () const;

    protected:
      void childEvent ( QChildEvent *ev );

    private:
      MainWindow *m_mainWin;
      Sidebar *m_sidebar;

      /**
       * unique id
       */
      QString id;

      /**
       * is visible in sidebar
       */
      bool m_toolVisible;

      /**
       * is this view persistent?
       */
      bool persistent;

      QPixmap icon;
      QString text;
  };

  class Sidebar : public KMultiTabBar
  {
      Q_OBJECT

    public:
      Sidebar (KMultiTabBar::KMultiTabBarPosition pos, class MainWindow *mainwin, QWidget *parent);
      virtual ~Sidebar ();

      void setSplitter (QSplitter *sp);

    public:
      ToolView *addWidget (const QPixmap &icon, const QString &text, ToolView *widget);
      bool removeWidget (ToolView *widget);

      bool showWidget (ToolView *widget);
      bool hideWidget (ToolView *widget);

      void setLastSize (int s)
      {
        m_lastSize = s;
      }
      int lastSize () const
      {
        return m_lastSize;
      }
      void updateLastSize ();

      bool splitterVisible () const
      {
        return m_ownSplit->isVisible();
      }

      void restoreSession ();

      /**
      * restore the current session config from given object, use current group
      * @param config config object to use
      */
      void restoreSession (KConfigGroup& config);

      /**
      * save the current session config to given object, use current group
      * @param config config object to use
      */
      void saveSession (KConfigGroup& config);

    public Q_SLOTS:
      // reimplemented, to block a show() call if all sidebars are forced hidden
      virtual void setVisible(bool visible);
    private Q_SLOTS:
      void tabClicked(int);

    protected:
      bool eventFilter(QObject *obj, QEvent *ev);

    private Q_SLOTS:
      void buttonPopupActivate (QAction *);

    private:
      MainWindow *m_mainWin;

      KMultiTabBar::KMultiTabBarPosition m_pos;
      QSplitter *m_splitter;
      KMultiTabBar *m_tabBar;
      QSplitter *m_ownSplit;

      Q3IntDict<ToolView> m_idToWidget;
      QMap<ToolView*, int> m_widgetToId;

      /**
       * list of all toolviews around in this sidebar
       */
      QList<ToolView*> m_toolviews;

      int m_lastSize;

      int m_popupButton;
  };

  class MainWindow : public KParts::MainWindow
  {
      Q_OBJECT

      friend class ToolView;

      //
      // Constructor area
      //
    public:
      /**
       * Constructor
       */
      MainWindow (QWidget* parentWidget = 0);

      /**
       * Destructor
       */
      virtual ~MainWindow ();

      //
      // public interfaces
      //
    public:
      /**
       * central widget ;)
       * use this as parent for your content
       * this widget will get focus if a toolview is hidden
       * @return central widget
       */
      QWidget *centralWidget () const;

      /**
       * add a given widget to the given sidebar if possible, name is very important
       * @param identifier unique identifier for this toolview
       * @param pos position for the toolview, if we are in session restore, this is only a preference
       * @param icon icon to use for the toolview
       * @param text text to use in addition to icon
       * @return created toolview on success or 0
       */
      ToolView *createToolView (const QString &identifier, KMultiTabBar::KMultiTabBarPosition pos, const QPixmap &icon, const QString &text);

      /**
       * give you handle to toolview for the given name, 0 if no toolview around
       * @param identifier toolview name
       * @return toolview if existing, else 0
       */
      ToolView *toolView (const QString &identifier) const;

      /**
       * set the toolview's tabbar style.
       * @param style the tabbar style.
       */
      void setToolViewStyle (KMultiTabBar::KMultiTabBarStyle style);

      /**
       * get the toolview's tabbar style. Call this before @p startRestore(),
       * otherwise you overwrite the usersettings.
       * @return toolview's tabbar style
       */
      KMultiTabBar::KMultiTabBarStyle toolViewStyle () const;

      /**
       * get the sidebars' visibility.
       * @return false, if the sidebars' visibility is forced hidden, otherwise true
       */
      bool sidebarsVisible() const;

    public Q_SLOTS:
      /**
       * set the sidebars' visibility to @p visible. If false, the sidebars
       * are @e always hidden. Usually you do not have to call this because
       * the user can set this in the menu.
       * @param visible sidebars visibility
       */
      void setSidebarsVisible( bool visible );

    protected:
      /**
       * called by toolview destructor
       * @param widget toolview which is destroyed
       */
      void toolViewDeleted (ToolView *widget);

      /**
       * modifiers for existing toolviews
       */
    public:
      /**
       * move a toolview around
       * @param widget toolview to move
       * @param pos position to move too, during session restore, only preference
       * @return success
       */
      bool moveToolView (ToolView *widget, KMultiTabBar::KMultiTabBarPosition pos);

      /**
       * show given toolview, discarded while session restore
       * @param widget toolview to show
       * @return success
       */
      bool showToolView (ToolView *widget);

      /**
       * hide given toolview, discarded while session restore
       * @param widget toolview to hide
       * @return success
       */
      bool hideToolView (ToolView *widget);

      /**
       * session saving and restore stuff
       */
    public:
      /**
       * start the restore
       * @param config config object to use
       * @param group config group to use
       */
      void startRestore (KConfigBase *config, const QString &group);

      /**
       * finish the restore
       */
      void finishRestore ();

      /**
       * save the current session config to given object and group
       * @param config config object to use
       * @param group config group to use
       */
      void saveSession (KConfigGroup &group);

      /**
       * internal data ;)
       */
    private:
      /**
       * map identifiers to widgets
       */
      Q3Dict<ToolView> m_idToWidget;

      /**
       * list of all toolviews around
       */
      QList<ToolView*> m_toolviews;

      /**
       * widget, which is the central part of the
       * main window ;)
       */
      QWidget *m_centralWidget;

      /**
       * horizontal splitter
       */
      QSplitter *m_hSplitter;

      /**
       * vertical splitter
       */
      QSplitter *m_vSplitter;

      /**
       * sidebars for the four sides
       */
      Sidebar *m_sidebars[4];

      /**
       * sidebars state.
       */
      bool m_sidebarsVisible;

      /**
       * config object for session restore, only valid between
       * start and finish restore calls
       */
      KConfigBase *m_restoreConfig;

      /**
       * restore group
       */
      QString m_restoreGroup;

      /**
       * out guiclient
       */
      GUIClient *m_guiClient;
  };

}

#endif

// kate: space-indent on; indent-width 2;
