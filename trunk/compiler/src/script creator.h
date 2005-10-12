#pragma once

#include <map>

class ScriptCreator
{
private:
	typedef void (ScriptCreator::*f_type)(const TiXmlElement*, uint);
	typedef std::map<std::string, f_type> map_type;
public:
	ScriptCreator();
	ScriptCreator(std::ostream &out);
	void Create(const TiXmlDocument &doc);
private:
	// node parsing
	void ParseNode       (const TiXmlElement *node, uint depth);
	void ParseArray      (const TiXmlElement *node, uint depth);
	void ParseDisjunction(const TiXmlElement *node, uint depth);
	void ParseFloat      (const TiXmlElement *node, uint depth);
	void ParseInt        (const TiXmlElement *node, uint depth);
	void ParseSet        (const TiXmlElement *node, uint depth);
	void ParseString     (const TiXmlElement *node, uint depth);
	void ParseValue      (const TiXmlElement *node, uint depth);
	// XML manipulation
	string GetCode(const TiXmlElement *node);
	string GetName(const TiXmlElement *node);
	string GetValue(const TiXmlElement *node);
	// miscellaneous
	void   InitializeFMap();
	string Offset(uint depth);
private:
	map_type      f_map_;
	bool          inline_;
	std::ostream &out_;
};
