#include "appdebug.h"
#include "ui_appdebug.h"

AppDebugWindow::AppDebugWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AppDebugWindow)
{
    ui->setupUi(this);
}

AppDebugWindow::~AppDebugWindow()
{
    delete ui;
}

void AppDebugWindow::AppendText(const QByteArray &text)
{
    ui->text->appendPlainText(text);
}
