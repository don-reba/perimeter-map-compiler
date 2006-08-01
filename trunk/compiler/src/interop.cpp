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


#include "stdafx.h"

#include "interop.h"

#include <algorithm>


namespace interop
{

	//-------------------------------
	// XmlSerializable implementation
	//-------------------------------

	TiXmlElement XmlSerializable::CreateNamedElement(const char *value, const char *name) const
	{
		TiXmlElement element(value);
		if (NULL != name)
			element.SetAttribute("name", name);
		return element;
	}

	TiXmlElement XmlSerializable::BasicToXml(const char *contents, const char *name) const
	{
		TiXmlElement element(CreateNamedElement("string", name));
		if (NULL != name)
			element.SetAttribute("name", name);
		element.InsertEndChild(TiXmlText(contents));
		return element;
	}

	TiXmlElement XmlSerializable::BasicToXml(bool contents, const char *name) const
	{
		TiXmlElement element(CreateNamedElement("value", name));
		if (NULL != name)
			element.SetAttribute("name", name);
		element.InsertEndChild(TiXmlText(contents ? "true" : "false"));
		return element;
	}

	TiXmlElement XmlSerializable::BasicToXml(int contents, const char *name) const
	{
		char text[33] = { 0 };
		itoa(contents, text, 10);
		TiXmlElement element(CreateNamedElement("int", name));
		if (NULL != name)
			element.SetAttribute("name", name);
		element.InsertEndChild(TiXmlText(text));
		return element;
	}

	TiXmlElement XmlSerializable::BasicToXml(float contents, const char *name) const
	{
		char text[_CVTBUFSIZE] = { 0 };
		gcvt(contents, _CVTBUFSIZE - 2, text);
		TiXmlElement element(CreateNamedElement("float", name));
		if (NULL != name)
			element.SetAttribute("name", name);
		element.InsertEndChild(TiXmlText(text));
		return element;
	}

	TiXmlElement XmlSerializable::BasicToXml(const Atoms &contents, const char *name) const
	{
		TiXmlElement root(CreateNamedElement("array", name));
		foreach (int e, contents)
			root.InsertEndChild(BasicToXml(e));
		return root;
	}

	TiXmlElement XmlSerializable::BasicToXml(AttributeID contents, const char *name) const
	{
		TiXmlElement root(CreateNamedElement("value", name));
		root.InsertEndChild(TiXmlText(AttributeIDNames[contents]));
		return root;
	}

	TiXmlElement XmlSerializable::BasicToXml(BuildingStatus contents, const char *name) const
	{
		TiXmlElement root(CreateNamedElement("value", name));
		root.InsertEndChild(TiXmlText(BuildingStatusNames[contents]));
		return root;
	}

	TiXmlElement XmlSerializable::BasicToXml(FilthAttackType contents, const char *name) const
	{
		TiXmlElement root(CreateNamedElement("value", name));
		root.InsertEndChild(TiXmlText(FilthAttackTypeNames[contents]));
		return root;
	}

	TiXmlElement XmlSerializable::BasicToXml(FilthType contents, const char *name) const
	{
		TiXmlElement root(CreateNamedElement("value", name));
		root.InsertEndChild(TiXmlText(FilthTypeNames[contents]));
		return root;
	}

	TiXmlElement XmlSerializable::BasicToXml(NatureFlag contents, const char *name) const
	{
		TiXmlElement root(CreateNamedElement("value", name));
		root.InsertEndChild(TiXmlText(NatureFlagNames[contents]));
		return root;
	}

	TiXmlElement XmlSerializable::BasicToXml(const PositionList &contents, const char *name) const
	{
		TiXmlElement root(CreateNamedElement("array", name));
		foreach (const Position &position, contents)
			root.InsertEndChild(position.BasicToXml("position"));
		return root;
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, string &target)
	{
		target = element.GetText();
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, int &target)
	{
		target = atoi(element.GetText());
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, bool &target)
	{
		const char * const text = element.GetText();
		target = (0 == stricmp(text, "true"));
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, float &target)
	{
		target = (float)atof(element.GetText());
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, Atoms &target)
	{
		const TiXmlElement *child(element.FirstChildElement());
		foreach (int &atom, target)
		{
			atom  = atoi(child->GetText());
			child = child->NextSiblingElement();
		}
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, AttributeID &target)
	{
		target = static_cast<AttributeID>(string_index(
			AttributeIDNames,
			array_size(AttributeIDNames),
			element.GetText()));
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, BuildingStatus &target)
	{
		target = static_cast<BuildingStatus>(string_index(
			BuildingStatusNames,
			array_size(BuildingStatusNames),
			element.GetText()));
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, FilthAttackType &target)
	{
		target = static_cast<FilthAttackType>(string_index(
			FilthAttackTypeNames,
			array_size(FilthAttackTypeNames),
			element.GetText()));
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, FilthType &target)
	{
		target = static_cast<FilthType>(string_index(
			FilthTypeNames,
			array_size(FilthTypeNames),
			element.GetText()));
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, NatureFlag &target)
	{
		target = static_cast<NatureFlag>(string_index(
			NatureFlagNames,
			array_size(NatureFlagNames),
			element.GetText()));
	}

	void XmlSerializable::BasicFromXml(const TiXmlElement &element, PositionList &target)
	{
		target.clear();
		const TiXmlElement *child(element.FirstChildElement());
		while (0 != child)
		{
			Position position;
			position.FromXml(*child);
			target.push_back(position);
			child = child->NextSiblingElement("set");
		}
	}

	const char * const * XmlSerializable::binary_search(const char * const * begin, const char * const * end, const char * val)
	{
		const char * const *l = begin;
		const char * const *r = end;
		while (l <= r)
		{
			const char * const * m = l + (r - l) / 2;
			int result(strcmp(*m, val));
			if (result < 0)
				l = m + 1;
			else if (result > 0)
				r = m - 1;
			else
				return m;
		}
		return end;
	}

	size_t XmlSerializable::string_index(const char * const name_array[], size_t array_size, const char * name)
	{
		const char * const * const begin  = name_array;
		const char * const * const end    = name_array + array_size;
		const char * const * const result = XmlSerializable::binary_search(begin, end, name);
		if (result != end)
			return static_cast<size_t>(end - result);
		_RPT1(_CRT_ERROR, "Unknown enumeration value: %s.", name);
		return static_cast<size_t>(-1);
	}

	//------------------------------
	// DamageMolecula implementation
	//------------------------------

	DamageMolecula::~DamageMolecula() {}

	TiXmlElement DamageMolecula::ToXml(const char *name) const
	{
		TiXmlElement root(CreateNamedElement("set", name));
		root.InsertEndChild(BasicToXml(isAlive, "isAlive"));
		root.InsertEndChild(BasicToXml(elementsDead, "elements"));
		return root;
	}

	void DamageMolecula::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("isAlive"), isAlive);
		BasicFromXml(*element.FirstChildElement("elementsDead"), elementsDead);
	}

	//---------------------------
	// orientation implementation
	//---------------------------

	Orientation::~Orientation() {}

	TiXmlElement Orientation::ToXml(const char *name) const
	{
		TiXmlElement root(CreateNamedElement("set", name));
		root.InsertEndChild(BasicToXml(s, "s"));
		root.InsertEndChild(BasicToXml(x, "x"));
		root.InsertEndChild(BasicToXml(y, "y"));
		root.InsertEndChild(BasicToXml(z, "z"));
		return root;
	}

	void Orientation::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("s"), s);
		BasicFromXml(*element.FirstChildElement("x"), x);
		BasicFromXml(*element.FirstChildElement("y"), y);
		BasicFromXml(*element.FirstChildElement("z"), z);
	}

	//------------------------
	// Position implementation
	//------------------------

	Position::~Position() {}

	TiXmlElement Position::ToXml(const char *name) const
	{
		TiXmlElement root(CreateNamedElement("set", name));
		root.InsertEndChild(BasicToXml(x, "x"));
		root.InsertEndChild(BasicToXml(y, "y"));
		root.InsertEndChild(BasicToXml(z, "z"));
		return root;
	}

	void Position::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("x"), x);
		BasicFromXml(*element.FirstChildElement("y"), y);
		BasicFromXml(*element.FirstChildElement("z"), z);
	}

	//-----------------------------
	// LocalPosition implementation
	//-----------------------------

	LocalPosition::~LocalPosition() {}

	TiXmlElement LocalPosition::ToXml(const char *name) const
	{
		TiXmlElement root(CreateNamedElement("set", name));
		root.InsertEndChild(BasicToXml(x, "x"));
		root.InsertEndChild(BasicToXml(y, "y"));
		return root;
	}

	void LocalPosition::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("x"), x);
		BasicFromXml(*element.FirstChildElement("y"), y);
	}

	//--------------------------
	// TargetUnit implementation
	//--------------------------

	TargetUnit::~TargetUnit() {}

	TiXmlElement TargetUnit::ToXml(const char *name) const
	{
		TiXmlElement root(CreateNamedElement("set", name));
		root.InsertEndChild(BasicToXml(unitID, "unitID"));
		root.InsertEndChild(BasicToXml(playerID, "playerID"));
		return root;
	}

	void TargetUnit::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("unitID"), unitID);
		BasicFromXml(*element.FirstChildElement("playerID"), playerID);
	}

	//-----------------------------
	// PlacedElement implementation
	//-----------------------------

	SaveUnitData::~SaveUnitData() {}

	TiXmlElement SaveUnitData::ToXml(const char *name) const
	{
		TiXmlElement root(CreateNamedElement("set", name));
		root.InsertEndChild(BasicToXml(unitID, "unitID"));
		root.InsertEndChild(BasicToXml(attributeID, "attributeID"));
		root.InsertEndChild(position.ToXml("position"));
		root.InsertEndChild(orientaion.ToXml("orientaion"));
		root.InsertEndChild(BasicToXml(radius, "radius"));
		root.InsertEndChild(BasicToXml(label.c_str(), "label"));
		root.InsertEndChild(damageMolecula.ToXml("damageMolecula"));
		return root;
	}

	void SaveUnitData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("unitID"), unitID);
		BasicFromXml(*element.FirstChildElement("attributeID"), attributeID);
		position.FromXml(*element.FirstChildElement("position"));
		orientaion.FromXml(*element.FirstChildElement("orientaion"));
		BasicFromXml(*element.FirstChildElement("radius"), radius);
		BasicFromXml(*element.FirstChildElement("label"), label);
		damageMolecula.FromXml(*element.FirstChildElement("damageMolecula"));
	}

	//-----------------------
	// SaveGeo implementation
	//-----------------------

	SaveGeo::~SaveGeo() {}

	TiXmlElement SaveGeo::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(sleep, "sleep"));
		root.InsertEndChild(BasicToXml(firstSleepTime, "firstSleepTime"));
		root.InsertEndChild(BasicToXml(sleepPeriod, "sleepPeriod"));
		root.InsertEndChild(BasicToXml(deltaSleepPeriod, "deltaSleepPeriod"));
		root.InsertEndChild(BasicToXml(attackPeriond, "attackPeriond"));
		root.InsertEndChild(BasicToXml(deltaAttackPeriond, "deltaAttackPeriond"));
		root.InsertEndChild(BasicToXml(activatingUnit, AttributeIDNames, "activatingUnit"));
		root.InsertEndChild(BasicToXml(activatingDistance, "activatingDistance"));
		return root;
	}

	void SaveGeo::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("sleep"), sleep);
		BasicFromXml(*element.FirstChildElement("firstSleepTime"), firstSleepTime);
		BasicFromXml(*element.FirstChildElement("sleepPeriod"), sleepPeriod);
		BasicFromXml(*element.FirstChildElement("deltaSleepPeriod"), deltaSleepPeriod);
		BasicFromXml(*element.FirstChildElement("attackPeriond"), attackPeriond);
		BasicFromXml(*element.FirstChildElement("deltaAttackPeriond"), deltaAttackPeriond);
		BasicFromXml(*element.FirstChildElement("activatingUnit"), AttributeIDNames, activatingUnit);
		BasicFromXml(*element.FirstChildElement("activatingDistance"), activatingDistance);
	}

	//----------------------------
	// SaveGeoBreak implementation
	//----------------------------

	SaveGeoBreak::~SaveGeoBreak() {}

	TiXmlElement SaveGeoBreak::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(geoRadius, "geoRadius"));
		root.InsertEndChild(BasicToXml(num_break, "num_break"));
		return root;
	}

	void SaveGeoBreak::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("geoRadius"), geoRadius);
		BasicFromXml(*element.FirstChildElement("num_break"), num_break);
	}

	//----------------------------
	// SaveGeoBreak implementation
	//----------------------------

	SaveGeoFault::~SaveGeoFault() {}

	TiXmlElement SaveGeoFault::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(length, "length"));
		root.InsertEndChild(BasicToXml(angle, "angle"));
		return root;
	}

	void SaveGeoFault::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("length"), length);
		BasicFromXml(*element.FirstChildElement("angle"), angle);
	}

	//-------------------------------------
	// SaveUnitLegionaryData implementation
	//-------------------------------------

	SaveUnitLegionaryData::~SaveUnitLegionaryData() {}

	TiXmlElement SaveUnitLegionaryData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(basementInstalled, "basementInstalled"));
		root.InsertEndChild(BasicToXml(accumulatedEnergy, "accumulatedEnergy"));
		root.InsertEndChild(BasicToXml(zeroLayerCounter, "zeroLayerCounter"));
		root.InsertEndChild(BasicToXml(weaponChargeLevel, "weaponChargeLevel"));
		root.InsertEndChild(BasicToXml(wayPoints, "wayPoints"));
		root.InsertEndChild(BasicToXml(weapon, "weapon"));
		root.InsertEndChild(BasicToXml(transportedSoldiers, "transportedSoldiers"));
		root.InsertEndChild(BasicToXml(transportedOfficers, "transportedOfficers"));
		root.InsertEndChild(BasicToXml(transportedTechnics, "transportedTechnics"));
		root.InsertEndChild(BasicToXml(flyingMode, "flyingMode"));
		root.InsertEndChild(BasicToXml(diggingMode, "diggingMode"));
		root.InsertEndChild(BasicToXml(inSquad, "inSquad"));
		root.InsertEndChild(localPosition.ToXml("localPosition"));
		root.InsertEndChild(BasicToXml(localPositionValid, "localPositionValid"));
		return root;
	}

	void SaveUnitLegionaryData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("basementInstalled"), basementInstalled);
		BasicFromXml(*element.FirstChildElement("accumulatedEnergy"), accumulatedEnergy);
		BasicFromXml(*element.FirstChildElement("zeroLayerCounter"), zeroLayerCounter);
		BasicFromXml(*element.FirstChildElement("weaponChargeLevel"), weaponChargeLevel);
		BasicFromXml(*element.FirstChildElement("wayPoints"), wayPoints);
		BasicFromXml(*element.FirstChildElement("weapon"), weapon);
		BasicFromXml(*element.FirstChildElement("transportedSoldiers"), transportedSoldiers);
		BasicFromXml(*element.FirstChildElement("transportedOfficers"), transportedOfficers);
		BasicFromXml(*element.FirstChildElement("transportedTechnics"), transportedTechnics);
		BasicFromXml(*element.FirstChildElement("flyingMode"), flyingMode);
		BasicFromXml(*element.FirstChildElement("diggingMode"), diggingMode);
		BasicFromXml(*element.FirstChildElement("inSquad"), inSquad);
		localPosition.FromXml(*element.FirstChildElement("localPosition"));
		BasicFromXml(*element.FirstChildElement("localPositionValid"), localPositionValid);

	}

	//---------------------------------
	// SaveUnitSquadData implementation
	//---------------------------------

	SaveUnitSquadData::~SaveUnitSquadData() {}

	TiXmlElement SaveUnitSquadData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(stablePosition.ToXml("stablePosition"));
		root.InsertEndChild(BasicToXml(currentMutation, "currentMutation"));
		root.InsertEndChild(BasicToXml(curvatureRadius, "curvatureRadius"));
		root.InsertEndChild(squadMemebers.ToXml("squadMemebers"));
		root.InsertEndChild(BasicToXml(wayPoints, "wayPoints"));
		root.InsertEndChild(BasicToXml(patrolPoints, "patrolPoints"));
		root.InsertEndChild(BasicToXml(patrolIndex, "patrolIndex"));
		root.InsertEndChild(BasicToXml(attackPoints, "attackPoints"));
		root.InsertEndChild(squadToFollow.ToXml("squadToFollow"));
		root.InsertEndChild(BasicToXml(offensiveMode, "offensiveMode"));
		root.InsertEndChild(BasicToXml(atomsRequested, "atomsRequested"));
		root.InsertEndChild(BasicToXml(atomsPaused, "atomsPaused"));
		root.InsertEndChild(BasicToXml(mutationEnergy, "mutationEnergy"));
		return root;
	}

	void SaveUnitSquadData::FromXml(const TiXmlElement &element)
	{
		stablePosition.FromXml(*element.FirstChildElement("stablePosition"));
		BasicFromXml(*element.FirstChildElement("currentMutation"), currentMutation);
		BasicFromXml(*element.FirstChildElement("curvatureRadius"), curvatureRadius);
		squadMemebers.FromXml(*element.FirstChildElement("squadMemebers"));
		BasicFromXml(*element.FirstChildElement("wayPoints"), wayPoints);
		BasicFromXml(*element.FirstChildElement("patrolPoints"), patrolPoints);
		BasicFromXml(*element.FirstChildElement("patrolIndex"), patrolIndex);
		BasicFromXml(*element.FirstChildElement("attackPoints"), attackPoints);
		squadToFollow.FromXml(*element.FirstChildElement("squadToFollow"));
		BasicFromXml(*element.FirstChildElement("offensiveMode"), offensiveMode);
		BasicFromXml(*element.FirstChildElement("atomsRequested"), atomsRequested);
		BasicFromXml(*element.FirstChildElement("atomsPaused"), atomsPaused);
		BasicFromXml(*element.FirstChildElement("mutationEnergy"), mutationEnergy);
	}

	//------------------------------------
	// SaveUnitBuildingData implementation
	//------------------------------------

	SaveUnitBuildingData::~SaveUnitBuildingData() {}

	TiXmlElement SaveUnitBuildingData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(basementInstalled, "basementInstalled"));
		root.InsertEndChild(BasicToXml(accumulatedEnergy, "accumulatedEnergy"));
		root.InsertEndChild(BasicToXml(zeroLayerCounter, "zeroLayerCounter"));
		root.InsertEndChild(BasicToXml(weaponChargeLevel, "weaponChargeLevel"));
		root.InsertEndChild(BasicToXml(wayPoints, "wayPoints"));
		root.InsertEndChild(BasicToXml(weapon, "weapon"));
		root.InsertEndChild(BasicToXml(buildingStatusBV, BuildingStatusNames, "buildingStatusBV"));
		root.InsertEndChild(BasicToXml(fireCount, "fireCount"));
		root.InsertEndChild(BasicToXml(visible, "visible"));
		return root;
	}

	void SaveUnitBuildingData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("basementInstalled"), basementInstalled);
		BasicFromXml(*element.FirstChildElement("accumulatedEnergy"), accumulatedEnergy);
		BasicFromXml(*element.FirstChildElement("zeroLayerCounter"), zeroLayerCounter);
		BasicFromXml(*element.FirstChildElement("weaponChargeLevel"), weaponChargeLevel);
		BasicFromXml(*element.FirstChildElement("wayPoints"), wayPoints);
		BasicFromXml(*element.FirstChildElement("weapon"), weapon);
		BasicFromXml(*element.FirstChildElement("buildingStatusBV"), BuildingStatusNames, buildingStatusBV);
		BasicFromXml(*element.FirstChildElement("fireCount"), fireCount);
		BasicFromXml(*element.FirstChildElement("visible"), visible);
	}

	//-----------------------------------------
	// SaveUnitCommandCenterData implementation
	//-----------------------------------------

	SaveUnitCommandCenterData::~SaveUnitCommandCenterData() {}

	TiXmlElement SaveUnitCommandCenterData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(basementInstalled, "basementInstalled"));
		root.InsertEndChild(BasicToXml(accumulatedEnergy, "accumulatedEnergy"));
		root.InsertEndChild(BasicToXml(zeroLayerCounter, "zeroLayerCounter"));
		root.InsertEndChild(BasicToXml(wayPoints, "wayPoints"));
		root.InsertEndChild(BasicToXml(buildingStatusBV, BuildingStatusNames, "buildingStatusBV"));
		root.InsertEndChild(BasicToXml(fireCount, "fireCount"));
		root.InsertEndChild(BasicToXml(visible, "visible"));
		root.InsertEndChild(squad.ToXml("squad"));
		return root;
	}

	void SaveUnitCommandCenterData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("basementInstalled"), basementInstalled);
		BasicFromXml(*element.FirstChildElement("accumulatedEnergy"), accumulatedEnergy);
		BasicFromXml(*element.FirstChildElement("zeroLayerCounter"), zeroLayerCounter);
		BasicFromXml(*element.FirstChildElement("wayPoints"), wayPoints);
		BasicFromXml(*element.FirstChildElement("buildingStatusBV"), BuildingStatusNames, buildingStatusBV);
		BasicFromXml(*element.FirstChildElement("fireCount"), fireCount);
		BasicFromXml(*element.FirstChildElement("visible"), visible);
		squad.FromXml(*element.FirstChildElement("squad"));
	}

	// -----------------
	// SaveUnitFrameData
	// -----------------

	SaveUnitFrameData::~SaveUnitFrameData() {}

	TiXmlElement SaveUnitFrameData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(basementInstalled, "basementInstalled"));
		root.InsertEndChild(BasicToXml(accumulatedEnergy, "accumulatedEnergy"));
		root.InsertEndChild(BasicToXml(zeroLayerCounter, "zeroLayerCounter"));
		root.InsertEndChild(BasicToXml(weaponChargeLevel, "weaponChargeLevel"));
		root.InsertEndChild(BasicToXml(wayPoints, "wayPoints"));
		root.InsertEndChild(BasicToXml(weapon, "weapon"));
		root.InsertEndChild(BasicToXml(attached, "attached"));
		root.InsertEndChild(BasicToXml(attaching, "attaching"));
		root.InsertEndChild(BasicToXml(powered, "powered"));
		root.InsertEndChild(BasicToXml(spiralLevel, "spiralLevel"));
		root.InsertEndChild(squad.ToXml("squad"));
		return root;
	}

	void SaveUnitFrameData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("basementInstalled"), basementInstalled);
		BasicFromXml(*element.FirstChildElement("accumulatedEnergy"), accumulatedEnergy);
		BasicFromXml(*element.FirstChildElement("zeroLayerCounter"), zeroLayerCounter);
		BasicFromXml(*element.FirstChildElement("weaponChargeLevel"), weaponChargeLevel);
		BasicFromXml(*element.FirstChildElement("wayPoints"), wayPoints);
		BasicFromXml(*element.FirstChildElement("weapon"), weapon);
		BasicFromXml(*element.FirstChildElement("attached"), attached);
		BasicFromXml(*element.FirstChildElement("attaching"), attaching);
		BasicFromXml(*element.FirstChildElement("powered"), powered);
		BasicFromXml(*element.FirstChildElement("spiralLevel"), spiralLevel);
		squad.FromXml(*element.FirstChildElement("squad"));
	}

	//---------------------------------
	// SaveUnitFilthData implementation
	//---------------------------------

	SaveUnitFilthData::~SaveUnitFilthData() {}

	TiXmlElement SaveUnitFilthData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(filthType, "filthType"));
		root.InsertEndChild(BasicToXml(attackDirection, "attackDirection"));
		root.InsertEndChild(BasicToXml(sleep, "sleep"));
		root.InsertEndChild(BasicToXml(firstSleepTime, "firstSleepTime"));
		root.InsertEndChild(BasicToXml(sleepPeriod, "sleepPeriod"));
		root.InsertEndChild(BasicToXml(deltaSleepPeriod, "deltaSleepPeriod"));
		root.InsertEndChild(BasicToXml(attackPeriond, "attackPeriond"));
		root.InsertEndChild(BasicToXml(deltaAttackPeriond, "deltaAttackPeriond"));
		root.InsertEndChild(BasicToXml(creatureNum, "creatureNum"));
		root.InsertEndChild(BasicToXml(activatingUnit, AttributeIDNames, "activatingUnit"));
		root.InsertEndChild(BasicToXml(activatingDistance, "activatingDistance"));
		root.InsertEndChild(BasicToXml(attack_player, "attack_player"));
		root.InsertEndChild(BasicToXml(initial_geoprocess, "initial_geoprocess"));
		root.InsertEndChild(BasicToXml(killTimer, "killTimer"));
		root.InsertEndChild(BasicToXml(sleep_timer, "sleep_timer"));
		root.InsertEndChild(BasicToXml(create_first, "create_first"));
		root.InsertEndChild(BasicToXml(hole_position, "hole_position"));
		root.InsertEndChild(BasicToXml(hole_position_inited, "hole_position_inited"));
		root.InsertEndChild(BasicToXml(kill_of_end, "kill_of_end"));
		root.InsertEndChild(BasicToXml(swarmList, "swarmList"));
		return root;
	}

	void SaveUnitFilthData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("filthType"), filthType);
		BasicFromXml(*element.FirstChildElement("attackDirection"), attackDirection);
		BasicFromXml(*element.FirstChildElement("sleep"), sleep);
		BasicFromXml(*element.FirstChildElement("firstSleepTime"), firstSleepTime);
		BasicFromXml(*element.FirstChildElement("sleepPeriod"), sleepPeriod);
		BasicFromXml(*element.FirstChildElement("deltaSleepPeriod"), deltaSleepPeriod);
		BasicFromXml(*element.FirstChildElement("attackPeriond"), attackPeriond);
		BasicFromXml(*element.FirstChildElement("deltaAttackPeriond"), deltaAttackPeriond);
		BasicFromXml(*element.FirstChildElement("creatureNum"), creatureNum);
		BasicFromXml(*element.FirstChildElement("activatingUnit"), AttributeIDNames, activatingUnit);
		BasicFromXml(*element.FirstChildElement("activatingDistance"), activatingDistance);
		BasicFromXml(*element.FirstChildElement("attack_player"), attack_player);
		BasicFromXml(*element.FirstChildElement("initial_geoprocess"), initial_geoprocess);
		BasicFromXml(*element.FirstChildElement("killTimer"), killTimer);
		BasicFromXml(*element.FirstChildElement("sleep_timer"), sleep_timer);
		BasicFromXml(*element.FirstChildElement("create_first"), create_first);
		BasicFromXml(*element.FirstChildElement("hole_position"), hole_position);
		BasicFromXml(*element.FirstChildElement("hole_position_inited"), hole_position_inited);
		BasicFromXml(*element.FirstChildElement("kill_of_end"), kill_of_end);
		BasicFromXml(*element.FirstChildElement("swarmList"), swarmList);            // NA
	}

	//----------------------------------
	// SaveUnitNatureData implementation
	//----------------------------------

	SaveUnitNatureData::~SaveUnitNatureData() {}

	TiXmlElement SaveUnitNatureData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(modelName.c_str(), "modelName"));
		root.InsertEndChild(BasicToXml(visible, "visible"));
		root.InsertEndChild(BasicToXml(natureFlag, NatureFlagNames, "natureFlag"));
		root.InsertEndChild(BasicToXml(chainName.c_str(), "chainName"));
		root.InsertEndChild(BasicToXml(chainPhase, "chainPhase"));
		root.InsertEndChild(BasicToXml(chainPeriod, "chainPeriod"));
		return root;
	}

	void SaveUnitNatureData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("modelName"), modelName);
		BasicFromXml(*element.FirstChildElement("visible"), visible);
		BasicFromXml(*element.FirstChildElement("natureFlag"), NatureFlagNames, natureFlag);
		BasicFromXml(*element.FirstChildElement("chainName"), chainName);
		BasicFromXml(*element.FirstChildElement("chainPhase"), chainPhase);
		BasicFromXml(*element.FirstChildElement("chainPeriod"), chainPeriod);
	}

	//--------------------------------------
	// SaveUnitFrameChildData implementation
	//--------------------------------------

	SaveUnitFrameChildData::~SaveUnitFrameChildData() {}

	TiXmlElement SaveUnitFrameChildData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(basementInstalled, "basementInstalled"));
		root.InsertEndChild(BasicToXml(accumulatedEnergy, "accumulatedEnergy"));
		root.InsertEndChild(BasicToXml(zeroLayerCounter, "zeroLayerCounter"));
		root.InsertEndChild(BasicToXml(weaponChargeLevel, "weaponChargeLevel"));
		root.InsertEndChild(BasicToXml(wayPoints, "wayPoints"));
		root.InsertEndChild(BasicToXml(weapon, "weapon"));
		root.InsertEndChild(BasicToXml(dockReadyStatus, "dockReadyStatus"));
		root.InsertEndChild(BasicToXml(alarmStatus, "alarmStatus"));
		return root;
	}

	void SaveUnitFrameChildData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("basementInstalled"), basementInstalled);
		BasicFromXml(*element.FirstChildElement("accumulatedEnergy"), accumulatedEnergy);
		BasicFromXml(*element.FirstChildElement("zeroLayerCounter"), zeroLayerCounter);
		BasicFromXml(*element.FirstChildElement("weaponChargeLevel"), weaponChargeLevel);
		BasicFromXml(*element.FirstChildElement("wayPoints"), wayPoints);
		BasicFromXml(*element.FirstChildElement("weapon"), weapon);
		BasicFromXml(*element.FirstChildElement("dockReadyStatus"), dockReadyStatus);
		BasicFromXml(*element.FirstChildElement("alarmStatus"), alarmStatus);
	}

	//--------------------------------------------
	// SaveUnitBuildingMilitaryData implementation
	//--------------------------------------------

	SaveUnitBuildingMilitaryData::~SaveUnitBuildingMilitaryData() {}

	TiXmlElement SaveUnitBuildingMilitaryData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(attackTarget.ToXml("attackTarget"));
		root.InsertEndChild(lastAttackTarget.ToXml("lastAttackTarget"));
		root.InsertEndChild(BasicToXml(manualAttackTarget, "manualAttackTarget"));
		return root;
	}

	void SaveUnitBuildingMilitaryData::FromXml(const TiXmlElement &element)
	{
		attackTarget.FromXml(*element.FirstChildElement("attackTarget"));
		lastAttackTarget.FromXml(*element.FirstChildElement("lastAttackTarget"));
		BasicFromXml(*element.FirstChildElement("manualAttackTarget"), manualAttackTarget);
	}

	//-----------------------------------------
	// SaveUnitCorridorAlphaData implementation
	//-----------------------------------------

	SaveUnitCorridorAlphaData::~SaveUnitCorridorAlphaData() {}

	TiXmlElement SaveUnitCorridorAlphaData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(free, "free"));
		root.InsertEndChild(BasicToXml(passTime, "passTime"));
		root.InsertEndChild(BasicToXml(timeOffset, "timeOffset"));
		return root;
	}

	void SaveUnitCorridorAlphaData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("free"), free);
		BasicFromXml(*element.FirstChildElement("passTime"), passTime);
		BasicFromXml(*element.FirstChildElement("timeOffset"), timeOffset);
	}

	//-----------------------------------------
	// SaveUnitCorridorOmegaData implementation
	//-----------------------------------------

	SaveUnitCorridorOmegaData::~SaveUnitCorridorOmegaData() {}

	TiXmlElement SaveUnitCorridorOmegaData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(upgraded, "upgraded"));
		return root;
	}

	void SaveUnitCorridorOmegaData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("upgraded"), upgraded);
	}

	//-------------------------------------
	// SaveUnitProtectorData implementation
	//-------------------------------------

	SaveUnitProtectorData::~SaveUnitProtectorData() {}

	TiXmlElement SaveUnitProtectorData::ToXml(const char *name) const
	{
		TiXmlElement root(__super::ToXml(name));
		root.InsertEndChild(BasicToXml(monksNumber, "monksNumber"));
		root.InsertEndChild(BasicToXml(fieldState, "fieldState"));
		root.InsertEndChild(BasicToXml(enableScharge, "enableScharge"));
		root.InsertEndChild(BasicToXml(startWhenCharged, "startWhenCharged"));
		return root;
	}

	void SaveUnitProtectorData::FromXml(const TiXmlElement &element)
	{
		BasicFromXml(*element.FirstChildElement("monksNumber"), monksNumber);
		BasicFromXml(*element.FirstChildElement("fieldState"), fieldState);
		BasicFromXml(*element.FirstChildElement("enableScharge"), enableScharge);
		BasicFromXml(*element.FirstChildElement("startWhenCharged"), startWhenCharged);
	}

} // namespace interop
