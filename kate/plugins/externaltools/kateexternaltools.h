/*
   This file is part of the Kate text editor of the KDE project.
   It describes a "external tools" action for kate and provides a dialog
   page to configure that.
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 
   ---
   Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/

#ifndef _KATE_EXTERNAL_TOOLS_H_
#define _KATE_EXTERNAL_TOOLS_H_

#include <ktexteditor/configpage.h>
#include <ktexteditor/commandinterface.h>
#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>

#include <kate/interfaces/plugin.h>
#include <kate/interfaces/mainwindow.h>
#include <kate/interfaces/documentmanager.h>
#include <kate/interfaces/application.h>

#include <KActionMenu>
#include <KMainWindow>
#include <KDialog>
#include <KWordMacroExpander>

#include <QPixmap>
#include <QHash>

/**
 * The external tools action
 * This action creates a menu, in which each item will launch a process
 * with the provided arguments, which may include the following macros:
 * - %URLS: the URLs of all open documents.
 * - %URL: The URL of the active document.
 * - %filedir: The directory of the current document, if that is a local file.
 * - %selection: The selection of the active document.
 * - %text: The text of the active document.
 * - %line: The line number of the cursor in the active view.
 * - %column: The column of the cursor in the active view.
 *
 * Each item has the following properties:
 * - Name: The friendly text for the menu
 * - Exec: The command to execute, including arguments.
 * - TryExec: the name of the executable, if not available, the
 *       item will not be displayed.
 * - MimeTypes: An optional list of mimetypes. The item will be disabled or
 *       hidden if the current file is not of one of the indicated types.
 *
 */
class KateExternalToolsMenuAction : public KActionMenu
{
    friend class KateExternalToolAction;

    Q_OBJECT
  public:
    KateExternalToolsMenuAction( const QString &text, QObject *parent, class Kate::MainWindow *mw = 0 );
    ~KateExternalToolsMenuAction()
    {};

    /**
     * This will load all the confiured services.
     */
    void reload();

    class KActionCollection *actionCollection()
    {
        return m_actionCollection;
    }

  private Q_SLOTS:
    void slotDocumentChanged();

  private:
    class KActionCollection *m_actionCollection;
    Kate::MainWindow *mainwindow; // for the actions to access view/doc managers
};

/**
 * This Action contains a KateExternalTool
 */
class KateExternalToolAction : public KAction, public KWordMacroExpander
{
    Q_OBJECT
  public:
    KateExternalToolAction( QObject *parent, class KateExternalTool *t );
    ~KateExternalToolAction();
  protected:
    virtual bool expandMacro( const QString &str, QStringList &ret );

  private Q_SLOTS:
    void slotRun();

  public:
    class KateExternalTool *tool;
};

/**
 * This class defines a single external tool.
 */
class KateExternalTool
{
  public:
    KateExternalTool( const QString &name = QString(),
                      const QString &command = QString(),
                      const QString &icon = QString(),
                      const QString &tryexec = QString(),
                      const QStringList &mimetypes = QStringList(),
                      const QString &acname = QString(),
                      const QString &cmdname = QString(),
                      int save = 0 );
    ~KateExternalTool()
    {};

    QString name; ///< The name used in the menu.
    QString command; ///< The command to execute.
    QString icon; ///< the icon to use in the menu.
    QString tryexec; ///< The name or path of the executable.
    QStringList mimetypes; ///< Optional list of mimetypes for which this action is valid.
    bool hasexec; ///< This is set by the constructor by calling checkExec(), if a value is present.
    QString acname; ///< The name for the action. This is generated first time the action is is created.
    QString cmdname; ///< The name for the commandline.
    int save; ///< We can save documents prior to activating the tool command: 0 = nothing, 1 = current document, 2 = all documents.

    /**
     * @return true if mimetypes is empty, or the @p mimetype matches.
     */
    bool valid( const QString &mimetype ) const;
    /**
     * @return true if "tryexec" exists and has the executable bit set, or is
     * empty.
     * This is run at least once, and the tool is disabled if it fails.
     */
    bool checkExec();

  private:
    QString m_exec; ///< The fully qualified path of the executable.
};

/**
 * The config widget.
 * The config widget allows the user to view a list of services of the type
 * "Kate/ExternalTool" and add, remove or edit them.
 */
class KateExternalToolsConfigWidget : public KTextEditor::ConfigPage
{
    Q_OBJECT
  public:
    KateExternalToolsConfigWidget( QWidget *parent, const char* name);
    virtual ~KateExternalToolsConfigWidget();

    virtual void apply();
    virtual void reset();
    virtual void defaults()
    {
      reset();
    } // double sigh

  private Q_SLOTS:
    void slotNew();
    void slotEdit();
    void slotRemove();
    void slotInsertSeparator();

    void slotMoveUp();
    void slotMoveDown();

    void slotSelectionChanged();

  private:
    QPixmap blankIcon();

    QStringList m_removed;

    class KListWidget *lbTools;
    class QPushButton *btnNew, *btnRemove, *btnEdit, *btnMoveUp, *btnMoveDwn;

    class KConfig *config;

    bool m_changed;
};

/**
 * A Singleton class for invoking external tools with the view command line
 */
class KateExternalToolsCommand : public KTextEditor::Command
{
  public:
    KateExternalToolsCommand ();
    virtual ~KateExternalToolsCommand ()
    {};
    static KateExternalToolsCommand *self();
    void reload();
  public:
    virtual const QStringList &cmds ();
    virtual bool exec (KTextEditor::View *view, const QString &cmd, QString &msg);
    virtual bool help (KTextEditor::View *view, const QString &cmd, QString &msg);
  private:
    static KateExternalToolsCommand *s_self;
    QStringList m_list;
    QHash<QString, QString> m_map;
    QHash<QString, QString> m_name;
    bool m_inited;
};

/**
 * A Dialog to edit a single KateExternalTool object
 */
class KateExternalToolServiceEditor : public KDialog
{
    Q_OBJECT

  public:

    KateExternalToolServiceEditor( KateExternalTool *tool = 0,
                                   QWidget *parent = 0, const char *name = 0 );

    class QLineEdit *leName, *leExecutable, *leMimetypes, *leCmdLine;
    class Q3TextEdit *teCommand;
    class KIconButton *btnIcon;
    class QComboBox *cmbSave;

  private Q_SLOTS:
    /**
     * Run when the OK button is clicked, to ensure critical values are provided
     */
    void slotOk();
    /**
     * show a mimetype chooser dialog
     */
    void showMTDlg();

  private:
    KateExternalTool *tool;
};
#endif //_KATE_EXTERNAL_TOOLS_H_
// kate: space-indent on; indent-width 2; replace-tabs on;

