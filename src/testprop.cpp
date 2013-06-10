#include <statefs/provider.hpp>
#include <iostream>

using namespace statefs;

static const statefs_node test_ns1_tpl = {
    statefs_node_ns,
    nullptr,
    nullptr,
    nullptr
};

static const statefs_branch test_ns1_b1 = {};

class TestNodeWrapper : public NodeWrapper<statefs_namespace>
{
public:
    virtual void release() {}
    TestNodeWrapper(char const *name)
        : NodeWrapper<statefs_namespace>(name, test_ns1_tpl) {}
};

static TestNodeWrapper test_ns1("test1");

template <typename T>
class TestBranchWrapper : public BranchWrapper<T>
{
public:
    virtual void release() {}
    TestBranchWrapper(char const *name)
        : BranchWrapper<T>(name, test_ns1_tpl, test_ns1_b1) {}
};

// static_assert((void*)&test_ns1 == (void*)static_cast<statefs_namespace*>(&test_ns1)
//               , "Same object pointer");

class NsChild : public Namespace
{
public:
    NsChild(char const *name) : Namespace(name)
    {
    }
    virtual void release() {}
};


class NsX : public Namespace
{
public:
    NsX(char const *name) : Namespace(name)
    {
        insert(new NsChild("t1"));
    }
    virtual void release() {}
};

class Provider : public AProvider
{
public:
    Provider(char const *name)
        : AProvider(name)
    {}
    virtual void release() {}
};

int main()
{
    TestBranchWrapper<statefs_namespace> bw("t1");
    auto b = branch_from(&bw);
    auto bw1 = branch_to<statefs_namespace>(b);
    std::cout << &bw << " = " << bw1 << " . " << b << std::endl;

    TestBranchWrapper<statefs_provider> pw("t2");
    auto b2 = branch_from(static_cast<statefs_provider*>(&pw));
    auto pw1 = branch_to<statefs_provider>(b2);
    std::cout << &pw << " = " << pw1 << " . " << b2 << std::endl;

    NsX ns1("name1");
    Provider prov("prov1");
    prov.insert(new NsChild("n1"));
    //, {"n2", new NsChild()}});
    return 0;
}
