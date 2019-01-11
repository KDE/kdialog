//
//  Copyright (C) 2004-2005 Stephan Binner <binner@kde.org>
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

#include "progressdialog.h"
#include "utils.h"
#include "progressdialogadaptor.h"
#include <KLocalizedString>

ProgressDialog::ProgressDialog(QWidget* parent, const QString& caption, const QString& text, int totalSteps)
    : QProgressDialog(parent)
{
    setWindowTitle(caption);
    setLabelText(text);
    (void)new ProgressDialogAdaptor( this );
    QDBusConnection::sessionBus().registerObject( QStringLiteral("/ProgressDialog"), this );
    setAutoClose( false );
    setMaximum( totalSteps );
    Utils::handleXGeometry(this);
}

void ProgressDialog::showCancelButton(bool show)
{
    setCancelButtonText(show ? i18n("Cancel") : QString());
}
