//  Copyright (C) 2017 David Faure <faure@kde.org>
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
    app.setApplicationName("kdialog");
    app.setOrganizationDomain("kde.org");
    KLocalizedString::setApplicationDomain("kdialog");

    KDBusService dbusService(KDBusService::Multiple);

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("progressbar"), i18n("Progress bar dialog, returns a D-Bus reference for communication"), QLatin1String("text")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("title"), i18n("Dialog title"), QLatin1String("text")));
    parser.addPositionalArgument(QLatin1String("[arg]"), i18n("Arguments - depending on main option"));
    parser.process(rawArgs);

    const QStringList args = parser.positionalArguments();

    const QString text = Utils::parseString(parser.value("progressbar"));
    const QString title = parser.value("title");

    int totalsteps = 100;
    if (args.count() == 1)
        totalsteps = args.at(0).toInt();

    ProgressDialog dlg(0, title, text, totalsteps);
    return dlg.exec() ? 0 : 1;
}
