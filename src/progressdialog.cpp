//  SPDX-FileCopyrightText: 2004-2005 Stephan Binner <binner@kde.org>
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "progressdialog.h"
#include "utils.h"
#include "progressdialogadaptor.h"
#include <KLocalizedString>

ProgressDialog::ProgressDialog(QWidget *parent, const QString &caption, const QString &text, int totalSteps)
    : QProgressDialog(parent)
{
    setWindowTitle(caption);
    setLabelText(text);
    (void)new ProgressDialogAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/ProgressDialog"), this);
    setAutoClose(false);
    setAutoReset(false);
    setMaximum(totalSteps);
    Utils::handleXGeometry(this);
}

void ProgressDialog::showCancelButton(bool show)
{
    setCancelButtonText(show ? i18n("Cancel") : QString());
}
