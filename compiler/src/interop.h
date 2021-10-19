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


#pragma once

#include <d3dx9.h>

#include <bitset>
#include <boost/array.hpp>

#include <Loki/Typelist.h>
#include <Loki/Visitor.h>


namespace interop
{

enum AttributeID
{
	UNIT_ATTRIBUTE_ANY = 0,
	UNIT_ATTRIBUTE_BLACK_HOLE,
	UNIT_ATTRIBUTE_BOMB_STATION1,
	UNIT_ATTRIBUTE_BOMB_STATION2,
	UNIT_ATTRIBUTE_BOMB_STATION3,
	UNIT_ATTRIBUTE_BOMBER,
	UNIT_ATTRIBUTE_BUILD_MASTER,
	UNIT_ATTRIBUTE_CEPTOR,
	UNIT_ATTRIBUTE_COLLECTOR,
	UNIT_ATTRIBUTE_COMMANDER,
	UNIT_ATTRIBUTE_CORE,
	UNIT_ATTRIBUTE_CORRIDOR_ALPHA,
	UNIT_ATTRIBUTE_CORRIDOR_OMEGA,
	UNIT_ATTRIBUTE_DIGGER,
	UNIT_ATTRIBUTE_DISINTEGRATOR,
	UNIT_ATTRIBUTE_EMPIRE_STATION1,
	UNIT_ATTRIBUTE_EMPIRE_STATION2,
	UNIT_ATTRIBUTE_EMPIRE_STATION3,
	UNIT_ATTRIBUTE_EXODUS_STATION1,
	UNIT_ATTRIBUTE_EXODUS_STATION2,
	UNIT_ATTRIBUTE_EXODUS_STATION3,
	UNIT_ATTRIBUTE_EXTIRPATOR,
	UNIT_ATTRIBUTE_FILTH_ANTS,
	UNIT_ATTRIBUTE_FILTH_RAT,
	UNIT_ATTRIBUTE_FILTH_SPOT0,
	UNIT_ATTRIBUTE_FILTH_SPOT1,
	UNIT_ATTRIBUTE_FILTH_SPOT2,
	UNIT_ATTRIBUTE_FILTH_SPOT3,
	UNIT_ATTRIBUTE_FLY_STATION1,
	UNIT_ATTRIBUTE_FLY_STATION2,
	UNIT_ATTRIBUTE_FRAME,
	UNIT_ATTRIBUTE_GEO_BREAK,
	UNIT_ATTRIBUTE_GEO_FAULT,
	UNIT_ATTRIBUTE_GEO_HEAD,
	UNIT_ATTRIBUTE_GEO_INFLUENCE,
	UNIT_ATTRIBUTE_GUN_BALLISTIC,
	UNIT_ATTRIBUTE_GUN_FILTH_NAVIGATOR,
	UNIT_ATTRIBUTE_GUN_HOWITZER,
	UNIT_ATTRIBUTE_GUN_SCUM_DISRUPTOR,
	UNIT_ATTRIBUTE_GUN_SUBCHASER,
	UNIT_ATTRIBUTE_GYROID,
	UNIT_ATTRIBUTE_HARKBACK_STATION1,
	UNIT_ATTRIBUTE_HARKBACK_STATION2,
	UNIT_ATTRIBUTE_HARKBACK_STATION3,
	UNIT_ATTRIBUTE_LASER_CANNON,
	UNIT_ATTRIBUTE_LASER_STATION1,
	UNIT_ATTRIBUTE_LASER_STATION2,
	UNIT_ATTRIBUTE_LASER_STATION3,
	UNIT_ATTRIBUTE_LEAMO,
	UNIT_ATTRIBUTE_LEECH,
	UNIT_ATTRIBUTE_MORTAR,
	UNIT_ATTRIBUTE_NONE,
	UNIT_ATTRIBUTE_OFFICER,
	UNIT_ATTRIBUTE_OFFICER_PLANT,
	UNIT_ATTRIBUTE_PIERCER,
	UNIT_ATTRIBUTE_R_PROJECTOR,
	UNIT_ATTRIBUTE_RELAY,
	UNIT_ATTRIBUTE_ROCKER,
	UNIT_ATTRIBUTE_ROCKET_LAUNCHER,
	UNIT_ATTRIBUTE_ROCKET_STATION1,
	UNIT_ATTRIBUTE_ROCKET_STATION2,
	UNIT_ATTRIBUTE_ROCKET_STATION3,
	UNIT_ATTRIBUTE_SCUM_HEATER,
	UNIT_ATTRIBUTE_SCUM_SPLITTER,
	UNIT_ATTRIBUTE_SCUM_SPOT,
	UNIT_ATTRIBUTE_SCUM_SPOT2,
	UNIT_ATTRIBUTE_SCUM_SPOT3,
	UNIT_ATTRIBUTE_SCUM_SPOT4,
	UNIT_ATTRIBUTE_SCUM_THROWER,
	UNIT_ATTRIBUTE_SCUM_TWISTER,
	UNIT_ATTRIBUTE_SCUMER,
	UNIT_ATTRIBUTE_SNIPER,
	UNIT_ATTRIBUTE_SOLDIER,
	UNIT_ATTRIBUTE_SOLDIER_PLANT,
	UNIT_ATTRIBUTE_SQUAD,
	UNIT_ATTRIBUTE_STATIC_BOMB,
	UNIT_ATTRIBUTE_STATIC_NATURE,
	UNIT_ATTRIBUTE_STRAFER,
	UNIT_ATTRIBUTE_SUBTERRA_STATION1,
	UNIT_ATTRIBUTE_SUBTERRA_STATION2,
	UNIT_ATTRIBUTE_TECHNIC,
	UNIT_ATTRIBUTE_TECHNIC_PLANT,
	UNIT_ATTRIBUTE_TERRAIN_MASTER,
	UNIT_ATTRIBUTE_UNSEEN,
	UNIT_ATTRIBUTE_WARGON,
	AttributeID_count
};

typedef std::bitset<AttributeID_count> AttributeIDSet;

const char * const AttributeIDNames[] =
{
	"UNIT_ATTRIBUTE_ANY",
	"UNIT_ATTRIBUTE_BLACK_HOLE",
	"UNIT_ATTRIBUTE_BOMB_STATION1",
	"UNIT_ATTRIBUTE_BOMB_STATION2",
	"UNIT_ATTRIBUTE_BOMB_STATION3",
	"UNIT_ATTRIBUTE_BOMBER",
	"UNIT_ATTRIBUTE_BUILD_MASTER",
	"UNIT_ATTRIBUTE_CEPTOR",
	"UNIT_ATTRIBUTE_COLLECTOR",
	"UNIT_ATTRIBUTE_COMMANDER",
	"UNIT_ATTRIBUTE_CORE",
	"UNIT_ATTRIBUTE_CORRIDOR_ALPHA",
	"UNIT_ATTRIBUTE_CORRIDOR_OMEGA",
	"UNIT_ATTRIBUTE_DIGGER",
	"UNIT_ATTRIBUTE_DISINTEGRATOR",
	"UNIT_ATTRIBUTE_EMPIRE_STATION1",
	"UNIT_ATTRIBUTE_EMPIRE_STATION2",
	"UNIT_ATTRIBUTE_EMPIRE_STATION3",
	"UNIT_ATTRIBUTE_EXODUS_STATION1",
	"UNIT_ATTRIBUTE_EXODUS_STATION2",
	"UNIT_ATTRIBUTE_EXODUS_STATION3",
	"UNIT_ATTRIBUTE_EXTIRPATOR",
	"UNIT_ATTRIBUTE_FILTH_ANTS",
	"UNIT_ATTRIBUTE_FILTH_RAT",
	"UNIT_ATTRIBUTE_FILTH_SPOT0",
	"UNIT_ATTRIBUTE_FILTH_SPOT1",
	"UNIT_ATTRIBUTE_FILTH_SPOT2",
	"UNIT_ATTRIBUTE_FILTH_SPOT3",
	"UNIT_ATTRIBUTE_FLY_STATION1",
	"UNIT_ATTRIBUTE_FLY_STATION2",
	"UNIT_ATTRIBUTE_FRAME",
	"UNIT_ATTRIBUTE_GEO_BREAK",
	"UNIT_ATTRIBUTE_GEO_FAULT",
	"UNIT_ATTRIBUTE_GEO_HEAD",
	"UNIT_ATTRIBUTE_GEO_INFLUENCE",
	"UNIT_ATTRIBUTE_GUN_BALLISTIC",
	"UNIT_ATTRIBUTE_GUN_FILTH_NAVIGATOR",
	"UNIT_ATTRIBUTE_GUN_HOWITZER",
	"UNIT_ATTRIBUTE_GUN_SCUM_DISRUPTOR",
	"UNIT_ATTRIBUTE_GUN_SUBCHASER",
	"UNIT_ATTRIBUTE_GYROID",
	"UNIT_ATTRIBUTE_HARKBACK_STATION1",
	"UNIT_ATTRIBUTE_HARKBACK_STATION2",
	"UNIT_ATTRIBUTE_HARKBACK_STATION3",
	"UNIT_ATTRIBUTE_LASER_CANNON",
	"UNIT_ATTRIBUTE_LASER_STATION1",
	"UNIT_ATTRIBUTE_LASER_STATION2",
	"UNIT_ATTRIBUTE_LASER_STATION3",
	"UNIT_ATTRIBUTE_LEAMO",
	"UNIT_ATTRIBUTE_LEECH",
	"UNIT_ATTRIBUTE_MORTAR",
	"UNIT_ATTRIBUTE_NONE",
	"UNIT_ATTRIBUTE_OFFICER",
	"UNIT_ATTRIBUTE_OFFICER_PLANT",
	"UNIT_ATTRIBUTE_PIERCER",
	"UNIT_ATTRIBUTE_R_PROJECTOR",
	"UNIT_ATTRIBUTE_RELAY",
	"UNIT_ATTRIBUTE_ROCKER",
	"UNIT_ATTRIBUTE_ROCKET_LAUNCHER",
	"UNIT_ATTRIBUTE_ROCKET_STATION1",
	"UNIT_ATTRIBUTE_ROCKET_STATION2",
	"UNIT_ATTRIBUTE_ROCKET_STATION3",
	"UNIT_ATTRIBUTE_SCUM_HEATER",
	"UNIT_ATTRIBUTE_SCUM_SPLITTER",
	"UNIT_ATTRIBUTE_SCUM_SPOT",
	"UNIT_ATTRIBUTE_SCUM_SPOT2",
	"UNIT_ATTRIBUTE_SCUM_SPOT3",
	"UNIT_ATTRIBUTE_SCUM_SPOT4",
	"UNIT_ATTRIBUTE_SCUM_THROWER",
	"UNIT_ATTRIBUTE_SCUM_TWISTER",
	"UNIT_ATTRIBUTE_SCUMER",
	"UNIT_ATTRIBUTE_SNIPER",
	"UNIT_ATTRIBUTE_SOLDIER",
	"UNIT_ATTRIBUTE_SOLDIER_PLANT",
	"UNIT_ATTRIBUTE_SQUAD",
	"UNIT_ATTRIBUTE_STATIC_BOMB",
	"UNIT_ATTRIBUTE_STATIC_NATURE",
	"UNIT_ATTRIBUTE_STRAFER",
	"UNIT_ATTRIBUTE_SUBTERRA_STATION1",
	"UNIT_ATTRIBUTE_SUBTERRA_STATION2",
	"UNIT_ATTRIBUTE_TECHNIC",
	"UNIT_ATTRIBUTE_TECHNIC_PLANT",
	"UNIT_ATTRIBUTE_TERRAIN_MASTER",
	"UNIT_ATTRIBUTE_UNSEEN",
	"UNIT_ATTRIBUTE_WARGON"
};

enum BuildingStatus
{
	BUILDING_STATUS_CONNECTED,
	BUILDING_STATUS_CONSTRUCTED,
	BUILDING_STATUS_ENABLED,
	BUILDING_STATUS_MOUNTED,
	BUILDING_STATUS_PLUGGED_IN,
	BUILDING_STATUS_POWERED,
	BuildingStatus_count
};

typedef std::bitset<BuildingStatus_count> BuildingStatusSet;

const char * const BuildingStatusNames[] =
{
	"BUILDING_STATUS_CONNECTED",
	"BUILDING_STATUS_CONSTRUCTED",
	"BUILDING_STATUS_ENABLED",
	"BUILDING_STATUS_MOUNTED",
	"BUILDING_STATUS_PLUGGED_IN",
	"BUILDING_STATUS_POWERED"
};

enum FilthAttackType
{
	FILTH_ATTACK_ALL,
	FILTH_ATTACK_PLAYER,
	FilthAttackType_count
};

typedef std::bitset<FilthAttackType_count> FilthAttackTypeSet;

const char * const FilthAttackTypeNames[] =
{
	"FILTH_ATTACK_ALL",
	"FILTH_ATTACK_PLAYER"
};

enum FilthType
{
	FILTH_SPOT_ID_A_ANTS,
	FILTH_SPOT_ID_A_CROW,
	FILTH_SPOT_ID_A_DAEMON,
	FILTH_SPOT_ID_A_DRAGON,
	FILTH_SPOT_ID_A_EYE,
	FILTH_SPOT_ID_A_RAT,
	FILTH_SPOT_ID_A_SPIDER,
	FILTH_SPOT_ID_A_WASP,
	FILTH_SPOT_ID_A_WORM,
	FILTH_SPOT_ID_ANTS,
	FILTH_SPOT_ID_ANTS2,
	FILTH_SPOT_ID_CROW,
	FILTH_SPOT_ID_DAEMON,
	FILTH_SPOT_ID_DRAGON,
	FILTH_SPOT_ID_DRAGON2,
	FILTH_SPOT_ID_EYE,
	FILTH_SPOT_ID_GHOST,
	FILTH_SPOT_ID_RAT,
	FILTH_SPOT_ID_SHARK,
	FILTH_SPOT_ID_SNAKE,
	FILTH_SPOT_ID_VOLCANO,
	FILTH_SPOT_ID_VOLCANO_SCUM_DISRUPTOR,
	FILTH_SPOT_ID_WASP,
	FILTH_SPOT_ID_WORM,
	FilthType_count
};

typedef std::bitset<FilthType_count> FilthTypeSet;

const char * const FilthTypeNames[] =
{
	"FILTH_SPOT_ID_A_ANTS",
	"FILTH_SPOT_ID_A_CROW",
	"FILTH_SPOT_ID_A_DAEMON",
	"FILTH_SPOT_ID_A_DRAGON",
	"FILTH_SPOT_ID_A_EYE",
	"FILTH_SPOT_ID_A_RAT",
	"FILTH_SPOT_ID_A_SPIDER",
	"FILTH_SPOT_ID_A_WASP",
	"FILTH_SPOT_ID_A_WORM",
	"FILTH_SPOT_ID_ANTS",
	"FILTH_SPOT_ID_ANTS2",
	"FILTH_SPOT_ID_CROW",
	"FILTH_SPOT_ID_DAEMON",
	"FILTH_SPOT_ID_DRAGON",
	"FILTH_SPOT_ID_DRAGON2",
	"FILTH_SPOT_ID_EYE",
	"FILTH_SPOT_ID_GHOST",
	"FILTH_SPOT_ID_RAT",
	"FILTH_SPOT_ID_SHARK",
	"FILTH_SPOT_ID_SNAKE",
	"FILTH_SPOT_ID_VOLCANO",
	"FILTH_SPOT_ID_VOLCANO_SCUM_DISRUPTOR",
	"FILTH_SPOT_ID_WASP",
	"FILTH_SPOT_ID_WORM"
};

enum NatureFlag
{
	NATURE_FLAG_DESTROY,
	NATURE_FLAG_LIGH,
	NATURE_FLAG_LIGHT,
	NATURE_FLAG_NONE,
	NATURE_FLAG_REAL,
	TNATURE_FLAG_NONE,
	NatureFlag_count
};

typedef std::bitset<NatureFlag_count> NatureFlagSet;

const char * const NatureFlagNames[] =
{
	"NATURE_FLAG_DESTROY",
	"NATURE_FLAG_LIGH",
	"NATURE_FLAG_LIGHT",
	"NATURE_FLAG_NONE",
	"NATURE_FLAG_REAL",
	"TNATURE_FLAG_NONE"
};

typedef boost::array<int, 5> Atoms;

struct Position;

typedef std::vector<Position> PositionList;

struct DamageMolecula 
{
	bool  isAlive;
	Atoms elementsDead;
};

struct Orientation 
{
	float s;
	float x;
	float y;
	float z;
};

struct Position 
{
	float x;
	float y;
	float z;
};

struct LocalPosition 
{
	float x;
	float y;
};

struct TargetUnit 
{
	int unitID;
	int playerID;
};

struct Attackpoint 
{
	TargetUnit unit;
	TargetUnit squad;
	Position   position;
	bool       positionTarget;
};

struct SaveUnitData : public Loki::BaseVisitable<>
{
	LOKI_DEFINE_VISITABLE();
	int            unitID;
	AttributeID    attributeID;
	Position       position;
	Orientation    orientaion;
	float          radius;
	tstring        label;
	DamageMolecula damageMolecula;
};

struct SaveGeo : public SaveUnitData
{
	LOKI_DEFINE_VISITABLE();
	bool           sleep;
	float          firstSleepTime;
	float          sleepPeriod;
	float          deltaSleepPeriod;
	float          attackPeriond;
	float          deltaAttackPeriond;
	AttributeIDSet activatingUnit;
	float          activatingDistance;
};

// AttributeID in { UNIT_ATTRIBUTE_GEO_BREAK, UNIT_ATTRIBUTE_GEO_HEAD }
struct SaveGeoBreak : public SaveGeo
{
	LOKI_DEFINE_VISITABLE();
	float geoRadius;
	int   num_break;
};

struct SaveGeoFault : public SaveGeo
{
	LOKI_DEFINE_VISITABLE();
	float length;
	float angle;
};

struct SaveGeoInfluence : public SaveGeo
{
	LOKI_DEFINE_VISITABLE();
	float geoRadius;
};

struct SaveUnitLegionaryData : public SaveUnitData
{
	LOKI_DEFINE_VISITABLE();
	bool          basementInstalled;
	float         accumulatedEnergy;
	int           zeroLayerCounter;
	float         weaponChargeLevel;
	PositionList  wayPoints;
	int           weapon;
	int           transportedSoldiers;
	int           transportedOfficers;
	int           transportedTechnics;
	bool          flyingMode;
	bool          diggingMode;
	bool          inSquad;
	LocalPosition localPosition;
	bool          localPositionValid;
};

struct SaveUnitSquadData : public SaveUnitData
{
	LOKI_DEFINE_VISITABLE();
	Position              stablePosition;
	AttributeID           currentMutation;
	float                 curvatureRadius;
	SaveUnitLegionaryData squadMemebers;
	PositionList          wayPoints;
	PositionList          patrolPoints;
	int                   patrolIndex;
	PositionList          attackPoints;
	TargetUnit            squadToFollow;
	bool                  offensiveMode;
	Atoms                 atomsRequested;
	Atoms                 atomsPaused;
	float                 mutationEnergy;
};

struct SaveUnitBuildingData : public SaveUnitData
{
	LOKI_DEFINE_VISITABLE();
	bool              basementInstalled;
	float             accumulatedEnergy;
	int               zeroLayerCounter;
	float             weaponChargeLevel;
	PositionList      wayPoints;
	int               weapon;
	BuildingStatusSet buildingStatusBV;
	int               fireCount;
	bool              visible;
};

struct SaveUnitCommandCenterData : public SaveUnitData
{
	LOKI_DEFINE_VISITABLE();
	bool               basementInstalled;
	float              accumulatedEnergy;
	int                zeroLayerCounter;
	PositionList       wayPoints;
	BuildingStatusSet  buildingStatusBV;
	int                fireCount;
	bool               visible;
	SaveUnitSquadData  squad;
};

struct SaveUnitFrameData : public SaveUnitData
{
	LOKI_DEFINE_VISITABLE();
	bool              basementInstalled;
	float             accumulatedEnergy;
	int               zeroLayerCounter;
	float             weaponChargeLevel;
	PositionList      wayPoints;
	int               weapon;
	bool              attached;
	bool              attaching;
	bool              powered;
	float             spiralLevel;
	SaveUnitSquadData squad;
};

struct SaveUnitFilthData : public SaveUnitData
{
	LOKI_DEFINE_VISITABLE();
	FilthType       filthType;
	float           attackDirection;
	bool            sleep;
	float           firstSleepTime;
	float           sleepPeriod;
	float           deltaSleepPeriod;
	float           attackPeriond;
	float           deltaAttackPeriond;
	int             creatureNum;
	AttributeIDSet  activatingUnit;
	float           activatingDistance;
	FilthAttackType attack_player;
	bool            initial_geoprocess;
	int             killTimer;
	int             sleep_timer;
	bool            create_first;
	PositionList    hole_position;
	PositionList    hole_position_inited;
	bool            kill_of_end;
	PositionList    swarmList;            // NA
};

struct SaveUnitNatureData : public SaveUnitData
{
	LOKI_DEFINE_VISITABLE();
	string        modelName;
	bool          visible;
	NatureFlagSet natureFlag;
	string        chainName;
	float         chainPhase;
	float         chainPeriod;
};

// AttributeID in { UNIT_ATTRIBUTE_BUILD_MASTER, UNIT_ATTRIBUTE_TERRAIN_MASTER }
struct SaveUnitFrameChildData : public SaveUnitData
{
	LOKI_DEFINE_VISITABLE();
	bool         basementInstalled;
	float        accumulatedEnergy;
	int          zeroLayerCounter;
	float        weaponChargeLevel;
	PositionList wayPoints;
	int          weapon;
	bool         dockReadyStatus;
	bool         alarmStatus;
};

struct SaveUnitBuildingMilitaryData : public SaveUnitBuildingData
{
	LOKI_DEFINE_VISITABLE();
	TargetUnit attackTarget;
	TargetUnit lastAttackTarget;
	bool       manualAttackTarget;
};

struct SaveUnitCorridorAlphaData : public SaveUnitBuildingData
{
	LOKI_DEFINE_VISITABLE();
	bool free;
	int  passTime;
	int  timeOffset;
};

struct SaveUnitCorridorOmegaData : public SaveUnitBuildingData
{
	LOKI_DEFINE_VISITABLE();
	bool upgraded;
};

struct SaveUnitProtectorData : public SaveUnitBuildingData
{
	LOKI_DEFINE_VISITABLE();
	int  monksNumber;
	int  fieldState;
	bool enableScharge;
	bool startWhenCharged;
};

//---------------------
// visitable type lists
//---------------------

typedef LOKI_TYPELIST_16
	(
		SaveUnitData,
		SaveGeo,
		SaveGeoBreak,
		SaveGeoFault,
		SaveUnitLegionaryData,
		SaveUnitSquadData,
		SaveUnitBuildingData,
		SaveUnitCommandCenterData,
		SaveUnitFrameData,
		SaveUnitFilthData,
		SaveUnitNatureData,
		SaveUnitFrameChildData,
		SaveUnitBuildingMilitaryData,
		SaveUnitCorridorAlphaData,
		SaveUnitCorridorOmegaData,
		SaveUnitProtectorData
	)
	UnitTypeList;

#define INTEROP_UNIT_TYPE_LIST      \
   (16, (                           \
      SaveUnitData,                 \
      SaveGeo,                      \
      SaveGeoBreak,                 \
      SaveGeoFault,                 \
      SaveUnitLegionaryData,        \
      SaveUnitSquadData,            \
      SaveUnitBuildingData,         \
      SaveUnitCommandCenterData,    \
      SaveUnitFrameData,            \
      SaveUnitFilthData,            \
      SaveUnitNatureData,           \
      SaveUnitFrameChildData,       \
      SaveUnitBuildingMilitaryData, \
      SaveUnitCorridorAlphaData,    \
      SaveUnitCorridorOmegaData,    \
      SaveUnitProtectorData         \
   ))




} // namespace interop

