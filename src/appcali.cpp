#include "appcali.h"
#include "ui_appcali.h"

#include <QScreen>
#include <QDebug>

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
    headerBitmap = new QGraphicsPixmapItem();
    headerBitmap->setScale(4);

    switch(windowMode) {
    case modeCalibrate:
        this->setWindowTitle("Calibration Window");
        ui->graphicsView->setBackgroundBrush(QBrush(QColor("dimgray")));

        crosshairItem = new QGraphicsSvgItem(":/cali/crosshair.svg");
        // this is supposed to work, but doesn't, so we just offset every setPos command by the center point of the SVG, mult'd by scale
        //crosshairItem->setTransformOriginPoint(crosshairItem->boundingRect().center());

        tutorialText = new QGraphicsTextItem();
        tutorialText->setFont(QFont("Monospace", 16));

        // add items to scene
        scene.addItem(headerBitmap);
        scene.addItem(tutorialText);
        scene.addItem(crosshairItem);
        crosshairItem->setVisible(false);

        // set scale, then set crosshair just offscreen at startup
        crosshairItem->setScale(3);

        ui->graphicsView->setMouseTracking(false);

        CaliModeSet(Cali_Init);
        break;
    case modeAlignment:
        this->setWindowTitle("Alignment Assistant");
        ui->graphicsView->setBackgroundBrush(QBrush(QColor("darkslategray")));

        headerText->setHtml("<p align=\"justify\">Depending on your desired layout,<br>"
                            "your IR Emitters should be aligned<br>"
                            "to either one of the two sets of colored boxes:</p>");
        headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                           (scene.sceneRect().height() * 0.25) - headerText->boundingRect().center().y());

        alignmentTextLeft = new QGraphicsTextItem();
        alignmentTextLeft->setHtml("<p align=\"justify\">For <b>Square Layout,</b> the emitters should<br>"
                                   "be placed at the top and bottom<br>"
                                   "of the display; each one being aligned to the"
                                   "<p style=\"color: tomato\">Red-colored boxes.</p></p>");
        alignmentTextLeft->setFont(QFont("Monospace", 14));
        alignmentTextLeft->setPos(scene.sceneRect().width() * 0.075,
                                  (scene.sceneRect().height() * 0.3) + alignmentTextLeft->boundingRect().center().y());

        alignmentTextRight = new QGraphicsTextItem();
        alignmentTextRight->setHtml("<p align=\"justify\">For <b>Diamond Layout,</b> the emitters should<br>"
                                    "be placed at the center of the four edges<br>"
                                    "of the display; each one being aligned to the</p>"
                                    "<p style=\"color: palegreen\">Green-colored boxes.</p>");
        alignmentTextRight->setFont(QFont("Monospace", 14));
        alignmentTextRight->setPos(scene.sceneRect().width() * 0.70,
                                   (scene.sceneRect().height() * 0.3) + alignmentTextRight->boundingRect().center().y());

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

        tutorialText = new QGraphicsTextItem();
        tutorialText->setPlainText("Press ESC to exit alignment tool.");
        tutorialText->setFont(QFont("Monospace", 14));
        tutorialText->setPos(scene.sceneRect().left()+100, scene.sceneRect().bottom() * 0.8);
        scene.addItem(tutorialText);
        scene.addItem(alignmentTextLeft);
        scene.addItem(alignmentTextRight);
        scene.addItem(headerText);

        break;
    case modeIRTest:
        this->setWindowTitle("IR Emitters Test");
        ui->graphicsView->setBackgroundBrush(QBrush(QColor("midnightblue")));

        headerText->setHtml("<p align=justify>The array of shapes onscreen represents the emitters that the camera can see.<br>"
                            "The points should move opposite to your aim, with the gray circle representing line-of-sight.</p>");
        headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                           (scene.sceneRect().bottom() * 0.1) - headerText->boundingRect().center().y());
        scene.addItem(headerText);

        tutorialText = new QGraphicsTextItem();
        tutorialText->setPlainText("Press ESC to exit alignment tool.");
        tutorialText->setFont(QFont("Monospace", 14));
        tutorialText->setPos(scene.sceneRect().center().x() - tutorialText->boundingRect().center().x(), scene.sceneRect().bottom() * 0.85);
        scene.addItem(tutorialText);

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

QPixmap AppCaliWindow::GenerateText(const QStringList &strings)
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

        headerBitmap->setPixmap(GenerateText({"Shoot at the target to start calibration."}));
        headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x()*headerBitmap->scale()),
                             scene.sceneRect().height() * 0.25 - (headerBitmap->boundingRect().center().y()*headerBitmap->scale()));

        tutorialText->setHtml("<p align=\"center\">Calibration can be exited without changes by pressing<br>"
                              "either <i>Button A,</i> <i>Button B,</i> or <i>Button C (if available).</i></p>");
        tutorialText->setPos(scene.sceneRect().center().x() - tutorialText->boundingRect().center().x(),
                             (scene.sceneRect().bottom() * 0.8) - tutorialText->boundingRect().center().y());
        break;
    case Cali_Top:
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().top() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        headerBitmap->setPixmap(GenerateText({"Shoot at the top edge of the screen."}));
        headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x()*headerBitmap->scale()),
                             scene.sceneRect().height() * 0.25 - (headerBitmap->boundingRect().center().y()*headerBitmap->scale()));

        tutorialText->setHtml("<p align=\"center\">The calibration process can be reset by pressing<br>"
                              "either <i>Button A</i> or <i>Button B,</i><br>"
                              "and can be canceled by pressing <i>Button C (if available).</i></p>");
        tutorialText->setPos(scene.sceneRect().center().x() - tutorialText->boundingRect().center().x(),
                             (scene.sceneRect().bottom() * 0.8) - tutorialText->boundingRect().center().y());
        break;
    case Cali_Bottom:
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().bottom() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));
        
        headerBitmap->setPixmap(GenerateText({"Shoot at the bottom edge of the screen."}));
        headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x()*headerBitmap->scale()),
                             scene.sceneRect().height() * 0.25 - (headerBitmap->boundingRect().center().y()*headerBitmap->scale()));
        break;
    case Cali_Left:
        crosshairItem->setPos(scene.sceneRect().left() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        headerBitmap->setPixmap(GenerateText({"Shoot at the left edge of the screen."}));
        headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x()*headerBitmap->scale()),
                             scene.sceneRect().height() * 0.25 - (headerBitmap->boundingRect().center().y()*headerBitmap->scale()));
        break;
    case Cali_Right:
        crosshairItem->setPos(scene.sceneRect().right() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        headerBitmap->setPixmap(GenerateText({"Shoot at the right edge of the screen."}));
        headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x()*headerBitmap->scale()),
                             scene.sceneRect().height() * 0.25 - (headerBitmap->boundingRect().center().y()*headerBitmap->scale()));
        break;
    case Cali_Center:
        crosshairItem->setPos(scene.sceneRect().center().x() - (crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                              scene.sceneRect().center().y() - (crosshairItem->boundingRect().center().y()*crosshairItem->scale()));

        headerBitmap->setPixmap(GenerateText({"Shoot at the final target in the center."}));
        headerBitmap->setPos(scene.sceneRect().center().x() - (headerBitmap->boundingRect().center().x()*headerBitmap->scale()),
                             scene.sceneRect().height() * 0.25 - (headerBitmap->boundingRect().center().y()*headerBitmap->scale()));
        break;
    case Cali_Verify:
        ui->graphicsView->setMouseTracking(true);
        mouseCanBeTracked = true;

        headerText->setHtml("<p align=\"center\">Confirm that the bullseye lines up with gun sight.<br>"
                                 "If calibration is acceptable, confirm by pulling the trigger.</p>");
        headerText->setPos(scene.sceneRect().center().x() - headerText->boundingRect().center().x(),
                           (scene.sceneRect().bottom() * 0.25) - headerText->boundingRect().center().y());

        tutorialText->setHtml("<p align=\"center\">If the target accuracy isn't desirable,<br>"
                              "press either <i>Button A</i> or <i>Button B</i> to restart the calibration process.<br>"
                              "You can also exit calibration without saving changes by pressing <i>Button C (if available).</i></p>");
        tutorialText->setPos(scene.sceneRect().center().x() - tutorialText->boundingRect().center().x(),
                             (scene.sceneRect().bottom() * 0.8) - tutorialText->boundingRect().center().y());

        break;
    case Cali_End:
        emit WindowExiting(mode);
        break;
    }
}

void AppCaliWindow::CaliModeTextUpdate(const QString &)
{

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
