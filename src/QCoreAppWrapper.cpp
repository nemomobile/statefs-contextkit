#include "QCoreAppWrapper.hpp"

int QCoreAppWrapper::argc_ = 0;
char* QCoreAppWrapper::argv_[] = {nullptr};
QCoreApplication *QCoreAppWrapper::app_ = nullptr;
QMutex QCoreAppWrapper::mutex_;
QWaitCondition QCoreAppWrapper::start_cond_;


QCoreAppWrapper::QCoreAppWrapper()
{
    mutex_.lock();
    start();
    if (!QCoreApplication::instance())
        start_cond_.wait(&mutex_, 5000);
    mutex_.unlock();
}

void QCoreAppWrapper::run()
{
    mutex_.lock();
    bool is_create = !QCoreApplication::instance();
    if (is_create)
        app_ = new QCoreApplication(argc_, argv_);

    start_cond_.wakeAll();
    mutex_.unlock();
    if (is_create)
        app_->exec();
}

QCoreAppWrapper::~QCoreAppWrapper()
{
    QCoreApplication::quit();
    wait(30000);
}
