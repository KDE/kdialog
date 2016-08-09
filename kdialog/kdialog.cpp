//
//  Copyright (C) 1998 Matthias Hoelzer <hoelzer@kde.org>
//  Copyright (C) 2002, 2016 David Faure <faure@kde.org>
//  Copyright (C) 2005 Brad Hards <bradh@frogmouth.net>
//  Copyright (C) 2008 by Dmitry Suzdalev <dimsuz@gmail.com>
//  Copyright (C) 2011 Kai Uwe Broulik <kde@privat.broulik.de>
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

#include "config-kdialog.h"
#include "widgets.h"

#include <kmessagebox.h>
#include <kpassivepopup.h>
#include <krecentdocument.h>

#include <kaboutdata.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kicondialog.h>
#include <kdirselectdialog.h>
#include <kcolordialog.h>
#include <kwindowsystem.h>
#include <kiconloader.h>
#include <KLocalizedString>
#include <kdebug.h>

#include <QApplication>
#include <QDate>
#include <QUrl>
#include <QTimer>
#include <QDesktopWidget>

#include <iostream>

#if defined HAVE_X11 && !defined K_WS_QTONLY
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include "config-kdialog.h"
#ifdef Qt5DBus_FOUND
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#endif

#ifdef Q_WS_WIN
#include <QFileDialog>
#endif
#include <unistd.h>
#include <QCommandLineParser>
#include <QCommandLineOption>

using namespace std;

// this class hooks into the eventloop and outputs the id
// of shown dialogs or makes the dialog transient for other winids.
// Will destroy itself on app exit.
class WinIdEmbedder: public QObject
{
public:
    WinIdEmbedder(bool printID, WId winId):
        QObject(qApp), print(printID), id(winId)
    {
        if (qApp)
            qApp->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject *o, QEvent *e);
private:
    bool print;
    WId id;
};

bool WinIdEmbedder::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Show && o->isWidgetType()
        && o->inherits("KDialog"))
    {
        QWidget *w = static_cast<QWidget*>(o);
        if (print)
            cout << "winId: " << w->winId() << endl;
        if (id)
            KWindowSystem::setMainWindow(w, id);
        deleteLater(); // WinIdEmbedder is not needed anymore after the first dialog was shown
        return false;
    }
    return QObject::eventFilter(o, e);
}

/**
 * Display a passive notification popup using the D-Bus interface, if possible.
 * @return true if the notification was successfully sent, false otherwise.
 */
static bool sendVisualNotification(const QString &text, const QString &title, const QString &icon, int timeout)
{
#ifdef Qt5DBus_FOUND
  const QString dbusServiceName = "org.freedesktop.Notifications";
  const QString dbusInterfaceName = "org.freedesktop.Notifications";
  const QString dbusPath = "/org/freedesktop/Notifications";

  // check if service already exists on plugin instantiation
  QDBusConnectionInterface* interface = QDBusConnection::sessionBus().interface();

  if (!interface || !interface->isServiceRegistered(dbusServiceName)) {
    //kDebug() << dbusServiceName << "D-Bus service not registered";
    return false;
  }

  if (timeout == 0)
    timeout = 10 * 1000;

  QDBusMessage m = QDBusMessage::createMethodCall(dbusServiceName, dbusPath, dbusInterfaceName, "Notify");
  QList<QVariant> args;

  args.append("kdialog"); // app_name
  args.append(0U); // replaces_id
  args.append(icon); // app_icon
  args.append(title); // summary
  args.append(text); // body
  args.append(QStringList()); // actions - unused for plain passive popups
  args.append(QVariantMap()); // hints - unused atm
  args.append(timeout); // expire timout

  m.setArguments(args);

  QDBusMessage replyMsg = QDBusConnection::sessionBus().call(m);
  if(replyMsg.type() == QDBusMessage::ReplyMessage) {
    if (!replyMsg.arguments().isEmpty()) {
      return true;
    }
    // Not displaying any error messages as this is optional for kdialog
    // and KPassivePopup is a perfectly valid fallback.
    //else {
    //  kDebug() << "Error: received reply with no arguments.";
    //}
  } else if (replyMsg.type() == QDBusMessage::ErrorMessage) {
    //kDebug() << "Error: failed to send D-Bus message";
    //kDebug() << replyMsg;
  } else {
    //kDebug() << "Unexpected reply type";
  }
#endif
  return false;
}

static void outputStringList(const QStringList &list, bool separateOutput)
{
    if ( separateOutput) {
        for ( QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it ) {
            cout << (*it).toLocal8Bit().data() << endl;
        }
    } else {
        for ( QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it ) {
            cout << (*it).toLocal8Bit().data() << " ";
        }
        cout << endl;
    }
}

static void outputStringList(const QList<QUrl> &list, bool separateOutput)
{
    if ( separateOutput) {
        for ( auto it = list.constBegin(); it != list.constEnd(); ++it ) {
            cout << (*it).toDisplayString().toLocal8Bit().data() << endl;
        }
    } else {
        for ( auto it = list.constBegin(); it != list.constEnd(); ++it ) {
            cout << (*it).toDisplayString().toLocal8Bit().data() << " ";
        }
        cout << endl;
    }
}


static KGuiItem configuredYes(const QString &text)
{
  return KGuiItem( text, "dialog-ok" );
}

static KGuiItem configuredNo(const QString &text)
{
  return KGuiItem( text, "process-stop" );
}

static KGuiItem configuredCancel(const QString &text)
{
  return KGuiItem( text, "dialog-cancel" );
}

static KGuiItem configuredContinue(const QString &text)
{
  return KGuiItem( text, "arrow-right" );
}

int main(int argc, char *argv[])
{
    KLocalizedString::setApplicationDomain("kdialog");
    QApplication app(argc, argv);

    // enable high dpi support
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KAboutData aboutData( "kdialog", i18n("KDialog"),
            "1.0", i18n( "KDialog can be used to show nice dialog boxes from shell scripts" ),
            KAboutLicense::GPL,
            i18n("(C) 2000, Nick Thompson"));
    aboutData.addAuthor(i18n("David Faure"), i18n("Current maintainer"),"faure@kde.org");
    aboutData.addAuthor(i18n("Brad Hards"), QString(), "bradh@frogmouth.net");
    aboutData.addAuthor(i18n("Nick Thompson"),QString(), 0/*"nickthompson@lucent.com" bounces*/);
    aboutData.addAuthor(i18n("Matthias Hölzer"),QString(),"hoelzer@kde.org");
    aboutData.addAuthor(i18n("David Gümbel"),QString(),"david.guembel@gmx.net");
    aboutData.addAuthor(i18n("Richard Moore"),QString(),"rich@kde.org");
    aboutData.addAuthor(i18n("Dawit Alemayehu"),QString(),"adawit@kde.org");
    aboutData.addAuthor(i18n("Kai Uwe Broulik"),QString(),"kde@privat.broulik.de");
    QApplication::setWindowIcon(QIcon::fromTheme("system-run"));

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("yesno"), i18n("Question message box with yes/no buttons"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("yesnocancel"), i18n("Question message box with yes/no/cancel buttons"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("warningyesno"), i18n("Warning message box with yes/no buttons"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("warningcontinuecancel"), i18n("Warning message box with continue/cancel buttons"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("warningyesnocancel"), i18n("Warning message box with yes/no/cancel buttons"), QLatin1String("text")));

    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("yes-label"), i18n("Use text as Yes button label"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("no-label"), i18n("Use text as No button label"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("cancel-label"), i18n("Use text as Cancel button label"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("continue-label"), i18n("Use text as Continue button label"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("sorry"), i18n("'Sorry' message box"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("detailedsorry"), i18n("'Sorry' message box with expandable Details field"), QLatin1String("text> <details")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("error"), i18n("'Error' message box"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("detailederror"), i18n("'Error' message box with expandable Details field"), QLatin1String("text> <details")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("msgbox"), i18n("Message Box dialog"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("inputbox"), i18n("Input Box dialog"), QLatin1String("text> <init")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("password"), i18n("Password dialog"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("textbox"), i18n("Text Box dialog"), QLatin1String("file")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("textinputbox"), i18n("Text Input Box dialog"), QLatin1String("text> <init")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("combobox"), i18n("ComboBox dialog"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("menu"), i18n("Menu dialog"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("checklist"), i18n("Check List dialog"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("radiolist"), i18n("Radio List dialog"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("passivepopup"), i18n("Passive Popup"), QLatin1String("text> <timeout")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("icon"), i18n("Passive popup icon"), QLatin1String("icon")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("getopenfilename"), i18n("File dialog to open an existing file (arguments [startDir] [filter])")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("getsavefilename"), i18n("File dialog to save a file (arguments [startDir] [filter])")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("getexistingdirectory"), i18n("File dialog to select an existing directory (arguments [startDir])")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("getopenurl"), i18n("File dialog to open an existing URL (arguments [startDir] [filter])")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("getsaveurl"), i18n("File dialog to save a URL (arguments [startDir] [filter])")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("geticon"), i18n("Icon chooser dialog (arguments [group] [context])")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("progressbar"), i18n("Progress bar dialog, returns a D-Bus reference for communication"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("getcolor"), i18n("Color dialog to select a color")));
    // TODO gauge stuff, reading values from stdin
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("title"), i18n("Dialog title"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("default"), i18n("Default entry to use for combobox, menu and color"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("multiple"), i18n("Allows the --getopenurl and --getopenfilename options to return multiple files")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("separate-output"), i18n("Return list items on separate lines (for checklist option and file open with --multiple)")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("print-winid"), i18n("Outputs the winId of each dialog")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("dontagain"), i18n("Config file and option name for saving the \"do-not-show/ask-again\" state"), QLatin1String("file:entry")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("slider"), i18n( "Slider dialog box, returns selected value" ), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("calendar"), i18n( "Calendar dialog box, returns selected date" ), QLatin1String("text")));
    /* kdialog originally used --embed for attaching the dialog box.  However this is misleading and so we changed to --attach.
     * For backwards compatibility, we silently map --embed to --attach */
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("attach"), i18n("Makes the dialog transient for an X app specified by winid"), QLatin1String("winid")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("embed"), i18n("A synonym for --attach"), QLatin1String("winid")));

    parser.addPositionalArgument(QLatin1String("[arg]"), i18n("Arguments - depending on main option"));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // execute kdialog command

    const QStringList args = parser.positionalArguments();
    QString title;
    bool separateOutput = false;
    bool printWId = parser.isSet("print-winid");
    QString defaultEntry;

    // --title text
    if (parser.isSet("title")) {
      title = parser.value("title");
    }

    // --separate-output
    if (parser.isSet("separate-output"))
    {
      separateOutput = true;
    }

    WId winid = 0;
    bool attach = parser.isSet("attach");
    if(attach) {
#ifdef Q_WS_WIN
        winid = reinterpret_cast<WId>(parser.value("attach").toLong(&attach, 0));  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#else
        winid = parser.value("attach").toLong(&attach, 0);  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#endif
    } else if(parser.isSet("embed")) {
        /* KDialog originally used --embed for attaching the dialog box.  However this is misleading and so we changed to --attach.
         * For consistancy, we silently map --embed to --attach */
        attach = true;
#ifdef Q_WS_WIN
        winid = reinterpret_cast<WId>(parser.value("embed").toLong(&attach, 0));  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#else
        winid = parser.value("embed").toLong(&attach, 0);  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#endif
    }

    if (printWId || attach)
    {
      (void)new WinIdEmbedder(printWId, winid);
    }

    // button labels
    // Initialize with default labels
    KGuiItem yesButton = KStandardGuiItem::yes();
    KGuiItem noButton = KStandardGuiItem::no();
    KGuiItem cancelButton = KStandardGuiItem::cancel();
    KGuiItem continueButton = KStandardGuiItem::cont();

    // Customize the asked labels
    if (parser.isSet("yes-label")) {
          yesButton = configuredYes( parser.value("yes-label") );
    }
    if (parser.isSet("no-label")) {
        noButton = configuredNo( parser.value("no-label") );
    }
    if (parser.isSet("cancel-label")) {
        cancelButton = configuredCancel( parser.value("cancel-label") );
    }
    if (parser.isSet("continue-label")) {
        continueButton = configuredContinue( parser.value("continue-label") );
    }

    // --yesno and other message boxes
    KMessageBox::DialogType type = static_cast<KMessageBox::DialogType>(0);
    QByteArray option;
    if (parser.isSet("yesno")) {
        option = "yesno";
        type = KMessageBox::QuestionYesNo;
    }
    else if (parser.isSet("yesnocancel")) {
        option = "yesnocancel";
        type = KMessageBox::QuestionYesNoCancel;
    }
    else if (parser.isSet("warningyesno")) {
        option = "warningyesno";
        type = KMessageBox::WarningYesNo;
    }
    else if (parser.isSet("warningcontinuecancel")) {
        option = "warningcontinuecancel";
        type = KMessageBox::WarningContinueCancel;
    }
    else if (parser.isSet("warningyesnocancel")) {
        option = "warningyesnocancel";
        type = KMessageBox::WarningYesNoCancel;
    }
    else if (parser.isSet("sorry")) {
        option = "sorry";
        type = KMessageBox::Sorry;
    }
    else if (parser.isSet("detailedsorry")) {
        option = "detailedsorry";
    }
    else if (parser.isSet("error")) {
        option = "error";
        type = KMessageBox::Error;
    }
    else if (parser.isSet("detailederror")) {
        option = "detailederror";
    }
    else if (parser.isSet("msgbox")) {
        option = "msgbox";
        type = KMessageBox::Information;
    }

    if ( !option.isEmpty() )
    {
        KConfig* dontagaincfg = NULL;
        // --dontagain
        QString dontagain; // QString()
        if (parser.isSet("dontagain"))
        {
          QString value = parser.value("dontagain");
          QStringList values = value.split( ':', QString::SkipEmptyParts );
          if( values.count() == 2 )
          {
            dontagaincfg = new KConfig( values[ 0 ] );
            KMessageBox::setDontShowAgainConfig( dontagaincfg );
            dontagain = values[ 1 ];
          }
          else
            qDebug( "Incorrect --dontagain!" );
        }
        int ret = 0;

        QString text = Widgets::parseString(parser.value(option));

        QString details;
        if (args.count() == 1) {
            details = Widgets::parseString(args.at(0));
        }

        if ( type == KMessageBox::WarningContinueCancel ) {
            ret = KMessageBox::messageBox( 0, type, text, title, continueButton,
                noButton, cancelButton, dontagain );
        } else if (option == "detailedsorry") {
            KMessageBox::detailedSorry( 0, text, details, title );
        } else if (option == "detailederror") {
            KMessageBox::detailedError( 0, text, details, title );
        } else {
            ret = KMessageBox::messageBox( 0, type, text, title,
                yesButton, noButton, cancelButton, dontagain );
        }
        delete dontagaincfg;
        // ret is 1 for Ok, 2 for Cancel, 3 for Yes, 4 for No and 5 for Continue.
        // We want to return 0 for ok, yes and continue, 1 for no and 2 for cancel
        return (ret == KMessageBox::Ok || ret == KMessageBox::Yes || ret == KMessageBox::Continue) ? 0
                     : ( ret == KMessageBox::No ? 1 : 2 );
    }

    // --inputbox text [init]
    if (parser.isSet("inputbox"))
    {
      QString result;
      QString init;

      if (args.count() > 0) {
          init = args.at(0);
      }

      const bool retcode = Widgets::inputBox(0, title, parser.value("inputbox"), init, result);
      cout << result.toLocal8Bit().data() << endl;
      return retcode ? 0 : 1;
    }


    // --password text
    if (parser.isSet("password"))
    {
      QString result;
      const bool retcode = Widgets::passwordBox(0, title, parser.value("password"), result);
      cout << qPrintable(result) << endl;
      return retcode ? 0 : 1;
    }

    // --passivepopup
    if (parser.isSet("passivepopup"))
    {
        int timeout = 0;
        if (args.count() > 0) {
            timeout = 1000 * args.at(0).toInt();
        }

        if (timeout < 0) {
            timeout = -1;
        }

        // Use --icon parameter for passivepopup as well
        QString icon;
        if (parser.isSet("icon")) {
          icon = parser.value("icon");
        } else {
          icon = "dialog-information";        // Use generic (i)-icon if none specified
        }

        // try to use more stylish notifications
        if (sendVisualNotification(Widgets::parseString(parser.value("passivepopup")), title, icon, timeout))
          return 0;

        // ...did not work, use KPassivePopup as fallback

        // parse timeout time again, so it does not auto-close the fallback (timer cannot handle -1 time)
        if (args.count() > 0) {
            timeout = 1000 * args.at(0).toInt();
        }
        if (timeout <= 0) {
            timeout = 10*1000;    // 10 seconds should be a decent time for auto-closing (you can override this using a parameter)
        }

        QPixmap passiveicon;
        if (parser.isSet("icon")) {  // Only show icon if explicitly requested
            passiveicon = KIconLoader::global()->loadIcon(icon, KIconLoader::Dialog);
        }
        KPassivePopup *popup = KPassivePopup::message( KPassivePopup::Boxed, // style
                                                       title,
                                                       Widgets::parseString(parser.value("passivepopup")),
                                                       passiveicon,
                                                       (QWidget*)0UL, // parent
                                                       timeout );
        KDialog::centerOnScreen( popup );
        QTimer *timer = new QTimer();
        QObject::connect( timer, SIGNAL(timeout()), qApp, SLOT(quit()) );
        QObject::connect( popup, SIGNAL(clicked()), qApp, SLOT(quit()) );
        timer->setSingleShot( true );
        timer->start( timeout );

#ifdef HAVE_X11
        QString geometry;
        if (parser.isSet("geometry"))
                geometry = parser.value("geometry");
    if ( !geometry.isEmpty()) {
            int x, y;
            int w, h;
        int m = XParseGeometry( geometry.toLatin1().constData(), &x, &y, (unsigned int*)&w, (unsigned int*)&h);
            if ( (m & XNegative) )
            x = QApplication::desktop()->width()  + x - w;
            if ( (m & YNegative) )
            y = QApplication::desktop()->height() + y - h;
            popup->setAnchor( QPoint(x, y) );
        }
#endif
    qApp->exec();
        return 0;
      }

    // --textbox file [width] [height]
    if (parser.isSet("textbox"))
    {
        int w = 0;
        int h = 0;

        if (args.count() == 2) {
            w = args.at(0).toInt();
            h = args.at(1).toInt();
        }

        return Widgets::textBox(0, w, h, title, parser.value("textbox"));
    }

    // --textinputbox file [width] [height]
    if (parser.isSet("textinputbox"))
    {
      int w = 400;
      int h = 200;

      if (args.count() >= 3) {
        w = args.at(1).toInt();
        h = args.at(2).toInt();
      }

      QString init;
      if (args.count() >= 1) {
          init = Widgets::parseString(args.at(0));
      }

      QString result;
      int ret = Widgets::textInputBox(0, w, h, title, Widgets::parseString(parser.value("textinputbox")), init, result);
      cout << qPrintable(result) << endl;
      return ret;
    }

    // --combobox <text> item [item] ..."
    if (parser.isSet("combobox")) {
        QStringList list;
        if (args.count() >= 1) {
            for (int i = 0; i < args.count(); i++) {
                list.append(args.at(i));
            }
            const QString text = Widgets::parseString(parser.value("combobox"));
            if (parser.isSet("default")) {
                defaultEntry = parser.value("default");
            }
            QString result;
            const bool retcode = Widgets::comboBox(0, title, text, list, defaultEntry, result);
            cout << result.toLocal8Bit().data() << endl;
            return retcode ? 0 : 1;
        }
        return -1;
    }

    // --menu text [tag item] [tag item] ...
    if (parser.isSet("menu")) {
        QStringList list;
        if (args.count() >= 2) {
            for (int i = 0; i < args.count(); i++) {
                list.append(args.at(i));
            }
            const QString text = Widgets::parseString(parser.value("menu"));
            if (parser.isSet("default")) {
                defaultEntry = parser.value("default");
            }
            QString result;
            const bool retcode = Widgets::listBox(0, title, text, list, defaultEntry, result);
            if (1 == retcode) { // OK was selected
                cout << result.toLocal8Bit().data() << endl;
            }
            return retcode ? 0 : 1;
        }
        return -1;
    }

    // --checklist text [tag item status] [tag item status] ...
    if (parser.isSet("checklist")) {
        QStringList list;
        if (args.count() >= 3) {
            for (int i = 0; i < args.count(); i++) {
                list.append(args.at(i));
            }

            const QString text = Widgets::parseString(parser.value("checklist"));
            QStringList result;

            const bool retcode = Widgets::checkList(0, title, text, list, separateOutput, result);

            for (int i=0; i<result.count(); i++)
                if (!result.at(i).toLocal8Bit().isEmpty()) {
                    cout << result.at(i).toLocal8Bit().data() << endl;
                }
            return retcode ? 0 : 1;
        }
        return -1;
    }

    // --radiolist text width height menuheight [tag item status]
    if (parser.isSet("radiolist")) {
        QStringList list;
        if (args.count() >= 3) {
            for (int i = 0; i < args.count(); i++) {
                list.append(args.at(i));
            }

            const QString text = Widgets::parseString(parser.value("radiolist"));
            QString result;
            const bool retcode = Widgets::radioBox(0, title, text, list, result);
            cout << result.toLocal8Bit().data() << endl;
            return retcode ? 0 : 1;
        }
        return -1;
    }

    // getopenfilename [startDir] [filter]
    if (parser.isSet("getopenfilename")) {
        QString startDir = args.count() > 0 ? args.at(0) : QString();
        const QUrl startUrl = QUrl::fromUserInput(startDir);
        QString filter;
        if (args.count() > 1)  {
            filter = Widgets::parseString(args.at(1));
        }
        KFileDialog dlg( startUrl, filter, 0 );
        dlg.setOperationMode( KFileDialog::Opening );

        if (parser.isSet("multiple")) {
            dlg.setMode(KFile::Files | KFile::LocalOnly);
        } else {
            dlg.setMode(KFile::File | KFile::LocalOnly);
        }
        Widgets::handleXGeometry(&dlg);
        dlg.setWindowTitle(title.isEmpty() ? i18nc("@title:window", "Open") : title);
        dlg.exec();

        if (parser.isSet("multiple")) {
            const QStringList result = dlg.selectedFiles();
            if ( !result.isEmpty() ) {
                outputStringList( result, separateOutput );
                return 0;
            }
        } else {
            const QString result = dlg.selectedFile();
            if (!result.isEmpty())  {
                cout << result.toLocal8Bit().data() << endl;
                return 0;
            }
        }
        return 1; // canceled
    }


    // getsaveurl [startDir] [filter]
    // getsavefilename [startDir] [filter]
    if ( (parser.isSet("getsavefilename") ) || (parser.isSet("getsaveurl") ) ) {
        QString startDir = args.count() > 0 ? args.at(0) : QString();
        QString filter;
        const QUrl startUrl = QUrl::fromUserInput(startDir);

        if (args.count() > 1)  {
            filter = Widgets::parseString(args.at(1));
        }
        // copied from KFileDialog::getSaveFileName(), so we can add geometry
        bool specialDir = startDir.startsWith(QLatin1Char(':'));
        if ( !specialDir ) {
            KFileItem kfi(startUrl);
            specialDir = kfi.isDir();
        }
        KFileDialog dlg( specialDir ? startUrl : QUrl(), filter, 0 );
        if ( !specialDir )
            dlg.setSelection( startDir );
        dlg.setOperationMode( KFileDialog::Saving );
        Widgets::handleXGeometry(&dlg);
        dlg.setWindowTitle(title.isEmpty() ? i18nc("@title:window", "Save As") : title);
        dlg.exec();

        if ( parser.isSet("getsaveurl") ) {
            const QUrl result = dlg.selectedUrl();
            if ( result.isValid())  {
                cout << result.url().toLocal8Bit().data() << endl;
                return 0;
            }
        } else { // getsavefilename
            const QString result = dlg.selectedFile();
            if (!result.isEmpty())  {
                KRecentDocument::add(QUrl::fromLocalFile(result));
                cout << result.toLocal8Bit().data() << endl;
                return 0;
            }
        }
        return 1; // canceled
    }

    // getexistingdirectory [startDir]
    if (parser.isSet("getexistingdirectory")) {
        QString startDir = args.count() > 0 ? args.at(0) : QString();
        const QUrl startUrl = QUrl::fromUserInput(startDir);
        QString result;
#ifdef Q_WS_WIN
        result = QFileDialog::getExistingDirectory( 0, title, startDir,
                                                    QFileDialog::DontResolveSymlinks |
                                                    QFileDialog::ShowDirsOnly);
#else
        QUrl url;
        KDirSelectDialog myDialog( startUrl, true, 0 );


        Widgets::handleXGeometry(&myDialog);
        if ( !title.isEmpty() )
            myDialog.setWindowTitle( title );

        if ( myDialog.exec() == QDialog::Accepted )
            url = myDialog.url();

        if ( url.isValid() )
            result = url.path();
#endif
        if (!result.isEmpty())  {
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // canceled
    }

    // getopenurl [startDir] [filter]
    if (parser.isSet("getopenurl")) {
        QString startDir = args.count() > 0 ? args.at(0) : QString();
        const QUrl startUrl = QUrl::fromUserInput(startDir);

        QString filter;
        if (args.count() > 1)  {
            filter = Widgets::parseString(args.at(1));
        }
        KFileDialog dlg( startUrl, filter, 0 );
        dlg.setOperationMode( KFileDialog::Opening );

        if (parser.isSet("multiple")) {
            dlg.setMode(KFile::Files);
        } else {
            dlg.setMode(KFile::File);
        }
        Widgets::handleXGeometry(&dlg);
        dlg.setWindowTitle(title.isEmpty() ? i18nc("@title:window", "Open") : title);
        dlg.exec();

        if (parser.isSet("multiple")) {
            const QList<QUrl> result = dlg.selectedUrls();
            if ( !result.isEmpty() ) {
                outputStringList( result, separateOutput );
                return 0;
            }
        } else {
            const QUrl result = dlg.selectedUrl();
            if (!result.isEmpty())  {
                cout << result.url().toLocal8Bit().data() << endl;
                return 0;
            }
        }
        return 1; // canceled
    }

    // geticon [group] [context]
    if (parser.isSet("geticon")) {
        QString groupStr, contextStr;
        groupStr = parser.value("geticon");
        if (args.count() >= 1)  {
            contextStr = args.at(0);
        }
        const KIconLoader::Group group =
            ( groupStr == QLatin1String( "Desktop" ) ) ?     KIconLoader::Desktop :
            ( groupStr == QLatin1String( "Toolbar" ) ) ?     KIconLoader::Toolbar :
            ( groupStr == QLatin1String( "MainToolbar" ) ) ? KIconLoader::MainToolbar :
            ( groupStr == QLatin1String( "Small" ) ) ?       KIconLoader::Small :
            ( groupStr == QLatin1String( "Panel" ) ) ?       KIconLoader::Panel :
            ( groupStr == QLatin1String( "Dialog" ) ) ?      KIconLoader::Dialog :
            ( groupStr == QLatin1String( "User" ) ) ?        KIconLoader::User :
            /* else */                                       KIconLoader::NoGroup;

        const KIconLoader::Context context =
            ( contextStr == QLatin1String( "Action" ) ) ?        KIconLoader::Action :
            ( contextStr == QLatin1String( "Application" ) ) ?   KIconLoader::Application :
            ( contextStr == QLatin1String( "Device" ) ) ?        KIconLoader::Device :
            ( contextStr == QLatin1String( "FileSystem" ) ) ?    KIconLoader::FileSystem :
            ( contextStr == QLatin1String( "MimeType" ) ) ?      KIconLoader::MimeType :
            ( contextStr == QLatin1String( "Animation" ) ) ?     KIconLoader::Animation :
            ( contextStr == QLatin1String( "Category" ) ) ?      KIconLoader::Category :
            ( contextStr == QLatin1String( "Emblem" ) ) ?        KIconLoader::Emblem :
            ( contextStr == QLatin1String( "Emote" ) ) ?         KIconLoader::Emote :
            ( contextStr == QLatin1String( "International" ) ) ? KIconLoader::International :
            ( contextStr == QLatin1String( "Place" ) ) ?         KIconLoader::Place :
            ( contextStr == QLatin1String( "StatusIcon" ) ) ?    KIconLoader::StatusIcon :
            // begin: KDE3 compatibility (useful?)
            ( contextStr == QLatin1String( "Devices" ) ) ?       KIconLoader::Device :
            ( contextStr == QLatin1String( "MimeTypes" ) ) ?     KIconLoader::MimeType :
            ( contextStr == QLatin1String( "FileSystems" ) ) ?   KIconLoader::FileSystem :
            ( contextStr == QLatin1String( "Applications" ) ) ?  KIconLoader::Application :
            ( contextStr == QLatin1String( "Actions" ) ) ?       KIconLoader::Action :
            // end: KDE3 compatibility
            /* else */                                           KIconLoader::Any;

        KIconDialog dlg((QWidget*)Q_NULLPTR);
        dlg.setup( group, context);
        dlg.setIconSize(KIconLoader::SizeHuge);

        if (!title.isEmpty())
            dlg.setWindowTitle(title);

        Widgets::handleXGeometry(&dlg);

        const QString result = dlg.openDialog();

        if (!result.isEmpty())  {
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // canceled
    }

    // --progressbar text totalsteps
    if (parser.isSet("progressbar"))
    {
       cout << "org.kde.kdialog-" << getpid() << " /ProgressDialog" << endl;
       if (fork())
           _exit(0);
       close(1);

       int totalsteps = 100;
       const QString text = Widgets::parseString(parser.value("progressbar"));

       if (args.count() == 1)
           totalsteps = args.at(0).toInt();

       return Widgets::progressBar(0, title, text, totalsteps) ? 1 : 0;
    }

    // --getcolor
    if (parser.isSet("getcolor")) {
        KColorDialog dlg((QWidget*)0L, true);

        if (parser.isSet("default")) {
            defaultEntry = parser.value("default");
            dlg.setColor(defaultEntry);
        }
        Widgets::handleXGeometry(&dlg);
        dlg.setWindowTitle(title.isEmpty() ? i18nc("@title:window", "Choose Color") : title);

        if (dlg.exec() == KColorDialog::Accepted) {
            QString result;
            if (dlg.color().isValid()) {
                result = dlg.color().name();
            } else {
                result = dlg.defaultColor().name();
            }
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // cancelled
    }
    if (parser.isSet("slider"))
    {
       int miniValue = 0;
       int maxValue = 0;
       int step = 0;
       const QString text = Widgets::parseString(parser.value("slider"));
       if (args.count() == 3) {
           miniValue = args.at(0).toInt();
           maxValue = args.at( 1 ).toInt();
           step = args.at( 2 ).toInt();
       }
       int result = 0;

       const bool returnCode = Widgets::slider(0, title, text, miniValue, maxValue, step, result);
       if ( returnCode )
           cout << result << endl;
       return returnCode;
    }
    if (parser.isSet("calendar"))
    {
       const QString text = Widgets::parseString(parser.value("calendar"));
       QDate result;

       const bool returnCode = Widgets::calendar(0, title, text, result);
       if ( returnCode )
           cout << result.toString().toLocal8Bit().data() << endl;
       return returnCode;
    }

    parser.showHelp();
    return -2; // NOTREACHED
}
