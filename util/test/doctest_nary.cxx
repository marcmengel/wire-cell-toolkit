#include "WireCellUtil/NaryTree.h"
#include "WireCellUtil/NaryTesting.h"

#include "WireCellUtil/doctest.h"

#include "WireCellUtil/Logging.h"

#include <string>

using namespace WireCell::NaryTree;
using namespace WireCell::NaryTesting;
using spdlog::debug;

TEST_CASE("nary tree depth iter") {
    depth_iter<int> a, b;

    CHECK( !a.node );

    CHECK( a == b );
    
    CHECK( a.begin() == a.end() );

    depth_const_iter<int> c(a);
    CHECK( a == c );
}





// return numerber of nodes in a tree with all nodes in a given having
// the same number of children as given by layer_sizes.  The root node
// is excluded from layer_sizes but added to the returned count.
size_t nodes_in_uniform_tree(const std::list<size_t>& layer_sizes)
{
    size_t sum=0, prod=1;

    for (auto s : layer_sizes) {
        prod *= s;
        sum += prod;
    }
    return sum + 1;
}



TEST_CASE("nary tree node construction") {

    {
        Introspective d("moved");
        Introspective::node_type n(std::move(d));
        CHECK(n.value.nctor == 1);
        CHECK(n.value.nmove == 1);
        CHECK(n.value.ncopy == 0);
        CHECK(n.value.ndef == 0);
    }

    {
        Introspective d("copied");
        Introspective::node_type n(d);
        CHECK(n.value.nctor == 1);
        CHECK(n.value.nmove == 0);
        CHECK(n.value.ncopy == 1);
        CHECK(n.value.ndef == 0);
    }
}

TEST_CASE("nary tree simple tree tests") {

    const size_t live_count = Introspective::live_count;
    CHECK(live_count == 0);

    {   // scope for tree life.
        auto root = make_simple_tree();

        const auto& childs = root->children();
        auto chit = childs.begin();

        {
            CHECK ( (*chit)->parent == root.get() );

            auto& d = (*chit)->value;

            CHECK( d.name == "0.0" );
            CHECK( d.nctor == 1);
            CHECK( d.nmove == 0);
            CHECK( d.ncopy == 1);
            CHECK( d.ndef == 0);

            ++chit;
        }
        {
            CHECK ( (*chit)->parent == root.get() );

            auto& d = (*chit)->value;

            CHECK( d.name == "0.1" );
            CHECK( d.nctor == 1);
            CHECK( d.nmove == 1);
            CHECK( d.ncopy == 0);
            CHECK( d.ndef == 0);

            ++chit;
        }
        {
            CHECK ( (*chit)->parent == root.get() );

            auto& d = (*chit)->value;

            CHECK( d.name == "0.2" );
            CHECK( d.nctor == 1);
            CHECK( d.nmove == 1);
            CHECK( d.ncopy == 0);
            CHECK( d.ndef == 0);

            ++chit;
        }

        CHECK(chit == childs.end());

        CHECK( childs.front()->next()->value.name == "0.1" );
        CHECK( childs.back()->prev()->value.name == "0.1" );

        {
            size_t nnodes = 0;
            depth_iter<Introspective> depth(root.get());
            std::vector<Introspective> data;
            for (auto it = depth.begin(); // could also use depth().begin()
                 it != depth.end();
                 ++it)
            {
                const Introspective& d = it.node->value;
                debug("depth {} {}", nnodes, d);
                ++nnodes;
                data.push_back(d);
            }
            CHECK( nnodes == 4 );

            CHECK( data[0].name == "0" );
            CHECK( data[1].name == "0.0" );
            CHECK( data[2].name == "0.1" );
            CHECK( data[3].name == "0.2" );
        }
        {   // same as above but range based loop
            size_t nnodes = 0;
            depth_iter<Introspective> depth(root.get());
            std::vector<Introspective> data;
            for (const auto& d : root->depth()) 
            {
                debug("depth {} {}", nnodes, d);                
                ++nnodes;
                data.push_back(d);
            }
            CHECK( nnodes == 4 );

            CHECK( data[0].name == "0" );
            CHECK( data[1].name == "0.0" );
            CHECK( data[2].name == "0.1" );
            CHECK( data[3].name == "0.2" );
        }

        {
            // Const version
            const auto* rc = root.get();
            size_t nnodes = 0;
            std::vector<Introspective> data;
            for (const auto& d : rc->depth()) 
            {
                ++nnodes;
                data.push_back(d);
            }
            CHECK( nnodes == 4 );
        }    

    } // tree r dies

    CHECK(live_count == Introspective::live_count);
}


TEST_CASE("nary tree bigger tree lifetime")
{
    size_t live_count = Introspective::live_count;
    CHECK(live_count == 0);


    {
        std::list<size_t> layer_sizes ={2,4,8};
        size_t niut = nodes_in_uniform_tree(layer_sizes);
        auto root = make_layered_tree(layer_sizes);
        debug("starting live count: {}, ending live count: {}, diff should be {}",
              live_count, Introspective::live_count, niut);
        CHECK(Introspective::live_count - live_count == niut);
    } // root and children all die

    CHECK(live_count == Introspective::live_count);

}

TEST_CASE("nary tree remove node")
{
    size_t live_count = Introspective::live_count;
    CHECK(live_count == 0);

    {
        std::list<size_t> layer_sizes ={2,4,8};
        // size_t niut = nodes_in_uniform_tree(layer_sizes);
        auto root = make_layered_tree(layer_sizes);

        auto& children = root->children();

        Introspective::node_type* doomed = children.front().get();
        CHECK(doomed);

        {                       // test find by itself.
            Introspective::node_type::sibling_iter sit = root->find(doomed);
            CHECK(sit == children.begin());
        }

        const size_t nbefore = children.size();
        auto dead = root->remove(doomed);
        CHECK(dead.get() == doomed);
        CHECK( ! dead->parent );
        const size_t nafter = children.size();
        CHECK(nbefore == nafter + 1);

    } // root and children all die

    CHECK(live_count == Introspective::live_count);

    { // insert node that is in another tree
        auto r1 = make_simple_tree("0");
        auto r2 = make_simple_tree("1");
        Introspective::node_type* traitor = r2->children().front().get();
        r1->insert(traitor);
        CHECK(r1->children().size() == 4);
        CHECK(r2->children().size() == 2);
        CHECK(r1->children().back()->value.name == "1.0");
        CHECK(r2->children().front()->value.name == "1.1");
    }

    CHECK(live_count == Introspective::live_count);
}

TEST_CASE("nary tree child iter")
{
    auto root = make_simple_tree();

    std::vector<std::string> nstack = {"0.2","0.1","0.0"};
    for (const auto& c : root->child_values()) {
        debug("child value: {}", c);
        CHECK(c.name == nstack.back());
        nstack.pop_back();
    }

    // const iterator
    const auto* rc = root.get();
    for (const auto& cc : rc->child_values()) {
        debug("child value: {} (const)", cc);
        // cc.name += " extra";    // should not compile
    }

    for (auto& n : root->child_nodes()) {
        n.value.name += " extra";
        debug("change child node value: {}", n.value);
    }
    for (const auto& cn : rc->child_nodes()) {
        debug("child node value: {}", cn.value);
        // cn.value.name += " extra"; // should not compile
    }
}

TEST_CASE("nary tree notify")
{
    {
        Introspective::node_type node;
        auto& nactions = node.value.nactions;
        CHECK(nactions.size() == 1);
        CHECK(nactions["constructed"] == 1);
    }


    debug("nary tree notify make simple tree");
    auto root = make_simple_tree();
    {
        auto& nactions = root->value.nactions;
        CHECK(nactions.size() == 1);
        CHECK(nactions["constructed"] == 1);
    }
    {
        Introspective::node_type* doomed = root->children().front().get();
        auto& nactions = doomed->value.nactions;
        debug("doomed: {}", doomed->value.name);
        CHECK(nactions.size() == 2);
        CHECK(nactions["constructed"] == 1);
        CHECK(nactions["inserted"] == 1);
        auto dead = root->remove(doomed);
        CHECK(nactions.size() == 3);
        CHECK(nactions["removing"] == 1);        
    }
    
}
TEST_CASE("nary tree flatten")
{
    Introspective::node_type root(Introspective("r"));
    auto& rval = root.value;
    REQUIRE(rval.node == &root);
    root.insert(make_simple_tree("r.0"));
    root.insert(make_simple_tree("r.1"));
    debug("descend root node \"{}\"", rval.name);
    for (auto& value : rval.node->depth()) {
        debug("\tpath to parents from node \"{}\":", value.name);
        auto* node = value.node;
        REQUIRE(node);
        std::string path="", slash="";
        for (auto n : node->sibling_path()) {
            path += slash + std::to_string(n);
            slash = "/";
        }
        debug("\t\tpath: {}  \t (\"{}\")", path, node->value.name);
    }
}
