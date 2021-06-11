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

};

#endif // AUDIOTHREAD_H

