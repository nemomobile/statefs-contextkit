#ifndef _QCOREAPWRAPPER_HPP_
#define _QCOREAPWRAPPER_HPP_

#include <QObject>
#include <QCoreApplication>

#include <chrono>
#include <thread>

class CoreAppImpl : public QCoreApplication
{
    Q_OBJECT;
public:
    CoreAppImpl(int &argc, char *argv[])
        : QCoreApplication(argc, argv)
    {}

    virtual bool event(QEvent*);
};

class CoreAppCondNotify : public QObject
{
    Q_OBJECT;
public:
    CoreAppCondNotify(std::unique_lock<std::mutex> &lock
                            , std::condition_variable &cond);
private slots:
    void started();
private:
    std::unique_lock<std::mutex> &lock_;
    std::condition_variable &cond_;
};

class CoreAppEvent : public QEvent
{
public:
    enum Type {
        Execute = QEvent::User
    };

    virtual ~CoreAppEvent() {}

protected:
    CoreAppEvent(Type t);
private:
    CoreAppEvent();
};

class CoreAppExecEvent : public CoreAppEvent
{
public:
    CoreAppExecEvent(std::function<void()> const& fn)
        : CoreAppEvent(CoreAppEvent::Execute)
        , fn_(fn)
    {
    }
    void execute() const;
private:
    std::function<void()> fn_;
};

class QCoreAppWrapper
{
public:
    QCoreAppWrapper();
    virtual ~QCoreAppWrapper();

    void execute(std::function<void()> const&);
private:
    static int argc_;
    static char* argv_[];

    std::mutex mutex_;
    std::condition_variable started_;
    std::thread thread_;
    CoreAppImpl *app_;
};
 
#endif // _QCOREAPWRAPPER_HPP_
