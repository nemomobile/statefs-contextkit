#include <statefs/provider.h>

struct APropertyHandle
{
    virtual int read(intptr_t h, char *dst, size_t len, off_t off) =0;
    virtual int write(intptr_t, char const*, size_t, off_t) =0;
    virtual void close(intptr_t) =0;
};

struct AProperty : public statefs_property
{
    virtual int getattr() const =0;
    virtual ssize_t size() const =0;
    virtual APropertyHandle *open(int flags) =0;

    virtual bool connect(::statefs_property *, ::statefs_slot *) =0;
    virtual void disconnect(::statefs_property *) =0;

};

static AProperty *property_impl(::statefs_property *p)
{
    return static_cast<AProperty*>(p);
}

static AProperty const* property_impl(::statefs_property const* p)
{
    return static_cast<AProperty const*>(p);
}

static int getattr_bridge(::statefs_property const *p)
{
    auto impl = property_impl(p);
}

static ssize_t size_bridge(::statefs_property const *p)
{
    auto impl = property_impl(p);
}

static intptr_t open_bridge(::statefs_property *p, int flags)
{
    auto impl = property_impl(p);
}

static int read_bridge(intptr_t h, char *dst, size_t len, off_t off)
{
}

static int write_bridge(intptr_t, char const*, size_t, off_t)
{
}

static void close_bridge(intptr_t p)
{
}

static bool connect_bridge(::statefs_property *p, ::statefs_slot *)
{
    auto impl = property_impl(p);
}

static void disconnect_bridge(::statefs_property *p)
{
    auto impl = property_impl(p);
}

struct ANamespace : public statefs_namespace
{
};
