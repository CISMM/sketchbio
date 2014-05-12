#ifndef SIGNALEMITTER_H
#define SIGNALEMITTER_H

#include <QObject>

class SignalEmitter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SignalEmitter);
public:
    explicit SignalEmitter(QObject *parent = 0) : QObject(parent) {}

    void emitSignal() { emit somethingHappened(); }

signals:
    void somethingHappened();

};

#endif // SIGNALEMITTER_H
