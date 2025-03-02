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
        this->setWindowTitle("Alignment Assistant");
        ui->graphicsView->setBackgroundBrush(QBrush(QColor("darkslategray")));

        if(bitmapText) {
            headerBitmap->setPixmap(GenerateText({"       Depending on your desired layout,       ",
                                                  "      your IR emitters should be aligned       ",
                                                  "to either one of the two sets of colored boxes:"}));
            headerBitmap->setPos(scene.sceneRect().center().x()     - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 scene.sceneRect().height() * 0.15  - (headerBitmap->boundingRect().center().y() * headerBitmap->scale()));

            alignmentBitmapLeft = new QGraphicsPixmapItem(GenerateText({"      For Square Layout,     ",
                                                                        "    the emitters should be   ",
                                                                        "    placed at the top and    ",
                                                                        "    bottom of the display;   ",
                                                                        "each one being aligned to the"}));
            alignmentBitmapLeft->setScale(GetTextScale(TextSmall));
            alignmentBitmapLeft->setPos(scene.sceneRect().width() * 0.05,
                                        (scene.sceneRect().height() * 0.3) + (alignmentBitmapLeft->boundingRect().center().y() * alignmentBitmapLeft->scale()));
            alignmentBitmapColoredLeft = new QGraphicsPixmapItem(GenerateText({"      Red-colored boxes.     "}, QColor(255, 100, 100)));
            alignmentBitmapColoredLeft->setScale(GetTextScale(TextSmall));
            alignmentBitmapColoredLeft->setPos(alignmentBitmapLeft->pos().x(),
                                               alignmentBitmapLeft->pos().y() + (alignmentBitmapLeft->boundingRect().height() * alignmentBitmapLeft->scale()));

            alignmentBitmapRight = new QGraphicsPixmapItem(GenerateText({"     For Diamond Layout,     ",
                                                                         "the emitters should be placed",
                                                                         "  at the center of the four  ",
                                                                         "    edges of the display;    ",
                                                                         "each one being aligned to the"}));
            alignmentBitmapRight->setScale(GetTextScale(TextSmall));
            alignmentBitmapRight->setPos(scene.sceneRect().width()   * 0.96   - (alignmentBitmapRight->boundingRect().width()      * alignmentBitmapRight->scale()),
                                         (scene.sceneRect().height() * 0.3) + (alignmentBitmapRight->boundingRect().center().y() * alignmentBitmapRight->scale()));
            alignmentBitmapColoredRight = new QGraphicsPixmapItem(GenerateText({"     Green-colored boxes.    "}, QColor(100, 255, 100)));
            alignmentBitmapColoredRight->setScale(GetTextScale(TextSmall));
            alignmentBitmapColoredRight->setPos(alignmentBitmapRight->pos().x(),
                                                alignmentBitmapRight->pos().y() + (alignmentBitmapRight->boundingRect().height() * alignmentBitmapRight->scale()));

            tutorialBitmap = new QGraphicsPixmapItem(GenerateText({"Press ESC to exit alignment tool."}));
            tutorialBitmap->setScale(GetTextScale(TextSub));
            tutorialBitmap->setPos(scene.sceneRect().width() * 0.05,
                                   (scene.sceneRect().bottom() * 0.9) - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));
        } else {
            headerText->setHtml("<p align=\"justify\">Depending on your desired layout,<br>"
                                "your IR Emitters should be aligned<br>"
                                "to either one of the two sets of colored boxes:</p>");
            headerText->setPos(scene.sceneRect().center().x()      - headerText->boundingRect().center().x(),
                               (scene.sceneRect().height() * 0.25) - headerText->boundingRect().center().y());

            alignmentTextLeft = new QGraphicsTextItem();
            alignmentTextLeft->setHtml("<p align=\"justify\">For <b>Square Layout,</b><br>"
                                       "the emitters should be<br>"
                                       "placed at the top and<br>"
                                       "bottom of the display;<br>"
                                       "each one being aligned to the<br>"
                                       "<p style=\"color: tomato\">Red-colored boxes.</p></p>");
            alignmentTextLeft->setFont(QFont("Monospace", GetTextScale(TextSub)));
            alignmentTextLeft->setPos(scene.sceneRect().width() * 0.075,
                                      (scene.sceneRect().height() * 0.3) + alignmentTextLeft->boundingRect().center().y());

            alignmentTextRight = new QGraphicsTextItem();
            alignmentTextRight->setHtml("<p align=\"justify\">For <b>Diamond Layout,</b><br>"
                                        "the emitters should be placed<br>"
                                        "at the center of the four<br>"
                                        "edges of the display;<br>"
                                        "each one being aligned to the</p>"
                                        "<p style=\"color: palegreen\">Green-colored boxes.</p>");
            alignmentTextRight->setFont(QFont("Monospace", TextSub));
            alignmentTextRight->setPos(scene.sceneRect().width() * 0.70,
                                       (scene.sceneRect().height() * 0.3) + alignmentTextRight->boundingRect().center().y());

            tutorialText = new QGraphicsTextItem();
            tutorialText->setPlainText("Press ESC to exit alignment tool.");
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

        alignmentBoxesSquare[0]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.330)-60, scene.sceneRect().top()-10),
                                                QPointF((scene.sceneRect().width()*0.330)+60, scene.sceneRect().top()+40)));
        alignmentBoxesSquare[1]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.665)-60, scene.sceneRect().top()-10),
                                                QPointF((scene.sceneRect().width()*0.665)+60, scene.sceneRect().top()+40)));
        alignmentBoxesSquare[2]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.330)-60, scene.sceneRect().bottom()+10),
                                                QPointF((scene.sceneRect().width()*0.330)+60, scene.sceneRect().bottom()-40)));
        alignmentBoxesSquare[3]->setRect(QRectF(QPointF((scene.sceneRect().width()*0.665)-60, scene.sceneRect().bottom()+10),
                                                QPointF((scene.sceneRect().width()*0.665)+60, scene.sceneRect().bottom()-40)));

        alignmentBoxesDiamond[0]->setRect(QRectF(QPointF(scene.sceneRect().center().x()-60, scene.sceneRect().top()-10),
                                                 QPointF(scene.sceneRect().center().x()+60, scene.sceneRect().top()+40)));
        alignmentBoxesDiamond[1]->setRect(QRectF(QPointF(scene.sceneRect().center().x()-60, scene.sceneRect().bottom()+10),
                                                 QPointF(scene.sceneRect().center().x()+60, scene.sceneRect().bottom()-40)));
        alignmentBoxesDiamond[2]->setRect(QRectF(QPointF(scene.sceneRect().left()-10,       scene.sceneRect().center().y()-60),
                                                 QPointF(scene.sceneRect().left()+40,       scene.sceneRect().center().y()+60)));
        alignmentBoxesDiamond[3]->setRect(QRectF(QPointF(scene.sceneRect().right()+10,      scene.sceneRect().center().y()-60),
                                                 QPointF(scene.sceneRect().right()-40,      scene.sceneRect().center().y()+60)));

        // square
        alignmentLines[0] = new QGraphicsPolygonItem(QPolygonF() << QPointF(scene.sceneRect().width()*0.330, scene.sceneRect().top()-10)
                                                                 << QPointF(scene.sceneRect().width()*0.330, scene.sceneRect().bottom()+10)
                                                                 << QPointF(scene.sceneRect().width()*0.665, scene.sceneRect().bottom()+10)
                                                                 << QPointF(scene.sceneRect().width()*0.665, scene.sceneRect().top()-10));
        alignmentLines[0]->setPen(QPen(QColor("firebrick"), 2));
        // diamond
        alignmentLines[1] = new QGraphicsPolygonItem(QPolygonF() << QPointF(scene.sceneRect().center().x(), scene.sceneRect().top()-10)
                                                                 << QPointF(scene.sceneRect().center().x(), scene.sceneRect().bottom()+10)
                                                                 << QPointF(scene.sceneRect().left()-10,    scene.sceneRect().bottom()+10)
                                                                 << QPointF(scene.sceneRect().left()-10,    scene.sceneRect().center().y())
                                                                 << QPointF(scene.sceneRect().right()+10,   scene.sceneRect().center().y())
                                                                 << QPointF(scene.sceneRect().right()+10,  scene.sceneRect().top()-10));
        alignmentLines[1]->setPen(QPen(QColor("olivedrab"), 2));

        for(int i = 0; i < 2; i++)
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
        this->setWindowTitle("IR Emitters Test");
        ui->graphicsView->setBackgroundBrush(QBrush(QColor("midnightblue")));

        if(bitmapText) {
            headerBitmap->setPixmap(GenerateText({"     The array of shapes displayed onscreen     ",
                                                  "represents the emitters that the camera can see.",
                                                  "   The colored points should move opposite to   ",
                                                  "     your aim, and the gray circle should be    ",
                                                  "          lining up with your gun sight.        "}));
            headerBitmap->setPos(scene.sceneRect().center().x()     - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 (scene.sceneRect().bottom() * 0.1) - (headerBitmap->boundingRect().center().y() * headerBitmap->scale()));
            scene.addItem(headerBitmap);

            tutorialBitmap = new QGraphicsPixmapItem(GenerateText({"Press ESC to exit test mode."}));
            tutorialBitmap->setScale(GetTextScale(TextSub));
            tutorialBitmap->setPos(scene.sceneRect().center().x() - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                   scene.sceneRect().bottom() * 0.85);
            scene.addItem(tutorialBitmap);
        } else {
            headerText->setHtml("<p align=justify>The array of shapes onscreen<br>"
                                "represents the emitters that the camera can see.<br>"
                                "The points should move opposite to your aim,<br>"
                                "and the gray circle should be lining up with your gun sight.</p>");
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               (scene.sceneRect().bottom() * 0.1) - headerText->boundingRect().center().y());
            scene.addItem(headerText);

            tutorialText = new QGraphicsTextItem();
            tutorialText->setPlainText("Press ESC to exit test mode.");
            tutorialText->setFont(QFont("Monospace", GetTextScale(TextSub)));
            tutorialText->setPos(scene.sceneRect().center().x() - tutorialText->boundingRect().center().x(), scene.sceneRect().bottom() * 0.85);
            scene.addItem(tutorialText);
        }

        for(int i = 0; i < testPointsCount; i++) {
            testPoints[i] = new QGraphicsEllipseItem();
            scene.addItem(testPoints[i]);
        }

        testBox = new QGraphicsPolygonItem();
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
        else    scale = ScaleSmall;
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
            return 3;
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
            return 2;
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
                painter.drawPixmap(QPoint(8*i, 0), QPixmap(QString(":/testFont/testFont/%1").arg(line.at(i).unicode()), "PNG"));

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

        crosshairItem->setVisible(true);
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        if(bitmapText) {
            caliStageBitmap->setPixmap( GenerateText({"Initialize Calibration:"}));
            caliStageBitmap->setPos(scene.sceneRect().center().x()      - (caliStageBitmap->boundingRect().center().x() * caliStageBitmap->scale()),
                                    (scene.sceneRect().height() * 0.25) - (caliStageBitmap->boundingRect().center().y() * caliStageBitmap->scale()));

            headerBitmap->setPixmap(    GenerateText({"Shoot at the target to start calibration."}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));

            tutorialBitmap->setPixmap(  GenerateText({"Calibration can be exited without changes by pressing",
                                                      "either Button A, Button B, or Button C (if available)."}));
            tutorialBitmap->setPos(scene.sceneRect().center().x()     - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                   (scene.sceneRect().bottom() * 0.8) - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));

            for(int i = 0; i < 6; i++) {
                profileBitmaps[i]->setVisible(false);
                profileBitmaps[i]->setPixmap(GenerateText({caliTypesPrefixes.at(i)}));
            }
        } else {
            caliStageText->setPlainText("Initialize Calibration:");
            caliStageText->setPos(scene.sceneRect().center().x()      - caliStageText->boundingRect().center().x(),
                                  (scene.sceneRect().bottom() * 0.25) - caliStageText->boundingRect().center().y());

            headerText->setPlainText("Shoot at the target to start calibration.");
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());

            tutorialText->setHtml("<p align=\"center\">Calibration can be exited without changes by pressing<br>"
                                  "either <i>Button A,</i> <i>Button B,</i> or <i>Button C (if available).</i></p>");
            tutorialText->setPos(scene.sceneRect().center().x()     - tutorialText->boundingRect().center().x(),
                                 (scene.sceneRect().bottom() * 0.8) - tutorialText->boundingRect().center().y());
        }

        break;
    case Cali_Top:
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().top() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({"Cali Step 1:"}));
            caliStageBitmap->setPos(scene.sceneRect().center().x()      - (caliStageBitmap->boundingRect().center().x() * caliStageBitmap->scale()),
                                    (scene.sceneRect().height() * 0.25) - (caliStageBitmap->boundingRect().center().y() * caliStageBitmap->scale()));

            headerBitmap->setPixmap(GenerateText({"Shoot at the top edge of the screen."}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));

            tutorialBitmap->setPixmap(GenerateText({"The calibration process can be reset by pressing",
                                                    "either Button A or Button B, and can be canceled",
                                                    "      by pressing Button C (if available).      "}));
            tutorialBitmap->setPos(scene.sceneRect().center().x()     - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                   (scene.sceneRect().bottom() * 0.8) - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));

            for(int i = 0; i < 6; i++)
                profileBitmaps[i]->setVisible(true);
        } else {
            caliStageText->setPlainText("Cali Step 1:");
            caliStageText->setPos(scene.sceneRect().center().x()      - caliStageText->boundingRect().center().x(),
                                  (scene.sceneRect().bottom() * 0.25) - caliStageText->boundingRect().center().y());

            headerText->setPlainText("Shoot at the top edge of the screen.");
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());

            tutorialText->setHtml("<p align=\"center\">The calibration process can be reset by pressing<br>"
                                  "either <i>Button A</i> or <i>Button B,</i><br>"
                                  "and can be canceled by pressing <i>Button C (if available).</i></p>");
            tutorialText->setPos(scene.sceneRect().center().x()     - tutorialText->boundingRect().center().x(),
                                 (scene.sceneRect().bottom() * 0.8) - tutorialText->boundingRect().center().y());
        }

        break;
    case Cali_Bottom:
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().bottom()     - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));
        
        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({"Cali Step 2:"}));

            headerBitmap->setPixmap(GenerateText({"Shoot at the bottom edge of the screen."}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));
        } else {
            caliStageText->setPlainText("Cali Step 2:");

            headerText->setPlainText("Shoot at the bottom edge of the screen.");
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());
        }

        break;
    case Cali_Left:
        crosshairItem->setPos(scene.sceneRect().left()       - (crosshairItem->boundingRect().center().x() * crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y() * crosshairItem->scale()));

        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({"Cali Step 3:"}));

            headerBitmap->setPixmap(GenerateText({"Shoot at the left edge of the screen."}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));
        } else {
            caliStageText->setPlainText("Cali Step 3:");

            headerText->setPlainText("Shoot at the left edge of the screen.");
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());
        }

        break;
    case Cali_Right:
        crosshairItem->setPos(scene.sceneRect().right() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({"Cali Step 4:"}));

            headerBitmap->setPixmap(GenerateText({"Shoot at the right edge of the screen."}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));
        } else {
            caliStageText->setPlainText("Cali Step 4:");

            headerText->setPlainText("Shoot at the right edge of the screen.");
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());
        }

        break;
    case Cali_Center:
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        if(bitmapText) {
            caliStageBitmap->setPixmap(GenerateText({"Cali Step 5:"}));

            headerBitmap->setPixmap(GenerateText({"Shoot at the final target in the center."}));
            headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                 caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));
        } else {
            caliStageText->setPlainText("Cali Step 5:");

            headerText->setPlainText("Shoot at the final target in the center.");
            headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()       + caliStageText->boundingRect().height());
        }

        break;
    case Cali_Verify:
        ui->graphicsView->setMouseTracking(true);
        mouseCanBeTracked = true;

        if(bitmapText) {
            if(topOffset >= -1000 && topOffset <= 1000 &&
                bottomOffset >= -1000 && bottomOffset <= 1000 &&
                leftOffset >= -1000 && leftOffset <= 1000 &&
                rightOffset >= -1000 && rightOffset <= 1000 &&
                topLeftLed >= 0 && topLeftLed <= 8000 &&
                topRightLed <= 8000) {

                caliStageBitmap->setPixmap( GenerateText({"Verify New Calibration:"}));
                caliStageBitmap->setPos(scene.sceneRect().center().x()      - (caliStageBitmap->boundingRect().center().x() * caliStageBitmap->scale()),
                                        scene.sceneRect().height() * 0.15   - (caliStageBitmap->boundingRect().center().y() * caliStageBitmap->scale()));

                headerBitmap->setPixmap(GenerateText({"    Confirm that the bullseye     ",
                                                      "   lines up with the gun sight.   ",
                                                      "If this calibration is acceptable,",
                                                      "  confirm by pulling the trigger. "}));
                headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                     caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));

                tutorialBitmap->setPixmap(GenerateText({"       If this target accuracy isn't desirable,      ",
                                                        "          press either Button A or Button B          ",
                                                        "         to restart the calibration process.         ",
                                                        "",
                                                        "[You can also exit calibration without saving changes",
                                                        "        by pressing Button C (if available).]        "}));
                tutorialBitmap->setPos(scene.sceneRect().center().x()       - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                       (scene.sceneRect().bottom() * 0.8)   - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));
            } else {
                caliStageBitmap->setPixmap( GenerateText({"WARNING: Possibly Malformed Calibration!!"}, QColor(225,25,25)));
                caliStageBitmap->setPos(scene.sceneRect().center().x()      - (caliStageBitmap->boundingRect().center().x() * caliStageBitmap->scale()),
                                        scene.sceneRect().height() * 0.15   - (caliStageBitmap->boundingRect().center().y() * caliStageBitmap->scale()));

                headerBitmap->setPixmap(GenerateText({"  The current pending values for this profile  ",
                                                      "will likely cause incorrect or broken tracking."},
                                                     QColor(225,25,25)));
                headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x() * headerBitmap->scale()),
                                     caliStageBitmap->pos().y()     + (caliStageBitmap->boundingRect().height()  * caliStageBitmap->scale()));

                tutorialBitmap->setPixmap(GenerateText({"Press Button A or Button B to restart calibration,",
                                                        "           or pull the trigger to exit.           "},
                                                       QColor(225,25,25)));
                tutorialBitmap->setPos(scene.sceneRect().center().x()       - (tutorialBitmap->boundingRect().center().x() * tutorialBitmap->scale()),
                                       (scene.sceneRect().bottom() * 0.8)   - (tutorialBitmap->boundingRect().center().y() * tutorialBitmap->scale()));
            }
        } else {
            caliStageText->setPlainText("Verify New Calibration:");
            caliStageText->setPos(scene.sceneRect().center().x()        - caliStageText->boundingRect().center().x(),
                                  (scene.sceneRect().bottom() * 0.25)   - caliStageText->boundingRect().center().y());

            headerText->setHtml("<p align=\"center\">Confirm that the bullseye lines up with gun sight.<br>"
                                "If this calibration is acceptable, confirm by pulling the trigger.</p>");
            headerText->setPos(scene.sceneRect().center().x()   - headerText->boundingRect().center().x(),
                               caliStageText->pos().y()         + caliStageText->boundingRect().height());

            tutorialText->setHtml("<p align=\"center\">If the target accuracy isn't desirable,<br>"
                                  "press either <i>Button A</i> or <i>Button B</i><br>"
                                  "to restart the calibration process.<br>"
                                  "<br>"
                                  "[You can also exit calibration without saving changes<br>"
                                  "by pressing <i>Button C (if available).</i>]</p>");
            tutorialText->setPos(scene.sceneRect().center().x() - tutorialText->boundingRect().center().x(),
                                 (scene.sceneRect().bottom() * 0.8) - tutorialText->boundingRect().center().y());
        }

        break;
    case Cali_End:
        emit WindowExiting(mode, topOffset, bottomOffset, leftOffset, rightOffset, topLeftLed, topRightLed);
        break;
    }
}

void AppCaliWindow::CaliModeTextUpdate(const QString &text)
{
    int type = text.front().digitValue();

    if(bitmapText) {
        switch(type) {
        case 1: // top offset
            topOffset = text.mid(text.indexOf('.')+1).trimmed().toInt();
            profileBitmaps[0]->setPixmap(GenerateText({caliTypesPrefixes.at(0) + QString::number(topOffset)}));
            break;
        case 2: // bottom offset
            bottomOffset = text.mid(text.indexOf('.')+1).trimmed().toInt();
            profileBitmaps[1]->setPixmap(GenerateText({caliTypesPrefixes.at(1) + QString::number(bottomOffset)}));
            break;
        case 3: // left offset
            leftOffset = text.mid(text.indexOf('.')+1).trimmed().toInt();
            profileBitmaps[2]->setPixmap(GenerateText({caliTypesPrefixes.at(2) + QString::number(leftOffset)}));
            break;
        case 4: // right offset
            rightOffset = text.mid(text.indexOf('.')+1).trimmed().toInt();
            profileBitmaps[3]->setPixmap(GenerateText({caliTypesPrefixes.at(3) + QString::number(rightOffset)}));
            break;
        case 5: // topleft
            topLeftLed = text.mid(text.indexOf('.')+1).trimmed().toFloat();
            profileBitmaps[4]->setPixmap(GenerateText({caliTypesPrefixes.at(4) + QString::number(topLeftLed, 'g', 6)}));
            break;
        case 6: // topright
            topRightLed = text.mid(text.indexOf('.')+1).trimmed().toFloat();
            profileBitmaps[5]->setPixmap(GenerateText({caliTypesPrefixes.at(5) + QString::number(topRightLed, 'g', 6)}));
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

void AppCaliWindow::TestModeDraw(const QStringList &coordsList)
{
    testPoints[testPointTL]->setRect(   coordsList[0].toInt()  - 25,  coordsList[1].toInt()  - 25, 50, 50);
    testPoints[testPointTR]->setRect(   coordsList[2].toInt()  - 25,  coordsList[3].toInt()  - 25, 50, 50);
    testPoints[testPointBL]->setRect(   coordsList[4].toInt()  - 25,  coordsList[5].toInt()  - 25, 50, 50);
    testPoints[testPointBR]->setRect(   coordsList[6].toInt()  - 25,  coordsList[7].toInt()  - 25, 50, 50);
    testPoints[testPointMed]->setRect(  coordsList[8].toInt()  - 25,  coordsList[9].toInt()  - 25, 50, 50);
    testPoints[testPointD]->setRect(    coordsList[10].toInt() - 25,  coordsList[11].toInt() - 25, 50, 50);

    testBox->setPolygon(QPolygonF() << QPointF(coordsList[0].toInt(), coordsList[1].toInt())
                                    << QPointF(coordsList[2].toInt(), coordsList[3].toInt())
                                    << QPointF(coordsList[6].toInt(), coordsList[7].toInt())
                                    << QPointF(coordsList[4].toInt(), coordsList[5].toInt())
                                    << QPointF(coordsList[0].toInt(), coordsList[1].toInt()));
}
