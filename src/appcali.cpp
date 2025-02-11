#include "appcali.h"
#include "ui_appcali.h"

#include <QScreen>

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
    scene.setBackgroundBrush(QBrush(QColor("black")));

    switch(windowMode) {
    case modeCalibrate:
        crosshairItem = new QGraphicsSvgItem(":/img/crosshair");
        // this is supposed to work, but doesn't, so we just offset every setPos command by the center point of the SVG, mult'd by scale
        //svgItem->setTransformOriginPoint(svgItem->boundingRect().center());

        // consider using QGraphicsTextItem here
        scene.addText("Testing fullscreen calibration screen: Tracking On!");
        scene.addItem(crosshairItem);

        // set scale, then set crosshair just offscreen at startup
        crosshairItem->setScale(3);
        crosshairItem->setPos(scene.sceneRect().width()  + (crosshairItem->boundingRect().center().x() * crosshairItem->scale() ),
                              scene.sceneRect().height() + (crosshairItem->boundingRect().center().y() * crosshairItem->scale() ));

        ui->graphicsView->setMouseTracking(false);
        break;
    case modeAlignment:
        // TODO: add boxes/lines to the right parts of the screen like the alignment webapp
        // for squares, left line should be screen width * 0.325, right line should be screen width * 0.665

        ui->graphicsView->setMouseTracking(false);
        break;
    case modeIRTest:
        for(int i = 0; i < testPointsCount; i++) {
            testPoints[i] = new QGraphicsEllipseItem();
            scene.addItem(testPoints[i]);
        }

        testBox = new QGraphicsPolygonItem();
        scene.addItem(testBox);

        testPoints[testPointTL]->setPen(    QPen(QBrush(Qt::green), 3));
        testPoints[testPointTR]->setPen(    QPen(QBrush(Qt::green), 3));
        testPoints[testPointBL]->setPen(    QPen(QBrush(Qt::blue ), 3));
        testPoints[testPointBR]->setPen(    QPen(QBrush(Qt::blue ), 3));
        testPoints[testPointMed]->setPen(   QPen(QBrush(Qt::gray ), 3));
        testPoints[testPointD]->setPen(     QPen(QBrush(Qt::red  ), 3));

        ui->graphicsView->setMouseTracking(false);
        break;
    }
}

AppCaliWindow::~AppCaliWindow()
{
    delete ui;
}

void AppCaliWindow::CaliModeSet(const int &caliStage)
{
    switch(caliStage) {
    case Cali_Init:
        ui->graphicsView->setMouseTracking(false);
        break;
    case Cali_Top:
        break;
    case Cali_Bottom:
        break;
    case Cali_Left:
        break;
    case Cali_Right:
        break;
    case Cali_Center:
        break;
    case Cali_Verify:
        ui->graphicsView->setMouseTracking(true);
        break;
    case Cali_End:
        emit WindowExiting(mode);
        close();
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
