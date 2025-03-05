#ifndef APPABOUT_H
#define APPABOUT_H

#include <QDialog>

namespace Ui {
class AppAbout;
}

class AppAbout : public QDialog
{
    Q_OBJECT

public:
    explicit AppAbout(QWidget *parent = nullptr);
    ~AppAbout();

private:
    Ui::AppAbout *ui;
};

#endif // APPABOUT_H
