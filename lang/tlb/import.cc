#include "import.hh"

#include "typemodel.hh"
#include "typebuilder.hh"
#include "registry.hh"

#include <iostream>

// Helpers
namespace 
{
    using namespace std;
    using namespace Typelib;
    
    template<typename Cat>
    Cat getCategoryFromNode(Cat* categories, xmlChar const* name )
    {
        for(const Cat* cur_cat = categories; cur_cat->name; ++cur_cat)
        {
            if (!xmlStrcmp(name, cur_cat->name)) 
                return *cur_cat;
        }
        return Cat();
    }
}

// Core function for loading
// Loading is done in two passes:
//  - first, we register all type names present and the associated xml nodes and loading functions
//    by calling readNodes
//  - second, we load the Type objects in dependency order by calling load
namespace
{
    // A factory function that builds a type object from a node description
    struct TypeNode;
    class Factory;
    typedef Type const* (*NodeLoader) (TypeNode const& type, Factory& factory);

    struct TypeNode
    {
        xmlNodePtr xml;
        string name;
        string file;
        NodeLoader  loader;

        TypeNode
            ( xmlNodePtr node = 0
            , string const& name_ = string()
            , string const& file_ = string()
            , NodeLoader loader_ = 0 )
            : xml(node), name(name_), file(file_), loader(loader_) {}
    };
    typedef std::map<string, TypeNode> TypeMap;
}

// load loads the \c name type using the type nodes in \c remaining, which holds 
// types defined in the tlb file that aren't loaded yet
namespace
{
    class Factory
    {
        TypeMap   m_map;
        Registry& m_registry;

    public:
        Factory(Registry& registry)
            : m_registry(registry) {}

        void insert(TypeNode const& type, Type* object)
        { m_registry.add(object, type.file); }

        void alias (TypeNode const& type, string const& new_name)
        { m_registry.alias(new_name, type.name); }

        void build(TypeMap const& map)
        {
            m_map = map;
            while(! m_map.empty())
                build(m_map.begin()->first);
        }

        // Do NOT pass name by reference. It could be destroyed by the
        // m_map.erase(it)
        Type const* build(string const name)
        {
            string basename = TypeBuilder::getBaseTypename(name);

            Type const* base = m_registry.get(basename);
            TypeMap::iterator it = m_map.find(basename);

            if (! base)
            {
                if (it == m_map.end())
                    throw Undefined(basename);

                TypeNode    type(it->second);
                m_map.erase(it);

                base = type.loader(type, *this);
            }
            else if (it != m_map.end())
            {
                // TODO check that the definition in the file
                // TODO and the definition in the registry 
                // TODO match
                m_map.erase(it);
            }

            if (basename == name)
                return base;

            return m_registry.build(name);
        }
    };
}

// Loading nodes
namespace
{
    struct NumericCategory
    {
        const xmlChar* name;
        Numeric::NumericCategory  type;
    };
    NumericCategory numeric_categories[] = {
        { reinterpret_cast<const xmlChar*>("sint"),  Numeric::SInt },
        { reinterpret_cast<const xmlChar*>("uint"),  Numeric::UInt },
        { reinterpret_cast<const xmlChar*>("float"), Numeric::Float },
        { 0, Numeric::SInt }
    };
    Type const* load_numeric(TypeNode const& node, Factory& factory)
    {
        NumericCategory category = getCategoryFromNode
            ( numeric_categories
            , reinterpret_cast<xmlChar const*>(getAttribute<string>(node.xml, "category").c_str()) );
        size_t      size = getAttribute<size_t>(node.xml, "size");

        Type* type = new Numeric(node.name, size, category.type);
        factory.insert(node, type);
        return type;
    }



    Type const* load_enum(TypeNode const& node, Factory& factory)
    { 
        Enum* type = new Enum(node.name);
        for(xmlNodePtr xml = node.xml->xmlChildrenNode; xml; xml=xml->next)
        {
            if (!xmlStrcmp(xml->name, reinterpret_cast<const xmlChar*>("text")))
                continue;

            string symbol = getAttribute<string>(xml, "symbol");
            Enum::integral_type value  = getAttribute<Enum::integral_type>(xml, "value");
            type->add(symbol, value);
        }

        factory.insert(node, type); 
        return type;
    }
    Type const* load_alias(TypeNode const& node, Factory& factory)
    {
        string source = getAttribute<string>(node.xml, "source");
        Type const* type = factory.build(source);
        factory.alias(node, source);
        return type;
    }

    
    Type const* load_compound(TypeNode const& node, Factory& factory)
    {
        Compound* compound = new Compound(node.name);

        for(xmlNodePtr xml = node.xml->xmlChildrenNode; xml; xml=xml->next)
        {
            if (!xmlStrcmp(xml->name, reinterpret_cast<const xmlChar*>("text")))
                continue;

            //checkNodeName<UnexpectedElement>    (xml, "field");
            string name   = getAttribute<string>(xml, "name");
            string tname  = getAttribute<string>(xml, "type");
            size_t offset = getAttribute<size_t>(xml, "offset");

            Type const* type = factory.build(tname);
            compound->addField(name, *type, offset);
        }

        factory.insert(node, compound);
        return compound;
    }
}


namespace
{
    struct NodeCategories
    {
        const xmlChar* name;
        NodeLoader  loader;
    };
    NodeCategories node_categories[] = {
        { reinterpret_cast<const xmlChar*>("alias"),     load_alias },
        { reinterpret_cast<const xmlChar*>("numeric"),   load_numeric },
        { reinterpret_cast<const xmlChar*>("enum"),      load_enum },
        { reinterpret_cast<const xmlChar*>("compound"),  load_compound },
        { 0, 0 }
    };

    void load(string const& file, TypeMap& map, xmlNodePtr type_node)
    {
        string name   = getStringAttribute(type_node, "name");
        NodeCategories cat = getCategoryFromNode(node_categories, type_node->name);

        TypeMap::iterator it = map.find(name);
        if (it != map.end())
        {
            string old_file = it->second.file;
            std::clog << "Type " << name << " has already been defined in " << old_file << endl;
            std::clog << "\tThe definition found in " << file << " will be ignored" << endl;
        }
        else
        {
            map[name] = TypeNode(type_node, name, file, cat.loader);
        }
    }
}

void TlbImport::load
    ( std::istream& stream
    , utilmm::config_set const& config
    , Typelib::Registry& registry)
{
    // libXML is not really iostream-compatible :(
    // Get the whole document
    std::string document;

    while (stream.good())
    {
        std::stringbuf buffer;
        stream >> &buffer;
        document += buffer.str();
    }
    parse("", xmlParseMemory(document.c_str(), document.length()), registry);
}


void TlbImport::load
    ( std::string const& path
    , utilmm::config_set const& config
    , Typelib::Registry& registry)
{
    // libxml needs a full path (?)
    string full_path;
    if (path[0] != '/')
    {
        char cwd[PATH_MAX];
        getcwd(cwd, PATH_MAX);

        full_path = string(cwd) + "/" + path;
    } else full_path = path;

    try {
        parse(full_path, xmlParseFile(path.c_str()), registry);
    }
    catch(ImportError& e)
    { 
        e.setFile(path);
        throw;
    }
}

void TlbImport::parse(std::string const& source_id, xmlDocPtr doc, Registry& registry)
{
    if (!doc) 
        throw Parsing::MalformedXML();

    try
    {
        xmlNodePtr root_node = xmlDocGetRootElement(doc);
        if (!root_node) 
            return;

        checkNodeName<Parsing::BadRootElement>(root_node, "typelib");

        TypeMap all_types;
        for(xmlNodePtr node = root_node -> xmlChildrenNode; node; node=node->next)
        {
            if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("text")))
                continue;

            ::load(source_id, all_types, node);
        }

        Factory factory(registry);
        factory.build(all_types);
    }
    catch(...) { 
        xmlFreeDoc(doc); 
        throw;
    }
}

