#ifndef SPG_H
#define SPG_H

#include "../interop.h"
#include "../error handler.h"
#include "../unit manager.h"
#include "save callback.h"

#include <loki/Typelist.h>
#include <loki/Visitor.h>

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/array/size.hpp>
#include <boost/preprocessor/array/elem.hpp>

namespace TaskCommon
{

class Spg : public ErrorHandler, public SaveCallback
{
// nested types
public:
	struct info_t
	{
		tstring   path_;
	};
private:
	class UnitToXmlVisitor
		:public Loki::BaseVisitor
		,public Loki::Visitor<interop::UnitTypeList>
	{
	public:
		UnitToXmlVisitor(const Spg & parent) : parent_(parent), element_("empty") {}
		TiXmlElement &GetElement() { return element_; }
	public:
		// generate Visitor implementations for each member of the type list
		// delegate to parent
		#define MacroCreateVisit(z, n, data)  \
		   void Visit(interop::BOOST_PP_ARRAY_ELEM(n, INTEROP_UNIT_TYPE_LIST) & unit) \
		   {                                  \
		      element_ = parent_.ToXml(unit); \
		   }
		BOOST_PP_REPEAT(BOOST_PP_ARRAY_SIZE(INTEROP_UNIT_TYPE_LIST), MacroCreateVisit, ~);
		#undef MacroCreateVisit
	// data
	private:
		const Spg & parent_;
		TiXmlElement element_;
	};
// interface
public:
	// construction
	Spg(const HWND &error_hwnd);
	// TaskResource support
	bool Load();
	void Unload();
	// packing
	void Pack(TiXmlNode &node) const;
	bool Unpack(TiXmlNode &node);
	// miscellaneous
	void Save() const;
	void SaveAs(LPCTSTR path) const;
	// unit manager support
	void SetUnits(UnitManager &unit_manager);
	void GetUnits(UnitManager &unit_manager);
// implementation
private:
	// XML parsing
	bool GetPlayers(boost::array<TiXmlElement*, prm_player_count> &players);
	TiXmlElement *GetNeutral();
	TiXmlElement *GetEnvironment();
	TiXmlElement *GetFilth();
	// XML output
	void SetElementCode(TiXmlElement &element, const char *code) const;
	void SetElementName(TiXmlElement &element, const char *name) const;
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
	// XML input
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
	// converts a bitset into XML
	template <int N>
	TiXmlElement ToXml(
		const std::bitset<N> &contents,
		const char * const (&value_names)[N],
		const char *name = NULL) const
	{
		TiXmlElement root("disjunction");
		SetElementName(root, name);
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
// data
public:
	TiXmlDocument doc_;
	info_t        info_;
};

}

#endif // SPG_H
