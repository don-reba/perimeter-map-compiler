//-----------------------------------------------------------------------------
// Perimeter Map Compiler
// Copyright (c) 2005, Don Reba
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// • Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
// • Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution. 
// • Neither the name of Don Reba nor the names of its contributors may be used
//   to endorse or promote products derived from this software without specific
//   prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------


#pragma once

#include <boost/spirit/core.hpp>
#include <boost/spirit/utility/chset.hpp> 

using boost::spirit::rule;
using boost::spirit::grammar;
using boost::spirit::parser_context;
using boost::spirit::parser_tag;

//------------------
// macro definitions
//------------------

#ifdef MacroGrammarRule
#error MacroGrammarRule redefinition
#else
#define MacroGrammarRule(ID) \
	rule<ScannerT, parser_context<>, parser_tag<ID> >
#endif

//-------------------------
// ScriptGrammar definition
//-------------------------

class ScriptGrammar : public grammar<ScriptGrammar>
{
// ids
public:
	enum
	{
		ArrayId = 1,
		DisjunctionId,
		FieldId,
		FieldNameId,
		FloatId,
		IntId,
		QStringId,
		ScriptId,
		SetId,
		SetNameId,
		ValueId
	};
// grammar interface
public:
	template <typename ScannerT>
	struct definition
	{
		definition(ScriptGrammar const& self)
		{
			using namespace boost::spirit;
			// rule definitions
			array_
				=	ch_p('{')
					>>	no_node_d[uint_p]
					>> no_node_d[ch_p(';')]
					>> !(field_ % no_node_d[ch_p(',')])
					>> no_node_d[ch_p('}')];
			disjunction_
				= value_ % no_node_d[ch_p('|')];
			field_
				=	!(field_name_ >> no_node_d[ch_p('=')])
					>> root_node_d[array_ | set_ | float_ | int_ | q_string_ | disjunction_];
			field_name_
				= leaf_node_d[+(chset_p("A-Za-z0-9_"))];
			float_
				=	strict_real_p;
			int_
				= int_p;
			q_string_
				=	lexeme_d[
						no_node_d[ch_p('\"')]
						>> leaf_node_d[*(anychar_p - '\"')]
						>> no_node_d[ch_p('\"')]
					];
			script_
				=	*(field_ >> no_node_d[ch_p(';')]);
			set_
				=	!set_name_
					>> ch_p('{')
					>> *(field_ >> no_node_d[ch_p(';')])
					>> no_node_d[ch_p('}')];
			set_name_
				=	no_node_d[ch_p('\"')]
					>> leaf_node_d[lexeme_d[*(anychar_p - '\"')]]
					>> no_node_d[ch_p('\"')];
			value_
				=	leaf_node_d[+chset_p("A-Za-zÀ-ßà-ÿ0-9\"\\_.-")];
			// debugging directives
			BOOST_SPIRIT_DEBUG_RULE(array_);
			BOOST_SPIRIT_DEBUG_RULE(disjunction_);
			BOOST_SPIRIT_DEBUG_RULE(field_);
			BOOST_SPIRIT_DEBUG_RULE(field_name_);
			BOOST_SPIRIT_DEBUG_RULE(float_);
			BOOST_SPIRIT_DEBUG_RULE(int_);
			BOOST_SPIRIT_DEBUG_RULE(q_string_);
			BOOST_SPIRIT_DEBUG_RULE(script_);
			BOOST_SPIRIT_DEBUG_RULE(set_);
			BOOST_SPIRIT_DEBUG_RULE(set_name_);
			BOOST_SPIRIT_DEBUG_RULE(value_);
		}
		rule<ScannerT, parser_context<>, parser_tag<ScriptId> > const& start() const
		{
			return script_;
		}
	private:
		MacroGrammarRule(ArrayId)       array_;
		MacroGrammarRule(DisjunctionId) disjunction_;
		MacroGrammarRule(FieldId)       field_;
		MacroGrammarRule(FieldNameId)   field_name_;
		MacroGrammarRule(FloatId)       float_;
		MacroGrammarRule(IntId)         int_;
		MacroGrammarRule(QStringId)     q_string_;
		MacroGrammarRule(ScriptId )     script_;
		MacroGrammarRule(SetId)         set_;
		MacroGrammarRule(SetNameId)     set_name_;
		MacroGrammarRule(ValueId)       value_;
	};
};

//--------------
// macro removal
//--------------

#undef MacroGrammarRule
