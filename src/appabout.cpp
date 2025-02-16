#include "appabout.h"
#include "ui_appabout.h"

AppAbout::AppAbout(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AppAbout)
{
    ui->setupUi(this);

    this->setFixedSize(this->width(), this->height());
    this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint);

    ui->topText->setText(ui->topText->text() +
#ifdef QT_VERSION_MAJOR
                         QString::number(QT_VERSION_MAJOR) + '.' +
#ifdef QT_VERSION_MINOR
                         QString::number(QT_VERSION_MINOR) + '.' +
#ifdef QT_VERSION_PATCH
                         QString::number(QT_VERSION_PATCH)
#endif
#endif
#endif
                         );
}

AppAbout::~AppAbout()
{
    delete ui;
}
