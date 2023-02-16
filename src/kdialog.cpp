//  SPDX-FileCopyrightText: 1998 Matthias Hoelzer <hoelzer@kde.org>
//  SPDX-FileCopyrightText: 2002, 2016 David Faure <faure@kde.org>
//  SPDX-FileCopyrightText: 2005 Brad Hards <bradh@frogmouth.net>
//  SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>
//  SPDX-FileCopyrightText: 2011 Kai Uwe Broulik <kde@privat.broulik.de>
//  SPDX-FileCopyrightText: 2020 Tristan Miller <psychonaut@nothingisreal.com>
//
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "config-kdialog.h"
#include "widgets.h"
#include "utils.h"
#include "kdialog_version.h"

// KF
#include <KMessageBox>
#include <knotifications_version.h>
#if KNOTIFICATIONS_VERSION < QT_VERSION_CHECK(5, 240, 0)
#include <KPassivePopup>
#endif
#include <KRecentDocument>
#include <KAboutData>
#include <KConfig>
#include <KIconDialog>
#include <KColorMimeData>
#include <KWindowSystem>
#include <KIconLoader>
#include <KLocalizedString>
#include <kwidgetsaddons_version.h>

#if KNOTIFICATIONS_VERSION < QT_VERSION_CHECK(5, 240, 0)
#include <QDesktopWidget>
#endif

// Qt
#include <QApplication>
#include <QDate>
#include <QDateTime>
#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QDBusServiceWatcher>
#include <QDebug>
#include <QFileDialog>
#include <QUrl>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

#include <iostream>
#include <memory>

#if defined HAVE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include <unistd.h>
#include <QCommandLineParser>
#include <QCommandLineOption>

using namespace std;

// this class hooks into the eventloop and outputs the id
// of shown dialogs or makes the dialog transient for other winids.
// Will destroy itself on app exit.
class WinIdEmbedder : public QObject
{
public:
    WinIdEmbedder(bool printID, WId winId)
        : QObject(qApp)
        , print(printID)
        , id(winId)
    {
        if (qApp) {
            qApp->installEventFilter(this);
        }
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
private:
    bool print;
    WId id;
};

bool WinIdEmbedder::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Show && o->isWidgetType()
        && o->inherits("QDialog")) {
        auto w = static_cast<QWidget *>(o);
        if (print) {
            cout << "winId: " << w->winId() << endl;
        }
        if (id) {
            w->setAttribute(Qt::WA_NativeWindow, true);
            KWindowSystem::setMainWindow(w->windowHandle(), id);
        }
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
    const QString dbusServiceName = QStringLiteral("org.freedesktop.Notifications");
    const QString dbusInterfaceName = QStringLiteral("org.freedesktop.Notifications");
    const QString dbusPath = QStringLiteral("/org/freedesktop/Notifications");

    // check if service already exists on plugin instantiation
    QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();

    if (!interface || !interface->isServiceRegistered(dbusServiceName)) {
        //kDebug() << dbusServiceName << "D-Bus service not registered";
        return false;
    }

    if (timeout == 0) {
        timeout = 10 * 1000;
    }

    QDBusMessage m = QDBusMessage::createMethodCall(dbusServiceName, dbusPath, dbusInterfaceName, QStringLiteral("Notify"));
    QList<QVariant> args;

    args.append(QStringLiteral("kdialog")); // app_name
    args.append(0U); // replaces_id
    args.append(icon); // app_icon
    args.append(title); // summary
    args.append(text); // body
    args.append(QStringList()); // actions - unused for plain passive popups
    args.append(QVariantMap()); // hints - unused atm
    args.append(timeout); // expire timeout

    m.setArguments(args);

    QDBusMessage replyMsg = QDBusConnection::sessionBus().call(m);
    if (replyMsg.type() == QDBusMessage::ReplyMessage) {
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

    return false;
}

static void outputStringList(const QStringList &list, bool separateOutput)
{
    if (separateOutput) {
        for (QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
            cout << (*it).toLocal8Bit().data() << endl;
        }
    } else {
        for (QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
            cout << (*it).toLocal8Bit().data() << " ";
        }
        cout << endl;
    }
}

static void outputStringList(const QList<QUrl> &list, bool separateOutput)
{
    if (separateOutput) {
        for (auto it = list.constBegin(); it != list.constEnd(); ++it) {
            cout << (*it).toDisplayString().toLocal8Bit().data() << endl;
        }
    } else {
        for (auto it = list.constBegin(); it != list.constEnd(); ++it) {
            cout << (*it).toDisplayString().toLocal8Bit().data() << " ";
        }
        cout << endl;
    }
}

static KGuiItem configuredYes(const QString &text)
{
    return KGuiItem(text, QStringLiteral("dialog-ok"));
}

static KGuiItem configuredNo(const QString &text)
{
    return KGuiItem(text, QStringLiteral("process-stop"));
}

static KGuiItem configuredCancel(const QString &text)
{
    return KGuiItem(text, QStringLiteral("dialog-cancel"));
}

static KGuiItem configuredContinue(const QString &text)
{
    return KGuiItem(text, QStringLiteral("arrow-right"));
}

static void setFileDialogFilter(QFileDialog &dlg, const QString &filter)
{
    if (filter.contains(QLatin1String("*"))) {
        QString qtFilter = filter;
        dlg.setNameFilter(qtFilter.replace(QLatin1Char('|'), QLatin1Char('\n')));
    } else if (!filter.isEmpty()) {
        dlg.setMimeTypeFilters(filter.trimmed().split(QLatin1Char(' ')));
    }
}

// taken from qfiledialog.cp
static QString initialSelection(const QUrl &url)
{
    if (url.isEmpty()) {
        return QString();
    }
    if (url.isLocalFile()) {
        QFileInfo info(url.toLocalFile());
        if (!info.isDir()) {
            return info.fileName();
        } else {
            return QString();
        }
    }
    // With remote URLs we can only assume.
    return url.fileName();
}

static QUrl initialDirectory(const QUrl &url)
{
    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir()) {
        return url;
    }
    return url.adjusted(QUrl::RemoveFilename);
}

int main(int argc, char *argv[])
{
    // Bug 373677: Qt removes various arguments it treats internally (such as title and icon) from args
    // and applies them to the first Qt::Window, while here we only show dialogs
    // so we need to store them before we even create our QApplication
    QStringList rawArgs;
    for (int i = 0; i < argc; ++i) {
        rawArgs << QString::fromLocal8Bit(argv[i]);
    }

    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kdialog");

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // enable high dpi support
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
    KAboutData aboutData(QStringLiteral("kdialog"), i18n("KDialog"),
                         QStringLiteral(KDIALOG_VERSION_STRING), i18n("KDialog can be used to show nice dialog boxes from shell scripts"),
                         KAboutLicense::GPL,
                         i18n("(C) 2000, Nick Thompson"));
    aboutData.addAuthor(i18n("David Faure"), i18n("Current maintainer"), QStringLiteral("faure@kde.org"));
    aboutData.addAuthor(i18n("Brad Hards"), QString(), QStringLiteral("bradh@frogmouth.net"));
    aboutData.addAuthor(i18n("Nick Thompson"), QString(), QString() /*"nickthompson@lucent.com" bounces*/);
    aboutData.addAuthor(i18n("Matthias Hölzer"), QString(), QStringLiteral("hoelzer@kde.org"));
    aboutData.addAuthor(i18n("David Gümbel"), QString(), QStringLiteral("david.guembel@gmx.net"));
    aboutData.addAuthor(i18n("Richard Moore"), QString(), QStringLiteral("rich@kde.org"));
    aboutData.addAuthor(i18n("Dawit Alemayehu"), QString(), QStringLiteral("adawit@kde.org"));
    aboutData.addAuthor(i18n("Kai Uwe Broulik"), QString(), QStringLiteral("kde@privat.broulik.de"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("yesno"), i18n("Question message box with yes/no buttons"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("yesnocancel"), i18n("Question message box with yes/no/cancel buttons"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("warningyesno"), i18n("Warning message box with yes/no buttons"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("warningcontinuecancel"), i18n("Warning message box with continue/cancel buttons"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("warningyesnocancel"), i18n("Warning message box with yes/no/cancel buttons"), QStringLiteral("text")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("ok-label"), i18n("Use text as OK button label"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("yes-label"), i18n("Use text as Yes button label"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("no-label"), i18n("Use text as No button label"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("cancel-label"), i18n("Use text as Cancel button label"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("continue-label"), i18n("Use text as Continue button label"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("sorry"), i18n("'Sorry' message box"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("detailedsorry"), i18n("'Sorry' message box with expandable Details field"), QStringLiteral("text> <details")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("error"), i18n("'Error' message box"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("detailederror"), i18n("'Error' message box with expandable Details field"), QStringLiteral("text> <details")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("msgbox"), i18n("Message Box dialog"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("inputbox"), i18n("Input Box dialog"), QStringLiteral("text> <init")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("imgbox"), i18n("Image Box dialog"), QStringLiteral("file")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("imginputbox"), i18n("Image Box Input dialog"), QStringLiteral("file> <text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("password"), i18n("Password dialog"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("newpassword"), i18n("New Password dialog"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("textbox"), i18n("Text Box dialog"), QStringLiteral("file")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("textinputbox"), i18n("Text Input Box dialog"), QStringLiteral("text> <init")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("combobox"), i18n("ComboBox dialog"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("menu"), i18n("Menu dialog"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("checklist"), i18n("Check List dialog"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("radiolist"), i18n("Radio List dialog"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("passivepopup"), i18n("Passive Popup"), QStringLiteral("text> <timeout")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("icon"), i18n("Popup icon"), QStringLiteral("icon")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("getopenfilename"), i18n("File dialog to open an existing file (arguments [startDir] [filter])")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("getsavefilename"), i18n("File dialog to save a file (arguments [startDir] [filter])")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("getexistingdirectory"), i18n("File dialog to select an existing directory (arguments [startDir])")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("getopenurl"), i18n("File dialog to open an existing URL (arguments [startDir] [filter])")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("getsaveurl"), i18n("File dialog to save a URL (arguments [startDir] [filter])")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("geticon"), i18n("Icon chooser dialog (arguments [group] [context])")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("progressbar"), i18n("Progress bar dialog, returns a D-Bus reference for communication"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("getcolor"), i18n("Color dialog to select a color")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("format"), i18n("Allow --getcolor to specify output format"), QStringLiteral("text")));
    // TODO gauge stuff, reading values from stdin
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("title"), i18n("Dialog title"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("default"), i18n("Default entry to use for combobox, menu, color, and calendar"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("multiple"), i18n("Allows the --getopenurl and --getopenfilename options to return multiple files")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("separate-output"), i18n("Return list items on separate lines (for checklist option and file open with --multiple)")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("print-winid"), i18n("Outputs the winId of each dialog")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("dontagain"), i18n("Config file and option name for saving the \"do-not-show/ask-again\" state"),
    QStringLiteral("file:entry")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("slider"), i18n("Slider dialog box, returns selected value"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("dateformat"), i18n("Date format for calendar result and/or default value (Qt-style); defaults to 'ddd MMM d yyyy'"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("calendar"), i18n("Calendar dialog box, returns selected date"), QStringLiteral("text")));
    /* kdialog originally used --embed for attaching the dialog box.  However this is misleading and so we changed to --attach.
     * For backwards compatibility, we silently map --embed to --attach */
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("attach"), i18n("Makes the dialog transient for an X app specified by winid"), QStringLiteral("winid")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("embed"), i18n("A synonym for --attach"), QStringLiteral("winid")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("geometry"), i18n("Dialog geometry: [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]"), QStringLiteral("geometry")));

    parser.addPositionalArgument(QStringLiteral("[arg]"), i18n("Arguments - depending on main option"));

    parser.process(rawArgs);
    aboutData.processCommandLine(&parser);

    // execute kdialog command

    QApplication::setWindowIcon(QIcon::fromTheme(parser.value(QStringLiteral("icon")), QIcon::fromTheme(QStringLiteral("system-run"))));

    const QStringList args = parser.positionalArguments();
    const QString title = parser.value(QStringLiteral("title"));
    const bool separateOutput = parser.isSet(QStringLiteral("separate-output"));
    const bool printWId = parser.isSet(QStringLiteral("print-winid"));
    QString defaultEntry;
    const QString geometry = parser.value(QStringLiteral("geometry"));
    Utils::setGeometry(geometry);

    WId winid = 0;
    bool attach = parser.isSet(QStringLiteral("attach"));
    if (attach) {
#ifdef Q_WS_WIN
        winid = reinterpret_cast<WId>(parser.value(QStringLiteral("attach")).toLong(&attach, 0));  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#else
        winid = parser.value(QStringLiteral("attach")).toLong(&attach, 0);  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#endif
    } else if (parser.isSet(QStringLiteral("embed"))) {
        /* KDialog originally used --embed for attaching the dialog box.  However this is misleading and so we changed to --attach.
         * For consistency, we silently map --embed to --attach */
        attach = true;
#ifdef Q_WS_WIN
        winid = reinterpret_cast<WId>(parser.value(QStringLiteral("embed")).toLong(&attach, 0));  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#else
        winid = parser.value(QStringLiteral("embed")).toLong(&attach, 0);  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#endif
    }

    if (printWId || attach) {
        (void)new WinIdEmbedder(printWId, winid);
    }

    // button labels
    // Initialize with default labels
    KGuiItem okButton = KStandardGuiItem::ok();
    KGuiItem yesButton = KStandardGuiItem::yes();
    KGuiItem noButton = KStandardGuiItem::no();
    KGuiItem cancelButton = KStandardGuiItem::cancel();
    KGuiItem continueButton = KStandardGuiItem::cont();

    // Customize the asked labels
    if (parser.isSet(QStringLiteral("ok-label"))) {
        okButton = configuredYes( parser.value(QStringLiteral("ok-label")) );
    }
    if (parser.isSet(QStringLiteral("yes-label"))) {
        yesButton = configuredYes(parser.value(QStringLiteral("yes-label")));
    }
    if (parser.isSet(QStringLiteral("no-label"))) {
        noButton = configuredNo(parser.value(QStringLiteral("no-label")));
    }
    if (parser.isSet(QStringLiteral("cancel-label"))) {
        cancelButton = configuredCancel(parser.value(QStringLiteral("cancel-label")));
    }
    if (parser.isSet(QStringLiteral("continue-label"))) {
        continueButton = configuredContinue(parser.value(QStringLiteral("continue-label")));
    }

    // --yesno and other message boxes
    auto type = static_cast<KMessageBox::DialogType>(0);
    QMessageBox::Icon icon = QMessageBox::Question;
    QByteArray option;
    if (parser.isSet(QStringLiteral("yesno"))) {
        option = "yesno";
        type = KMessageBox::QuestionYesNo;
    } else if (parser.isSet(QStringLiteral("yesnocancel"))) {
        option = "yesnocancel";
        type = KMessageBox::QuestionYesNoCancel;
    } else if (parser.isSet(QStringLiteral("warningyesno"))) {
        option = "warningyesno";
        type = KMessageBox::WarningYesNo;
        icon = QMessageBox::Warning;
    } else if (parser.isSet(QStringLiteral("warningcontinuecancel"))) {
        option = "warningcontinuecancel";
        type = KMessageBox::WarningContinueCancel;
        icon = QMessageBox::Warning;
    } else if (parser.isSet(QStringLiteral("warningyesnocancel"))) {
        option = "warningyesnocancel";
        type = KMessageBox::WarningYesNoCancel;
        icon = QMessageBox::Warning;
    } else if (parser.isSet(QStringLiteral("sorry"))) {
        option = "sorry";
        type = KMessageBox::Error;
        icon = QMessageBox::Warning;
    } else if (parser.isSet(QStringLiteral("detailedsorry"))) {
        option = "detailedsorry";
        type = KMessageBox::Error;
        icon = QMessageBox::Warning;
    } else if (parser.isSet(QStringLiteral("error"))) {
        option = "error";
        type = KMessageBox::Error;
        icon = QMessageBox::Critical;
    } else if (parser.isSet(QStringLiteral("detailederror"))) {
        option = "detailederror";
        type = KMessageBox::Error;
        icon = QMessageBox::Critical;
    } else if (parser.isSet(QStringLiteral("msgbox"))) {
        option = "msgbox";
        type = KMessageBox::Information;
        icon = QMessageBox::Information;
    }

    if (!option.isEmpty()) {
        std::unique_ptr<KConfig> dontagaincfg;
        // --dontagain
        QString dontagain; // QString()
        if (parser.isSet(QStringLiteral("dontagain"))) {
            QString value = parser.value(QStringLiteral("dontagain"));
            const QStringList values = value.split(QLatin1Char(':'), Qt::SkipEmptyParts);
            if (values.count() == 2) {
                dontagaincfg.reset(new KConfig(values[0]));
                KMessageBox::setDontShowAgainConfig(dontagaincfg.get());
                dontagain = values[ 1 ];

                if (type == KMessageBox::WarningContinueCancel) {
                    if (!KMessageBox::shouldBeShownContinue(dontagain)) {
                        return 0;
                    }
                } else {
                    KMessageBox::ButtonCode code;
                    if (!KMessageBox::shouldBeShownYesNo(dontagain, code)) {
                        return code == KMessageBox::Yes ? 0 : 1;
                    }
                }
            } else {
                qDebug("Incorrect --dontagain!");
            }
        }

        QString text = Utils::parseString(parser.value(QString::fromLatin1(option)));

        QString details;
        if (args.count() == 1) {
            details = Utils::parseString(args.at(0));
        }

        QDialog dialog;
        dialog.setWindowTitle(title);
        auto buttonBox = new QDialogButtonBox(&dialog);
        KMessageBox::Options options = KMessageBox::NoExec;

        switch (type) {
        case KMessageBox::QuestionYesNo:
        case KMessageBox::WarningYesNo:
            buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::Yes), yesButton);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::No), noButton);
            break;
        case KMessageBox::QuestionYesNoCancel:
        case KMessageBox::WarningYesNoCancel:
            buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::Yes), yesButton);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::No), noButton);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::Cancel), cancelButton);
            break;
        case KMessageBox::WarningContinueCancel:
            buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::Yes), continueButton);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::No), cancelButton);
            break;
        case KMessageBox::Error:
        case KMessageBox::Information:
            buttonBox->addButton(QDialogButtonBox::Ok);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::Ok), okButton);
            buttonBox->button(QDialogButtonBox::Ok)->setFocus();
            break;
        }
        (void)KMessageBox::createKMessageBox(&dialog,			// dialog
                                             buttonBox,			// buttons
                                             icon,			// icon
                                             text,			// text
                                             {},			// strlist
                                             dontagain.isEmpty() ? QString() : i18n("Do not ask again"), // ask
                                             nullptr,			// checkboxReturn
                                             options,			// options
                                             details);			// details
        Utils::handleXGeometry(&dialog);
        const auto ret = QDialogButtonBox::StandardButton(dialog.exec());
        if (!dontagain.isEmpty()) {
            // We use NoExec in order to call handleXGeometry before exec
            // But that means we need to query the state of the dontShowAgain checkbox ourselves too...
            auto cb = dialog.findChild<QCheckBox *>();
            Q_ASSERT(cb);
            if (cb && cb->isChecked()) {
                if (type == KMessageBox::WarningContinueCancel) {
                    if (ret == QDialogButtonBox::Yes) {
                        KMessageBox::saveDontShowAgainContinue(dontagain);
                    }
                } else if (ret != QDialogButtonBox::Cancel) {
                    KMessageBox::saveDontShowAgainYesNo(dontagain, ret == QDialogButtonBox::Yes ? KMessageBox::Yes : KMessageBox::No);
                }
            }
        }
        // We want to return 0 for ok, yes and continue, 1 for no and 2 for cancel
        return (ret == QDialogButtonBox::Ok || ret == QDialogButtonBox::Yes) ? 0
               : (ret == QDialogButtonBox::No ? 1 : 2);
    }

    // --inputbox text [init]
    if (parser.isSet(QStringLiteral("inputbox"))) {
        QString result;
        QString init;

        if (args.count() > 0) {
            init = args.at(0);
        }

        const bool retcode = Widgets::inputBox(nullptr, title, Utils::parseString(parser.value(QStringLiteral("inputbox"))), init, result);
        cout << result.toLocal8Bit().data() << endl;
        return retcode ? 0 : 1;
    }

    // --password text
    if (parser.isSet(QStringLiteral("password"))) {
        QString result;
        // Note that KPasswordDialog (as of 5.73.0) word-wraps the prompt, so newlines will be converted to spaces
        const bool retcode = Widgets::passwordBox(nullptr, title, Utils::parseString(parser.value(QStringLiteral("password"))), result);
        cout << qPrintable(result) << endl;
        return retcode ? 0 : 1;
    }

    // --newpassword text
    if (parser.isSet(QStringLiteral("newpassword"))) {
        QString result;
        const bool retcode = Widgets::newPasswordBox(nullptr, title, Utils::parseString(parser.value(QStringLiteral("newpassword"))), result);
        cout << qPrintable(result) << endl;
        return retcode ? 0 : 1;
    }

    // --passivepopup
    if (parser.isSet(QStringLiteral("passivepopup"))) {
        int timeout = 0;
        if (args.count() > 0) {
            timeout = 1000 * args.at(0).toInt();
        }

        if (timeout < 0) {
            timeout = -1;
        }

        // Use --icon parameter for passivepopup as well
        QString iconArg;
        if (parser.isSet(QStringLiteral("icon"))) {
            iconArg = parser.value(QStringLiteral("icon"));
        } else {
            iconArg = QStringLiteral("dialog-information");      // Use generic (i)-icon if none specified
        }

        // try to use more stylish notifications
        const bool result = sendVisualNotification(Utils::parseString(parser.value(QStringLiteral("passivepopup"))), title, iconArg, timeout);
        if (result) {
            return 0;
        }

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
#if KNOTIFICATIONS_VERSION < QT_VERSION_CHECK(5, 240, 0)
        // ...did not work, use KPassivePopup as fallback

        // parse timeout time again, so it does not auto-close the fallback (timer cannot handle -1 time)
        if (args.count() > 0) {
            timeout = 1000 * args.at(0).toInt();
        }
        if (timeout <= 0) {
            timeout = 10*1000;    // 10 seconds should be a decent time for auto-closing (you can override this using a parameter)
        }

        QPixmap passiveicon;
        if (parser.isSet(QStringLiteral("icon"))) {  // Only show icon if explicitly requested
            passiveicon = KIconLoader::global()->loadIcon(iconArg, KIconLoader::Dialog);
        }
        KPassivePopup *popup = KPassivePopup::message(KPassivePopup::Boxed,  // style
                                                      title,
                                                      Utils::parseString(parser.value(QStringLiteral("passivepopup"))),
                                                      passiveicon,
                                                      (QWidget *)nullptr, // parent
                                                      timeout);
        auto timer = new QTimer();
        QObject::connect(timer, SIGNAL(timeout()), qApp, SLOT(quit()));
        QObject::connect(popup, QOverload<>::of(&KPassivePopup::clicked), qApp, &QApplication::quit);
        timer->setSingleShot(true);
        timer->start(timeout);

#ifdef HAVE_X11
        if (!geometry.isEmpty()) {
            int x, y;
            int w, h;
            int m = XParseGeometry(geometry.toLatin1().constData(), &x, &y, (unsigned int *)&w, (unsigned int *)&h);
            if ((m & XNegative)) {
                x = QApplication::desktop()->width()  + x - w;
            }
            if ((m & YNegative)) {
                y = QApplication::desktop()->height() + y - h;
            }
            popup->setAnchor(QPoint(x, y));
        }
#endif
#endif // KNOTIFICATIONS_BUILD_DEPRECATED
QT_WARNING_POP
        qApp->exec();
        return 0;
    }

    // --textbox file [width] [height]
    if (parser.isSet(QStringLiteral("textbox"))) {
        int w = 0;
        int h = 0;

        if (args.count() == 2) {
            w = args.at(0).toInt();
            h = args.at(1).toInt();
        }

        return Widgets::textBox(nullptr, w, h, title, parser.value(QStringLiteral("textbox")));
    }

    // --imgbox <file>
    if (parser.isSet(QStringLiteral("imgbox"))) {
        return Widgets::imgBox(nullptr, title, parser.value(QStringLiteral("imgbox")));
    }

    // --imginputbox <file> [text]
    if (parser.isSet(QStringLiteral("imginputbox"))) {
        QString result;
        QString text;

        if (args.count() > 0) {
            text = args.at(0);
        }

        const bool retcode = Widgets::imgInputBox(nullptr, title, parser.value(QStringLiteral("imginputbox")), Utils::parseString(text), result);
        cout << result.toLocal8Bit().data() << endl;
        return retcode ? 0 : 1;
    }

    // --textinputbox file [width] [height]
    if (parser.isSet(QStringLiteral("textinputbox"))) {
        int w = 400;
        int h = 200;

        if (args.count() >= 3) {
            w = args.at(1).toInt();
            h = args.at(2).toInt();
        }

        QString init;
        if (args.count() >= 1) {
            init = Utils::parseString(args.at(0));
        }

        QString result;
        int ret = Widgets::textInputBox(nullptr, w, h, title, Utils::parseString(parser.value(QStringLiteral("textinputbox"))), init, result);
        cout << qPrintable(result) << endl;
        return ret;
    }

    // --combobox <text> item [item] ..."
    if (parser.isSet(QStringLiteral("combobox"))) {
        QStringList list;
        if (args.count() >= 1) {
            for (int i = 0; i < args.count(); i++) {
                list.append(args.at(i));
            }
            const QString text = Utils::parseString(parser.value(QStringLiteral("combobox")));
            if (parser.isSet(QStringLiteral("default"))) {
                defaultEntry = parser.value(QStringLiteral("default"));
            }
            QString result;
            const bool retcode = Widgets::comboBox(nullptr, title, text, list, defaultEntry, result);
            cout << result.toLocal8Bit().data() << endl;
            return retcode ? 0 : 1;
        }
        cerr << qPrintable(i18n("Syntax: --combobox <text> item [item] ...")) << endl;
        return -1;
    }

    // --menu text tag item [tag item] ...
    if (parser.isSet(QStringLiteral("menu"))) {
        QStringList list;
        if (args.count() >= 2) {
            for (int i = 0; i < args.count(); i++) {
                list.append(args.at(i));
            }
            const QString text = Utils::parseString(parser.value(QStringLiteral("menu")));
            if (parser.isSet(QStringLiteral("default"))) {
                defaultEntry = parser.value(QStringLiteral("default"));
            }
            QString result;
            const bool retcode = Widgets::listBox(nullptr, title, text, list, defaultEntry, result);
            if (1 == retcode) { // OK was selected
                cout << result.toLocal8Bit().data() << endl;
            }
            return retcode ? 0 : 1;
        }
        cerr << qPrintable(i18n("Syntax: --menu text tag item [tag item] ...")) << endl;
        return -1;
    }

    // --checklist text tag item status [tag item status] ...
    if (parser.isSet(QStringLiteral("checklist"))) {
        QStringList list;
        if (args.count() >= 3) {
            for (int i = 0; i < args.count(); i++) {
                list.append(args.at(i));
            }

            const QString text = Utils::parseString(parser.value(QStringLiteral("checklist")));
            QStringList result;

            const bool retcode = Widgets::checkList(nullptr, title, text, list, separateOutput, result);

            for (int i = 0; i < result.count(); i++) {
                if (!result.at(i).isEmpty()) {
                    cout << result.at(i).toLocal8Bit().data() << endl;
                }
            }
            return retcode ? 0 : 1;
        }
        cerr << qPrintable(i18n("Syntax: --checklist text tag item on/off [tag item on/off] ...")) << endl;
        return -1;
    }

    // --radiolist text tag item status [tag item status] ...
    if (parser.isSet(QStringLiteral("radiolist"))) {
        QStringList list;
        if (args.count() >= 3) {
            for (int i = 0; i < args.count(); i++) {
                list.append(args.at(i));
            }

            const QString text = Utils::parseString(parser.value(QStringLiteral("radiolist")));
            QString result;
            const bool retcode = Widgets::radioBox(nullptr, title, text, list, result);
            cout << result.toLocal8Bit().data() << endl;
            return retcode ? 0 : 1;
        }
        cerr << qPrintable(i18n("Syntax: --radiolist text tag item on/off [tag item on/off] ...")) << endl;
        return -1;
    }

    // getopenfilename [startDir] [filter]
    // getopenurl [startDir] [filter]
    if (parser.isSet(QStringLiteral("getopenfilename")) || parser.isSet(QStringLiteral("getopenurl"))) {
        QString startDir = args.count() > 0 ? args.at(0) : QString();
        const QUrl startUrl = QUrl::fromUserInput(startDir);
        QString filter;
        if (args.count() > 1) {
            filter = Utils::parseString(args.at(1));
        }
        const bool multiple = parser.isSet(QStringLiteral("multiple"));

        QFileDialog dlg;
        dlg.setAcceptMode(QFileDialog::AcceptOpen);
        dlg.setDirectoryUrl(initialDirectory(startUrl));
        dlg.selectFile(initialSelection(startUrl));

        if (multiple) {
            dlg.setFileMode(QFileDialog::ExistingFiles);
        } else {
            dlg.setFileMode(QFileDialog::ExistingFile);
        }
        const bool openUrls = parser.isSet(QStringLiteral("getopenurl"));
        if (!openUrls) {
            dlg.setSupportedSchemes({QStringLiteral("file")});
        }
        setFileDialogFilter(dlg, filter);
        Utils::handleXGeometry(&dlg);
        dlg.setWindowTitle(title.isEmpty() ? i18nc("@title:window", "Open") : title);
        if (!dlg.exec()) {
            return 1; // canceled
        }

        if (openUrls) {
            const QList<QUrl> result = dlg.selectedUrls();
            if (!result.isEmpty()) {
                if (multiple) {
                    outputStringList(result, separateOutput);
                    return 0;
                } else {
                    cout << result.at(0).url().toLocal8Bit().data() << endl;
                    return 0;
                }
            }
        } else {
            const QStringList result = dlg.selectedFiles();
            if (!result.isEmpty()) {
                if (multiple) {
                    outputStringList(result, separateOutput);
                    return 0;
                } else {
                    cout << result.at(0).toLocal8Bit().data() << endl;
                    return 0;
                }
            }
        }
        return 1;
    }

    // getsaveurl [startDir] [filter]
    // getsavefilename [startDir] [filter]
    if ((parser.isSet(QStringLiteral("getsavefilename"))) || (parser.isSet(QStringLiteral("getsaveurl")))) {
        QString startDir = args.count() > 0 ? args.at(0) : QString();
        QString filter;
        const QUrl startUrl = QUrl::fromUserInput(startDir);

        if (args.count() > 1) {
            filter = Utils::parseString(args.at(1));
        }
        QFileDialog dlg;
        dlg.setAcceptMode(QFileDialog::AcceptSave);
        dlg.setFileMode(QFileDialog::AnyFile);
        dlg.setDirectoryUrl(initialDirectory(startUrl));
        dlg.selectFile(initialSelection(startUrl));
        setFileDialogFilter(dlg, filter);
        const bool saveUrls = parser.isSet(QStringLiteral("getsaveurl"));
        if (!saveUrls) {
            dlg.setSupportedSchemes({QStringLiteral("file")});
        }
        Utils::handleXGeometry(&dlg);
        dlg.setWindowTitle(title.isEmpty() ? i18nc("@title:window", "Save As") : title);
        if (!dlg.exec()) {
            return 1; // canceled
        }

        if (saveUrls) {
            const QList<QUrl> result = dlg.selectedUrls();
            if (!result.isEmpty()) {
                cout << result.at(0).toString().toLocal8Bit().data() << endl;
                return 0;
            }
        } else { // getsavefilename
            const QStringList result = dlg.selectedFiles();
            if (!result.isEmpty()) {
                const QString file = result.at(0);
                KRecentDocument::add(QUrl::fromLocalFile(file));
                cout << file.toLocal8Bit().data() << endl;
                return 0;
            }
        }
        return 1; // canceled
    }

    // getexistingdirectory [startDir]
    if (parser.isSet(QStringLiteral("getexistingdirectory"))) {
        QString startDir = args.count() > 0 ? args.at(0) : QString();
        const QUrl startUrl = QUrl::fromUserInput(startDir);

        QFileDialog dlg;
        dlg.setOption(QFileDialog::ShowDirsOnly, true);
        dlg.setFileMode(QFileDialog::Directory);
        dlg.setDirectoryUrl(initialDirectory(startUrl));
        dlg.selectFile(initialSelection(startUrl));
        Utils::handleXGeometry(&dlg);
        dlg.setWindowTitle(title.isEmpty() ? i18nc("@title:window", "Select Directory") : title);
        if (!dlg.exec()) {
            return 1; // canceled
        }
        const QList<QUrl> urls = dlg.selectedUrls();
        if (!urls.isEmpty()) {
            cout << qPrintable(urls.at(0).toDisplayString(QUrl::PreferLocalFile)) << endl;
            return 0;
        }
        return 1; // canceled
    }

    // geticon [group] [context]
    if (parser.isSet(QStringLiteral("geticon"))) {
        QString groupStr, contextStr;
        groupStr = parser.value(QStringLiteral("geticon"));
        if (args.count() >= 1) {
            contextStr = args.at(0);
        }
        const KIconLoader::Group group
            = (groupStr == QLatin1String("Desktop")) ? KIconLoader::Desktop
              : (groupStr == QLatin1String("Toolbar")) ? KIconLoader::Toolbar
              : (groupStr == QLatin1String("MainToolbar")) ? KIconLoader::MainToolbar
              : (groupStr == QLatin1String("Small")) ? KIconLoader::Small
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
              : (groupStr == QLatin1String("Panel")) ? KIconLoader::Panel
#endif
              : (groupStr == QLatin1String("Dialog")) ? KIconLoader::Dialog
              : (groupStr == QLatin1String("User")) ? KIconLoader::User
              : /* else */ KIconLoader::NoGroup;

        const KIconLoader::Context context
            = (contextStr == QLatin1String("Action")) ? KIconLoader::Action
              : (contextStr == QLatin1String("Application")) ? KIconLoader::Application
              : (contextStr == QLatin1String("Device")) ? KIconLoader::Device
              : (contextStr == QLatin1String("FileSystem")) ? KIconLoader::Place
              : (contextStr == QLatin1String("MimeType")) ? KIconLoader::MimeType
              : (contextStr == QLatin1String("Animation")) ? KIconLoader::Animation
              : (contextStr == QLatin1String("Category")) ? KIconLoader::Category
              : (contextStr == QLatin1String("Emblem")) ? KIconLoader::Emblem
              : (contextStr == QLatin1String("Emote")) ? KIconLoader::Emote
              : (contextStr == QLatin1String("International")) ? KIconLoader::International
              : (contextStr == QLatin1String("Place")) ? KIconLoader::Place
              : (contextStr == QLatin1String("StatusIcon")) ? KIconLoader::StatusIcon
              : // begin: KDE3 compatibility (useful?)
              (contextStr == QLatin1String("Devices")) ? KIconLoader::Device
              : (contextStr == QLatin1String("MimeTypes")) ? KIconLoader::MimeType
              : (contextStr == QLatin1String("FileSystems")) ? KIconLoader::Place
              : (contextStr == QLatin1String("Applications")) ? KIconLoader::Application
              : (contextStr == QLatin1String("Actions")) ? KIconLoader::Action
              : // end: KDE3 compatibility
                /* else */ KIconLoader::Any;

        KIconDialog dlg;
        dlg.setup(group, context);
        dlg.setIconSize(KIconLoader::SizeHuge);

        if (!title.isEmpty()) {
            dlg.setWindowTitle(title);
        }

        Utils::handleXGeometry(&dlg);

        const QString result = dlg.openDialog();

        if (!result.isEmpty()) {
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // canceled
    }

    // --progressbar text totalsteps
    if (parser.isSet(QStringLiteral("progressbar"))) {
        const QString text = Utils::parseString(parser.value(QStringLiteral("progressbar")));

        QProcess process;
        QStringList arguments;
        arguments << QStringLiteral("--progressbar");
        arguments << text;
        arguments << QStringLiteral("--title");
        arguments << title;
        if (args.count() == 1) {
            arguments << args.at(0);
        }
        qint64 pid = 0;
        if (process.startDetached(QStandardPaths::findExecutable(QStringLiteral("kdialog_progress_helper")), arguments, QString(), &pid)) {
            QDBusConnection dbus = QDBusConnection::sessionBus();
            const QString serviceName = QStringLiteral("org.kde.kdialog-") + QString::number(pid);
            QDBusServiceWatcher watcher(serviceName, dbus, QDBusServiceWatcher::WatchForRegistration);
            QObject::connect(&watcher, &QDBusServiceWatcher::serviceRegistered, [serviceName](){
                std::cout << serviceName.toLatin1().constData() << " /ProgressDialog" << std::endl << std::flush;
                qApp->quit();
            });
            qApp->exec();
            return 0;
        }
        qWarning() << "Error starting kdialog_progress_helper";
        return 1;
    }

    // --getcolor
    if (parser.isSet(QStringLiteral("getcolor"))) {
        QColorDialog dlg;

        if (parser.isSet(QStringLiteral("default"))) {
            defaultEntry = parser.value(QStringLiteral("default"));
            dlg.setCurrentColor(defaultEntry);
        } else {
            QColor color = KColorMimeData::fromMimeData(QApplication::clipboard()->mimeData(QClipboard::Clipboard));
            if (color.isValid()) {
                dlg.setCurrentColor(color);
            }
        }

        Utils::handleXGeometry(&dlg);
        dlg.setWindowTitle(title.isEmpty() ? i18nc("@title:window", "Choose Color") : title);

        if (dlg.exec()) {
            QString result;
            const QColor color = dlg.selectedColor();
            if (color.isValid() && parser.isSet(QStringLiteral("format"))) {
                bool found = false;
                QString format = parser.value(QStringLiteral("format"));
                format.remove(QLatin1Char('*')); // stripped out * for safety
                QList<QRegularExpression> pattern_pool;
                pattern_pool << QRegularExpression(QStringLiteral("(%#?[-+]?\\d*\\.?\\d*(?:ll|hh|l|h)?[diouxX])"))
                             << QRegularExpression(QStringLiteral("(%#?[-+]?\\d*\\.?\\d*[l]?[efgEFG])"));

                for (int i = 0; i < pattern_pool.size(); i++) {
                    QRegularExpressionMatchIterator itor = pattern_pool.at(i).globalMatch(format);
                    QRegularExpressionMatch match;
                    int match_count = 0;
                    while (itor.hasNext()) {
                        match = itor.next();
                        if (match.hasMatch()) {
                            match_count++;
                        }
                    }
                    // currently only handle RGB, when alpha is ready, should hit 4
                    if (3 == match_count) {
                        found = true;
                        if (match.captured(0).contains(QRegularExpression(QStringLiteral("[diouxX]")))) {
                            result = QString::asprintf(format.toUtf8().constData(), color.red(), color.green(), color.blue());
                        } else {
                            result = QString::asprintf(format.toUtf8().constData(), color.redF(), color.greenF(), color.blueF());
                        }
                        break;
                    }
                }
                if (false == found) {
                    cout << "Invalid format pattern";
                }
            } else {
                result = color.name();
            }
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // canceled
    }
    if (parser.isSet(QStringLiteral("slider"))) {
        int miniValue = 0;
        int maxValue = 0;
        int step = 0;
        const QString text = Utils::parseString(parser.value(QStringLiteral("slider")));
        if (args.count() == 3) {
            miniValue = args.at(0).toInt();
            maxValue = args.at(1).toInt();
            step = args.at(2).toInt();
        }
        int result = 0;

        const bool returnCode = Widgets::slider(nullptr, title, text, miniValue, maxValue, step, result);
        if (returnCode) {
            cout << result << endl;
        }
        return returnCode;
    }

    // --calendar text [--default default] [--dateformat format]
    if (parser.isSet(QStringLiteral("calendar"))) {
        // The default format is weird and non-standard. Sadly, it's too late to change this now.
        QString dateFormat = QStringLiteral("ddd MMM d yyyy");
        if (parser.isSet(QStringLiteral("dateformat"))) {
            dateFormat = parser.value(QStringLiteral("dateformat"));
        }

        QDate defaultDate = QDateTime::currentDateTime().date(); // today
        if (parser.isSet(QStringLiteral("default"))) {
            defaultEntry = parser.value(QStringLiteral("default"));
            defaultDate = QDate::fromString(defaultEntry, dateFormat);
        }

        const QString text = Utils::parseString(parser.value(QStringLiteral("calendar")));
        QDate result;

        const bool returnCode = Widgets::calendar(nullptr, title, text, result, defaultDate);
        if (returnCode) {
            cout << result.toString(dateFormat).toLocal8Bit().data() << endl;
        }
        return returnCode;
    }

    parser.showHelp();
    return -2; // NOTREACHED
}
