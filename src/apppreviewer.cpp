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

#include "apppreviewer.h"
#include "ui_apppreviewer.h"
#include "appcommon.h"
#include "../boards/OpenFIREshared.h"

#include <QSvgRenderer>
#include <QFile>

AppBoardsPreviewer::AppBoardsPreviewer(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AppBoardsPreviewer)
{
    ui->setupUi(this);

    //boldFont.setBold(true);
    boldFont.setPointSize(12);

    // Add center board pic above the Sub Pins layout
    ui->PinsCenter->insertWidget(0, &boardPic, 1);
    boardPic.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    for(auto &item : App_Common::OFPresets.boardNames)
        if(strcmp(item.first.data(), "generic"))
            ui->boardSelector->addItem(item.second);
}

AppBoardsPreviewer::~AppBoardsPreviewer()
{
    delete ui;
}

bool AppBoardsPreviewer::eventFilter(QObject* object, QEvent* event)
{
    if(event->type() == QEvent::Enter) {
        // Copy and modify board pic array to change opacity of selected pin element, if existing.
        highlightBoardPic = origBoardPicFile;
        int i = highlightBoardPic.indexOf(QString("id=\"OF_pin%1\"").arg(object->property("slot").toInt()));
        if(i > -1) {
            i = highlightBoardPic.indexOf("opacity:0", i);
            if(i > -1) {
                highlightBoardPic.replace(i+8, 1, '1');
                boardPic.load(highlightBoardPic.toLocal8Bit());
                boardPic.renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
            }
        }
    } else if(event->type() == QEvent::Leave) {
        boardPic.load(origBoardPicFile);
        boardPic.renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
    }

    return QWidget::eventFilter(object, event);
}

void AppBoardsPreviewer::on_boardSelector_currentTextChanged(const QString &arg1)
{
    for(auto &board : App_Common::OFPresets.boardNames) {
        if(!strcmp(board.second, arg1.toLocal8Bit().constData())) {
            // Clears old board layout items
            if(pinDefaultFunc.count()) {
                for(uint8_t i = 0; i < pinDefaultFunc.count(); i++)
                    delete pinDefaultFunc.at(i);
                for(uint8_t i = 0; i < padding.count(); i++)
                    delete padding.at(i);
                for(uint8_t i = 0; i < pinLabel.count(); i++)
                    delete pinLabel.at(i);

                pinDefaultFunc.clear();
                padding.clear();
                pinLabel.clear();
            }

            for(uint8_t i = 0; i < App_Common::OFPresets.boardsPresetsMap.at(board.first).size(); i++) {
                pinDefaultFunc << new QLabel();
                pinDefaultFunc.at(i)->setFont(boldFont);
                pinDefaultFunc.at(i)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                pinDefaultFunc.at(i)->setProperty("slot", i);
                pinDefaultFunc.at(i)->installEventFilter(this);

                padding << new QWidget();
                padding.at(i)->setMinimumHeight(25);

                // I2C channel coloring
                if(i & 0b0000010)
                    pinLabel  << new QLabel(QString("<font color=#FF8800>«GPIO%1»</font>").arg(i));
                else pinLabel << new QLabel(QString("<font color=#0099FF>«GPIO%1»</font>").arg(i));

                pinLabel.at(i)->setEnabled(false);
                pinLabel.at(i)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                pinLabel.at(i)->setToolTip(QString("GPIO Pin number %1\n\nBlue pin numbers are members of I2C0\nOrange are members of I2C1").arg(i));
            }

            // Drawing the actual board view page by referencing the board maps data from OpenFIREshared.h
            QFile resource(QString(":/boardPics/%1").arg(board.first.data()));
            resource.open(QIODevice::ReadOnly);
            origBoardPicFile = resource.readAll();

            for(int i = 0; i < pinDefaultFunc.count(); i++) {
                if(App_Common::OFPresets.boardsBoxPositions.at(board.first).at(i) & OF_Const::posLeft) {
                    ui->PinsLeft->addWidget(pinDefaultFunc.at(i),
                                            App_Common::OFPresets.boardsBoxPositions.at(board.first).at(i) ^ OF_Const::posLeft, 0, Qt::AlignRight);
                    ui->PinsLeft->addWidget(pinLabel.at(i),
                                            App_Common::OFPresets.boardsBoxPositions.at(board.first).at(i) ^ OF_Const::posLeft, 1);
                    pinDefaultFunc.at(i)->setText(App_Common::OFPresets.boardInputs_sortedStr[App_Common::OFPresets.boardsPresetsMap.at(board.first).at(i)+1]);
                } else if(App_Common::OFPresets.boardsBoxPositions.at(board.first).at(i) & OF_Const::posRight) {
                    ui->PinsRight->addWidget(pinDefaultFunc.at(i),
                                             App_Common::OFPresets.boardsBoxPositions.at(board.first).at(i) ^ OF_Const::posRight, 1);
                    ui->PinsRight->addWidget(pinLabel.at(i),
                                             App_Common::OFPresets.boardsBoxPositions.at(board.first).at(i) ^ OF_Const::posRight, 0);
                    pinDefaultFunc.at(i)->setText(App_Common::OFPresets.boardInputs_sortedStr[App_Common::OFPresets.boardsPresetsMap.at(board.first).at(i)+1]);
                } else if(App_Common::OFPresets.boardsBoxPositions.at(board.first).at(i) & OF_Const::posMiddle) {
                    ui->PinsCenterSub->addWidget(pinDefaultFunc.at(i), 1,
                                                 App_Common::OFPresets.boardsBoxPositions.at(board.first).at(i) ^ OF_Const::posMiddle, Qt::AlignCenter);
                    ui->PinsCenterSub->addWidget(pinLabel.at(i), 0,
                                                 App_Common::OFPresets.boardsBoxPositions.at(board.first).at(i) ^ OF_Const::posMiddle, Qt::AlignCenter);
                    pinDefaultFunc.at(i)->setText(App_Common::OFPresets.boardInputs_sortedStr[App_Common::OFPresets.boardsPresetsMap.at(board.first).at(i)+1]);
                }
            }

            // aspect ratio hint needs to be set every time a new asset is loaded
            boardPic.load(origBoardPicFile);
            boardPic.renderer()->setAspectRatioMode(Qt::KeepAspectRatio);

            int prevPadCount = 0;
            for(int i = 1, padCount = 0; i < ui->PinsLeft->rowCount(); i++) {
                ui->PinsLeft->setRowMinimumHeight(i, 28);
                if(ui->PinsLeft->itemAtPosition(i, 0) == nullptr) {
                    ui->PinsLeft->addWidget(padding.at(padCount), i, 0);
                    padCount++;
                    prevPadCount = padCount;
                }
            }
            for(int i = 1, padCount = prevPadCount; i < ui->PinsRight->rowCount(); i++) {
                ui->PinsRight->setRowMinimumHeight(i, 28);
                if(ui->PinsRight->itemAtPosition(i, 0) == nullptr) {
                    ui->PinsRight->addWidget(padding.at(padCount), i, 0);
                    padCount++;
                }
            }
        }
    }
}
