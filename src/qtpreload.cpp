#include "wrapqt.hpp"

#include <statefs/util.h>
#include <cor/so.hpp>
#include <memory>
#include <iostream>

typedef std::unique_ptr
<statefs_provider, void (*)(statefs_provider*)> provider_handle_type;

static void provider_release(statefs_provider *p) {
    statefs_provider_release(p);
};

static std::unique_ptr<cor::qt::CoreAppContainer> app;
static provider_handle_type provider(nullptr, statefs_provider_release);
static std::unique_ptr<cor::SharedLib> lib;
static statefs_provider provider_copy;

static void loader_release(struct statefs_node *node)
{
    std::cerr << "Releasing Qt preload provider" << std::endl;
    lib.reset(nullptr);
    provider.release();
    app.reset(nullptr);
}

static void load_provider()
{
    // hard-coded path to contextkit provider to quickly fix issue
    // with qt threading. Preload provider should be modified to load
    // any Qt-based provider
    static const char *provider_path = "/usr/lib/statefs/libprovider-contextkit.so";
    static const char *sym_name = "statefs_provider_get";

    std::cerr << "Loading Qt preload provider" << std::endl;
    if (provider) {
        std::cerr << "Provider is already loaded" << std::endl;
        return;
    }
    lib.reset(new cor::SharedLib(provider_path, RTLD_LAZY));
    if (!lib->is_loaded()) {
        std::cerr << "qtpreloader: Can't load library " << provider_path
                  << ", reason: " << ::dlerror() << std::endl;
        return;
    }

    auto fn = lib->sym<statefs_provider_fn>(sym_name);
    if (!fn) {
        std::cerr << "Can't resolve " << sym_name << std::endl;
        return;
    }

    provider = provider_handle_type(fn(), statefs_provider_release);
    if (!provider) {
        std::cerr << "qtpreloader: null provider\n";
        return;
    } else if (!statefs_is_compatible(provider.get())) {
        std::cerr << "qtpreloader: Incompatible provider version "
                  << provider->version << " vs " << STATEFS_CURRENT_VERSION;
        provider.reset(nullptr);
        return;
    }
    memcpy(&provider_copy, provider.get(), sizeof(provider_copy));
    provider_copy.root.node.release = loader_release;
}

EXTERN_C struct statefs_provider * statefs_provider_get(void)
{
    if (!app) {
        app.reset(new cor::qt::CoreAppContainer());
        app->execute(load_provider);
        if (!provider)
            app.reset(nullptr);
    }
    return provider ? &provider_copy : nullptr;
}
