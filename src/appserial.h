#ifndef APPSERIAL_H
#define APPSERIAL_H

#include <QObject>

class AppSerialDevice : public QObject
{
    Q_OBJECT
public:
    explicit AppSerialDevice(QObject *parent = nullptr);



private:


signals:

};

#endif // APPSERIAL_H
