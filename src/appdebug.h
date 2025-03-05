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
