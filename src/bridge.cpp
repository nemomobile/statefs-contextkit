#include "util.hpp"
#include "bridge.hpp"

#include <cor/mt.hpp>

#include "provider.hpp"
#include <statefs/util.h>

#include <cor/error.hpp>
#include <cor/util.hpp>

#include <QProcess>
#include <QFile>
#include <QDomDocument>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QLibrary>
#include <QSharedPointer>
#include <QCoreApplication>
#include <QSocketNotifier>

#include <memory>
#include <map>
#include <set>
#include <stdexcept>
#include <chrono>
#include <thread>

#include <fcntl.h>



// --------------------------------------------------


class PropInfo;
typedef std::shared_ptr<PropInfo> property_ptr;
typedef std::map<QString, std::list<property_ptr> > plugin_properties_type;

typedef std::map<QString, property_ptr> prop_info_type;
typedef std::map<QString, prop_info_type> info_tree_type;
typedef std::shared_ptr<QLibrary> plugin_ptr;

class NamespaceNode;

static std::vector<std::unique_ptr<NamespaceNode> > namespaces;
static std::unique_ptr<QtBridge> qt_app;

// TODO hard-coded path
static const char *contextkit_config_dir = "/usr/share/contextkit/providers/";
static const char *contextkit_plugins_dir = "/usr/lib/contextkit/subscriber-plugins/";

class ProviderFactory
{
public:
    ProviderFactory(QString const &id) : id_(id) {}
    virtual provider_ptr get() =0;
    QString get_id() const { return id_; }
private:
    QString id_;
};

class SharedObjFactory : public ProviderFactory
{
public:
    SharedObjFactory(QString const &baseName
                     , QString const &plugin
                     , QString const &constructionString)
        : ProviderFactory(baseName)
        , plugin_name_(plugin)
        , constructionString_(constructionString)
    {}

    virtual ~SharedObjFactory() {}
    virtual provider_ptr get()
    {
        if (provider_)
            return provider_;

        plugin_ = plugin_get(plugin_name_);

        auto factory = reinterpret_cast<plugin_factory_type>
            (plugin_->resolve("pluginFactory"));
        if (!factory) {
            auto msg = QString("No pluginFactory in ") + plugin_name_;
            throw cor::Error(msg.toStdString());
        }

        provider_.reset(factory(constructionString_));
        return provider_;
    }

private:

    static plugin_ptr plugin_get(QString const &name);

    QString plugin_name_;
    QString constructionString_;
    plugin_ptr plugin_;
    provider_ptr provider_;
};


plugin_ptr SharedObjFactory::plugin_get(QString const &name)
{
    QString path(contextkit_plugins_dir);
    path += (name + ".so");
    plugin_ptr lib(new QLibrary(path));
    lib->load();
    if (!lib->isLoaded())
        throw cor::Error((QString("Can't find ") + path).toStdString());

    return lib;
}

class DBusFactory : public ProviderFactory
{
public:
    DBusFactory(QString const &baseName
                , QString const &bus
                , QString const &service)
        : ProviderFactory(baseName)
        , bus_(bus)
        , service_(service)
    {}

    virtual provider_ptr get()
    {
        if (!provider_)
            initialize();

        return provider_;
    }
private:

    void initialize()
    {
        auto sym = ::dlsym(RTLD_DEFAULT, "contextKitPluginFactory");
        auto factory = reinterpret_cast<plugin_factory_type>(sym);

        QStringList parts({bus_, service_});
        provider_.reset(factory
                        ? factory(parts.join(":"))
                        : nullptr);
    }

    QString bus_;
    QString service_;
    provider_ptr provider_;
};

class PropInfo
{
public:
    static property_ptr create(QString const&, provider_factory_ptr);

private:
    PropInfo(QString const &full_name
             , QString const &ns
             , QString const &name
             , provider_factory_ptr factory)
        : full_name_(full_name)
        , ns_(ns)
        , name_(name)
        , factory_(factory)
    {}

public:
    QString full_name_;
    QString ns_;
    QString name_;
    provider_factory_ptr factory_;
};

QDebug & operator << (QDebug &dst, PropInfo const &src)
{
    dst << src.ns_ << "." << src.name_
        << " (" << src.full_name_ << ")"
        << "->" << src.factory_->get_id();
    return dst;
}

property_ptr PropInfo::create
(QString const& name, provider_factory_ptr factory)
{
    QStringList parts;
    if (!getPropertyInfo(name, parts) || parts.size() != 2)
        throw cor::Error((name + " is not correct?").toStdString());
    return property_ptr(new PropInfo(name, parts[0], parts[1], factory));
}

/**
 * reads information from contextkit plugin description file (*.context)
 *
 * @param fileInfo
 * @param dst destination properties container
 *
 * @return
 */
static bool read_plugin_info(QFileInfo const &fileInfo, plugin_properties_type &dst)
{
    QString baseName(fileInfo.baseName());

    QString fileName(fileInfo.canonicalFilePath());
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)){
        std::cerr << "No file";
        return false;
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        qDebug() << "No content";
        return false;
    }
    file.close();

    auto docElem = doc.documentElement();
    if (docElem.tagName() != "provider") {
        qDebug() << "Invalid context xml, root tag is " << docElem.tagName()
                 << " expected 'provider'";
        return false;
    }

    auto plugin = docElem.attribute("plugin");
    auto constructionString = docElem.attribute("constructionString");
    auto &info = dst[baseName];
    provider_factory_ptr factory;

    // if has "plugin" attr it is just a standard plugin, otherwise -
    // dbus service
    if (plugin.size()) {
        plugin.replace(QRegExp("^/"), "");
        factory.reset
            (new SharedObjFactory(baseName, plugin, constructionString));
    } else {
        auto bus = docElem.attribute("bus");
        auto service = docElem.attribute("service");

        if (!(bus.size() && service.size())) {
            qDebug() << "Unknown plugin description, skipping";
            return false;
        }
        factory.reset(new DBusFactory(baseName, bus, service));

    }
    auto nodes = docElem.elementsByTagName("key");

    for(int i = 0; i < nodes.count(); ++i) {
        auto e = nodes.at(i).toElement();
        auto name = e.attribute("name");
        info.push_back(PropInfo::create(name, factory));
    }
    return true;
}

/**
 * builds tree describing contextkit providers and their properties
 *
 * @param infoTree
 */
static void build_tree(info_tree_type &infoTree)
{
    // NB! path is hardcoded
    QString info_path("/usr/share/contextkit/providers");
    plugin_properties_type plugins_info;
    QStringList filters({"*.context"});
    QDir info_dir(info_path);
    for (auto const &f: info_dir.entryInfoList(filters)) {
        read_plugin_info(f, plugins_info);
    }

    for(auto const& plugin_props: plugins_info) {
        auto name = plugin_props.first;
        for (auto prop: plugin_props.second) {
            auto &ns = infoTree[prop->ns_];
            ns[prop->name_] = prop;
        }
    }
}

class CKitProperty : public statefs::Property
{
public:
    CKitProperty(property_ptr p)
        : statefs::Property(p->name_.toUtf8().constData())
        , info_(p)
        , slot_(nullptr)
        , conn_count_(0)
        , is_first_access_(true)
    {}

    ~CKitProperty() {}

    std::string get_name() const
    {
        return info_->name_.toStdString();
    }

    void notify(QVariant const &v)
    {
        v_ = cKitValueEncode(v);
        if (is_first_access_) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (is_first_access_)
                initialized_.notify_all();

            is_first_access_ = false;
        }

        if (slot_)
            slot_->on_changed(slot_, &prop_);
    }

    void connect(statefs_slot *slot)
    {
        if (!bridge_) {
            bridge_ = qt_app->bridge_get(info_->factory_);
        }
        if (!conn_count_)
            bridge_->subscribe(info_->full_name_, this);

        if (slot)
            slot_ = slot;
        ++conn_count_;
    }

    void disconnect()
    {
        if (--conn_count_ == 0) {
            if (bridge_)
                bridge_->unsubscribe(info_->full_name_);
            slot_ = nullptr;
        }
    }

    QString const& value() const
    {
        // if value is read w/o preceeding polling there is no way to
        // read it until provider notify about it, so waiting for
        // notification supplying data a bit but only on first access,
        // cached value will be used on consequent access (this is not
        // correct because cached value can be outdated but it should
        // descrease access time) TODO but this logic should be
        // changed if statefs contextkit provider will be alive for
        // too long time :)
        if (is_first_access_) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (is_first_access_) {
                initialized_.wait_for(lock,  std::chrono::milliseconds(500)
                                      , [this]() { return is_first_access_; });
                is_first_access_ = false;
            }
        }
        return v_;
    }

    statefs_size_t size() const
    {
        return is_first_access_ ? 1024 : std::max(256, v_.size());
    }

    static CKitProperty *self_cast(statefs_property*);
    static CKitProperty const* self_cast(statefs_property const*);

private:
    property_ptr info_;
    QString v_;
    bridge_ptr bridge_;
    statefs_slot *slot_;
    int conn_count_;

    //@{ qt event loop is working in another thread, so need thread
    // sync
    mutable std::mutex mutex_;
    mutable std::condition_variable initialized_;
    mutable bool is_first_access_;
    //@}
};

CKitProperty *CKitProperty::self_cast(statefs_property *from)
{
    return static_cast<CKitProperty*>(statefs::Property::self_cast(from));
}

CKitProperty const* CKitProperty::self_cast(statefs_property const* from)
{
    return static_cast<CKitProperty const*>(statefs::Property::self_cast(from));
}

class PropHandle
{
public:
    PropHandle(CKitProperty *prop)
        : prop_(prop)
    {
        prop_->connect(nullptr);
    }

    ~PropHandle()
    {
        prop_->disconnect();
    }

    int read(char *dst, statefs_size_t len, statefs_off_t off)
    {
        if (!len)
            return 0;

        if (!off)
            cache_ = prop_->value();

        auto clen = cache_.size();

        if (off > clen)
            return 0;

        if (off + len > clen)
            len = clen - off;
        memcpy(dst, cache_.toUtf8().constData() + off, len);
        return len;
    }

private:
    CKitProperty *prop_;
    QString cache_;
};

class NamespaceNode : public statefs::Namespace
{
public:
    NamespaceNode(QString const &aname)
        : statefs::Namespace(aname.toUtf8().constData())
    {}

    virtual ~NamespaceNode() {}

    void insert(CKitProperty *p) {
        Namespace::insert(p->get_name(), p);
    }
};

ProviderBridge::ProviderBridge(provider_factory_ptr factory, ProviderThread *parent)
    : factory_(factory)
{}

ProviderBridge::~ProviderBridge()
{
    if (provider_) {
        disconnect(provider_.get(), SIGNAL(subscribeFinished(QString, TimedValue))
                , this, SLOT(onSubscribed(QString, TimedValue)));
        disconnect(provider_.get(), SIGNAL(subscribeFinished(QString))
                   , this, SLOT(onSubscribed(QString)));
        disconnect(provider_.get(), SIGNAL(valueChanged(QString, QVariant))
                   , this, SLOT(onValue(QString, QVariant)));
        disconnect(provider_.get(), SIGNAL(subscribeFailed(QString, QString))
                , this, SLOT(onSubscribeFailed(QString, QString)));
    }
    for (auto &v: subscribers_) {
        QSet<QString> nset;
        nset.insert(v.first);
        provider()->unsubscribe(nset);
    }
}

void ProviderBridge::subscribe(QString name, CKitProperty *dst)
{
    subscribers_[name] = dst;
    QSet<QString> nset;
    nset.insert(name);

    provider()->subscribe(nset);
    auto p = cache_.find(name);
    if (p != cache_.end()) {
        // provider is already supplied it, notify subscriber
        dst->notify(p->second);
    }
}

void ProviderBridge::unsubscribe(QString name)
{
    subscribers_.erase(name);

    QSet<QString> nset;
    nset.insert(name);
    provider()->unsubscribe(nset);
}

void ProviderBridge::onValue(QString key, QVariant value)
{
    auto p = subscribers_[key];
    if (p) {
        p->notify(value);
    } else {
        // not subscribed yet but provider already sent first value,
        // so remember it to instantly provide to subscriber later
        cache_[key] = value;
    }
}

void ProviderBridge::onSubscribed(QString key)
{
    qDebug() << "Subscribed for " << key << " but no value provided";
}

void ProviderBridge::onSubscribed(QString key, TimedValue value)
{
    onValue(key, value.value);
}

void ProviderBridge::onSubscribeFailed(QString failedKey, QString error)
{
    qDebug() << "Can't subscribe for " << failedKey
             << ": " << error << ". Providing empty value";
    onValue(failedKey, QVariant());
}

provider_ptr ProviderBridge::provider()
{
    if (!provider_) {
        provider_ = factory_->get();
        connect(provider_.get(), SIGNAL(valueChanged(QString, QVariant))
                , this, SLOT(onValue(QString, QVariant)));
        connect(provider_.get(), SIGNAL(subscribeFinished(QString))
                , this, SLOT(onSubscribed(QString)));
        connect(provider_.get(), SIGNAL(subscribeFailed(QString, QString))
                , this, SLOT(onSubscribeFailed(QString, QString)));
        connect(provider_.get(), SIGNAL(subscribeFinished(QString, TimedValue))
                , this, SLOT(onSubscribed(QString, TimedValue)));
    }
    return provider_;
}

QtBridge::QtBridge()
    : watch_(ih_, contextkit_config_dir, IN_CREATE | IN_DELETE | IN_MODIFY)
    , notify_(new QSocketNotifier(ih_.fd(), QSocketNotifier::Read, this))
{
    connect(notify_, SIGNAL(activated(int)), SLOT(on_config_changed()));
    notify_->setEnabled(true);
}

void QtBridge::on_config_changed()
{
    qDebug() << "Contextkit configuration is changed, reregistering";
    notify_->setEnabled(false);
    char buf[sizeof(inotify_event) + PATH_MAX + 1];
    ih_.read(buf, sizeof(buf));
    notify_->setEnabled(true);
    QProcess::startDetached("statefs-contextkit-register");
}

bridge_ptr QtBridge::bridge_get(provider_factory_ptr factory)
{
    auto name = factory->get_id();
    auto iter = bridges.find(name);
    if (iter != bridges.end())
        return iter->second;

    bridge_ptr p(new ProviderThread(factory));
    bridges.insert(std::make_pair(name, p));
    return p;
}

ProviderThread::ProviderThread(provider_factory_ptr factory)
    : factory_(factory)
{
    mutex_.lock();
    start();
    cond_.wait(&mutex_, 1000);
    mutex_.unlock();
}

ProviderThread::~ProviderThread()
{
    exit(0);
    wait(5000);
}

void ProviderThread::run()
{
    bridge_.reset(new ProviderBridge(factory_, this));
    mutex_.lock();
    cond_.wakeAll();
    mutex_.unlock();

    exec();
}

void ProviderThread::subscribe(QString const &name, CKitProperty *dst)
{
    auto fn = std::bind(std::mem_fn(&ProviderBridge::subscribe)
                        , bridge_.get(), name, dst);
    // do not wait until subscribed, data changing notifications are
    // coming async and can be supplied directly from subscription
    // code causing deadlock
    QCoreApplication::postEvent(bridge_.get(), new cor::qt::EventExecute(fn));
}

void ProviderThread::unsubscribe(QString const &name)
{
    cor::Future f;
    auto fn = std::bind(std::mem_fn(&ProviderBridge::unsubscribe)
                        , bridge_.get(), name);
    QCoreApplication::postEvent
        (bridge_.get(), new cor::qt::EventExecute(f.wrap(fn)));
    // wait until unsubcribed to avoid infinite asynchronous
    // subscribe/unsubscribe cycles lagging behind access to file
    if (f.wait(std::chrono::milliseconds(1000)) == std::cv_status::timeout)
        qDebug() << "Timeout unsubscribing from " << name;
}

bool ProviderBridge::event(QEvent *e)
{
    try {
        using namespace cor::qt;
        switch (static_cast<Event::Type>(e->type())) {
        case (Event::Execute): {
            auto s = static_cast<EventExecute*>(e);
            s->execute();
            return true;
        }
        default:
            return QObject::event(e);
        }
    } catch (cor::Error const &e) {
        qDebug() << "Caught cor::Error " << e.what();
    } catch (...) { // Qt does not allow exceptions from event handlers
        qDebug() << "event: caught unknown exception";
    }
    return false;
}

EXTERN_C struct statefs_node * ns_find
(struct statefs_branch const* self, char const *name)
{
    if (!name)
        return nullptr;

    // just a linear search now, not so much namespaces expected
    for (size_t i = 0; i < namespaces.size(); ++i) {
        auto &node = namespaces[i]->node;
        if (!strcmp(name, node.name))
            return &node;
    }
    return nullptr;
}

EXTERN_C struct statefs_node * ns_get
(struct statefs_branch const* self, statefs_handle_t idx)
{
    if ((unsigned)idx >= namespaces.size())
        return nullptr;
    return &(namespaces[idx]->node);
}

EXTERN_C statefs_handle_t ns_first(struct statefs_branch const* self)
{
    return 0;
}

EXTERN_C void ns_next(struct statefs_branch const*, statefs_handle_t *idx_ptr)
{
    ++*idx_ptr;
}

EXTERN_C bool ckit_connect
(struct statefs_property *p, struct statefs_slot *slot)
{
    auto self = CKitProperty::self_cast(p);
    self->connect(slot);
    return false;
}

EXTERN_C void ckit_disconnect(struct statefs_property *p)
{
    auto self = CKitProperty::self_cast(p);
    self->disconnect();
}

EXTERN_C int ckit_getattr(struct statefs_property const* p)
{
    return STATEFS_ATTR_READ | STATEFS_ATTR_DISCRETE;
}

EXTERN_C statefs_ssize_t ckit_size(struct statefs_property const* p)
{
    auto self = CKitProperty::self_cast(p);
    return self->size();
}

EXTERN_C statefs_handle_t ckit_open(struct statefs_property *p, int mode)
{
    if (mode & O_WRONLY) {
        errno = EINVAL;
        return 0;
    }
    auto self = CKitProperty::self_cast(p);
    return cor::new_tagged_handle<PropHandle>(self);
}

EXTERN_C int ckit_read(statefs_handle_t h, char *dst, statefs_size_t len, statefs_off_t off)
{
    auto p = cor::tagged_handle_pointer<PropHandle>(h);
    return (p) ? p->read(dst, len, off) : 0;
}

EXTERN_C void ckit_close(statefs_handle_t h)
{
    cor::delete_tagged_handle<PropHandle>(h);
}

EXTERN_C void ckit_release(struct statefs_node *node)
{
    namespaces.clear();
    qt_app.reset(nullptr);
}

static struct statefs_provider provider = {
    version:  STATEFS_CURRENT_VERSION,
    root: {
    node:  {
        type:  statefs_node_root,
        name:  "contextkit",
        release:  &ckit_release,
        info: nullptr
    },
    branch:  {
        find:  ns_find,
        first:  &ns_first,
        next:  &ns_next,
        get:  &ns_get,
        release: nullptr
    }
    },
    io:  {
        getattr:  ckit_getattr,
        size:  ckit_size,
        open:  ckit_open,
        read:  ckit_read,
        write:  nullptr,
        close:  ckit_close,
        connect:  ckit_connect,
        disconnect:  ckit_disconnect
    }
};

static bool is_loaded = false;
static void load_info()
{
    info_tree_type info_tree;
    build_tree(info_tree);

    namespaces.resize(info_tree.size());
    auto pns = namespaces.begin();
    for (auto const &ns_prop: info_tree) {
        pns->reset(new NamespaceNode(ns_prop.first));

        for (auto const &prop: ns_prop.second) {
            auto pinfo = prop.second;
            (*pns)->insert(new CKitProperty(prop.second));
        }
        ++pns;
    }
    is_loaded = true;
}

EXTERN_C struct statefs_provider * statefs_provider_get(void)
{
    if (is_loaded)
        return &provider;

    qt_app.reset(new QtBridge());
    load_info();

    return &provider;
}
