#ifndef AUDIOTHREAD_H
#define AUDIOTHREAD_H

#include <QThread>

class AudioThread : public QThread{
    Q_OBJECT


public:
    explicit AudioThread(QObject* parent = nullptr); //有参构造、初始化了参数parent
    ~AudioThread();

private:
    void run() override;

signals:
    void timeChanged(unsigned long long ms); //时间改变信号
};

#endif // AUDIOTHREAD_H

