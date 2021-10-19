
#include "stdafx.h"

#include "script creator.h"

#include <vector>

ScriptCreator::ScriptCreator()
	:out_   (std::cout)
	,inline_(false)
{
	InitializeFMap();
}

ScriptCreator::ScriptCreator(std::ostream &out)
	:out_   (out)
	,inline_(false)
{
	InitializeFMap();
}

bool ScriptCreator::Create(const TiXmlDocument &doc)
{
	const TiXmlNode *node(doc.RootElement());
	// simple check for document validity
	{
		// go to root
		node = doc.RootElement();
		if (NULL == node)
			return false;
		// go to <set name="MissionDescriptionPrm">
		for (node = node->FirstChild(); NULL != node; node = node->NextSibling())
		{
			const TiXmlElement *e(node->ToElement());
			if (e->ValueStr() == "set" && GetName(e) == "MissionDescriptionPrm")
					break;
		}
		if (NULL == node)
			return false;
		// go to <string name="worldName">
		for (node = node->FirstChild(); NULL != node; node = node->NextSibling())
		{
			const TiXmlElement *e(node->ToElement());
			if (e->ValueStr() == "string" && GetName(e) == "worldName")
					break;
		}
		if (NULL == node)
			return false;
	}
	// process the document
	for (const TiXmlElement *node(doc.RootElement()->FirstChild()->ToElement());
		  NULL != node;
		  node = node->NextSibling()->ToElement())
	{
		ParseNode(node, 0);
		out_ << ";\n";
	}
	return true;
}

void ScriptCreator::ParseNode(const TiXmlElement *node, uint depth)
{
	const string type(node->Value());
	map_type::iterator iter(f_map_.find(type));
	if (f_map_.end() != iter)
		(this->*iter->second)(node, depth);
	else
		_RPT1(_CRT_ERROR, "%s is an unknown field node type", type.c_str());
}

void ScriptCreator::ParseArray(const TiXmlElement *node, uint depth)
{
	// count the number of children
	size_t count(0);
	for (const TiXmlNode *child(node->FirstChild()); child; child = child->NextSibling())
		++count;
	// start the array
	const string offset(Offset(depth));
	const string name(GetName(node));
	out_ << offset;
	if (!name.empty())
		out_ << name << " = ";
	out_ << "{\n";
	out_ << offset << '\t' << count << ";\n";
	// process array elements
	++depth;
	for (const TiXmlNode *child(node->FirstChild()); NULL != child; child = child->NextSibling())
	{
		ParseNode(child->ToElement(), depth);
		if (NULL != child->NextSibling())
			out_ << ',';
		out_ << '\n';
	}
	// finish the array
	out_ << offset << '}';
}

void ScriptCreator::ParseDisjunction(const TiXmlElement *node, uint depth)
{
	// begin the disjunction
	out_ << Offset(depth);
	const string name(GetName(node));
	if (!name.empty())
		out_ << name << " = ";
	// set the inline flag
	bool was_inline(inline_);
	inline_ = true;
	// process disjunction elements
	++depth;
	for (const TiXmlNode *child(node->FirstChild()); NULL != child; child = child->NextSibling())
	{
		ParseNode(child->ToElement(), depth);
		if (NULL != child->NextSibling())
			out_ << " | ";
	}
	// reset the inline flag
	inline_ = was_inline;
}

void ScriptCreator::ParseFloat(const TiXmlElement *node, uint depth)
{
	if (!inline_)
		out_ << Offset(depth);
	string name(GetName(node));
	if (!name.empty())
		out_ << name << " = ";
	out_ << GetValue(node);
}

void ScriptCreator::ParseInt(const TiXmlElement *node, uint depth)
{
	if (!inline_)
		out_ << Offset(depth);
	string name(GetName(node));
	if (!name.empty())
		out_ << name << " = ";
	out_ << GetValue(node);
}

void ScriptCreator::ParseSet(const TiXmlElement *node, uint depth)
{
	// start the set
	const string offset(Offset(depth));
	const string name(GetName(node));
	const string code(GetCode(node));
	out_ << offset;
	if (!name.empty())
		out_ << name << " = ";
	if (!code.empty())
		out_ << '"' << code << "\" ";
	out_ << "{\n";
	// process set elements
	++depth;
	for (const TiXmlNode *child(node->FirstChild()); child; child = child->NextSibling())
	{
		ParseNode(child->ToElement(), depth);
		out_ << ";\n";
	}
	// finish the set
	out_ << offset << '}';
}

void ScriptCreator::ParseString(const TiXmlElement *node, uint depth)
{
	if (!inline_)
		out_ << Offset(depth);
	string name(GetName(node));
	if (!name.empty())
		out_ << name << " = ";
	out_ << '"' << GetValue(node) << '"';
}

void ScriptCreator::ParseValue(const TiXmlElement *node, uint depth)
{
	if (!inline_)
		out_ << Offset(depth);
	string name(GetName(node));
	if (!name.empty())
		out_ << name << " = ";
	out_ << GetValue(node);
}

void ScriptCreator::ParseVector(const TiXmlElement *node, uint depth)
{
	// start the vector
	const string name(GetName(node));
	out_ << Offset(depth);
	if (!name.empty())
		out_ << name << " = ";
	out_ << "{\n";
	// process the vector's values
	{
		std::vector<const TiXmlElement*> children;
		for (const TiXmlNode *child(node->FirstChild()); child; child = child->NextSibling())
			children.push_back(child->ToElement());
		const TiXmlElement *last_child = children.back();
		children.pop_back();
		out_ << Offset(depth + 1);
		foreach (const TiXmlElement *child, children)
			out_ << GetValue(child) << ' ';
		out_ << GetValue(last_child) << '\n';
	}
	// end the vector
	out_ << Offset(depth) << '}';
}

string ScriptCreator::GetCode(const TiXmlElement *node)
{
	const char * code_string(node->Attribute("code"));
	if (NULL == code_string)
		code_string = "";
	return code_string;
}

string ScriptCreator::GetName(const TiXmlElement *node)
{
	const char * name_string(node->Attribute("name"));
	if (NULL == name_string)
		name_string = "";
	return name_string;
}

string ScriptCreator::GetValue(const TiXmlElement *node)
{
	const TiXmlNode *value_node(node->FirstChild());
	return (NULL == value_node) ? "" : value_node->ToText()->Value();
}

void ScriptCreator::InitializeFMap()
{
	f_map_["array"]       = &ScriptCreator::ParseArray;
	f_map_["disjunction"] = &ScriptCreator::ParseDisjunction;
	f_map_["float"]       = &ScriptCreator::ParseFloat;
	f_map_["int"]         = &ScriptCreator::ParseInt;
	f_map_["set"]         = &ScriptCreator::ParseSet;
	f_map_["string"]      = &ScriptCreator::ParseString;
	f_map_["value"]       = &ScriptCreator::ParseValue;
	f_map_["vector"]      = &ScriptCreator::ParseVector;
}

string ScriptCreator::Offset(uint depth)
{
	return string(depth, '\t');
}
