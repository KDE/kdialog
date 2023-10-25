//  SPDX-FileCopyrightText: 2017 David Faure <faure@kde.org>
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <KDBusService>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <KLocalizedString>
#include "progressdialog.h"
#include "utils.h"

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    QStringList rawArgs;
    rawArgs.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        rawArgs << QString::fromLocal8Bit(argv[i]);
    }

    // This code was given by Thiago as a solution for the issue that
    // otherwise bash waits for a SIGPIPE from kdialog that never comes in.
    int fd = open("/dev/null", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kdialog"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kdialog"));

    KDBusService dbusService(KDBusService::Multiple);

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("progressbar"), i18n("Progress bar dialog, returns a D-Bus reference for communication"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("title"), i18n("Dialog title"), QStringLiteral("text")));
    parser.addPositionalArgument(QStringLiteral("[arg]"), i18n("Arguments - depending on main option"));
    parser.process(rawArgs);

    const QStringList args = parser.positionalArguments();

    const QString text = Utils::parseString(parser.value(QStringLiteral("progressbar")));
    const QString title = parser.value(QStringLiteral("title"));

    int totalsteps = 100;
    if (args.count() == 1) {
        totalsteps = args.at(0).toInt();
    }

    ProgressDialog dlg(nullptr, title, text, totalsteps);
    return dlg.exec() ? 0 : 1;
}
