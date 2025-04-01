/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    App info window.

    Copyright (C) 2025  Team OpenFIRE

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "appabout.h"
#include "ui_appabout.h"

AppAbout::AppAbout(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AppAbout)
{
    ui->setupUi(this);

    this->setFixedSize(this->width(), this->height());
    this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint);

    ui->topText->setText(ui->topText->text() +
#ifdef QT_VERSION_MAJOR
                         QString::number(QT_VERSION_MAJOR) + '.' +
#ifdef QT_VERSION_MINOR
                         QString::number(QT_VERSION_MINOR) + '.' +
#ifdef QT_VERSION_PATCH
                         QString::number(QT_VERSION_PATCH)
#endif
#endif
#endif
                         );
}

AppAbout::~AppAbout()
{
    delete ui;
}
