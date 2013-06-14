#include "wrapqt.hpp"
#include <QTimer>
#include <QDebug>
#include <stdexcept>

namespace wrapqt
{

int CoreAppContainer::argc_ = 0;
char* CoreAppContainer::argv_[] = {nullptr};

CoreAppCondNotify::CoreAppCondNotify
(std::unique_lock<std::mutex> &lock, std::condition_variable &cond)
    : lock_(lock), cond_(cond) {}

void CoreAppCondNotify::started()
{
    cond_.notify_all();
    lock_.unlock();
}

CoreAppContainer::CoreAppContainer()
    : thread_([this]() {
            std::unique_lock<std::mutex> lock(mutex_);
            app_ = new CoreAppImpl(argc_, argv_);
            CoreAppCondNotify u(lock, started_);
            QTimer::singleShot(0, &u, SLOT(started()));
            app_->exec();
        })
{
    std::unique_lock<std::mutex> lock(mutex_);
    started_.wait_for(lock,  std::chrono::milliseconds(10000));
}

CoreAppContainer::~CoreAppContainer() {
    if (app_) {
        app_->quit();
    }
    thread_.join();
}

void CoreAppContainer::execute(std::function<void()> const& fn)
{
    CoreAppExecEvent e(fn);
    app_->sendEvent(app_, &e);
}

bool CoreAppImpl::event(QEvent *e)
{
    try {
        switch (static_cast<CoreAppEvent::Type>(e->type())) {
        case (CoreAppEvent::Execute): {
            auto s = static_cast<CoreAppExecEvent*>(e);
            s->execute();
            return true;
        }
        default:
            return QObject::event(e);
        }
    } catch (std::exception const& e) {
        qDebug() << "event: caught std::exception: " << e.what();
    } catch (...) { // Qt does not allow exceptions from event handlers
        qDebug() << "event: caught some exception";
    }
    return false;
}

CoreAppEvent::CoreAppEvent(Type t)
    : QEvent(static_cast<QEvent::Type>(t))
{}

void CoreAppExecEvent::execute() const
{
    fn_();
}

}
