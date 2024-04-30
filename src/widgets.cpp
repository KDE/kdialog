//  SPDX-FileCopyrightText: 1998 Matthias Hoelzer <hoelzer@kde.org>
//  SPDX-FileCopyrightText: 2002-2005 David Faure <faure@kde.org>
//  SPDX-License-Identifier: GPL-2.0-or-later

// Own
#include <config-kdialog.h>
#include "widgets.h"
#include "utils.h"

// Qt
#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QInputDialog>
#include <QTextStream>
#include <QTextCursor>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>

// KF
#include <KPasswordDialog>
#include <KNewPasswordDialog>
#include <KTextEdit>
#include <KDatePicker>
#include <KLocalizedString>

// Local
#include "klistboxdialog.h"

static void addButtonBox(QDialog &dlg, QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel)
{
    auto buttonBox = new QDialogButtonBox(buttons, &dlg);
    dlg.layout()->addWidget(buttonBox);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
}

bool Widgets::inputBox(QWidget *parent, const QString &title, const QString &text, const QString &init, QString &result)
{
    QInputDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setLabelText(text);
    dlg.setTextValue(init);

    Utils::handleXGeometry(&dlg);

    bool retcode = (dlg.exec() == QDialog::Accepted);
    if (retcode) {
        result = dlg.textValue();
    }
    return retcode;
}

bool Widgets::passwordBox(QWidget *parent, const QString &title, const QString &text, QString &result)
{
    KPasswordDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setPrompt(text);

    Utils::handleXGeometry(&dlg);

    bool retcode = (dlg.exec() == QDialog::Accepted);
    if (retcode) {
        result = dlg.password();
    }
    return retcode;
}

bool Widgets::newPasswordBox(QWidget *parent, const QString &title, const QString &text, QString &result)
{
    KNewPasswordDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setPrompt(text);

    Utils::handleXGeometry(&dlg);

    bool retcode = (dlg.exec() == QDialog::Accepted);
    if (retcode) {
        result = dlg.password();
    }
    return retcode;
}

bool Widgets::textBox(QWidget *parent, int width, int height, const QString &title, const QString &file)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);

    auto mainLayout = new QVBoxLayout(&dlg);

    auto edit = new KTextEdit(&dlg);
    mainLayout->addWidget(edit);
    edit->setReadOnly(true);
    edit->setFocus();

    addButtonBox(dlg, QDialogButtonBox::Ok);

    if (file == QLatin1String("-")) {
        QTextStream s(stdin, QIODevice::ReadOnly);
        while (!s.atEnd()) {
            edit->append(s.readLine());
        }
    } else {
        QFile f(file);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << i18n("kdialog: could not open file %1", file);
            return -1;
        }
        QTextStream s(&f);
        while (!s.atEnd()) {
            edit->append(s.readLine());
        }
    }

    edit->setTextCursor(QTextCursor(edit->document()));

    if (width > 0 && height > 0) {
        dlg.resize(QSize(width, height));
    }

    Utils::handleXGeometry(&dlg);
    dlg.setWindowTitle(title);
    return dlg.exec() == QDialog::Accepted;
}

bool Widgets::imgBox(QWidget *parent, const QString &title, const QString &file)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);

    auto mainLayout = new QVBoxLayout(&dlg);

    auto label = new QLabel(&dlg);
    mainLayout->addWidget(label);

    addButtonBox(dlg, QDialogButtonBox::Ok);

    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << i18n("kdialog: could not open file %1", file);
        return -1;
    }

    label->setPixmap(QPixmap(file));
    Utils::handleXGeometry(&dlg);
    return dlg.exec() == QDialog::Accepted;
}

bool Widgets::imgInputBox(QWidget *parent, const QString &title, const QString &file, const QString &text, QString &result)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << i18n("kdialog: could not open file %1", file);
        return -1;
    }

    QDialog dlg(parent);
    dlg.setWindowTitle(title);

    auto mainLayout = new QVBoxLayout(&dlg);

    if (!text.isEmpty()) {
        auto head = new QLabel(&dlg);
        head->setText(text);
        mainLayout->addWidget(head);
    }

    auto label = new QLabel(&dlg);
    mainLayout->addWidget(label);

    label->setPixmap(QPixmap(file));

    auto edit = new QLineEdit(&dlg);
    mainLayout->addWidget(edit);
    edit->setReadOnly(false);
    edit->setFocus();

    addButtonBox(dlg, QDialogButtonBox::Ok);
    Utils::handleXGeometry(&dlg);

    bool retcode = (dlg.exec() == QDialog::Accepted);

    if (retcode) {
        result = edit->text();
    }

    return retcode;
}

bool Widgets::textInputBox(QWidget *parent, int width, int height, const QString &title, const QString &text, const QString &init, QString &result)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    auto mainLayout = new QVBoxLayout(&dlg);

    if (!text.isEmpty()) {
        auto label = new QLabel(&dlg);
        mainLayout->addWidget(label);
        label->setText(text);
    }

    auto edit = new KTextEdit(&dlg);
    mainLayout->addWidget(edit);
    edit->setReadOnly(false);
    edit->setFocus();
    edit->insertPlainText(init);

    addButtonBox(dlg, QDialogButtonBox::Ok);

    if (width > 0 && height > 0) {
        dlg.resize(QSize(width, height));
    }

    Utils::handleXGeometry(&dlg);
    dlg.setWindowTitle(title);
    const int returnDialogCode = dlg.exec();
    result = edit->toPlainText();
    return returnDialogCode == QDialog::Accepted;
}

bool Widgets::comboBox(QWidget *parent, const QString &title, const QString &text, const QStringList &args, const QString &defaultEntry, QString &result)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);

    auto mainLayout = new QVBoxLayout(&dlg);

    auto label = new QLabel(&dlg);
    label->setText(text);
    mainLayout->addWidget(label);
    auto combo = new QComboBox(&dlg);
    combo->addItems(args);
    combo->setCurrentIndex(combo->findText(defaultEntry));
    combo->setFocus();
    mainLayout->addWidget(combo);
    addButtonBox(dlg);
    Utils::handleXGeometry(&dlg);

    bool retcode = (dlg.exec() == QDialog::Accepted);

    if (retcode) {
        result = combo->currentText();
    }

    return retcode;
}

bool Widgets::listBox(QWidget *parent, const QString &title, const QString &text, const QStringList &args, const QString &defaultEntry, QString &result)
{
    KListBoxDialog box(text, parent);

    box.setWindowTitle(title);

    for (int i = 0; i+1 < args.count(); i += 2) {
        box.insertItem(args[i+1]);
    }
    box.setCurrentItem(defaultEntry);

    Utils::handleXGeometry(&box);

    const bool retcode = (box.exec() == QDialog::Accepted);
    if (retcode) {
        result = args[ box.currentItem()*2 ];
    }
    return retcode;
}

bool Widgets::checkList(QWidget *parent, const QString &title, const QString &text, const QStringList &args, bool separateOutput, QStringList &result)
{
    QStringList entries, tags;

    result.clear();

    KListBoxDialog box(text, parent);

    QListWidget &table = box.getTable();

    box.setWindowTitle(title);

    for (int i = 0; i+2 < args.count(); i += 3) {
        tags.append(args[i]);
        entries.append(args[i+1]);
    }

    table.addItems(entries);
    table.setSelectionMode(QListWidget::MultiSelection);
    table.setCurrentItem(nullptr); // This is to circumvent a Qt bug

    for (int i = 0; i+2 < args.count(); i += 3) {
        table.item(i/3)->setSelected(args[i+2] == QLatin1String("on"));
    }

    Utils::handleXGeometry(&box);

    const bool retcode = (box.exec() == QDialog::Accepted);

    if (retcode) {
        if (separateOutput) {
            for (int i = 0; i < table.count(); i++) {
                if (table.item(i)->isSelected()) {
                    result.append(tags[i]);
                }
            }
        } else {
            QString rs;
            for (int i = 0; i < table.count(); i++) {
                if (table.item(i)->isSelected()) {
                    rs += QLatin1String("\"") + tags[i] + QLatin1String("\" ");
                }
            }
            result.append(rs);
        }
    }
    return retcode;
}

bool Widgets::radioBox(QWidget *parent, const QString &title, const QString &text, const QStringList &args, QString &result)
{
    QStringList entries, tags;

    KListBoxDialog box(text, parent);

    QListWidget &table = box.getTable();

    box.setWindowTitle(title);

    for (int i = 0; i+2 < args.count(); i += 3) {
        tags.append(args[i]);
        entries.append(args[i+1]);
    }

    table.addItems(entries);

    for (int i = 0; i+2 < args.count(); i += 3) {
        if (args[i+2] == QLatin1String("on")) {
            table.setCurrentRow(i/3);
        }
    }

    Utils::handleXGeometry(&box);

    const bool retcode = (box.exec() == QDialog::Accepted);
    if (retcode) {
        result = tags[ table.currentRow() ];
    }
    return retcode;
}

bool Widgets::slider(QWidget *parent, const QString &title, const QString &text, int minValue, int maxValue, int step, int &result)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);

    auto mainLayout = new QVBoxLayout(&dlg);

    auto label = new QLabel(&dlg);
    mainLayout->addWidget(label);
    label->setText(text);
    auto slider = new QSlider(&dlg);
    mainLayout->addWidget(slider);
    slider->setMinimum(minValue);
    slider->setMaximum(maxValue);
    slider->setSingleStep(step);
    slider->setTickInterval(step);
    slider->setTickPosition(QSlider::TicksAbove);
    slider->setOrientation(Qt::Horizontal);
    slider->setFocus();
    Utils::handleXGeometry(&dlg);

    addButtonBox(dlg);

    Utils::handleXGeometry(&dlg);
    const bool retcode = (dlg.exec() == QDialog::Accepted);

    if (retcode) {
        result = slider->value();
    }

    return retcode;
}

bool Widgets::calendar(QWidget *parent, const QString &title, const QString &text, QDate &result, QDate defaultEntry)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);

    auto mainLayout = new QVBoxLayout(&dlg);

    auto label = new QLabel(&dlg);
    mainLayout->addWidget(label);
    label->setText(text);
    auto dateWidget = new KDatePicker(defaultEntry, &dlg);
    mainLayout->addWidget(dateWidget);
    dateWidget->setFocus();
    addButtonBox(dlg);
    Utils::handleXGeometry(&dlg);

    const bool retcode = (dlg.exec() == QDialog::Accepted);

    if (retcode) {
        result = dateWidget->date();
    }

    return retcode;
}
