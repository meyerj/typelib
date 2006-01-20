#include "testsuite.hh"
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include "test_cimport.1"
using namespace Typelib;

class TC_CImport
{
public:
    TC_CImport() { }
    ~TC_CImport() { }

    void test_import()
    {
        Registry registry;
        static const char* test_file = TEST_DATA_PATH("test_cimport.h");

        PluginManager::self manager;
        Importer* importer = manager->importer("c");
        utilmm::config_set config;
        
        // Load the file in registry
        BOOST_REQUIRE( !importer->load("does_not_exist", config, registry) );
        BOOST_REQUIRE( !importer->load("test_cimport.h", config, registry) );
        {
            Registry temp_registry;
            config.set("includes", "../");
            config.set("defines", "GOOD");
            BOOST_REQUIRE( importer->load("test_cimport.h", config, temp_registry) );
        }
        {
            Registry temp_registry;
            config.insert("rawflags", "-I../");
            config.insert("rawflags", "-DGOOD");
            BOOST_REQUIRE( importer->load("test_cimport.h", config, temp_registry) );
        }

        BOOST_REQUIRE( importer->load(test_file, config, registry) );

        // Check that the types are defined
        BOOST_REQUIRE( registry.has("/struct A") );
        BOOST_REQUIRE( registry.has("/struct B") );
        BOOST_REQUIRE( registry.has("/ADef") );
        BOOST_REQUIRE( registry.has("/B") );
        BOOST_REQUIRE( registry.has("/OpaqueType") );

        // Check that the size of B.a is the same as A
        Compound const* b   = static_cast<Compound const*>(registry.get("/B"));
        Field  const* b_a = b->getField("a");
        BOOST_REQUIRE_EQUAL( &(b_a->getType()), registry.get("/ADef") );

        // Check the type of c (array of floats)
        Field  const* b_c = b->getField("c");
        BOOST_REQUIRE_EQUAL( &(b_c->getType()), registry.get("/float[100]") );
        BOOST_REQUIRE_EQUAL( b_c->getType().getCategory(), Type::Array );

        // Check the array indirection
        Array const& b_c_array(dynamic_cast<Array const&>(b_c->getType()));
        BOOST_REQUIRE_EQUAL( &b_c_array.getIndirection(), registry.get("/float") );

        // Check the sizes
        BOOST_REQUIRE_EQUAL( registry.get("/struct A")->getSize(), sizeof(A) );
        BOOST_REQUIRE_EQUAL( b->getSize(), sizeof(B) );
        BOOST_REQUIRE_EQUAL( b_c_array.getDimension(), 100UL );

        // Check the enum behaviour
        BOOST_REQUIRE( registry.has("/E") );
        BOOST_REQUIRE_EQUAL(registry.get("/E")->getCategory(), Type::Enum);
        Enum const& e(dynamic_cast<Enum const&>(*registry.get("/E")));
        Enum::ValueMap const& map = e.values();

        // Check that the values in Enum are the ones we are expecting
        struct ExpectedValue
        {
            char const* name;
            int         value;
        };
        ExpectedValue expected[] = {
            { "FIRST", FIRST },
            { "SECOND", SECOND },
            { "THIRD", THIRD },
            { "LAST", LAST },
            { 0, 0 }
        };
            
        for (ExpectedValue* exp_it = expected; exp_it->name; ++exp_it)
        {
            Enum::ValueMap::const_iterator it = map.find(exp_it->name);
            BOOST_REQUIRE( it != map.end() );
            BOOST_REQUIRE_EQUAL(exp_it->value, it->second);
        }
    }
};

void test_lang_c(test_suite* ts) {
    boost::shared_ptr<TC_CImport> instance( new TC_CImport );
    ts->add( BOOST_CLASS_TEST_CASE( &TC_CImport::test_import, instance ) );
}
