/*
 * PUBLIC DOMAIN PCCTS-BASED C++ GRAMMAR (cplusplus.g, stat.g, expr.g)
 *
 * Authors: Sumana Srinivasan, NeXT Inc.;            sumana_srinivasan@next.com
 *          Terence Parr, Parr Research Corporation; parrt@parr-research.com
 *          Russell Quong, Purdue University;        quong@ecn.purdue.edu
 *
 * VERSION 1.1
 *
 * SOFTWARE RIGHTS
 *
 * This file is a part of the ANTLR-based C++ grammar and is free
 * software.  We do not reserve any LEGAL rights to its use or
 * distribution, but you may NOT claim ownership or authorship of this
 * grammar or support code.  An individual or company may otherwise do
 * whatever they wish with the grammar distributed herewith including the
 * incorporation of the grammar or the output generated by ANTLR into
 * commerical software.  You may redistribute in source or binary form
 * without payment of royalties to us as long as this header remains
 * in all source distributions.
 *
 * We encourage users to develop parsers/tools using this grammar.
 * In return, we ask that credit is given to us for developing this
 * grammar.  By "credit", we mean that if you incorporate our grammar or
 * the generated code into one of your programs (commercial product,
 * research project, or otherwise) that you acknowledge this fact in the
 * documentation, research report, etc....  In addition, you should say nice
 * things about us at every opportunity.
 *
 * As long as these guidelines are kept, we expect to continue enhancing
 * this grammar.  Feel free to send us enhancements, fixes, bug reports,
 * suggestions, or general words of encouragement at parrt@parr-research.com.
 * 
 * NeXT Computer Inc.
 * 900 Chesapeake Dr.
 * Redwood City, CA 94555
 * 12/02/1994
 * 
 * Restructured for public consumption by Terence Parr late February, 1995.
 *
 * DISCLAIMER: we make no guarantees that this grammar works, makes sense,
 *             or can be used to do anything useful.
 */
/*
 * 2001-2003 Version 2.0 September 2003
 *
 * Some modifications were made to this file to support a project by
 * Jianguo Zuo and David Wigg at
 * The Centre for Systems and Software Engineering
 * South Bank University
 * London, UK.
 * wiggjd@bcs.ac.uk
 * blackse@lsbu.ac.uk
 */
/* 2003-2004 Version 3.0 July 2004
 * Modified by David Wigg at London South Bank University for CPP_parser.g
 *
 * See MyReadMe.txt for further information
 *
 * This file is best viewed in courier font with tabs set to 4 spaces
 */

#include <iostream>
#include "CPPParser.hpp"
#include "CPPSymbol.hh"

using namespace std;

void CPPParser::init()
{
    //antlrTrace(false);	// This is a dynamic trace facility for use with -traceParser etc.
    // It requires modification in LLkParser.cpp and LLkParser.hpp
    //  otherwise it should be commented out (see MyReadMe.txt)
    // true shows antlr trace (can be set and reset during parsing)
    // false stops showing antlr trace 
    // Provided the parser is always generated with -traceParser this
    //  facility allows trace output to be turned on or off by changing
    //  the setting here from false to true or vice versa and then
    //  recompiling and linking CPPParser only thus avoiding the need
    //  to use antlr.Tool to re-generate the lexer and parser again. 

    // Creates a dictionary to hold symbols with 4001 buckets, 200 scopes and 800,000 characters
    // These can be changed to suit the size of program(s) being parsed
    symbols = new CPPDictionary(4001, 200, 800000);

    symbols->saveScope();	// Advance currentScope from 0 to 1
    externalScope = symbols->getCurrentScopeIndex();	// Set external scope to 1

    // Global flags to allow for nested declarations
    _td = false;		// For typedef
    _sc = scInvalid;	// For StorageClass
    _tq = tqInvalid;	// For TypeQualifier
    _ts = tsInvalid;	// For TypeSpecifier
    _ds = dsInvalid;	// For DeclSpecifier

    functionDefinition = 0;
    assign_stmt_RHS_found = 0;
    in_parameter_list = false;
    in_return = false;
}

CPPParser::~CPPParser()
{
    delete symbols;
}

/*
 * Return true if 's' can pose as a type name
 */
int CPPParser::isTypeName(const std::string& s)
{
    CPPSymbol *cs = (CPPSymbol *) symbols->lookup(s);

    if (cs==NULL)
    {
        //printf("support.cpp isTypeName %s not found\n",s);
        return 0;
    }

    if (cs->getType()==CPPSymbol::otTypedef||
            cs->getType()==CPPSymbol::otEnum||
            cs->getType()==CPPSymbol::otClass||
            cs->getType()==CPPSymbol::otStruct||
            cs->getType()==CPPSymbol::otUnion)
    {

        // DW 13/06/03 Check that scope level found is not greater 
        //  than the current scope i.e. lower level
        //		if (cs->this_scope > symbols->getCurrentScopeIndex() )
        //			{
        //			return 0;
        //			}
        //
        //		if (cs->getInScope()==false)
        //			{
        //			return 1;
        //			}
        return 1;
    }

    return 0;
}

void CPPParser::beginDeclaration() { }

void CPPParser::endDeclaration() { }

void CPPParser::beginFunctionDefinition()
{
    functionDefinition = 1;
}

void CPPParser::endFunctionDefinition()
{
    // Remove parameter scope
    symbols->dumpScope(stdout);
    //fprintf(stdout, "endFunctionDefinition remove parameter scope(%d):\n",symbols->getCurrentScopeIndex());
    symbols->removeScope();
    symbols->restoreScope();
    //printf("endFunctionDefinition restoreScope() now %d\n",symbols->getCurrentScopeIndex());
    functionDefinition = 0;
}

void CPPParser::beginParameterDeclaration()
{ }

void CPPParser::beginFieldDeclaration()
{ }

void CPPParser::declarationSpecifier(bool td, StorageClass sc, TypeQualifier tq,
        TypeSpecifier ts, DeclSpecifier ds)
{
    //printf("support.cpp declarationSpecifier td %d, fd %d, sc %d, tq, %d, ts %d, ds %d, qi %d\n",
    //	td,fd,sc,tq,ts,ds,qi);
    _td = td;	// For typedef
    _sc = sc;
    _tq = tq;
    _ts = ts;
    _ds = ds;
    //	_qi = qi;
}

/* Symbols from declarators are added to the symbol table here. 
 * The symbol is added to whatever the current scope is in the symbol table. 
 * See list of object types below.
 */
void CPPParser::declaratorID(const std::string& id, QualifiedItem qi)
{
    CPPSymbol *c;
    if (qi==qiType || _td)	// Check for type declaration
    {
        if (!(qi==qiType && _td))
        {
            std::cerr << LT(1) -> getLine()
                << "support.cpp declaratorID warning qi " << qi << ", _td " << _td 
                << " inconsistent for " << id << std::endl;
        }

        c = new CPPSymbol(id, CPPSymbol::otTypedef);
        symbols->defineInScope(id, c, externalScope);
    }
    else if (qi==qiFun)	// For function declaration
    {
        c = new CPPSymbol(id, CPPSymbol::otFunction);
        symbols->define(id, c);	// Add to current scope
    }
    else	   
    {
        if (qi!=qiVar)
        {
            std::cerr << LT(1) -> getLine()
                << "support.cpp declaratorID warning qi " << qi << "not qiVar _td " << qiVar
                << " inconsistent for " << id << std::endl;
        }

        c = new CPPSymbol(id, CPPSymbol::otVariable);	// Used to leave otInvalid DW 02/07/03 Think this should be otVariable
        symbols->define(id, c);	// Add to current scope
    }
}

void CPPParser::enterNamespace(std::string const& name) { }
void CPPParser::exitNamespace() { }

/* These are the object types
   0 = otInvalid
   1 = otFunction
   2 = otVariable
   3 = otTypedef	Note. 3-7 are type names
   4 = otStruct	Note. 4, 5 & 7 are class names
   5 = otUnion
   6 = otEnum
   7 = otClass
   8 = otEnumElement
   */

void CPPParser::declaratorArray(int size) { }

void CPPParser::declaratorParameterList(int def)
{
    symbols->saveScope();
    //printf("declaratorParameterList saveScope() now %d\n",symbols->getCurrentScopeIndex());
}

void CPPParser::declaratorEndParameterList(int def)
{
    if (!def)
    {
        symbols->dumpScope(stdout);
        symbols->removeScope();
        symbols->restoreScope();
        //printf("declaratorEndParameterList restoreScope() now %d\n",symbols->getCurrentScopeIndex());
    }
}

void CPPParser::functionParameterList()
{
    symbols->saveScope();
    //printf("functionParameterList saveScope() now %d\n",symbols->getCurrentScopeIndex());
    // DW 25/3/97 change flag from function to parameter list
    functionDefinition = 2;
}

void CPPParser::functionEndParameterList(int def)
{
    // If this parameter list is not in a definition then this
    if ( !def)
    {
        symbols->dumpScope(stdout);
        symbols->removeScope();
        symbols->restoreScope();
        //printf("functionEndParameterList restoreScope() now %d\n",symbols->getCurrentScopeIndex());
    }
    else
    {
        // Change flag from parameter list to body of definition
        functionDefinition = 3;   
    }
    /* Otherwise endFunctionDefinition removes the parameters from scope */
}

void CPPParser::enterNewLocalScope()
{
    symbols->saveScope();
    //printf("enterNewLocalScope saveScope() now %d\n",symbols->getCurrentScopeIndex());
}

void CPPParser::exitLocalScope()
{
    symbols->dumpScope(stdout);
    symbols->removeScope();
    symbols->restoreScope();
    //printf("exitLocalScope restoreScope() now %d\n",symbols->getCurrentScopeIndex());
}

void CPPParser::enterExternalScope()
{// Scope has been initialised to 1 in CPPParser.init() in CPPParser.hpp
    // DW 25/3/97 initialise
    functionDefinition = 0;
    //classDefinition = 0;	DW 19/03/04 not used anywhere
}

void CPPParser::exitExternalScope()
{
    symbols->dumpScope(stdout);
    symbols->removeScope();		// This just removes the symbols stored in the current scope
    symbols->restoreScope();	// This just reduces the current scope by 1
    //if (symbols->getCurrentScopeIndex()==0)
    //    printf("\nSupport exitExternalScope, scope now %d\n",symbols->getCurrentScopeIndex());
    //else
    //    printf("\nSupport exitExternalScope, scope now %d, should be 0\n",symbols->getCurrentScopeIndex());
}

void CPPParser::classForwardDeclaration(TypeSpecifier ts, DeclSpecifier ds, const std::string& tag)
{
    CPPSymbol *c = NULL;

    // if already in symbol table as a class, don't add
    // of course, this is incorrect as you can rename
    // classes by entering a new scope, but our limited
    // example basically keeps all types globally visible.
    if ( symbols->lookup(tag)!=NULL )
        return;

    switch (ts)
    {
        case tsSTRUCT :
            c = new CPPSymbol(tag, CPPSymbol::otStruct);
            break;
        case tsUNION :
            c = new CPPSymbol(tag, CPPSymbol::otUnion);
            break;
        case tsCLASS :
            c = new CPPSymbol(tag, CPPSymbol::otClass);
            break;
    }

    symbols->defineInScope(tag, c, externalScope);
}

void CPPParser::beginClassDefinition(TypeSpecifier ts,const std::string& tag)
{
    CPPSymbol *c;

    // if already in symbol table as a class, don't add
    // of course, this is incorrect as you can rename
    // classes by entering a new scope, but our limited
    // example basically keeps all types globally visible.
    if (symbols->lookup(tag) != NULL)
    {
        symbols->saveScope();   // still have to use scope to collect members
        //printf("support.cpp beginClassDefinition_1 saveScope() now %d\n",symbols->getCurrentScopeIndex());
        return;
    }

    switch (ts)
    {
        case tsSTRUCT:
            c = new CPPSymbol(tag, CPPSymbol::otStruct);
            break;
        case tsUNION:
            c = new CPPSymbol(tag, CPPSymbol::otUnion);
            break;
        case tsCLASS:
            c = new CPPSymbol(tag, CPPSymbol::otClass);
            break;
        default:
            throw std::runtime_error("unexpected TypeSpecifier in CPPParser::beginClassDefinition");
    }
    symbols->defineInScope(tag, c, externalScope);
    qualifierPrefix += tag;
    qualifierPrefix += "::";

    // add all member type symbols into the global scope (not correct, but
    // will work for most code).
    // This symbol lives until the end of the file

    symbols->saveScope();   // use the scope to collect list of fields
}

void CPPParser::endClassDefinition()
{
    symbols->dumpScope(stdout);
    symbols->removeScope();
    symbols->restoreScope();
    // remove final T:: from A::B::C::T::
    // upon backing out of last class, qualifierPrefix is set to ""
    size_t index = qualifierPrefix.rfind("::");
    if (index == std::string::npos)
        return;
    qualifierPrefix.resize(index);
}

void CPPParser::enumElement(const std::string& e, bool has_value, int value)
{ }

void CPPParser::beginEnumDefinition(const std::string& e)
{// DW 26/3/97 Set flag for new class

    // add all enum tags into the global scope (not correct, but
    // will work for most code).
    // This symbol lives until the end of the file
    CPPSymbol *c = new CPPSymbol(e, CPPSymbol::otEnum);
    symbols->defineInScope(e, c, externalScope);
}

void CPPParser::endEnumDefinition() {	}

void CPPParser::foundSimpleType(const std::list<std::string>& full_type) { }

void CPPParser::end_of_stmt() { }                    


