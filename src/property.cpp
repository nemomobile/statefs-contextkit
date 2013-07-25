#include "property.hpp"
#include "util.hpp"

#include <contextproperty.h>
#include <QDebug>
#include <QTimer>
#include <QSocketNotifier>
#include <QMutex>

namespace ckit
{

static QMutex actorGuard;
static ckit::Actor<ckit::PropertyMonitor> *propertyMonitor = nullptr;
static bool isActorCreated = false;

Event::Event(Event::Type t)
    : QEvent(static_cast<QEvent::Type>(t))
{}

Event::~Event() {}
PropertyRequest::~PropertyRequest() {}

PropertyRequest::PropertyRequest(Event::Type t
                                 , ContextPropertyPrivate const *tgt
                                 , QString const &key)
    : Event(t)
    , tgt_(tgt)
    , key_(key)
{}

bool PropertyMonitor::event(QEvent *e)
{
    if (e->type() < QEvent::User)
        return QObject::event(e);

    auto t = static_cast<Event::Type>(e->type());
    switch (t) {
    case Event::Subscribe: {
        auto p = static_cast<PropertyRequest*>(e);
        subscribe(p->tgt_, p->key_);
        break;
    }
    case Event::Unsubscribe: {
        auto p = static_cast<PropertyRequest*>(e);
        unsubscribe(p->tgt_, p->key_);
        break;
    }
    default:
        return QObject::event(e);
    }
    return true;
}

void PropertyMonitor::subscribe(ContextPropertyPrivate const *tgt, const QString &key)
{
    CKitProperty *handler;
    auto it = targets_.find(key);
    if (it == targets_.end()) {
        targets_.insert(key, QSet<ContextPropertyPrivate const*>({tgt}));
        handler = add(key);
    } else {
        if (it->contains(tgt)) {
            return;
        }
        it->insert(tgt);
        handler = properties_[key];
    }
    connect(handler, &CKitProperty::changed, tgt, &ContextPropertyPrivate::changed);
    handler->subscribe();
}

void PropertyMonitor::unsubscribe
(ContextPropertyPrivate const *tgt, const QString &key)
{
    auto t_it = targets_.find(key);
    if (t_it == targets_.end())
        return;

    auto tgt_set = t_it.value();
    auto ptgt = tgt_set.find(tgt);
    if (ptgt == tgt_set.end())
        return;
    
    auto h_it = properties_.find(key);
    if (h_it == properties_.end())
        return;

    auto handler = h_it.value();

    disconnect(handler, &CKitProperty::changed
               , tgt, &ContextPropertyPrivate::changed);
    tgt_set.erase(ptgt);
    if (!tgt_set.isEmpty())
        return;

    targets_.erase(t_it);
    handler->deleteLater();
    properties_.erase(h_it);
}

CKitProperty* PropertyMonitor::add(const QString &key)
{
    auto it = properties_.insert(key, new CKitProperty(key, this));
    return it.value();
}

CKitProperty::CKitProperty(const QString &key, QObject *parent)
    : QObject(parent)
    , key_(key)
    , file_(getStateFsPath(key))
    , notifier_(nullptr)
    , reopen_interval_(100)
    , reopen_timer_(new QTimer(this))
    , is_subscribed_(false)
    , is_cached_(false)
{
    reopen_timer_->setSingleShot(true);
    connect(reopen_timer_, SIGNAL(timeout()), this, SLOT(trySubscribe()));
}

CKitProperty::~CKitProperty()
{
    unsubscribe();
}

void CKitProperty::trySubscribe() const
{
    if (tryOpen()) {
        reopen_interval_ = 100;
        return subscribe();
    }

    reopen_interval_ *= 2;
    if (reopen_interval_ > 1000 * 60 * 3)
        reopen_interval_ = 1000 * 60 * 3;

    reopen_timer_->start(reopen_interval_);
}

void CKitProperty::resubscribe() const
{
    bool was_subscribed = is_subscribed_;
    unsubscribe();

    if (was_subscribed)
        subscribe();
}

QVariant CKitProperty::value() const
{
    QVariant res;
    static const size_t cap = 8;

    if (is_cached_)
        return cache_;

    if (!tryOpen()) {
        qWarning() << "Can't open " << file_.fileName();
        return res;
    }

    // WORKAROUND: file is just opened and closed before reading from
    // real source to make vfs (?) reread file data to cache
    QFile touchFile(file_.fileName());
    touchFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    file_.seek(0);
    auto size = file_.size();
    if (buffer_.size() < size)
        buffer_.resize(size + cap);

    int rc = file_.read(buffer_.data(), size + cap - 1);
    touchFile.close();
    if (rc >= 0) {
        buffer_[rc] = '\0';
        auto s = QString(buffer_);
        if (s.size()) // use read data if not empty
            res = cKitValueDecode(s);

        cache_ = res;
        is_cached_ = true;

        if (notifier_)
            notifier_->setEnabled(true);
    } else {
        qWarning() << "Error accessing? " << rc << "..." << file_.fileName();
        resubscribe();
    }
    return res;
}

void CKitProperty::handleActivated(int)
{
    if (notifier_)
        notifier_->setEnabled(false);
    is_cached_ = false;
    emit changed(value());
}

bool CKitProperty::tryOpen() const
{
    if (file_.isOpen())
        return true;

    if (!file_.exists()) {
        qWarning() << "No property file " << file_.fileName();
        return false;
    }
    file_.open(QIODevice::ReadOnly);// | QIODevice::Unbuffered);
    if (!file_.isOpen()) {
        qWarning() << "Can't open " << file_.fileName();
        return false;
    }
    is_cached_ = false;
    return true;
}

void CKitProperty::subscribe() const
{
    if (is_subscribed_)
        return;

    is_subscribed_ = true;
    if (!tryOpen())
        return reopen_timer_->start(reopen_interval_);

    notifier_.reset(new QSocketNotifier(file_.handle(), QSocketNotifier::Read));
    connect(notifier_.data(), SIGNAL(activated(int))
            , this, SLOT(handleActivated(int)));
    notifier_->setEnabled(true);
}

void CKitProperty::unsubscribe() const
{
    if (!is_subscribed_)
        return;

    is_subscribed_ = false;
    if (!file_.isOpen())
        return;

    if (notifier_) {
        notifier_->setEnabled(false);
        notifier_.reset();
    }
    file_.close();
}

}


ContextPropertyPrivate::ContextPropertyPrivate(const QString &key, QObject *parent)
    : QObject(parent)
    , key_(key)
    , is_subscribed_(false)
    , is_cached_(false)
{
}

ContextPropertyPrivate::~ContextPropertyPrivate()
{
    unsubscribe();
}

QString ContextPropertyPrivate::key() const
{
    return key_;
}

QVariant ContextPropertyPrivate::value(const QVariant &defVal) const
{
    return is_cached_ ? cache_ : defVal;
}

QVariant ContextPropertyPrivate::value() const
{
    return value(QVariant());
}

ckit::Actor<ckit::PropertyMonitor> * ContextPropertyPrivate::actor()
{
    using namespace ckit;
    if (isActorCreated)
        return propertyMonitor;
    QMutexLocker lock(&ckit::actorGuard);
    if (isActorCreated)
        return propertyMonitor;
    propertyMonitor = new ckit::Actor<ckit::PropertyMonitor>([=]() {
            return new ckit::PropertyMonitor();
        });
    isActorCreated = true;
    return propertyMonitor;
}
 
void ContextPropertyPrivate::changed(QVariant v)
{
    if (v.isNull() || (is_cached_ && v == cache_))
        return;
    is_cached_ = true;
    cache_ = v;
    emit valueChanged();
}


const ContextPropertyInfo* ContextPropertyPrivate::info() const
{
    return nullptr; // TODO
}

void ContextPropertyPrivate::subscribe() const
{
    if (is_subscribed_)
        return;

    actor()->postEvent(new ckit::PropertyRequest(ckit::Event::Subscribe, this, key_));
    QCoreApplication::processEvents();
    is_subscribed_ = true;
}

void ContextPropertyPrivate::unsubscribe() const
{
    if (!is_subscribed_)
        return;

    actor()->postEvent(new ckit::PropertyRequest(ckit::Event::Unsubscribe, this, key_));
    is_subscribed_ = false;
}

void ContextPropertyPrivate::waitForSubscription() const
{
}

void ContextPropertyPrivate::waitForSubscription(bool block) const
{
}

void ContextPropertyPrivate::ignoreCommander()
{
}

void ContextPropertyPrivate::setTypeCheck(bool typeCheck)
{
}


ContextProperty::ContextProperty(const QString &key, QObject *parent)
    : QObject(parent)
    , priv(new ContextPropertyPrivate(key, this))
{
    connect(priv, SIGNAL(valueChanged()), this, SIGNAL(valueChanged()));
    priv->subscribe();
}

ContextProperty::~ContextProperty()
{
    disconnect(priv, SIGNAL(valueChanged()), this, SIGNAL(valueChanged()));
}

QString ContextProperty::key() const
{
    return priv->key();
}

QVariant ContextProperty::value(const QVariant &def) const
{
    return priv->value(def);
}

QVariant ContextProperty::value() const
{
    return priv->value();
}

const ContextPropertyInfo* ContextProperty::info() const
{
    return priv->info();
}

void ContextProperty::subscribe () const
{
    return priv->subscribe();
}

void ContextProperty::unsubscribe () const
{
    return priv->unsubscribe();
}

void ContextProperty::waitForSubscription() const
{
    return priv->waitForSubscription();
}

void ContextProperty::waitForSubscription(bool block) const
{
    return priv->waitForSubscription(block);
}

void ContextProperty::ignoreCommander()
{
}

void ContextProperty::setTypeCheck(bool typeCheck)
{
}

#if QT_VERSION < 0x050000
void ContextProperty::onValueChanged() { }
#endif
