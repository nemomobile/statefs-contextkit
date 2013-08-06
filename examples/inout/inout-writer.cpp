#include <statefs/qt/util.hpp>
#include <QCoreApplication>
#include <QTimer>
#include <QDate>
#include <QVariant>

class Test : public QObject
{
    Q_OBJECT;
public:
    Test(QObject *parent = nullptr)
        : QObject(parent)
        , time_("Time.SecondsSinceEpoch")
        , date_("Date.Current")
        , timer_(new QTimer(this))
        , prev_date_()
    {
    }
    void run()
    {
        timer_->setSingleShot(false);
        connect(timer_, SIGNAL(timeout()), this, SLOT(update()));
        update();
        timer_->start(1000);
    }

private slots:
    void update() {
        int now = ::time(nullptr);
        time_.set(QVariant(now));
        auto current = QDate::currentDate();
        if (current != prev_date_)
            date_.set(current);
    }
private:
    statefs::qt::InOutWriter time_;
    statefs::qt::InOutWriter date_;
    QTimer *timer_;
    QDate prev_date_;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Test *t = new Test(&app);
    t->run();
    app.exec();
    return 0;
}

#include "inout-writer.moc"
