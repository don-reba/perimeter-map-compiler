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
// • Neither the name of Don Reba nor the names of his contributors may be used
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

#include "../stdafx.h"

#include "spg.h"

#include <algorithm>

#include <boost/array.hpp>

#include <loki/ScopeGuard.h>
#include <loki/static_check.h>

using namespace interop;

namespace TaskCommon
{

//-----------------
// Spg construction
//-----------------

Spg::Spg(const HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
	,SaveCallback(RS_SPG)
{}

//-------------------------
// Spg interface
// TaskResource support
//-------------------------

bool Spg::Load()
{
	if (false == doc_.LoadFile(info_.path_))
		return false;
	return true;
}

void Spg::Unload()
{
	Save();
	doc_.Clear();
}

//--------------
// Spg interface
// packing
//--------------

void Spg::Pack(TiXmlNode &node) const
{
	node.InsertEndChild(*doc_.RootElement());
}

bool Spg::Unpack(TiXmlNode &node)
{
	doc_.Clear();
	TiXmlElement *script(node.FirstChildElement());
	if (NULL == script)
		return false;
	else
		doc_.InsertEndChild(*script);
	return true;
}

//--------------
// Spg interface
// miscellaneous
//--------------

void Spg::Save() const
{
	using namespace Loki;
	SaveBegin();
	LOKI_ON_BLOCK_EXIT_OBJ(*this, &Spg::SaveEnd);
	SaveAs(info_.path_.c_str());
}

void Spg::SaveAs(LPCTSTR path) const
{
	doc_.SaveFile(info_.path_);
}

//---------------------
// Spg interface
// unit manager support
//---------------------

void Spg::SetUnits(UnitManager &unit_manager)
{
	// convert the units to XML and sort them by category
	boost::array<std::vector<TiXmlElement>, UnitManager::Category_count> unit_elements;
	UnitToXmlVisitor visitor(*this);
	foreach (const UnitManager::Unit &unit, unit_manager)
	{
		unit.data_->Accept(visitor);
		unit_elements[unit.category_].push_back(visitor.GetElement());
	}
	// create the building nodes
	{
		// get the player nodes
		boost::array<TiXmlElement*, prm_player_count> players;
		if (!GetPlayers(players))
		{
			MacroDisplayError("Units could not be saved into the SPG script.");
			return;
		}
		// set their buildings
		for (int i(0); i != prm_player_count; ++i)
			foreach (TiXmlElement unit_element, unit_elements[i])
				players[i]->InsertEndChild(unit_element);
	}
	// create the neutral building node
	{
		TiXmlElement *neutral(GetNeutral());
		if (NULL == neutral)
		{
			MacroDisplayError("Units could not be saved into the SPG script.");
			return;
		}
		foreach (TiXmlElement unit_element, unit_elements[UnitManager::Category::Neutral])
			neutral->InsertEndChild(unit_element);
	}
	// create the environment node
	{
		TiXmlElement *environment(GetNeutral());
		if (NULL == environment)
		{
			MacroDisplayError("Units could not be saved into the SPG script.");
			return;
		}
		foreach (TiXmlElement unit_element, unit_elements[UnitManager::Category::Environment])
			environment->InsertEndChild(unit_element);
	}
	// create the filth node
	{
		TiXmlElement *filth(GetNeutral());
		if (NULL == filth)
		{
			MacroDisplayError("Units could not be saved into the SPG script.");
			return;
		}
		foreach (TiXmlElement unit_element, unit_elements[UnitManager::Category::Filth])
			filth->InsertEndChild(unit_element);
	}
}

void Spg::GetUnits(UnitManager &unit_manager)
{
	unit_manager.Clear();
	// find out the offset for the code strings
	const size_t prefix_length(strlen("struct "));
	// get the building nodes
	{
		// get the player nodes
		boost::array<TiXmlElement*, prm_player_count> players;
		if (!GetPlayers(players))
		{
			MacroDisplayError("Units could not be saved into the SPG script.");
			return;
		}
		// extract the units
		for (int i(0); i != prm_player_count; ++i)
		{
			TiXmlElement *element(players[i]->FirstChildElement("set"));
			while (NULL != element)
			{
				const char * const code(element->Attribute("code"));
				if (NULL == code || strlen(code) <= prefix_length)
				{
					MacroDisplayError("Invalid code attribute. Units could not be loaded from the SPG script.");
					return;
				}
				unit_manager.CreateUnit(code + prefix_length, static_cast<UnitManager::Category>(i));
				element = element->NextSiblingElement("set");
			}
		}
	}
	// get the neutral units
	{
		// get the neutral building node
		TiXmlElement *neutral(GetNeutral());
		if (NULL == neutral)
		{
			MacroDisplayError("Units could not be saved into the SPG script.");
			return;
		}
		// extract the units
		TiXmlElement *element(neutral->FirstChildElement("set"));
		while (NULL != element)
		{
				const char * const code(element->Attribute("code"));
				if (NULL == code || strlen(code) <= prefix_length)
				{
					MacroDisplayError("Invalid code attribute. Units could not be loaded from the SPG script.");
					return;
				}
				unit_manager.CreateUnit(code + prefix_length, UnitManager::Category::Neutral);
				element = element->NextSiblingElement("set");
		}
	}
	// get the environment units
	{
		// get the neutral building node
		TiXmlElement *environment(GetEnvironment());
		if (NULL == environment)
		{
			MacroDisplayError("Units could not be saved into the SPG script.");
			return;
		}
		// extract the units
		TiXmlElement *element(environment->FirstChildElement("set"));
		while (NULL != element)
		{
				const char * const code(element->Attribute("code"));
				if (NULL == code || strlen(code) <= prefix_length)
				{
					MacroDisplayError("Invalid code attribute. Units could not be loaded from the SPG script.");
					return;
				}
				unit_manager.CreateUnit(code + prefix_length, UnitManager::Category::Environment);
				element = element->NextSiblingElement("set");
		}
	}
	// get the filth units
	{
		// get the neutral building node
		TiXmlElement *filth(GetFilth());
		if (NULL == filth)
		{
			MacroDisplayError("Units could not be saved into the SPG script.");
			return;
		}
		// extract the units
		TiXmlElement *element(filth->FirstChildElement("set"));
		while (NULL != element)
		{
				const char * const code(element->Attribute("code"));
				if (NULL == code || strlen(code) <= prefix_length)
				{
					MacroDisplayError("Invalid code attribute. Units could not be loaded from the SPG script.");
					return;
				}
				unit_manager.CreateUnit(code + prefix_length, UnitManager::Category::Environment);
				element = element->NextSiblingElement("set");
		}
	}
}

//-------------------
// Spg implementation
// XML parsing
//-------------------

bool Spg::GetPlayers(boost::array<TiXmlElement*, prm_player_count> &players)
{
	TinyXPath::xpath_processor processor(
		&doc_,
		"//set[@name=\"SavePrm\"]/array[@name=\"players\"]");
	uint player_count(processor.u_compute_xpath_node_set());
	if (prm_player_count != player_count)
	{
		MacroDisplayError("Unsupported player count.");
		return false;
	}
	for (int i = 0; i != prm_player_count; ++i)
		players[i] = processor.XNp_get_xpath_node(i)->ToElement();
	return true;
}

TiXmlElement *Spg::GetNeutral()
{
	TinyXPath::xpath_processor processor(
		&doc_,
		"//set[@name=\"SavePrm\"]/set[@name=\"nobodysBuildings\"]/array[@name=\"objects\"]");
	uint count(processor.u_compute_xpath_node_set());
	if (1 != count)
	{
		MacroDisplayError("Unsupported nobodysBuildings format.");
		return NULL;
	}
	return processor.XNp_get_xpath_node(0)->ToElement();
}

TiXmlElement *Spg::GetEnvironment()
{
	TinyXPath::xpath_processor processor(
		&doc_,
		"//set[@name=\"SavePrm\"]/set[@name=\"environment\"]/array[@name=\"objects\"]");
	uint count(processor.u_compute_xpath_node_set());
	if (1 != count)
	{
		MacroDisplayError("Unsupported environment format.");
		return NULL;
	}
	return processor.XNp_get_xpath_node(0)->ToElement();
}

TiXmlElement *Spg::GetFilth()
{
	TinyXPath::xpath_processor processor(
		&doc_,
		"//set[@name=\"SavePrm\"]/set[@name=\"filth\"]/array[@name=\"objects\"]");
	uint count(processor.u_compute_xpath_node_set());
	if (1 != count)
	{
		MacroDisplayError("Unsupported filth format.");
		return NULL;
	}
	return processor.XNp_get_xpath_node(0)->ToElement();
}

//-------------------
// Spg implementation
// XML output
//-------------------

void Spg::SetElementCode(TiXmlElement &element, const char *code) const
{
	if (NULL != code)
		element.SetAttribute("code", code);
}

void Spg::SetElementName(TiXmlElement &element, const char *name) const
{
	if (NULL != name)
		element.SetAttribute("name", name);
}

TiXmlElement Spg::ToXml(const char *contents, const char *name) const
{
	TiXmlElement root("string");
	SetElementName(root, name);
	root.InsertEndChild(TiXmlText(contents));
	return root;
}

TiXmlElement Spg::ToXml(bool contents, const char *name) const
{
	TiXmlElement root("value");
	SetElementName(root, name);
	root.InsertEndChild(TiXmlText(contents ? "true" : "false"));
	return root;
}

TiXmlElement Spg::ToXml(int contents, const char *name) const
{
	TiXmlElement root("int");
	SetElementName(root, name);
	char text[33] = { 0 };
	itoa(contents, text, 10);
	root.InsertEndChild(TiXmlText(text));
	return root;
}

TiXmlElement Spg::ToXml(float contents, const char *name) const
{
	char text[_CVTBUFSIZE] = { 0 };
	gcvt(contents, _CVTBUFSIZE - 2, text);
	TiXmlElement root("float");
	SetElementName(root, name);
	root.InsertEndChild(TiXmlText(text));
	return root;
}

TiXmlElement Spg::ToXml(const Atoms &contents, const char *name) const
{
	TiXmlElement root("array");
	SetElementName(root, name);
	foreach (int e, contents)
		root.InsertEndChild(ToXml(e));
	return root;
}

TiXmlElement Spg::ToXml(AttributeID contents, const char *name) const
{
	TiXmlElement root("value");
	SetElementName(root, name);
	root.InsertEndChild(TiXmlText(AttributeIDNames[contents]));
	return root;
}

TiXmlElement Spg::ToXml(BuildingStatus contents, const char *name) const
{
	TiXmlElement root("value");
	SetElementName(root, name);
	root.InsertEndChild(TiXmlText(BuildingStatusNames[contents]));
	return root;
}

TiXmlElement Spg::ToXml(FilthAttackType contents, const char *name) const
{
	TiXmlElement root("value");
	SetElementName(root, name);
	root.InsertEndChild(TiXmlText(FilthAttackTypeNames[contents]));
	return root;
}

TiXmlElement Spg::ToXml(FilthType contents, const char *name) const
{
	TiXmlElement root("value");
	SetElementName(root, name);
	root.InsertEndChild(TiXmlText(FilthTypeNames[contents]));
	return root;
}

TiXmlElement Spg::ToXml(NatureFlag contents, const char *name) const
{
	TiXmlElement root("value");
	SetElementName(root, name);
	root.InsertEndChild(TiXmlText(NatureFlagNames[contents]));
	return root;
}

TiXmlElement Spg::ToXml(const PositionList &contents, const char *name) const
{
	TiXmlElement root("array");
	SetElementName(root, name);
	foreach (const Position &position, contents)
		root.InsertEndChild(ToXml(position, "position"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, string &target) const
{
	target = element.GetText();
}

void Spg::FromXml(const TiXmlElement &element, int &target) const
{
	target = atoi(element.GetText());
}

void Spg::FromXml(const TiXmlElement &element, bool &target) const
{
	const char * const text = element.GetText();
	target = (0 == stricmp(text, "true"));
}

void Spg::FromXml(const TiXmlElement &element, float &target) const
{
	target = (float)atof(element.GetText());
}

void Spg::FromXml(const TiXmlElement &element, Atoms &target) const
{
	const TiXmlElement *child(element.FirstChildElement());
	foreach (int &atom, target)
	{
		atom  = atoi(child->GetText());
		child = child->NextSiblingElement();
	}
}

void Spg::FromXml(const TiXmlElement &element, AttributeID &target) const
{
	target = static_cast<AttributeID>(string_index(
		AttributeIDNames,
		MacroArraySize(AttributeIDNames),
		element.GetText()));
}

void Spg::FromXml(const TiXmlElement &element, BuildingStatus &target) const
{
	target = static_cast<BuildingStatus>(string_index(
		BuildingStatusNames,
		MacroArraySize(BuildingStatusNames),
		element.GetText()));
}

void Spg::FromXml(const TiXmlElement &element, FilthAttackType &target) const
{
	target = static_cast<FilthAttackType>(string_index(
		FilthAttackTypeNames,
		MacroArraySize(FilthAttackTypeNames),
		element.GetText()));
}

void Spg::FromXml(const TiXmlElement &element, FilthType &target) const
{
	target = static_cast<FilthType>(string_index(
		FilthTypeNames,
		MacroArraySize(FilthTypeNames),
		element.GetText()));
}

void Spg::FromXml(const TiXmlElement &element, NatureFlag &target) const
{
	target = static_cast<NatureFlag>(string_index(
		NatureFlagNames,
		MacroArraySize(NatureFlagNames),
		element.GetText()));
}

void Spg::FromXml(const TiXmlElement &element, PositionList &target) const
{
	target.clear();
	const TiXmlElement *child(element.FirstChildElement());
	while (0 != child)
	{
		Position position;
		FromXml(*child, position);
		target.push_back(position);
		child = child->NextSiblingElement("set");
	}
}

TiXmlElement Spg::ToXml(const DamageMolecula &contents, const char *name) const
{
	TiXmlElement root("set");
	SetElementName(root, name);
	root.InsertEndChild(ToXml(contents.isAlive,      "isAlive"));
	root.InsertEndChild(ToXml(contents.elementsDead, "elements"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, DamageMolecula &target) const
{
	FromXml(*element.FirstChildElement("isAlive"),      target.isAlive);
	FromXml(*element.FirstChildElement("elementsDead"), target.elementsDead);
}

TiXmlElement Spg::ToXml(const Orientation &contents, const char *name) const
{
	TiXmlElement root("set");
	SetElementName(root, name);
	root.InsertEndChild(ToXml(contents.s, "s"));
	root.InsertEndChild(ToXml(contents.x, "x"));
	root.InsertEndChild(ToXml(contents.y, "y"));
	root.InsertEndChild(ToXml(contents.z, "z"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, Orientation &target) const
{
	FromXml(*element.FirstChildElement("s"), target.s);
	FromXml(*element.FirstChildElement("x"), target.x);
	FromXml(*element.FirstChildElement("y"), target.y);
	FromXml(*element.FirstChildElement("z"), target.z);
}

TiXmlElement Spg::ToXml(const Position &contents, const char *name) const
{
	TiXmlElement root("set");
	SetElementName(root, name);
	root.InsertEndChild(ToXml(contents.x, "x"));
	root.InsertEndChild(ToXml(contents.y, "y"));
	root.InsertEndChild(ToXml(contents.z, "z"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, Position &target) const
{
	FromXml(*element.FirstChildElement("x"), target.x);
	FromXml(*element.FirstChildElement("y"), target.y);
	FromXml(*element.FirstChildElement("z"), target.z);
}

TiXmlElement Spg::ToXml(const LocalPosition &contents, const char *name) const
{
	TiXmlElement root("set");
	SetElementName(root, name);
	root.InsertEndChild(ToXml(contents.x, "x"));
	root.InsertEndChild(ToXml(contents.y, "y"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, LocalPosition &target) const
{
	FromXml(*element.FirstChildElement("x"), target.x);
	FromXml(*element.FirstChildElement("y"), target.y);
}

TiXmlElement Spg::ToXml(const TargetUnit &contents, const char *name) const
{
	TiXmlElement root("set");
	SetElementName(root, name);
	root.InsertEndChild(ToXml(contents.unitID,   "unitID"));
	root.InsertEndChild(ToXml(contents.playerID, "playerID"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, TargetUnit &target) const
{
	FromXml(*element.FirstChildElement("unitID"),   target.unitID);
	FromXml(*element.FirstChildElement("playerID"), target.playerID);
}

TiXmlElement Spg::ToXml(const SaveUnitData &contents, const char *name) const
{
	TiXmlElement root("set");
	SetElementName(root, name);
	SetElementCode(root, "struct SaveUnitData");
	root.InsertEndChild(ToXml(contents.unitID,         "unitID"));
	root.InsertEndChild(ToXml(contents.attributeID,    "attributeID"));
	root.InsertEndChild(ToXml(contents.position,       "attributeID"));
	root.InsertEndChild(ToXml(contents.position,       "position"));
	root.InsertEndChild(ToXml(contents.orientaion,     "orientaion"));
	root.InsertEndChild(ToXml(contents.radius,         "radius"));
	root.InsertEndChild(ToXml(contents.label.c_str(),  "label"));
	root.InsertEndChild(ToXml(contents.damageMolecula, "damageMolecula"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitData &target) const
{
	FromXml(*element.FirstChildElement("unitID"),         target.unitID);
	FromXml(*element.FirstChildElement("attributeID"),    target.attributeID);
	FromXml(*element.FirstChildElement("position"),       target.position);
	FromXml(*element.FirstChildElement("orientation"),    target.orientaion);
	FromXml(*element.FirstChildElement("radius"),         target.radius);
	FromXml(*element.FirstChildElement("label"),          target.label);
	FromXml(*element.FirstChildElement("damageMolecula"), target.damageMolecula);
}

TiXmlElement Spg::ToXml(const SaveGeo &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitData *>(&contents), name));
	SetElementCode(root, "struct SaveGeo");
	root.InsertEndChild(ToXml(contents.sleep,              "sleep"));
	root.InsertEndChild(ToXml(contents.firstSleepTime,     "firstSleepTime"));
	root.InsertEndChild(ToXml(contents.sleepPeriod,        "sleepPeriod"));
	root.InsertEndChild(ToXml(contents.deltaSleepPeriod,   "deltaSleepPeriod"));
	root.InsertEndChild(ToXml(contents.attackPeriond,      "attackPeriond"));
	root.InsertEndChild(ToXml(contents.deltaAttackPeriond, "deltaAttackPeriond"));
	root.InsertEndChild(ToXml(contents.activatingUnit, AttributeIDNames, "activatingUnit"));
	root.InsertEndChild(ToXml(contents.activatingDistance, "activatingDistance"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveGeo &target) const
{
	FromXml(*element.FirstChildElement("sleep"), target.sleep);
	FromXml(*element.FirstChildElement("firstSleepTime"),     target.firstSleepTime);
	FromXml(*element.FirstChildElement("sleepPeriod"),        target.sleepPeriod);
	FromXml(*element.FirstChildElement("deltaSleepPeriod"),   target.deltaSleepPeriod);
	FromXml(*element.FirstChildElement("attackPeriond"),      target.attackPeriond);
	FromXml(*element.FirstChildElement("deltaAttackPeriond"), target.deltaAttackPeriond);
	FromXml(*element.FirstChildElement("activatingUnit"), AttributeIDNames, target.activatingUnit);
	FromXml(*element.FirstChildElement("activatingDistance"), target.activatingDistance);
}

TiXmlElement Spg::ToXml(const SaveGeoBreak &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveGeo *>(&contents), name));
	root.InsertEndChild(ToXml(contents.geoRadius, "geoRadius"));
	root.InsertEndChild(ToXml(contents.num_break, "num_break"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveGeoBreak &target) const
{
	FromXml(*element.FirstChildElement("geoRadius"), target.geoRadius);
	FromXml(*element.FirstChildElement("num_break"), target.num_break);
}

TiXmlElement Spg::ToXml(const SaveGeoFault &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveGeo *>(&contents), name));
	root.InsertEndChild(ToXml(contents.length, "length"));
	root.InsertEndChild(ToXml(contents.angle,  "angle"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveGeoFault &target) const
{
	FromXml(*element.FirstChildElement("length"), target.length);
	FromXml(*element.FirstChildElement("angle"),  target.angle);
}

TiXmlElement Spg::ToXml(const SaveUnitLegionaryData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitLegionaryData");
	root.InsertEndChild(ToXml(contents.basementInstalled,   "basementInstalled"));
	root.InsertEndChild(ToXml(contents.accumulatedEnergy,   "accumulatedEnergy"));
	root.InsertEndChild(ToXml(contents.zeroLayerCounter,    "zeroLayerCounter"));
	root.InsertEndChild(ToXml(contents.weaponChargeLevel,   "weaponChargeLevel"));
	root.InsertEndChild(ToXml(contents.wayPoints,           "wayPoints"));
	root.InsertEndChild(ToXml(contents.weapon,              "weapon"));
	root.InsertEndChild(ToXml(contents.transportedSoldiers, "transportedSoldiers"));
	root.InsertEndChild(ToXml(contents.transportedOfficers, "transportedOfficers"));
	root.InsertEndChild(ToXml(contents.transportedTechnics, "transportedTechnics"));
	root.InsertEndChild(ToXml(contents.flyingMode,          "flyingMode"));
	root.InsertEndChild(ToXml(contents.diggingMode,         "diggingMode"));
	root.InsertEndChild(ToXml(contents.inSquad,             "inSquad"));
	root.InsertEndChild(ToXml(contents.localPosition,       "localPosition"));
	root.InsertEndChild(ToXml(contents.localPositionValid,  "localPositionValid"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitLegionaryData &target) const
{
	FromXml(*element.FirstChildElement("basementInstalled"),   target.basementInstalled);
	FromXml(*element.FirstChildElement("accumulatedEnergy"),   target.accumulatedEnergy);
	FromXml(*element.FirstChildElement("zeroLayerCounter"),    target.zeroLayerCounter);
	FromXml(*element.FirstChildElement("weaponChargeLevel"),   target.weaponChargeLevel);
	FromXml(*element.FirstChildElement("wayPoints"),           target.wayPoints);
	FromXml(*element.FirstChildElement("weapon"),              target.weapon);
	FromXml(*element.FirstChildElement("transportedSoldiers"), target.transportedSoldiers);
	FromXml(*element.FirstChildElement("transportedOfficers"), target.transportedOfficers);
	FromXml(*element.FirstChildElement("transportedTechnics"), target.transportedTechnics);
	FromXml(*element.FirstChildElement("flyingMode"),          target.flyingMode);
	FromXml(*element.FirstChildElement("diggingMode"),         target.diggingMode);
	FromXml(*element.FirstChildElement("inSquad"),             target.inSquad);
	FromXml(*element.FirstChildElement("localPosition"),       target.localPosition);
	FromXml(*element.FirstChildElement("localPositionValid"),  target.localPositionValid);
}

TiXmlElement Spg::ToXml(const SaveUnitSquadData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitSquadData");
	root.InsertEndChild(ToXml(contents.stablePosition,  "stablePosition"));
	root.InsertEndChild(ToXml(contents.currentMutation, "currentMutation"));
	root.InsertEndChild(ToXml(contents.curvatureRadius, "curvatureRadius"));
	root.InsertEndChild(ToXml(contents.squadMemebers,   "squadMemebers"));
	root.InsertEndChild(ToXml(contents.wayPoints,       "wayPoints"));
	root.InsertEndChild(ToXml(contents.patrolPoints,    "patrolPoints"));
	root.InsertEndChild(ToXml(contents.patrolIndex,     "patrolIndex"));
	root.InsertEndChild(ToXml(contents.attackPoints,    "attackPoints"));
	root.InsertEndChild(ToXml(contents.squadToFollow,   "squadToFollow"));
	root.InsertEndChild(ToXml(contents.offensiveMode,   "offensiveMode"));
	root.InsertEndChild(ToXml(contents.atomsRequested,  "atomsRequested"));
	root.InsertEndChild(ToXml(contents.atomsPaused,     "atomsPaused"));
	root.InsertEndChild(ToXml(contents.mutationEnergy,  "mutationEnergy"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitSquadData &target) const
{
	FromXml(*element.FirstChildElement("stablePosition"),  target.stablePosition);
	FromXml(*element.FirstChildElement("currentMutation"), target.currentMutation);
	FromXml(*element.FirstChildElement("curvatureRadius"), target.curvatureRadius);
	FromXml(*element.FirstChildElement("squadMemebers"),   target.squadMemebers);
	FromXml(*element.FirstChildElement("wayPoints"),       target.wayPoints);
	FromXml(*element.FirstChildElement("patrolPoints"),    target.patrolPoints);
	FromXml(*element.FirstChildElement("patrolIndex"),     target.patrolIndex);
	FromXml(*element.FirstChildElement("attackPoints"),    target.attackPoints);
	FromXml(*element.FirstChildElement("squadToFollow"),   target.squadToFollow);
	FromXml(*element.FirstChildElement("offensiveMode"),   target.offensiveMode);
	FromXml(*element.FirstChildElement("atomsRequested"),  target.atomsRequested);
	FromXml(*element.FirstChildElement("atomsPaused"),     target.atomsPaused);
	FromXml(*element.FirstChildElement("mutationEnergy"),  target.mutationEnergy);
}

TiXmlElement Spg::ToXml(const SaveUnitBuildingData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitBuildingData");
	root.InsertEndChild(ToXml(contents.basementInstalled, "basementInstalled"));
	root.InsertEndChild(ToXml(contents.accumulatedEnergy, "accumulatedEnergy"));
	root.InsertEndChild(ToXml(contents.zeroLayerCounter,  "zeroLayerCounter"));
	root.InsertEndChild(ToXml(contents.weaponChargeLevel, "weaponChargeLevel"));
	root.InsertEndChild(ToXml(contents.wayPoints,         "wayPoints"));
	root.InsertEndChild(ToXml(contents.weapon,            "weapon"));
	root.InsertEndChild(ToXml(contents.buildingStatusBV,  BuildingStatusNames,   "buildingStatusBV"));
	root.InsertEndChild(ToXml(contents.fireCount,         "fireCount"));
	root.InsertEndChild(ToXml(contents.visible,           "visible"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitBuildingData &target) const
{
	FromXml(*element.FirstChildElement("basementInstalled"), target.basementInstalled);
	FromXml(*element.FirstChildElement("accumulatedEnergy"), target.accumulatedEnergy);
	FromXml(*element.FirstChildElement("zeroLayerCounter"),  target.zeroLayerCounter);
	FromXml(*element.FirstChildElement("weaponChargeLevel"), target.weaponChargeLevel);
	FromXml(*element.FirstChildElement("wayPoints"),         target.wayPoints);
	FromXml(*element.FirstChildElement("weapon"),            target.weapon);
	FromXml(*element.FirstChildElement("buildingStatusBV"),  BuildingStatusNames, target.buildingStatusBV);
	FromXml(*element.FirstChildElement("fireCount"),         target.fireCount);
	FromXml(*element.FirstChildElement("visible"),           target.visible);
}

TiXmlElement Spg::ToXml(const SaveUnitCommandCenterData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitCommandCenterData");
	root.InsertEndChild(ToXml(contents.basementInstalled, "basementInstalled"));
	root.InsertEndChild(ToXml(contents.accumulatedEnergy, "accumulatedEnergy"));
	root.InsertEndChild(ToXml(contents.zeroLayerCounter,  "zeroLayerCounter"));
	root.InsertEndChild(ToXml(contents.wayPoints,         "wayPoints"));
	root.InsertEndChild(ToXml(contents.buildingStatusBV, BuildingStatusNames, "buildingStatusBV"));
	root.InsertEndChild(ToXml(contents.fireCount, "fireCount"));
	root.InsertEndChild(ToXml(contents.visible,   "visible"));
	root.InsertEndChild(ToXml(contents.squad,     "squad"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitCommandCenterData &target) const
{
	FromXml(*element.FirstChildElement("basementInstalled"), target.basementInstalled);
	FromXml(*element.FirstChildElement("accumulatedEnergy"), target.accumulatedEnergy);
	FromXml(*element.FirstChildElement("zeroLayerCounter"),  target.zeroLayerCounter);
	FromXml(*element.FirstChildElement("wayPoints"),         target.wayPoints);
	FromXml(*element.FirstChildElement("buildingStatusBV"), BuildingStatusNames, target.buildingStatusBV);
	FromXml(*element.FirstChildElement("fireCount"), target.fireCount);
	FromXml(*element.FirstChildElement("visible"),   target.visible);
	FromXml(*element.FirstChildElement("squad"),     target.squad);
}

TiXmlElement Spg::ToXml(const SaveUnitFrameData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitFrameData");
	root.InsertEndChild(ToXml(contents.basementInstalled, "basementInstalled"));
	root.InsertEndChild(ToXml(contents.accumulatedEnergy, "accumulatedEnergy"));
	root.InsertEndChild(ToXml(contents.zeroLayerCounter,  "zeroLayerCounter"));
	root.InsertEndChild(ToXml(contents.weaponChargeLevel, "weaponChargeLevel"));
	root.InsertEndChild(ToXml(contents.wayPoints,         "wayPoints"));
	root.InsertEndChild(ToXml(contents.weapon,            "weapon"));
	root.InsertEndChild(ToXml(contents.attached,          "attached"));
	root.InsertEndChild(ToXml(contents.attaching,         "attaching"));
	root.InsertEndChild(ToXml(contents.powered,           "powered"));
	root.InsertEndChild(ToXml(contents.spiralLevel,       "spiralLevel"));
	root.InsertEndChild(ToXml(contents.squad,             "squad"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitFrameData &target) const
{
	FromXml(*element.FirstChildElement("basementInstalled"), target.basementInstalled);
	FromXml(*element.FirstChildElement("accumulatedEnergy"), target.accumulatedEnergy);
	FromXml(*element.FirstChildElement("zeroLayerCounter"),  target.zeroLayerCounter);
	FromXml(*element.FirstChildElement("weaponChargeLevel"), target.weaponChargeLevel);
	FromXml(*element.FirstChildElement("wayPoints"),         target.wayPoints);
	FromXml(*element.FirstChildElement("weapon"),            target.weapon);
	FromXml(*element.FirstChildElement("attached"),          target.attached);
	FromXml(*element.FirstChildElement("attaching"),         target.attaching);
	FromXml(*element.FirstChildElement("powered"),           target.powered);
	FromXml(*element.FirstChildElement("spiralLevel"),       target.spiralLevel);
	FromXml(*element.FirstChildElement("squad"),             target.spiralLevel);
}

TiXmlElement Spg::ToXml(const SaveUnitFilthData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitFilthData");
	root.InsertEndChild(ToXml(contents.filthType,          "filthType"));
	root.InsertEndChild(ToXml(contents.attackDirection,    "attackDirection"));
	root.InsertEndChild(ToXml(contents.sleep,              "sleep"));
	root.InsertEndChild(ToXml(contents.firstSleepTime,     "firstSleepTime"));
	root.InsertEndChild(ToXml(contents.sleepPeriod,        "sleepPeriod"));
	root.InsertEndChild(ToXml(contents.deltaSleepPeriod,   "deltaSleepPeriod"));
	root.InsertEndChild(ToXml(contents.attackPeriond,      "attackPeriond"));
	root.InsertEndChild(ToXml(contents.deltaAttackPeriond, "deltaAttackPeriond"));
	root.InsertEndChild(ToXml(contents.creatureNum,        "creatureNum"));
	root.InsertEndChild(ToXml(contents.activatingUnit, AttributeIDNames, "activatingUnit"));
	root.InsertEndChild(ToXml(contents.activatingDistance,   "activatingDistance"));
	root.InsertEndChild(ToXml(contents.attack_player,        "attack_player"));
	root.InsertEndChild(ToXml(contents.initial_geoprocess,   "initial_geoprocess"));
	root.InsertEndChild(ToXml(contents.killTimer,            "killTimer"));
	root.InsertEndChild(ToXml(contents.sleep_timer,          "sleep_timer"));
	root.InsertEndChild(ToXml(contents.create_first,         "create_first"));
	root.InsertEndChild(ToXml(contents.hole_position,        "hole_position"));
	root.InsertEndChild(ToXml(contents.hole_position_inited, "hole_position_inited"));
	root.InsertEndChild(ToXml(contents.kill_of_end,          "kill_of_end"));
	root.InsertEndChild(ToXml(contents.swarmList,            "swarmList"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitFilthData &target) const
{
	FromXml(*element.FirstChildElement("filthType"),          target.filthType);
	FromXml(*element.FirstChildElement("attackDirection"),    target.attackDirection);
	FromXml(*element.FirstChildElement("sleep"),              target.sleep);
	FromXml(*element.FirstChildElement("firstSleepTime"),     target.firstSleepTime);
	FromXml(*element.FirstChildElement("sleepPeriod"),        target.sleepPeriod);
	FromXml(*element.FirstChildElement("deltaSleepPeriod"),   target.deltaSleepPeriod);
	FromXml(*element.FirstChildElement("attackPeriond"),      target.attackPeriond);
	FromXml(*element.FirstChildElement("deltaAttackPeriond"), target.deltaAttackPeriond);
	FromXml(*element.FirstChildElement("creatureNum"),        target.creatureNum);
	FromXml(*element.FirstChildElement("activatingUnit"), AttributeIDNames, target.activatingUnit);
	FromXml(*element.FirstChildElement("activatingDistance"),   target.activatingDistance);
	FromXml(*element.FirstChildElement("attack_player"),        target.attack_player);
	FromXml(*element.FirstChildElement("initial_geoprocess"),   target.initial_geoprocess);
	FromXml(*element.FirstChildElement("killTimer"),            target.killTimer);
	FromXml(*element.FirstChildElement("sleep_timer"),          target.sleep_timer);
	FromXml(*element.FirstChildElement("create_first"),         target.create_first);
	FromXml(*element.FirstChildElement("hole_position"),        target.hole_position);
	FromXml(*element.FirstChildElement("hole_position_inited"), target.hole_position_inited);
	FromXml(*element.FirstChildElement("kill_of_end"),          target.kill_of_end);
	FromXml(*element.FirstChildElement("swarmList"),            target.swarmList);
}

TiXmlElement Spg::ToXml(const SaveUnitNatureData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitNatureData");
	root.InsertEndChild(ToXml(contents.modelName.c_str(), "modelName"));
	root.InsertEndChild(ToXml(contents.visible,           "visible"));
	root.InsertEndChild(ToXml(contents.natureFlag, NatureFlagNames, "natureFlag"));
	root.InsertEndChild(ToXml(contents.chainName.c_str(), "chainName"));
	root.InsertEndChild(ToXml(contents.chainPhase,        "chainPhase"));
	root.InsertEndChild(ToXml(contents.chainPeriod,       "chainPeriod"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitNatureData &target) const
{
	FromXml(*element.FirstChildElement("modelName"), target.modelName);
	FromXml(*element.FirstChildElement("visible"),   target.visible);
	FromXml(*element.FirstChildElement("natureFlag"), NatureFlagNames, target.natureFlag);
	FromXml(*element.FirstChildElement("chainName"),   target.chainName);
	FromXml(*element.FirstChildElement("chainPhase"),  target.chainPhase);
	FromXml(*element.FirstChildElement("chainPeriod"), target.chainPeriod);
}

TiXmlElement Spg::ToXml(const SaveUnitFrameChildData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitFrameChildData");
	root.InsertEndChild(ToXml(contents.basementInstalled, "basementInstalled"));
	root.InsertEndChild(ToXml(contents.accumulatedEnergy, "accumulatedEnergy"));
	root.InsertEndChild(ToXml(contents.zeroLayerCounter,  "zeroLayerCounter"));
	root.InsertEndChild(ToXml(contents.weaponChargeLevel, "weaponChargeLevel"));
	root.InsertEndChild(ToXml(contents.wayPoints,         "wayPoints"));
	root.InsertEndChild(ToXml(contents.weapon,            "weapon"));
	root.InsertEndChild(ToXml(contents.dockReadyStatus,   "dockReadyStatus"));
	root.InsertEndChild(ToXml(contents.alarmStatus,       "alarmStatus"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitFrameChildData &target) const
{
	FromXml(*element.FirstChildElement("basementInstalled"), target.basementInstalled);
	FromXml(*element.FirstChildElement("accumulatedEnergy"), target.accumulatedEnergy);
	FromXml(*element.FirstChildElement("zeroLayerCounter"),  target.zeroLayerCounter);
	FromXml(*element.FirstChildElement("weaponChargeLevel"), target.weaponChargeLevel);
	FromXml(*element.FirstChildElement("wayPoints"),         target.wayPoints);
	FromXml(*element.FirstChildElement("weapon"),            target.weapon);
	FromXml(*element.FirstChildElement("dockReadyStatus"),   target.dockReadyStatus);
	FromXml(*element.FirstChildElement("alarmStatus"),       target.alarmStatus);
}

TiXmlElement Spg::ToXml(const SaveUnitBuildingMilitaryData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitBuildingData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitBuildingMilitaryData");
	root.InsertEndChild(ToXml(contents.attackTarget,       "attackTarget"));
	root.InsertEndChild(ToXml(contents.lastAttackTarget,   "lastAttackTarget"));
	root.InsertEndChild(ToXml(contents.manualAttackTarget, "manualAttackTarget"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitBuildingMilitaryData &target) const
{
	FromXml(*element.FirstChildElement("attackTarget"),       target.attackTarget);
	FromXml(*element.FirstChildElement("lastAttackTarget"),   target.lastAttackTarget);
	FromXml(*element.FirstChildElement("manualAttackTarget"), target.manualAttackTarget);
}

TiXmlElement Spg::ToXml(const SaveUnitCorridorAlphaData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitBuildingData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitCorridorAlphaData");
	root.InsertEndChild(ToXml(contents.free,       "free"));
	root.InsertEndChild(ToXml(contents.passTime,   "passTime"));
	root.InsertEndChild(ToXml(contents.timeOffset, "timeOffset"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitCorridorAlphaData &target) const
{
	FromXml(*element.FirstChildElement("free"),       target.free);
	FromXml(*element.FirstChildElement("passTime"),   target.passTime);
	FromXml(*element.FirstChildElement("timeOffset"), target.timeOffset);
}

TiXmlElement Spg::ToXml(const SaveUnitCorridorOmegaData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitBuildingData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitCorridorOmegaData");
	root.InsertEndChild(ToXml(contents.upgraded, "upgraded"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitCorridorOmegaData &target) const
{
	FromXml(*element.FirstChildElement("upgraded"), target.upgraded);
}

TiXmlElement Spg::ToXml(const SaveUnitProtectorData &contents, const char *name) const
{
	TiXmlElement root(ToXml(*static_cast<const SaveUnitBuildingData *>(&contents), name));
	SetElementCode(root, "struct SaveUnitProtectorData");
	root.InsertEndChild(ToXml(contents.monksNumber,      "monksNumber"));
	root.InsertEndChild(ToXml(contents.fieldState,       "fieldState"));
	root.InsertEndChild(ToXml(contents.enableScharge,    "enableScharge"));
	root.InsertEndChild(ToXml(contents.startWhenCharged, "startWhenCharged"));
	return root;
}

void Spg::FromXml(const TiXmlElement &element, SaveUnitProtectorData &target) const
{
	FromXml(*element.FirstChildElement("monksNumber"),      target.monksNumber);
	FromXml(*element.FirstChildElement("fieldState"),       target.fieldState);
	FromXml(*element.FirstChildElement("enableScharge"),    target.enableScharge);
	FromXml(*element.FirstChildElement("startWhenCharged"), target.startWhenCharged);
}

} // namespace TaskCommon

