/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Fullscreen Windows interface for Calibration et al.

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

#include "appcali.h"
#include "ui_appcali.h"

#include <QScreen>
#include <QBitmap>

AppCaliWindow::AppCaliWindow(QWidget *parent, const int &windowMode)
    : QWidget(parent)
    , ui(new Ui::AppCaliWindow)
{
    ui->setupUi(this);

    mode = windowMode;

    connect(&scene, &AppCaliGraphicsScene::sceneMouseEvent, this, &AppCaliWindow::sceneEventReceiver);
    connect(&scene, &AppCaliGraphicsScene::sceneKeyCloseSignal, this, &AppCaliWindow::sceneKeyCloseReceiver);

    ui->graphicsView->setScene(&scene);

    // limit scale to screen size
    // (lightgun only functions properly on a single screen anyways, so no need to worry I think)
    scene.setSceneRect(QApplication::primaryScreen()->geometry());

    // every mode at least has a header line
    // set a global scale/point size for headers
    if(bitmapText) {
        headerBitmap = new QGraphicsPixmapItem();
        headerBitmap->setScale(GetTextScale(TextHeading));
    } else {
        headerText = new QGraphicsTextItem();
        headerText->setFont(QFont("Monospace", GetTextScale(TextHeading)));
    }

    switch(windowMode) {
    case modeCalibrate:
        this->setWindowTitle("Calibration Window");
        ui->graphicsView->setBackgroundBrush(QBrush(QColor("dimgray")));

        crosshairItem = new QGraphicsSvgItem(":/cali/crosshair.svg");
        // this is supposed to work, but doesn't, so we just offset every setPos command by the center point of the SVG, mult'd by scale
        //crosshairItem->setTransformOriginPoint(crosshairItem->boundingRect().center());

        // add items to scene
        // alignment lines, used during all but the initial Cali stage.
        alignmentLines[0] = new QGraphicsPolygonItem(QPolygonF() << QPointF(scene.sceneRect().center().x(), scene.sceneRect().top()-10)
                                                                 << QPointF(scene.sceneRect().center().x(), scene.sceneRect().bottom()+10)
                                                                 << QPointF(scene.sceneRect().left()-10,    scene.sceneRect().bottom()+10)
                                                                 << QPointF(scene.sceneRect().left()-10,    scene.sceneRect().center().y())
                                                                 << QPointF(scene.sceneRect().right()+10,   scene.sceneRect().center().y())
                                                                 << QPointF(scene.sceneRect().right()+10,  scene.sceneRect().top()-10));
        alignmentLines[0]->setPen(QPen(QColor("white"), 2));
        scene.addItem(alignmentLines[0]);

        if(bitmapText) {
            caliStageBitmap = new QGraphicsPixmapItem();
            caliStageBitmap->setScale(GetTextScale(TextHeading));
            tutorialBitmap = new QGraphicsPixmapItem();
            tutorialBitmap->setScale(GetTextScale(TextSub));
            for(int i = 0; i < 6; i++) {
                profileBitmaps[i] = new QGraphicsPixmapItem();
                profileBitmaps[i]->setScale(GetTextScale(TextSub));
                scene.addItem(profileBitmaps[i]);
                profileBitmaps[i]->setPixmap(GenerateText({caliTypesPrefixes.at(i)}));
                if(i == 0)
                     profileBitmaps[i]->setPos(scene.sceneRect().left()       + (80 * GetTextScale(TextSub)),
                                               scene.sceneRect().center().y() - (24 * GetTextScale(TextSub)));
                else profileBitmaps[i]->setPos(profileBitmaps[i-1]->pos().x(),
                                               profileBitmaps[i-1]->pos().y() + (profileBitmaps[i-1]->boundingRect().height()*GetTextScale(TextSub)));
                profileBitmaps[i]->setVisible(false);
            }

            scene.addItem(headerBitmap);
            scene.addItem(caliStageBitmap);
            scene.addItem(tutorialBitmap);
        } else {
            caliStageText = new QGraphicsTextItem();
            caliStageText->setFont(QFont("Monospace", GetTextScale(TextHeading)));
            tutorialText = new QGraphicsTextItem();
            tutorialText->setFont(QFont("Monospace", GetTextScale(TextSub)));
            for(int i = 0; i < 6; i++) {
                profileText[i] = new QGraphicsTextItem();
                profileText[i]->setFont(QFont("Monospace", GetTextScale(TextSub)));
                scene.addItem(profileText[i]);
            }

            scene.addItem(headerText);
            scene.addItem(caliStageText);
            scene.addItem(tutorialText);
        }

        crosshairItem->setVisible(false);
        crosshairItem->setScale(GetTextScale(Crosshair));
        scene.addItem(crosshairItem);

        ui->graphicsView->setMouseTracking(false);

        CaliModeSet(Cali_Init);
        break;
    case modeAlignment:
        this->setWindowTitle(tr("Alignment Assistant"));
        ui->graphicsView->setBackgroundBrush(QBrush(QColor("darkslategray")));

        if(bitmapText) {
            headerBitmap->setPixmap(GenerateText({tr("       Depending on your desired layout,       "),
                                                  tr("      your IR emitters should be aligned       "),
                                                  tr("         to either one of the two sets         "),
                                                  tr("               of colored boxes:               ")}));
            headerBitmap->setPos(scene.sceneRect().center().x()     - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 scene.sceneRect().height() * 0.15  - (headerBitmap->boundingRect().center().y() * headerBitmap->scale()));

            alignmentBitmapLeft = new QGraphicsPixmapItem(GenerateText({tr("      For Square Layout,     "),
                                                                        tr("    the emitters should be   "),
                                                                        tr("    placed at the top and    "),
                                                                        tr("    bottom of the display;   "),
                                                                        tr("each one being aligned to the")}));
            alignmentBitmapLeft->setScale(GetTextScale(TextSmall));
            alignmentBitmapLeft->setPos(scene.sceneRect().width() * 0.02,
                                        (scene.sceneRect().height() * 0.3) + (alignmentBitmapLeft->boundingRect().center().y() * alignmentBitmapLeft->scale()));
            alignmentBitmapColoredLeft = new QGraphicsPixmapItem(GenerateText({tr("      Red-colored boxes.     ")}, QColor(255, 100, 100)));
            alignmentBitmapColoredLeft->setScale(GetTextScale(TextSmall));
            alignmentBitmapColoredLeft->setPos(alignmentBitmapLeft->pos().x(),
                                               alignmentBitmapLeft->pos().y() + (alignmentBitmapLeft->boundingRect().height() * alignmentBitmapLeft->scale()));

            alignmentBitmapRight = new QGraphicsPixmapItem(GenerateText({tr("     For Diamond Layout,     "),
                                                                         tr("the emitters should be placed"),
                                                                         tr("  at the center of the four  "),
                                                                         tr("    edges of the display;    "),
                                                                         tr("each one being aligned to the")}));
            alignmentBitmapRight->setScale(GetTextScale(TextSmall));
            alignmentBitmapRight->setPos(scene.sceneRect().width()   * 0.98   - (alignmentBitmapRight->boundingRect().width()      * alignmentBitmapRight->scale()),
                                         (scene.sceneRect().height() * 0.3) + (alignmentBitmapRight->boundingRect().center().y() * alignmentBitmapRight->scale()));
            alignmentBitmapColoredRight = new QGraphicsPixmapItem(GenerateText({tr("     Green-colored boxes.    ")}, QColor(100, 255, 100)));
            alignmentBitmapColoredRight->setScale(GetTextScale(TextSmall));
            alignmentBitmapColoredRight->setPos(alignmentBitmapRight->pos().x(),
                                                alignmentBitmapRight->pos().y() + (alignmentBitmapRight->boundingRect().height() * alignmentBitmapRight->scale()));

            tutorialBitmap = new QGraphicsPixmapItem(GenerateText({tr("Press ESC to exit alignment tool.")}));
            tutorialBitmap->setScale(GetTextScale(TextSub));
            tutorialBitmap->setPos(scene.sceneRect().width() * 0.05,
                                   (scene.sceneRect().bottom() * 0.9) - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));
        } else {
            headerText->setHtml(tr("<p align=\"justify\">Depending on your desired layout,<br>"
                                   "your IR Emitters should be aligned<br>"
                                   "to either one of the two sets<br>"
                                   "of colored boxes:</p>"));
            headerText->setPos(scene.sceneRect().center().x()      - headerText->boundingRect().center().x(),
                               (scene.sceneRect().height() * 0.25) - headerText->boundingRect().center().y());

            alignmentTextLeft = new QGraphicsTextItem();
            alignmentTextLeft->setHtml(tr("<p align=\"justify\">For <b>Square Layout,</b><br>"
                                          "the emitters should be<br>"
                                          "placed at the top and<br>"
                                          "bottom of the display;<br>"
                                          "each one being aligned to the<br>"
                                          "<p style=\"color: tomato\">Red-colored boxes.</p></p>"));
            alignmentTextLeft->setFont(QFont("Monospace", GetTextScale(TextSub)));
            alignmentTextLeft->setPos(scene.sceneRect().width() * 0.075,
                                      (scene.sceneRect().height() * 0.3) + alignmentTextLeft->boundingRect().center().y());

            alignmentTextRight = new QGraphicsTextItem();
            alignmentTextRight->setHtml(tr("<p align=\"justify\">For <b>Diamond Layout,</b><br>"
                                           "the emitters should be placed<br>"
                                           "at the center of the four<br>"
                                           "edges of the display;<br>"
                                           "each one being aligned to the</p>"
                                           "<p style=\"color: palegreen\">Green-colored boxes.</p>"));
            alignmentTextRight->setFont(QFont("Monospace", TextSub));
            alignmentTextRight->setPos(scene.sceneRect().width() * 0.70,
                                       (scene.sceneRect().height() * 0.3) + alignmentTextRight->boundingRect().center().y());

            tutorialText = new QGraphicsTextItem();
            tutorialText->setPlainText(tr("Press ESC to exit alignment tool."));
            tutorialText->setFont(QFont("Monospace", TextSub));
            tutorialText->setPos(scene.sceneRect().width() * 0.05,
                                 (scene.sceneRect().bottom() * 0.9) - tutorialText->boundingRect().center().y());
        }

        // Construct scene with shapes + lines
        for(int i = 0; i < 4; i++) {
            alignmentBoxesSquare[i] = new QGraphicsRectItem();
            alignmentBoxesSquare[i]->setBrush(QBrush(QColor("firebrick")));
            alignmentBoxesDiamond[i] = new QGraphicsRectItem();
            alignmentBoxesDiamond[i]->setBrush(QBrush(QColor("olivedrab")));
            scene.addItem(alignmentBoxesSquare[i]);
            scene.addItem(alignmentBoxesDiamond[i]);
        }

        // square aligners
        // widescreen
        if(scene.sceneRect().width() / scene.sceneRect().height() > 1.4) {
            alignmentBoxesSquare[0]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.3)-(15*GetTextScale(TextSub)), scene.sceneRect().top()-10),
                                                    QPointF((scene.sceneRect().width()*0.3)+(15*GetTextScale(TextSub)), scene.sceneRect().top()+(10*GetTextScale(TextSub)))));
            alignmentBoxesSquare[1]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.7)-(15*GetTextScale(TextSub)), scene.sceneRect().top()-10),
                                                    QPointF((scene.sceneRect().width()*0.7)+(15*GetTextScale(TextSub)), scene.sceneRect().top()+(10*GetTextScale(TextSub)))));
            alignmentBoxesSquare[2]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.3)-(15*GetTextScale(TextSub)), scene.sceneRect().bottom()+10),
                                                    QPointF((scene.sceneRect().width()*0.3)+(15*GetTextScale(TextSub)), scene.sceneRect().bottom()-(10*GetTextScale(TextSub)))));
            alignmentBoxesSquare[3]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.7)-(15*GetTextScale(TextSub)), scene.sceneRect().bottom()+10),
                                                    QPointF((scene.sceneRect().width()*0.7)+(15*GetTextScale(TextSub)), scene.sceneRect().bottom()-(10*GetTextScale(TextSub)))));
            alignmentLines[0] = new QGraphicsPolygonItem(QPolygonF() << QPointF(scene.sceneRect().width()*0.3, scene.sceneRect().top()-10)
                                                                     << QPointF(scene.sceneRect().width()*0.3, scene.sceneRect().bottom()+10)
                                                                     << QPointF(scene.sceneRect().width()*0.7, scene.sceneRect().bottom()+10)
                                                                     << QPointF(scene.sceneRect().width()*0.7, scene.sceneRect().top()-10));
        // 4:3
        } else {
            alignmentBoxesSquare[0]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.2)-(15*GetTextScale(TextSub)), scene.sceneRect().top()-10),
                                                    QPointF((scene.sceneRect().width()*0.2)+(15*GetTextScale(TextSub)), scene.sceneRect().top()+(10*GetTextScale(TextSub)))));
            alignmentBoxesSquare[1]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.8)-(15*GetTextScale(TextSub)), scene.sceneRect().top()-10),
                                                    QPointF((scene.sceneRect().width()*0.8)+(15*GetTextScale(TextSub)), scene.sceneRect().top()+(10*GetTextScale(TextSub)))));
            alignmentBoxesSquare[2]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.2)-(15*GetTextScale(TextSub)), scene.sceneRect().bottom()+10),
                                                    QPointF((scene.sceneRect().width()*0.2)+(15*GetTextScale(TextSub)), scene.sceneRect().bottom()-(10*GetTextScale(TextSub)))));
            alignmentBoxesSquare[3]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.8)-(15*GetTextScale(TextSub)), scene.sceneRect().bottom()+10),
                                                    QPointF((scene.sceneRect().width()*0.8)+(15*GetTextScale(TextSub)), scene.sceneRect().bottom()-(10*GetTextScale(TextSub)))));
            alignmentLines[0] = new QGraphicsPolygonItem(QPolygonF() << QPointF(scene.sceneRect().width()*0.2, scene.sceneRect().top()-10)
                                                                     << QPointF(scene.sceneRect().width()*0.2, scene.sceneRect().bottom()+10)
                                                                     << QPointF(scene.sceneRect().width()*0.8, scene.sceneRect().bottom()+10)
                                                                     << QPointF(scene.sceneRect().width()*0.8, scene.sceneRect().top()-10));
        }

        // diamond aligners
        alignmentBoxesDiamond[0]->setRect(QRectF(QPointF(scene.sceneRect().center().x()-(15*GetTextScale(TextSub)), scene.sceneRect().top()-10),
                                                 QPointF(scene.sceneRect().center().x()+(15*GetTextScale(TextSub)), scene.sceneRect().top()+(10*GetTextScale(TextSub)))));
        alignmentBoxesDiamond[1]->setRect(QRectF(QPointF(scene.sceneRect().center().x()-(15*GetTextScale(TextSub)), scene.sceneRect().bottom()+10),
                                                 QPointF(scene.sceneRect().center().x()+(15*GetTextScale(TextSub)), scene.sceneRect().bottom()-(10*GetTextScale(TextSub)))));
        alignmentBoxesDiamond[2]->setRect(QRectF(QPointF(scene.sceneRect().left()-10,                               scene.sceneRect().center().y()-(15*GetTextScale(TextSub))),
                                                 QPointF(scene.sceneRect().left()+(10*GetTextScale(TextSub)),       scene.sceneRect().center().y()+(15*GetTextScale(TextSub)))));
        alignmentBoxesDiamond[3]->setRect(QRectF(QPointF(scene.sceneRect().right()+10,                              scene.sceneRect().center().y()-(15*GetTextScale(TextSub))),
                                                 QPointF(scene.sceneRect().right()-(10*GetTextScale(TextSub)),      scene.sceneRect().center().y()+(15*GetTextScale(TextSub)))));
        alignmentLines[1] = new QGraphicsPolygonItem(QPolygonF() << QPointF(scene.sceneRect().center().x(), scene.sceneRect().top()-10)
                                                                 << QPointF(scene.sceneRect().center().x(), scene.sceneRect().bottom()+10)
                                                                 << QPointF(scene.sceneRect().left()-10,    scene.sceneRect().bottom()+10)
                                                                 << QPointF(scene.sceneRect().left()-10,    scene.sceneRect().center().y())
                                                                 << QPointF(scene.sceneRect().right()+10,   scene.sceneRect().center().y())
                                                                 << QPointF(scene.sceneRect().right()+10,  scene.sceneRect().top()-10));

        alignmentLines[0]->setPen(QPen(QColor("firebrick"), 2));
        alignmentLines[1]->setPen(QPen(QColor("olivedrab"), 2));

        for(int i = 0; i < 2; ++i)
            scene.addItem(alignmentLines[i]);

        if(bitmapText) {
            scene.addItem(tutorialBitmap);
            scene.addItem(alignmentBitmapLeft);
            scene.addItem(alignmentBitmapColoredLeft);
            scene.addItem(alignmentBitmapRight);
            scene.addItem(alignmentBitmapColoredRight);
            scene.addItem(headerBitmap);
        } else {
            scene.addItem(tutorialText);
            scene.addItem(alignmentTextLeft);
            scene.addItem(alignmentTextRight);
            scene.addItem(headerText);
        }

        break;
    case modeIRTest:
        this->setWindowTitle(tr("IR Emitters Test"));
        ui->graphicsView->setBackgroundBrush(QBrush(QColor("midnightblue")));

        qreal scaleX = scene.sceneRect().width() / 1920.0;
        qreal scaleY = scene.sceneRect().height() / 1080.0;

        if(bitmapText) {
            headerBitmap->setPixmap(GenerateText({tr("     The array of shapes displayed onscreen     "),
                                                  tr("represents the emitters that the camera can see."),
                                                  tr("   The colored points should move opposite to   "),
                                                  tr("     your aim, and the gray circle should be    "),
                                                  tr("          lining up with your gun sight.        ")}));
            headerBitmap->setPos(scene.sceneRect().center().x()     - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 (scene.sceneRect().bottom() * 0.1) - (headerBitmap->boundingRect().center().y() * headerBitmap->scale()));
            scene.addItem(headerBitmap);

            tutorialBitmap = new QGraphicsPixmapItem(GenerateText({tr("Press ESC to exit test mode.")}));
            tutorialBitmap->setScale(GetTextScale(TextSub));
            tutorialBitmap->setPos(scene.sceneRect().center().x() - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                   scene.sceneRect().bottom() * 0.85);
            scene.addItem(tutorialBitmap);
        } else {
            headerText->setHtml(tr("<p align=justify>The array of shapes onscreen<br>"
                                   "represents the emitters that the camera can see.<br>"
                                   "The points should move opposite to your aim,<br>"
                                   "and the gray circle should be lining up with your gun sight.</p>"));
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               (scene.sceneRect().bottom() * 0.1) - headerText->boundingRect().center().y());
            scene.addItem(headerText);

            tutorialText = new QGraphicsTextItem();
            tutorialText->setPlainText(tr("Press ESC to exit test mode."));
            tutorialText->setFont(QFont("Monospace", GetTextScale(TextSub)));
            tutorialText->setPos(scene.sceneRect().center().x() - tutorialText->boundingRect().center().x(), scene.sceneRect().bottom() * 0.85);
            scene.addItem(tutorialText);
        }

        for(int i = 0; i < testPointsCount; i++) {
            testPoints[i] = new QGraphicsEllipseItem();
            testPoints[i]->setTransform(QTransform::fromScale(scaleX, scaleY));
            scene.addItem(testPoints[i]);
        }

        testBox = new QGraphicsPolygonItem();
        testBox->setTransform(QTransform::fromScale(scaleX, scaleY));
        testBox->setPen(QPen(Qt::gray, 2));
        scene.addItem(testBox);

        testPoints[testPointTL] ->setPen(QPen(Qt::green, 3));
        testPoints[testPointTR] ->setPen(QPen(Qt::green, 3));
        testPoints[testPointBL] ->setPen(QPen(Qt::cyan,  3));
        testPoints[testPointBR] ->setPen(QPen(Qt::cyan,  3));
        testPoints[testPointMed]->setPen(QPen(Qt::gray,  3));
        testPoints[testPointD]  ->setPen(QPen(Qt::red,   3));
        break;
    }
}

AppCaliWindow::~AppCaliWindow()
{
    delete ui;
}

int AppCaliWindow::GetTextScale(const int &type)
{
    if(scale == -1) {
        if(scene.sceneRect().height() >= 1440)
             scale = ScaleHiDPI;
        else if(scene.sceneRect().height() >= 1080)
             scale = ScaleBig;
        else if(scene.sceneRect().height() >= 720)
             scale = ScaleSmall;
        else scale = ScaleTiny;
    }

    switch(scale) {
    case ScaleHiDPI:
        switch(type) {
        case TextHeading:
            if(bitmapText) return 5;
            else return 20;
        case TextSub:
            if(bitmapText) return 4;
            else return 18;
        case TextSmall:
            if(bitmapText) return 3;
            else return 16;
        case Crosshair:
            return 4;
        }
    case ScaleBig:
        switch(type) {
        case TextHeading:
            if(bitmapText) return 4;
            else return 18;
        case TextSub:
            if(bitmapText) return 3;
            else return 16;
        case TextSmall:
            if(bitmapText) return 2;
            else return 14;
        case Crosshair:
            return 3;
        }
    case ScaleSmall:
        switch(type) {
        case TextHeading:
            if(bitmapText) return 3;
            else return 16;
        case TextSub:
            if(bitmapText) return 2;
            else return 14;
        case TextSmall:
            if(bitmapText) return 2;
            else return 12;
        case Crosshair:
            return 3;
        }
    case ScaleTiny:
        switch(type) {
        case TextHeading:
            if(bitmapText) return 2;
            else return 14;
        case TextSub:
            if(bitmapText) return 2;
            else return 12;
        case TextSmall:
            if(bitmapText) return 1;
            else return 10;
        case Crosshair:
            return 2;
        }
    }

    // Shouldn't be possible to reach this, otherwise something horrible's gone wrong.
    return 0;
}

QPixmap AppCaliWindow::GenerateText(const QStringList &strings, const QColor &tintColor)
{
    QVector<QPixmap> imageBuffer;
    int maxWidth = 0;

    // used to copy data from resources bitmaps to individual images.
    QPainter painter;

    for(auto const &line : strings) {
        imageBuffer << QPixmap(8*line.length(), 8);
        // init with transparent color
        imageBuffer.last().fill(QColor(0,0,0,0));
        painter.begin(&imageBuffer.last());

        if(maxWidth < imageBuffer.last().width()) maxWidth = imageBuffer.last().width();

        for(int i = 0; i < line.length(); i++)
            if(line.at(i) >= '!' && line.at(i) <= '~')
                painter.drawPixmap(QPoint(8*i, 0), QPixmap(QString(":/testFont/testFont/%1").arg((int)line.at(i).unicode()), "PNG"));

        painter.end();
    }

    QPixmap combinedBuffer(maxWidth, 8*strings.count());
    combinedBuffer.fill(QColor(0,0,0,0));

    painter.begin(&combinedBuffer);
    for(int i = 0; i < imageBuffer.count(); i++)
        painter.drawPixmap(QPoint(0,8*i), imageBuffer.at(i));

    if(tintColor.isValid()) {
        QBitmap mask1 = combinedBuffer.createMaskFromColor(QColor(255, 255, 255), Qt::MaskOutColor);
        QBitmap mask2 = combinedBuffer.createMaskFromColor(QColor(115, 115, 115), Qt::MaskOutColor);
        painter.setPen(QPen(tintColor));
        painter.drawPixmap(combinedBuffer.rect(), mask1);
        // shadowed area gets tinted with a 140 units darker shade
        // need to be careful not to dip into negatives, as that invalidates the QColor
        painter.setPen(QPen(QColor(tintColor.red()   > 140 ? tintColor.red()   - 140 : 0,
                                   tintColor.green() > 140 ? tintColor.green() - 140 : 0,
                                   tintColor.blue()  > 140 ? tintColor.blue()  - 140 : 0)));
        painter.drawPixmap(combinedBuffer.rect(), mask2);
    }

    painter.end();

    return combinedBuffer;
}

void AppCaliWindow::CaliModeSet(const int &caliStage)
{
    switch(caliStage) {
    case Cali_Init:
        ui->graphicsView->setMouseTracking(false);
        mouseCanBeTracked = false;

        alignmentLines[0]->setVisible(false);

        crosshairItem->setVisible(true);
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        // reset calibration info, in case we're restarted to here
        topOffset = -1, bottomOffset = -1, leftOffset = -1, rightOffset = -1, topLeftLed = -1, topRightLed = -1;

        if(bitmapText) {
            caliStageBitmap->setPixmap( GenerateText({tr("Initialize Calibration:")}));
            caliStageBitmap->setPos(scene.sceneRect().center().x()      - (caliStageBitmap->boundingRect().center().x() * caliStageBitmap->scale()),
                                    (scene.sceneRect().height() * 0.25) - (caliStageBitmap->boundingRect().center().y() * caliStageBitmap->scale()));

            headerBitmap->setPixmap(    GenerateText({tr("Shoot at the target to start calibration.")}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));

            tutorialBitmap->setPixmap(  GenerateText({tr("Calibration can be exited without changes"),
                                                      tr("  by pressing either Button A, Button B, "),
                                                      tr("       or Button C (if available).       ")}));
            tutorialBitmap->setPos(scene.sceneRect().center().x()     - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                   (scene.sceneRect().bottom() * 0.8) - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));

            for(int i = 0; i < 6; i++) {
                profileBitmaps[i]->setVisible(false);
                profileBitmaps[i]->setPixmap(GenerateText({caliTypesPrefixes.at(i)}));
            }
        } else {
            caliStageText->setPlainText(tr("Initialize Calibration:"));
            caliStageText->setPos(scene.sceneRect().center().x()      - caliStageText->boundingRect().center().x(),
                                  (scene.sceneRect().bottom() * 0.25) - caliStageText->boundingRect().center().y());

            headerText->setPlainText(tr("Shoot at the target to start calibration."));
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());

            tutorialText->setHtml(tr("<p align=\"center\">Calibration can be exited without changes<br>"
                                     "by pressing either <i>Button A,</i> <i>Button B,</i><br>"
                                     "or <i>Button C (if available).</i></p>"));
            tutorialText->setPos(scene.sceneRect().center().x()     - tutorialText->boundingRect().center().x(),
                                 (scene.sceneRect().bottom() * 0.8) - tutorialText->boundingRect().center().y());
        }

        break;
    case Cali_Top:
        alignmentLines[0]->setVisible(true);

        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().top() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({tr("Cali Step 1:")}));
            caliStageBitmap->setPos(scene.sceneRect().center().x()      - (caliStageBitmap->boundingRect().center().x() * caliStageBitmap->scale()),
                                    (scene.sceneRect().height() * 0.25) - (caliStageBitmap->boundingRect().center().y() * caliStageBitmap->scale()));

            headerBitmap->setPixmap(GenerateText({tr("Shoot at the top edge of the screen.")}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));

            tutorialBitmap->setPixmap(GenerateText({tr("The calibration process can be reset by pressing"),
                                                    tr("either Button A or Button B, and can be canceled"),
                                                    tr("      by pressing Button C (if available).      ")}));
            tutorialBitmap->setPos(scene.sceneRect().center().x()     - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                   (scene.sceneRect().bottom() * 0.8) - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));

            for(int i = 0; i < 6; i++)
                profileBitmaps[i]->setVisible(true);
        } else {
            caliStageText->setPlainText(tr("Cali Step 1:"));
            caliStageText->setPos(scene.sceneRect().center().x()      - caliStageText->boundingRect().center().x(),
                                  (scene.sceneRect().bottom() * 0.25) - caliStageText->boundingRect().center().y());

            headerText->setPlainText(tr("Shoot at the top edge of the screen."));
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());

            tutorialText->setHtml(tr("<p align=\"center\">The calibration process can be reset by pressing<br>"
                                     "either <i>Button A</i> or <i>Button B,</i><br>"
                                     "and can be canceled by pressing <i>Button C (if available).</i></p>"));
            tutorialText->setPos(scene.sceneRect().center().x()     - tutorialText->boundingRect().center().x(),
                                 (scene.sceneRect().bottom() * 0.8) - tutorialText->boundingRect().center().y());
        }

        break;
    case Cali_Bottom:
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().bottom()     - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));
        
        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({tr("Cali Step 2:")}));

            headerBitmap->setPixmap(GenerateText({tr("Shoot at the bottom edge of the screen.")}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));
        } else {
            caliStageText->setPlainText(tr("Cali Step 2:"));

            headerText->setPlainText(tr("Shoot at the bottom edge of the screen."));
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());
        }

        break;
    case Cali_Left:
        crosshairItem->setPos(scene.sceneRect().left()       - (crosshairItem->boundingRect().center().x() * crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y() * crosshairItem->scale()));

        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({tr("Cali Step 3:")}));

            headerBitmap->setPixmap(GenerateText({tr("Shoot at the left edge of the screen.")}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));
        } else {
            caliStageText->setPlainText(tr("Cali Step 3:"));

            headerText->setPlainText(tr("Shoot at the left edge of the screen."));
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());
        }

        break;
    case Cali_Right:
        crosshairItem->setPos(scene.sceneRect().right() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({tr("Cali Step 4:")}));

            headerBitmap->setPixmap(GenerateText({tr("Shoot at the right edge of the screen.")}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));
        } else {
            caliStageText->setPlainText(tr("Cali Step 4:"));

            headerText->setPlainText(tr("Shoot at the right edge of the screen."));
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());
        }

        break;
    case Cali_Center:
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({tr("Cali Step 5:")}));

            headerBitmap->setPixmap(GenerateText({tr("Shoot at the final target in the center.")}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));
        } else {
            caliStageText->setPlainText(tr("Cali Step 5:"));

            headerText->setPlainText(tr("Shoot at the final target in the center."));
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());
        }

        break;
    case Cali_Verify:
        ui->graphicsView->setMouseTracking(true);
        mouseCanBeTracked = true;

        if(bitmapText) {
            if(topOffset    >= -32768  &&  topOffset    <= 32768    &&
               bottomOffset >= -32768  &&  bottomOffset <= 32768    &&
               leftOffset   >= -32768  &&  leftOffset   <= 32768    &&
               rightOffset  >= -32768  &&  rightOffset  <= 32768    &&
               topLeftLed   >= -32768  &&  topLeftLed   <= 32768    &&
               topRightLed  >= -32768  &&  topRightLed  <= 32768) {

                caliStageBitmap->setPixmap( GenerateText({tr("Verify New Calibration:")}));
                caliStageBitmap->setPos(scene.sceneRect().center().x()      - (caliStageBitmap->boundingRect().center().x() * caliStageBitmap->scale()),
                                        scene.sceneRect().height() * 0.15   - (caliStageBitmap->boundingRect().center().y() * caliStageBitmap->scale()));

                headerBitmap->setPixmap(GenerateText({tr("    Confirm that the bullseye     "),
                                                      tr("   lines up with the gun sight.   "),
                                                      tr("If this calibration is acceptable,"),
                                                      tr("  confirm by pulling the trigger. ")}));
                headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                     caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));

                tutorialBitmap->setPixmap(GenerateText({tr("       If this target accuracy isn't desirable,      "),
                                                        tr("          press either Button A or Button B          "),
                                                        tr("         to restart the calibration process.         "),
                                                        tr(""),
                                                        tr("[You can also exit calibration without saving changes"),
                                                        tr("        by pressing Button C (if available).]        ")}));
                tutorialBitmap->setPos(scene.sceneRect().center().x()       - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                       (scene.sceneRect().bottom() * 0.8)   - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));
            } else if(topLeftLed == -1 || topRightLed == -1) {
                caliStageBitmap->setPixmap( GenerateText({tr("Verify New Calibration:")}));
                caliStageBitmap->setPos(scene.sceneRect().center().x()      - (caliStageBitmap->boundingRect().center().x() * caliStageBitmap->scale()),
                                        scene.sceneRect().height() * 0.15   - (caliStageBitmap->boundingRect().center().y() * caliStageBitmap->scale()));

                tutorialBitmap->setPixmap(GenerateText({tr("")}));
            } else {
                caliStageBitmap->setPixmap(GenerateText({tr("WARNING: Possibly Malformed Calibration!!")}, QColor(225,25,25)));
                caliStageBitmap->setPos(scene.sceneRect().center().x()      - (caliStageBitmap->boundingRect().center().x() * caliStageBitmap->scale()),
                                        scene.sceneRect().height() * 0.15   - (caliStageBitmap->boundingRect().center().y() * caliStageBitmap->scale()));

                headerBitmap->setPixmap(GenerateText({tr("  The current pending values for this profile  "),
                                                      tr("will likely cause incorrect or broken tracking.")},
                                                     QColor(225,25,25)));
                headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                     caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));

                tutorialBitmap->setPixmap(GenerateText({tr("  Press Button A or Button B to restart calibration, "),
                                                        tr("   or pull trigger to continue with these settings.  "),
                                                        tr(""),
                                                        tr("[You can also exit calibration without saving changes"),
                                                        tr("        by pressing Button C (if available).]        ")},
                                                       QColor(225,25,25)));
                tutorialBitmap->setPos(scene.sceneRect().center().x()       - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                       (scene.sceneRect().bottom() * 0.8)   - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));
            }
        } else {
            caliStageText->setPlainText(tr("Verify New Calibration:"));
            caliStageText->setPos(scene.sceneRect().center().x()        - caliStageText->boundingRect().center().x(),
                                  (scene.sceneRect().bottom() * 0.25)   - caliStageText->boundingRect().center().y());

            headerText->setHtml(tr("<p align=\"center\">Confirm that the bullseye lines up with gun sight.<br>"
                                   "If this calibration is acceptable, confirm by pulling the trigger.</p>"));
            headerText->setPos(scene.sceneRect().center().x()   - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()         + caliStageText->boundingRect().height());

            tutorialText->setHtml(tr("<p align=\"center\">If the target accuracy isn't desirable,<br>"
                                     "press either <i>Button A</i> or <i>Button B</i><br>"
                                     "to restart the calibration process.<br>"
                                     "<br>"
                                     "[You can also exit calibration without saving changes<br>"
                                     "by pressing <i>Button C (if available).</i>]</p>"));
            tutorialText->setPos(scene.sceneRect().center().x() - tutorialText->boundingRect().center().x(),
                                 (scene.sceneRect().bottom() * 0.8) - tutorialText->boundingRect().center().y());
        }

        break;
    case Cali_End:
        emit WindowExiting(mode, topOffset, bottomOffset, leftOffset, rightOffset, topLeftLed, topRightLed);
        break;
    }
}

void AppCaliWindow::CaliModeTextUpdate(const uint8_t &type, const char* data)
{
    if(bitmapText) {
        switch(type) {
        case 1: // top offset
            memcpy(&topOffset, data, 4);
            profileBitmaps[0]->setPixmap(GenerateText({caliTypesPrefixes.at(0) + QString::number(topOffset)}));
            break;
        case 2: // bottom offset
            memcpy(&bottomOffset, data, 4);
            profileBitmaps[1]->setPixmap(GenerateText({caliTypesPrefixes.at(1) + QString::number(bottomOffset)}));
            break;
        case 3: // left offset
            memcpy(&leftOffset, data, 4);
            profileBitmaps[2]->setPixmap(GenerateText({caliTypesPrefixes.at(2) + QString::number(leftOffset)}));
            break;
        case 4: // right offset
            memcpy(&rightOffset, data, 4);
            profileBitmaps[3]->setPixmap(GenerateText({caliTypesPrefixes.at(3) + QString::number(rightOffset)}));
            break;
        case 5: // topleft
            memcpy(&topLeftLed, data, 4);
            profileBitmaps[4]->setPixmap(GenerateText({caliTypesPrefixes.at(4) + QString::number(topLeftLed, 'g', 6)}));
            break;
        case 6: // topright
            memcpy(&topRightLed, data, 4);
            profileBitmaps[5]->setPixmap(GenerateText({caliTypesPrefixes.at(5) + QString::number(topRightLed, 'g', 6)}));
            // HACK: re-update final scene since this may happen AFTER the verification check stage
            CaliModeSet(Cali_Verify);
            break;
        }
    } else {
        switch(type) {
        case 1: // top offset
            break;
        case 2: // bottom offset
            break;
        case 3: // left offset
            break;
        case 4: // right offset
            break;
        case 5: // topleft
            break;
        case 6: // topright
            break;
        }
    }
}

void AppCaliWindow::TestModeDraw(const int coordsList[12])
{
    testPoints[testPointTL]->setRect(   coordsList[0]  - 25,  coordsList[1]  - 25, 50, 50);
    testPoints[testPointTR]->setRect(   coordsList[2]  - 25,  coordsList[3]  - 25, 50, 50);
    testPoints[testPointBL]->setRect(   coordsList[4]  - 25,  coordsList[5]  - 25, 50, 50);
    testPoints[testPointBR]->setRect(   coordsList[6]  - 25,  coordsList[7]  - 25, 50, 50);
    testPoints[testPointMed]->setRect(  coordsList[8]  - 25,  coordsList[9]  - 25, 50, 50);
    testPoints[testPointD]->setRect(    coordsList[10] - 25,  coordsList[11] - 25, 50, 50);

    testBox->setPolygon(QPolygonF() << QPointF(coordsList[0], coordsList[1])
                                    << QPointF(coordsList[2], coordsList[3])
                                    << QPointF(coordsList[6], coordsList[7])
                                    << QPointF(coordsList[4], coordsList[5])
                                    << QPointF(coordsList[0], coordsList[1]));
}
