#ifndef SPG_H
#define SPG_H

#include "../interop.h"

namespace TaskCommon
{

class Spg
{
private:
	// xml output
	TiXmlElement CreateNamedElement(const char *value, const char *name) const;
	TiXmlElement ToXml(const char                                  *contents, const char *name = NULL) const;
	TiXmlElement ToXml(int                                          contents, const char *name = NULL) const;
	TiXmlElement ToXml(bool                                         contents, const char *name = NULL) const;
	TiXmlElement ToXml(float                                        contents, const char *name = NULL) const;
	TiXmlElement ToXml(interop::AttributeID                         contents, const char *name = NULL) const;
	TiXmlElement ToXml(interop::BuildingStatus                      contents, const char *name = NULL) const;
	TiXmlElement ToXml(interop::FilthAttackType                     contents, const char *name = NULL) const;
	TiXmlElement ToXml(interop::FilthType                           contents, const char *name = NULL) const;
	TiXmlElement ToXml(interop::NatureFlag                          contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::Atoms                        &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::PositionList                 &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::DamageMolecula               &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::Orientation                  &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::Position                     &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::LocalPosition                &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::TargetUnit                   &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::Attackpoint                  &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitData                 &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveGeo                      &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveGeoBreak                 &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveGeoFault                 &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitLegionaryData        &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitSquadData            &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitBuildingData         &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitCommandCenterData    &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitFrameData            &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitFilthData            &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitNatureData           &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitFrameChildData       &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitBuildingMilitaryData &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitCorridorAlphaData    &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitCorridorOmegaData    &contents, const char *name = NULL) const;
	TiXmlElement ToXml(const interop::SaveUnitProtectorData        &contents, const char *name = NULL) const;
	// xml input
	void FromXml(const TiXmlElement &element, string                                &target) const;
	void FromXml(const TiXmlElement &element, int                                   &target) const;
	void FromXml(const TiXmlElement &element, bool                                  &target) const;
	void FromXml(const TiXmlElement &element, float                                 &target) const;
	void FromXml(const TiXmlElement &element, interop::Atoms                        &target) const;
	void FromXml(const TiXmlElement &element, interop::AttributeID                  &target) const;
	void FromXml(const TiXmlElement &element, interop::BuildingStatus               &target) const;
	void FromXml(const TiXmlElement &element, interop::FilthAttackType              &target) const;
	void FromXml(const TiXmlElement &element, interop::FilthType                    &target) const;
	void FromXml(const TiXmlElement &element, interop::NatureFlag                   &target) const;
	void FromXml(const TiXmlElement &element, interop::PositionList                 &target) const;
	void FromXml(const TiXmlElement &element, interop::DamageMolecula               &target) const;
	void FromXml(const TiXmlElement &element, interop::Orientation                  &target) const;
	void FromXml(const TiXmlElement &element, interop::Position                     &target) const;
	void FromXml(const TiXmlElement &element, interop::LocalPosition                &target) const;
	void FromXml(const TiXmlElement &element, interop::TargetUnit                   &target) const;
	void FromXml(const TiXmlElement &element, interop::Attackpoint                  &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitData                 &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveGeo                      &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveGeoBreak                 &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveGeoFault                 &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitLegionaryData        &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitSquadData            &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitBuildingData         &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitCommandCenterData    &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitFrameData            &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitFilthData            &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitNatureData           &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitFrameChildData       &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitBuildingMilitaryData &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitCorridorAlphaData    &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitCorridorOmegaData    &target) const;
	void FromXml(const TiXmlElement &element, interop::SaveUnitProtectorData        &target) const;
private:
	// converts bitset to XML
	template <int N>
	TiXmlElement ToXml(
		const std::bitset<N> &contents,
		const char * const (&value_names)[N],
		const char *name = NULL) const
	{
		TiXmlElement root(CreateNamedElement("disjunction", name));
		for (size_t i(0); i != contents.size(); ++i)
			if (contents[i])
			{
				TiXmlElement value("value");
				value.InsertEndChild(TiXmlText(value_names[i]));
				root.InsertEndChild(value);
			}
		return root;
	}
	// converts XML to a bitset
	template <int N>
	void FromXml(
		const TiXmlElement &element,
		const char * const (&names)[N],
		std::bitset<N> &target) const
	{
		target.none();
		const char * value(element.Value());
		if (0 == stricmp(value, "value"))
			target[string_index(names, N, element.GetText())] = true;
		else if (0 == stricmp(value, "disjunction"))
		{
			const TiXmlElement *child(element.FirstChildElement());
			while (0 != child)
			{
				target[string_index(names, N, child->GetText())] = true;
				child = child->NextSiblingElement("value");
			}
		}
		else
			_RPT1(_CRT_ERROR, "Invalid disjunction type: %s", value);
	}
private:
	static const char * const * binary_search(const char * const * begin, const char * const * end, const char * val);
	static size_t string_index(const char * const name_array[], size_t array_size, const char * name);
	template <typename T, size_t N>
	static size_t array_size(T (&)[N])
	{
		return N;
	}
};

}

#endif // SPG_H
