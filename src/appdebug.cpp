/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Serial info window for debugging.

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

#include "appdebug.h"
#include "ui_appdebug.h"

AppDebugWindow::AppDebugWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AppDebugWindow)
{
    ui->setupUi(this);
}

AppDebugWindow::~AppDebugWindow()
{
    delete ui;
}

void AppDebugWindow::AppendText(const QByteArray &text)
{
    ui->text->appendPlainText(text);
}
