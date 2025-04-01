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

#ifndef APPDEBUG_H
#define APPDEBUG_H

#include <QDialog>

namespace Ui {
class AppDebugWindow;
}

class AppDebugWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AppDebugWindow(QWidget *parent = nullptr);
    ~AppDebugWindow();

    void AppendText(const QByteArray &);

private:
    Ui::AppDebugWindow *ui;
};

#endif // APPDEBUG_H
