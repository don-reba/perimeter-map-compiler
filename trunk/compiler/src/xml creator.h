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

#include <iterator>

#include <boost\spirit\tree\ast.hpp>

class XmlCreator
{
public:
	struct LoadResult
	{
		int  chars_consumed_;
		bool success_;
	};
private:
	typedef const char *                      iter_t;
	typedef boost::spirit::tree_match<iter_t> match_t;
	typedef match_t::tree_iterator            tree_iter_t;
	typedef match_t::node_t                   tree_node_t;
public:
	XmlCreator();
	LoadResult LoadFromFile(const char *file_name);
	LoadResult LoadFromString(const char *data);
	void       Read(std::ostream &out) const;
private:
	void Create(const tree_iter_t &iter);
private:
	// node parsing
	void ParseNode       (const tree_node_t &node);
	void ParseArray      (const tree_node_t &node);
	void ParseDisjunction(const tree_node_t &node);
	void ParseFieldName  (const tree_node_t &node);
	void ParseFloat      (const tree_node_t &node);
	void ParseInt        (const tree_node_t &node);
	void ParseQString    (const tree_node_t &node);
	void ParseScript     (const tree_node_t &node);
	void ParseSet        (const tree_node_t &node);
	void ParseSetName    (const tree_node_t &node);
	void ParseValue      (const tree_node_t &node);
	void ParseVector     (const tree_node_t &node);
	// XML manipulation
	void StartNode(const char *name);
	void EndNode();
	void SetAttribute(const char *name, const char *value);
	void SetValue(const char *value);
private:
	TiXmlDocument  doc_;
	TiXmlElement  *doc_cursor_;
};
