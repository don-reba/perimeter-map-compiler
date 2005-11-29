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


#include "stdafx.h"

#include "script grammar.h"
#include "xml creator.h"

#include <boost\spirit\tree\parse_tree.hpp>
#include <fstream>

using namespace boost::spirit;

XmlCreator::XmlCreator()
	:doc_cursor_(NULL)
{}

XmlCreator::LoadResult XmlCreator::LoadFromFile(const char *file_name)
{
	LoadResult result;
	result.success_ = false;
	result.chars_consumed_ = 0;
	// open the file
	std::ifstream file(file_name);
	if (!file)
		return result;
	file.seekg(0, std::ios_base::end);
	const int file_size(file.tellg());
	if (-1 == file_size)
		return result;
	file.seekg(0, std::ios_base::beg);
	// read the file
	string file_string;
	file_string.resize(file_size);
	file.read(&file_string[0], file_size);
	// parse
	return LoadFromString(file_string.c_str());
}

XmlCreator::LoadResult XmlCreator::LoadFromString(const char *data)
{
	LoadResult result;
	// parse
	ScriptGrammar grammar;
	tree_parse_info<> info(ast_parse(
		data,
		grammar,
		space_p));
	result.chars_consumed_ = info.match ? info.length : 0;
	if (!info.full)
		return result;
	doc_.InsertEndChild(TiXmlDeclaration("1.0", "windows-1251", "no"));
	foreach (const tree_node_t &node, info.trees)
		ParseNode(node);
	result.success_ = true;
	return result;
}

void XmlCreator::Read(std::ostream &out) const
{
	out << doc_;
}

void XmlCreator::ParseNode(const tree_node_t &node)
{
	size_t id(node.value.id().to_long());
	switch (id)
	{
	case ScriptGrammar::ArrayId:       ParseArray      (node); break;
	case ScriptGrammar::DisjunctionId: ParseDisjunction(node); break;
	case ScriptGrammar::FieldNameId:   ParseFieldName  (node); break;
	case ScriptGrammar::FloatId:       ParseFloat      (node); break;
	case ScriptGrammar::IntId:         ParseInt        (node); break;
	case ScriptGrammar::QStringId:     ParseQString    (node); break;
	case ScriptGrammar::ScriptId:      ParseScript     (node); break;
	case ScriptGrammar::SetId:         ParseSet        (node); break;
	case ScriptGrammar::SetNameId:     ParseSetName    (node); break;
	case ScriptGrammar::ValueId:       ParseValue      (node); break;
	case ScriptGrammar::VectorId:      ParseVector     (node); break;
	}
}

void XmlCreator::ParseArray(const tree_node_t &node)
{
	StartNode("array");
	foreach (const tree_node_t &child, node.children)
		ParseNode(child);
	EndNode();
}

void XmlCreator::ParseDisjunction(const tree_node_t &node)
{
	StartNode("disjunction");
	foreach (const tree_node_t &child, node.children)
		ParseNode(child);
	EndNode();
}

void XmlCreator::ParseFieldName(const tree_node_t &node)
{
	string value(node.value.begin(), node.value.end());
	SetAttribute("name", value.c_str());
}

void XmlCreator::ParseFloat(const tree_node_t &node)
{
	string value(node.value.begin(), node.value.end());
	StartNode("float");
	SetValue(value.c_str());
	foreach (const tree_node_t &child, node.children)
		ParseNode(child);
	EndNode();
}

void XmlCreator::ParseInt(const tree_node_t &node)
{
	string value(node.value.begin(), node.value.end());
	StartNode("int");
	SetValue(value.c_str());
	foreach (const tree_node_t &child, node.children)
		ParseNode(child);
	EndNode();
}

void XmlCreator::ParseQString(const tree_node_t &node)
{
	string value(node.value.begin(), node.value.end());
	StartNode("string");
	SetValue(value.c_str());
	foreach (const tree_node_t &child, node.children)
		ParseNode(child);
	EndNode();
}

void XmlCreator::ParseScript(const tree_node_t &node)
{
	doc_cursor_ = doc_.InsertEndChild(TiXmlElement("script"))->ToElement();
	foreach (const tree_node_t &child, node.children)
		ParseNode(child);
}

void XmlCreator::ParseSet(const tree_node_t &node)
{
	StartNode("set");
	foreach (const tree_node_t &child, node.children)
		ParseNode(child);
	EndNode();
}

void XmlCreator::ParseSetName(const tree_node_t &node)
{
	string value(node.value.begin(), node.value.end());
	SetAttribute("code", value.c_str());
}

void XmlCreator::ParseValue(const tree_node_t &node)
{
	string value(node.value.begin(), node.value.end());
	StartNode("value");
	SetValue(value.c_str());
	foreach (const tree_node_t &child, node.children)
		ParseNode(child);
	EndNode();
}

void XmlCreator::ParseVector(const tree_node_t &node)
{
	StartNode("vector");
	foreach (const tree_node_t &child, node.children)
	{
		string value(child.value.begin(), child.value.end());
		if (ScriptGrammar::FieldNameId == child.value.id().to_long())
			SetAttribute("name", value.c_str());
		else
		{
			StartNode("val");
			SetValue(value.c_str());
			EndNode();
		}
	}
	EndNode();
}

void XmlCreator::StartNode(const char *name)
{
	doc_cursor_ = doc_cursor_->InsertEndChild(TiXmlElement(name))->ToElement();
}

void XmlCreator::EndNode()
{
	doc_cursor_ = doc_cursor_->Parent()->ToElement();
}

void XmlCreator::SetAttribute(const char *name, const char *value)
{
	doc_cursor_->SetAttribute(name, value);
}

void XmlCreator::SetValue(const char *value)
{
	doc_cursor_->InsertEndChild(TiXmlText(value));
}
