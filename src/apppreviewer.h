/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Boards preview window.

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

#ifndef APPPREVIEWER_H
#define APPPREVIEWER_H

#include <QDialog>
#include <QSvgWidget>
#include <QComboBox>
#include <QLabel>

namespace Ui {
class AppBoardsPreviewer;
}

class AppBoardsPreviewer : public QDialog
{
    Q_OBJECT

public:
    explicit AppBoardsPreviewer(QWidget *parent = nullptr);
    ~AppBoardsPreviewer();

private slots:
    void on_boardSelector_currentTextChanged(const QString &arg1);

private:
    Ui::AppBoardsPreviewer *ui;

    bool eventFilter(QObject* object, QEvent* event) override;

    /// @brief      Macro font to enable bold
    QFont boldFont;

    /// @brief      Renderer that makes up the centerpiece of the board view tab
    QSvgWidget boardPic;

    /// @brief      Current board picture's byte array representation
    /// @details    Used to quickly copy/modify for board view highlights
    QByteArray origBoardPicFile;
    QString highlightBoardPic;

    /// @brief      Objects that makes up the elements of the board view tab
    QVector<QLabel*> pinDefaultFunc;
    QVector<QLabel*> pinLabel;
    QVector<QLabel*> pinCapabilityMarks;
    int boardType;
};

#endif // APPPREVIEWER_H
