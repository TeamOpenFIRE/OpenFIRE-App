#ifndef APPCALI_H
#define APPCALI_H

#include <QWidget>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSvgItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class AppCaliWindow;
}
QT_END_NAMESPACE

class AppCaliGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

signals:
    void sceneMouseEvent(const QPointF &);
    void sceneKeyCloseSignal();

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override {
        emit sceneMouseEvent(mouseEvent->scenePos());
    }

    void keyPressEvent(QKeyEvent *keyEvent) override {
        if(keyEvent->key() == Qt::Key_Escape)
            emit sceneKeyCloseSignal();
    }

};

class AppCaliWindow : public QWidget
{
    Q_OBJECT

public:
    enum {
        modeCalibrate = 0,
        modeAlignment,
        modeIRTest
    } AppCaliStates_e;

    enum {
        testPointTL = 0,
        testPointTR,
        testPointBL,
        testPointBR,
        testPointMed,
        testPointD,
        testPointsCount
    } TestPointsPos_e;

    enum {
        Cali_Init = 0,
        Cali_Top,
        Cali_Bottom,
        Cali_Left,
        Cali_Right,
        Cali_Center,
        Cali_Verify,
        Cali_End
    } CaliSteps_e;

    explicit AppCaliWindow(QWidget *parent = nullptr, const int & = modeCalibrate);
    ~AppCaliWindow();

    /// @brief      Update calibration window with new status
    /// @details    Stuff
    void CaliModeSet(const int &);

    /// @brief      Update calibration window's profile text with new data
    void CaliModeTextUpdate(const QString &);

    /// @brief      Update test mode window with list of new coords
    void TestModeDraw(const QStringList &);

    int GetWindowMode() { return mode; }

    /// @brief      Objects to hold new cali profile values
    int topOffset = -1;
    int bottomOffset = -1;
    int leftOffset = -1;
    int rightOffset = -1;
    float topLeftLed = -1;
    float topRightLed = -1;

    void Shutdown() { emit WindowExiting(mode); }

signals:
    /// @brief      Signals back to the main app that the window is exiting
    /// @details    This should be emitted in any situation that the window may exit
    /// @param      mode
    ///             Window mode this object was launched in; must be one of ApCaliStates_e
    /// @param      ints & floats
    ///             Profile offsets for Calibration Mode
    void WindowExiting(const int &mode, const int & = -1, const int & = -1, const int & = -1, const int & = -1, const float & = -1, const float & = -1);

private:
    enum {
        ScaleSmall = 0,
        ScaleBig,
        ScaleHiDPI
    } ScaleTypes_e;

    enum {
        TextHeading = 0,
        TextSub,
        TextSmall,
        Crosshair
    } TextScaleTypes_e;

    const QStringList caliTypesPrefixes = {
        "   Top Offset: ",
        "Bottom Offset: ",
        "  Left Offset: ",
        " Right Offset: ",
        " Top Left LED: ",
        "Top Right LED: "
    };

    Ui::AppCaliWindow *ui;

    /// @brief      Very basic approximated HiDPI-aware values for both bitmap and font texts
    /// @returns    An appropriate scaling value, depending on value passed through of TextScaleTypes_e
    ///             and depending on whether bitmapText is enabled or not.
    int GetTextScale(const int &);

    /// @brief      Generates input from text into a bitmap representation using the app's builtin "test" typeface
    /// @details    Optional color bit tints the pixmap into the provided color.
    /// @returns    Pixmap of the string list provided.
    QPixmap GenerateText(const QStringList &, const QColor & = QColor());

    /// @brief      Type of text scale appropriate for this window
    /// @details    Value is one of ScaleTypes_e
    int scale = -1;

    /// @brief      What mode this window was opened as
    /// @returns    A value of AppCaliStates_e
    int mode = -1;

    /// @brief      Whether to use bitmaps or font-based text
    /// @details    In case normal fonts are ever desirable.
    bool bitmapText = true;

    /// @brief      Indicator that mouse tracking is allowed
    bool mouseCanBeTracked = false;

    /// @brief      The cali screen renderer itself
    /// @details    Subclass was needed to implement specific mouse capture and key event listening
    AppCaliGraphicsScene scene;

    /// @brief      The crosshair used in calibration UX
    QGraphicsSvgItem *crosshairItem;

    /// @brief      Objects used to construct the look of the alignment view
    /// @details    Two sets of differently colored boxes for Square and Diamond IR layouts,
    ///             And two polygons that connects these together.
    QGraphicsRectItem *alignmentBoxesSquare[4];
    QGraphicsRectItem *alignmentBoxesDiamond[4];
    QGraphicsPolygonItem *alignmentLines[2];

    /// @brief      Test Mode screen points
    QGraphicsEllipseItem *testPoints[testPointsCount];

    /// @brief      Test Mode polygon that connects across the test points
    QGraphicsPolygonItem *testBox;

    /// @brief      Text bitmaps generated from test font files
    /// @details    Contents are generated at runtime from GenerateText

    /// @brief      Main top text bitmap, used for primary info strings
    /// @details    For cali, this displays tutorial text to the user
    QGraphicsPixmapItem *headerBitmap;

    /// @brief      Indicator of cali stage that goes above headerText in Calibration Mode
    QGraphicsPixmapItem *caliStageBitmap;

    /// @brief      Text bitmaps used for showing current profile data during calibration
    /// @details    In order: Top Offset, Bottom Offset, Left Offset, Right Offset, Top Left LED, Top Right LED
    QGraphicsPixmapItem *profileBitmaps[6];

    /// @brief      Bottom text bitmap used for showing how to exit calibration (or other button related instructions) to the user
    QGraphicsPixmapItem *tutorialBitmap;

    /// @brief      Side text used to present Alignment Mode information about the different IR emitter layouts
    QGraphicsPixmapItem *alignmentBitmapLeft;
    QGraphicsPixmapItem *alignmentBitmapRight;

    /// @brief      Side colored text used to emphasize different layout types in Alignment Mode
    QGraphicsPixmapItem *alignmentBitmapColoredLeft;
    QGraphicsPixmapItem *alignmentBitmapColoredRight;

    // Below is older native font-based implementations of text objects
    // Could be useful in future for non-English languages?
    /// @brief      Main top text item, used for primary info strings
    /// @details    For cali, this displays tutorial text to the user
    QGraphicsTextItem *headerText;

    /// @brief      Indicator of cali stage that goes above headerText in Calibration Mode
    QGraphicsTextItem *caliStageText;

    /// @brief      Text used for showing current profile data during calibration
    /// @details    In order: Top Offset, Bottom Offset, Left Offset, Right Offset, Top Left LED, Top Right LED
    QGraphicsTextItem *profileText[6];

    /// @brief      Bottom text used for showing how to exit calibration (or other button related instructions) to the user
    QGraphicsTextItem *tutorialText;

    /// @brief      Side text used to present Alignment Mode information about the different IR emitter layouts
    QGraphicsTextItem *alignmentTextLeft;
    QGraphicsTextItem *alignmentTextRight;

private slots:

    /// @brief      Takes mouse coords from QGraphicsScene mouse move event and update crosshair position
    /// @details    Crosshair image is offset (adjusted by crosshair scale) to center it to the mouse's hotspot,
    ///             as otherwise it wouldn't be accurate at all.
    void sceneEventReceiver(const QPointF &currentPos) {
        if(mouseCanBeTracked) crosshairItem->setPos(
                                    currentPos.x()-(crosshairItem->boundingRect().center().x()*crosshairItem->scale()),
                                    currentPos.y()-(crosshairItem->boundingRect().center().y()*crosshairItem->scale()));
    }

    /// @brief      Key listener, which only listens for ESC keypresses and exits when pressed in not Cali mode
    void sceneKeyCloseReceiver() {
        if(mode != modeCalibrate) {
            emit WindowExiting(mode);
        }
    }

};

#endif // APPCALI_H
