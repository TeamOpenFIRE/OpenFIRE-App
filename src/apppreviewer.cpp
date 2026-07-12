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

    ui->line->setVisible(false);

    //boldFont.setBold(true);
    mainFont.setPointSize(12);
    boldFont.setPointSize(12);
    boldFont.setBold(12);
    smallFont.setPointSize(8);

    // Add center board pic above the Sub Pins layout
    ui->PinsCenter->insertWidget(0, &boardPic, 1);
    boardPic.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    for(auto &item : App_Common::OFPresets.boardNames)
        if(strstr(item.first.data(), "generic") == nullptr)
            ui->boardSelector->addItem(item.second);
}

AppBoardsPreviewer::~AppBoardsPreviewer()
{
    delete ui;
}

bool AppBoardsPreviewer::eventFilter(QObject* object, QEvent* event)
{
    if(event->type() == QEvent::Enter && object->property("slot").toInt() != lastHighlight) {
        // Copy and modify board pic array to change opacity of selected pin element, if existing.
        highlightBoardPic = origBoardPicFile;
        int i = highlightBoardPic.indexOf(QString("id=\"OF_pin%1\"").arg(object->property("slot").toInt()));
        if(i > -1 && i != lastHighlight) {
            i = highlightBoardPic.indexOf("opacity:0", i);
            if(i > -1) {
                highlightBoardPic.replace(i+8, 1, '1');
                boardPic.load(highlightBoardPic.toLocal8Bit());
                boardPic.renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
            }
            pinDefaultFunc.at(object->property("slot").toInt())->setFont(boldFont);
            lastHighlight = object->property("slot").toInt();
        }
    } else if(event->type() == QEvent::Leave) {
        boardPic.load(origBoardPicFile);
        boardPic.renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
        if(lastHighlight > -1) {
            pinDefaultFunc.at(lastHighlight)->setFont(mainFont);
            lastHighlight = -1;
        }
    }

    return QWidget::eventFilter(object, event);
}

void AppBoardsPreviewer::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    if(!App_Common::board.type.isEmpty())
        ui->boardSelector->setCurrentText(App_Common::OFPresets.boardNames.at(App_Common::board.type.toStdString()));
    else ui->boardSelector->setCurrentIndex(-1);
}

void AppBoardsPreviewer::on_boardSelector_currentTextChanged(const QString &arg1)
{
    // Clears old board layout items
    if(pinDefaultFunc.count()) {
        for(int i = 0; i < pinDefaultFunc.count(); ++i) {
            delete pinDefaultFunc.at(i);
            delete pinLabel.at(i);
            delete pinCapabilityMarks.at(i);
        }

        pinDefaultFunc.clear();
        pinLabel.clear();
        pinCapabilityMarks.clear();
    }

    lastHighlight = -1;

    if(arg1.isEmpty()) {
        ui->line->setVisible(false);
        ui->subTextLabel->clear();
        boardPic.setVisible(false);
        return;
    }

    for(auto &board : App_Common::OFPresets.boardNames) {
        if(!strcmp(board.second, arg1.toLocal8Bit().constData())) {
            if(board.first.find("esp32") != std::string::npos) {
                ui->subTextLabel->setText(tr("<p>Compatible with the "
                                             "<a href='https://github.com/alessandro-satanassi/OpenFIRE-Firmware-ESP32'><span style=' text-decoration: underline; color:#8ab4f8;'>ESP-IDF fork of the OpenFIRE Firmware</span></a> by <i>Alessandro Satanassi.</i><br>"
                                             "Any issues should be reported <b><a href='https://github.com/alessandro-satanassi/OpenFIRE-Firmware-ESP32/issues'><span style=' text-decoration: underline; color:#8ab4f8;'>here!</span></a></b></p>"));
                // NOTE: if we get any non-S3 boards, will need to determine if S3 or other arch-type ESP board.
                boardType = OF_Const::boardESP32_S3;
            } else {
                ui->subTextLabel->setText(tr("<p>Compatible with "
                                             "<a href='https://github.com/TeamOpenFIRE/OpenFIRE-Firmware'><span style=' text-decoration: underline; color:#8ab4f8;'>upstream OpenFIRE Firmware</span></a> by <i>Team OpenFIRE.</i></p>"));
                boardType = OF_Const::boardRP;
            }

            ui->line->setVisible(true);

            // get maps for current board
            // (in this case, safe to assume it's always a valid entry, otherwise it wouldn't show in the first place)
            std::unordered_map<std::string_view, std::vector<int>>::const_iterator presetMap = App_Common::OFPresets.boardsPresetsMap.find(board.first);
            std::unordered_map<std::string_view, std::vector<unsigned int>>::const_iterator layoutMap = App_Common::OFPresets.boardsBoxPositions.find(board.first);

            // check if board has pin capability overrides, else fallback to architecture capabilities
            std::unordered_map<std::string_view, std::vector<int>>::const_iterator pinCapableMap = App_Common::OFPresets.mcuCapableMaps.find(board.first);
            if(pinCapableMap == App_Common::OFPresets.mcuCapableMaps.cend())
                pinCapableMap = App_Common::OFPresets.mcuCapableMaps.find(App_Common::OFPresets.boardArchs[boardType]);

            for(int i = 0; i < presetMap->second.size(); ++i) {
                pinDefaultFunc << new QLabel();
                pinDefaultFunc.at(i)->setFont(mainFont);
                pinDefaultFunc.at(i)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                pinDefaultFunc.at(i)->setProperty("slot", i);
                pinDefaultFunc.at(i)->installEventFilter(this);

                pinCapabilityMarks << new QLabel();
                pinCapabilityMarks.at(i)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                pinCapabilityMarks.at(i)->setFont(smallFont);
                pinCapabilityMarks.at(i)->setProperty("slot", i);
                pinCapabilityMarks.at(i)->installEventFilter(this);

                // render pin capabilities
                // Analog pin
                if(pinCapableMap->second.at(i) & OF_Const::pinHasADC)
                    pinCapabilityMarks.at(i)->setText("<font color=#FF0099><tt><b>ADC</b></tt></font>");
                else pinCapabilityMarks.at(i)->setText("<font color=#555555><tt>ADC</tt></font>");

                // I2C channel coloring
                // check if ESP-style any pin kinda setup
                if(pinCapableMap->second.at(i) & OF_Const::pinAnyI2C) {
                    pinLabel << new QLabel(QString("<font color=#BE00B0>«GPIO%1»</font>").arg(i));
                    pinCapabilityMarks.at(i)->setText(pinCapabilityMarks.at(i)->text() + " <font color=#BE00B0><tt><b>I2C(*)</b></tt></font>");
                // check for channels
                } else if(pinCapableMap->second.at(i) & OF_Const::pinCanI2C) {
                    if(pinCapableMap->second.at(i) & OF_Const::pinIsI2C1) {
                        pinLabel << new QLabel(QString("<font color=#FF8800>«GPIO%1»</font>").arg(i));
                        pinCapabilityMarks.at(i)->setText(pinCapabilityMarks.at(i)->text() +
                                                          QString(" <font color=#FF8800><tt><b>I2C%1%2</b></tt></font>")
                                                          .arg((pinCapableMap->second.at(i) & OF_Const::pinIsI2C1) >> 3)
                                                          .arg(App_Common::i2cTypeLabels[(pinCapableMap->second.at(i) & OF_Const::pinIsI2CSCL) >> 2]));
                    } else {
                        pinLabel << new QLabel(QString("<font color=#0099FF>«GPIO%1»</font>").arg(i));
                        pinCapabilityMarks.at(i)->setText(pinCapabilityMarks.at(i)->text() +
                                                          QString(" <font color=#0099FF><tt><b>I2C%1%2</b></tt></font>")
                                                          .arg((pinCapableMap->second.at(i) & OF_Const::pinIsI2C1) >> 3)
                                                          .arg(App_Common::i2cTypeLabels[(pinCapableMap->second.at(i) & OF_Const::pinIsI2CSCL) >> 2]));
                    }
                // no I2C capability
                } else {
                    pinLabel << new QLabel(QString("«GPIO%1»").arg(i));
                    pinCapabilityMarks.at(i)->setText(pinCapabilityMarks.at(i)->text() + " <font color=#555555><tt>I2C</tt></font>");
                }

                // SPI
                // check if ESP-style any pin kinda setup
                if(pinCapableMap->second.at(i) & OF_Const::pinAnySPI)
                    pinCapabilityMarks.at(i)->setText(pinCapabilityMarks.at(i)->text() + " <font color=#D1003D><tt>SPI(*)</tt></font>");
                else if(pinCapableMap->second.at(i) & OF_Const::pinCanSPI)
                    pinCapabilityMarks.at(i)->setText(pinCapabilityMarks.at(i)->text() +
                                                      QString(" <font color=#009C3A><tt><b>SPI%1%2</b></tt></font>")
                                                      .arg((pinCapableMap->second.at(i) & OF_Const::pinIsSPI1) >> 4)
                                                      .arg(App_Common::spiTypeLabels[((pinCapableMap->second.at(i) & OF_Const::pinCanSPI) >> 5)-1]));
                else pinCapabilityMarks.at(i)->setText(pinCapabilityMarks.at(i)->text() + " <font color=#555555><tt>SPI</tt></font>");

                pinLabel.at(i)->setEnabled(false);
                pinLabel.at(i)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                pinLabel.at(i)->setProperty("slot", i);
                pinLabel.at(i)->installEventFilter(this);
                pinLabel.at(i)->setToolTip(tr("GPIO Pin No. %1.\n\n"
                                              "Blue pin numbers are members of I2C0.\n"
                                              "Orange are members of I2C1.\n"
                                              "Purple pin numbers can automatically select any I2C channel in software.\n"
                                              "Gray cannot use I2C devices.").arg(i));

                pinCapabilityMarks.at(i)->setToolTip(tr("ADC indicates whether this pin can read Analog Inputs.\n"
                                                        "I2C indicates if this pin can interact with I2C devices, and what channel and type it uses.\n"
                                                        "SPI indicates if this pin can interact with SPI devices, and what channel and type it uses.\n"
                                                        "(*) means pin can use any type via automated software selectable channels/type."));
            }

            // Drawing the actual board view page by referencing the board maps data from OpenFIREshared.h
            QFile resource((QString)":/boardPics/" + board.first.data());
            resource.open(QIODevice::ReadOnly);
            origBoardPicFile = resource.readAll();

            for(int i = 0; i < pinDefaultFunc.count(); ++i) {
                switch(layoutMap->second.at(i) & OF_Const::posCheck) {
                case OF_Const::posLeft:
                    ui->PinsLeft->addWidget(pinDefaultFunc.at(i),
                                            layoutMap->second.at(i) ^ OF_Const::posLeft, 0, Qt::AlignRight);
                    ui->PinsLeft->addWidget(pinLabel.at(i),
                                            layoutMap->second.at(i) ^ OF_Const::posLeft, 1, Qt::AlignCenter);
                    ui->PinsLeft->addWidget(pinCapabilityMarks.at(i),
                                            layoutMap->second.at(i) ^ OF_Const::posLeft, 3, Qt::AlignCenter);
                    pinDefaultFunc.at(i)->setText(App_Common::OFPresets.boardInputs_sortedStr[presetMap->second.at(i)+1]);
                    break;
                case OF_Const::posRight:
                    ui->PinsRight->addWidget(pinCapabilityMarks.at(i),
                                             layoutMap->second.at(i) ^ OF_Const::posRight, 0, Qt::AlignCenter);
                    ui->PinsRight->addWidget(pinLabel.at(i),
                                             layoutMap->second.at(i) ^ OF_Const::posRight, 2, Qt::AlignCenter);
                    ui->PinsRight->addWidget(pinDefaultFunc.at(i),
                                             layoutMap->second.at(i) ^ OF_Const::posRight, 3, Qt::AlignLeft);
                    pinDefaultFunc.at(i)->setText(App_Common::OFPresets.boardInputs_sortedStr[presetMap->second.at(i)+1]);
                    break;
                case OF_Const::posMiddle:
                    ui->PinsCenterSub->addWidget(pinCapabilityMarks.at(i), 0,
                                                 layoutMap->second.at(i) ^ OF_Const::posMiddle, Qt::AlignCenter);
                    ui->PinsCenterSub->addWidget(pinLabel.at(i), 2,
                                                 layoutMap->second.at(i) ^ OF_Const::posMiddle, Qt::AlignCenter);
                    ui->PinsCenterSub->addWidget(pinDefaultFunc.at(i), 3,
                                                 layoutMap->second.at(i) ^ OF_Const::posMiddle, Qt::AlignCenter);
                    pinDefaultFunc.at(i)->setText(App_Common::OFPresets.boardInputs_sortedStr[presetMap->second.at(i)+1]);
                    break;
                case OF_Const::posNothing:
                    break;
                }
            }

            // aspect ratio hint needs to be set every time a new asset is loaded
            boardPic.setVisible(true);
            boardPic.load(origBoardPicFile);
            boardPic.renderer()->setAspectRatioMode(Qt::KeepAspectRatio);

            for(int i = 1; i < ui->PinsLeft->rowCount(); ++i)
                if(ui->PinsLeft->itemAtPosition(i, 0) == nullptr)
                    ui->PinsLeft->setRowMinimumHeight(i, 25);
                else ui->PinsLeft->setRowMinimumHeight(i, 28);

            for(int i = 1; i < ui->PinsRight->rowCount(); ++i)
                if(ui->PinsRight->itemAtPosition(i, 0) == nullptr)
                    ui->PinsRight->setRowMinimumHeight(i, 25);
                else ui->PinsRight->setRowMinimumHeight(i, 28);

            break;
        }
    }
}
