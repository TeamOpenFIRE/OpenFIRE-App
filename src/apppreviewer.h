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
    QVector<QWidget*> padding;
};

#endif // APPPREVIEWER_H
