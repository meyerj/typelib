#include "export.hh"
#include <iostream>

#include "typevisitor.hh"

namespace
{
    using namespace std;
    using namespace Typelib;
    class TlbExportVisitor : public TypeVisitor
    {
        ostream&  m_stream;
        string    m_indent;

        template<typename T>
        void display_compound(T const& type, char const* compound_name);
        bool display_field(Field const& field);

    protected:
        bool visit_(Compound const& type);
        bool visit_(Compound const& type, Field const& field);

        bool visit_(Numeric const& type);

        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

        bool visit_(Enum const& type);

    public:
        TlbExportVisitor(ostream& stream, string const& base_indent);
    };

    struct Indent
    {
        string& m_indent;
        string  m_save;
        Indent(string& current)
            : m_indent(current), m_save(current)
        { m_indent += "  "; }
        ~Indent() { m_indent = m_save; }
    };

    TlbExportVisitor::TlbExportVisitor(ostream& stream, string const& base_indent)
        : m_stream(stream), m_indent(base_indent) {}
                

    bool TlbExportVisitor::visit_(Compound const& type)
    { 
        m_stream << "<compound name=\"" << type.getName() << "\">\n";
        
        { Indent indenter(m_indent);
            TypeVisitor::visit_(type);
        }

        m_stream << m_indent
            << "</compound>";

        return true;
    }
    bool TlbExportVisitor::visit_(Compound const& type, Field const& field)
    { 
        m_stream 
            << m_indent
            << "<field name=\"" << field.getName() << "\""
            << " type=\"" << field.getType().getName()  << "\""
            << " offset=\"" << field.getOffset() << "\"/>\n";
        return true;
    }

    namespace
    {
        char const* getStringCategory(Numeric::NumericCategory category)
        {
            switch(category)
            {
                case Numeric::SInt:
                    return "sint";
                case Numeric::UInt:
                    return "uint";
                case Numeric::Float:
                    return "float";
            }
            // never reached
            return 0;
        }
    }

    bool TlbExportVisitor::visit_(Numeric const& type)
    {
        m_stream 
            << "<numeric name=\"" << type.getName() << "\" " 
            << "category=\"" << getStringCategory(type.getNumericCategory()) << "\" "
            << "size=\"" << type.getSize() << "\"/>";
        return true;
    }

    void indirect(ostream& stream, Indirect const& type)
    {
        stream 
            << " name=\"" << type.getName() 
            << "\" of=\"" << type.getIndirection().getName() << "\"";
    }

    bool TlbExportVisitor::visit_ (Pointer const& type)
    {
        m_stream << "<pointer ";
        indirect(m_stream, type);
        m_stream << "/>";
        return true;
    }
    bool TlbExportVisitor::visit_ (Array const& type)
    {
        m_stream << "<array ";
        indirect(m_stream, type);
        m_stream << " dimension=\"" << type.getDimension() << "\"/>";
        return true;
    }

    bool TlbExportVisitor::visit_ (Enum const& type)
    {
        m_stream << "<enum name=\"" << type.getName() << "\"/>";
        return true;
    }

}

using namespace std;
bool TlbExport::begin
    ( ostream& stream
    , Typelib::Registry const& /*registry*/ )
{
    stream << 
        "<?xml version=\"1.0\"?>\n"
        "<typelib>\n";
    return true;
}
bool TlbExport::end
    ( ostream& stream
    , Typelib::Registry const& /*registry*/ )
{
    stream << 
        "</typelib>\n";
    return true;
}

bool TlbExport::save
    ( ostream& stream
    , Typelib::RegistryIterator const& type )
{
    if (type.isAlias())
    {
        stream << "  <alias "
            "name=\"" << type.getName() << "\" "
            "source=\"" << type->getName() << "\"/>\n";
    }
    else
    {
        stream << "  ";
        TlbExportVisitor exporter(stream, "  ");
        exporter.apply(*type);
        stream << "\n";
    }
    return true;
}
