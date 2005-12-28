using System;
using System.Collections;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Xml;
using TriggerEdit.Definitions;

namespace TriggerEdit
{
	public struct Trigger
	{
		#region nested types

		public enum State
		{
			Checking,
			Sleeping,
			Done
		}
		public struct Link
		{
			public int   target;
			public Color color;
			public bool  is_active;
		};

		#endregion

		#region interface

		public void Serialize(XmlNode node)
		{
			// read coordinates
			X = int.Parse(node.SelectSingleNode(
				"set[@name=\"cellIndex\"]/int[@name=\"x\"]").InnerText);
			Y = int.Parse(node.SelectSingleNode(
				"set[@name=\"cellIndex\"]/int[@name=\"y\"]").InnerText);
			// read name
			name = node.SelectSingleNode(
				"string[@name=\"name\"]").InnerText;
			// read state
			XmlNode state_node = node.SelectSingleNode(
				"value[@name=\"state_\"]");
			switch (state_node.InnerText)
			{
				case "CHECKING": state = State.Checking; break;
				case "SLEEPING": state = State.Sleeping; break;
				case "DONE":     state = State.Done;     break;
			}
			// read action
//			XmlNode action = node.SelectSingleNode("set[@name=\"action\"]");
//			if (null != action)
//				triggers_[i].action = new Property(action);
			// read condition
			XmlNode condition_node = node.SelectSingleNode("set[@name=\"condition\"]");
			if (null != condition_node)
				condition = ReadCondition(condition_node);
		}

		public XmlNode Serialize()
		{
			return null;
		}

		#endregion

		#region internal implementation

		private Condition ReadCondition(XmlNode node)
		{
			Condition condition;
			// get the "code" attribute node
			XmlNode code_node = node.Attributes["code"];
			if (null == code_node)
				return  null;
			// get the condition name
			string code   = code_node.InnerText;
			string prefix = "struct Condition";
			if (code.StartsWith(prefix))
				code = code.Substring(prefix.Length);
			// get type information
			try
			{
				// initialize main object
				Type condition_type = Type.GetType("TriggerEdit.Definitions." + code);
				condition = (Condition)Activator.CreateInstance(condition_type);
				// set properties
				PropertyInfo[] properties = condition_type.GetProperties();
				foreach (PropertyInfo property in properties)
				{
					XmlNode property_node = node.SelectSingleNode(
						"*[@name=\"" + property.Name + "\"]");
					if (null == property_node)
						continue;
					if (property.PropertyType.IsEnum)
						property.SetValue(
							condition,
							Enum.Parse(property.PropertyType, property_node.InnerText),
							null);
					else
						property.SetValue(
							condition,
							Convert.ChangeType(property_node.InnerText, property.PropertyType),
							null);
				}
				// set preconditions
				if ("Switcher" == code)
				{
					XmlNodeList wrappers = node.SelectNodes(
						"*[@name=\"conditions\"]/set");
					condition.preconditions = new ArrayList(wrappers.Count);
					if (wrappers.Count > 0)
					{
						foreach (XmlNode wrapper in wrappers)
						{
							XmlNode condition_node = wrapper.SelectSingleNode("set[@name=\"condition\"]");
							Condition precondition = ReadCondition(condition_node);
							if (null == precondition)
								continue;
							XmlNode type = wrapper.SelectSingleNode("value[@name=\"type\"]");
							if (null != type)
								precondition.condition_type =
									(ConditionType)Enum.Parse(typeof(ConditionType), type.InnerText);
							condition.preconditions.Add(precondition);
						}
					}
				}
			}
			catch (Exception e)
			{
				Debug.WriteLine("Condition property could not be set.");
				Debug.WriteLine(e.ToString());
				return null;
			}
			return condition;
		}

		#endregion

		#region data

		public int       X;
		public int       Y;
		public float     v_x_;
		public float     v_y_;
		public Property  action;
		public Color     color_;
		public Color     outline_;
		public Condition condition;
		public ArrayList links;
		public State     state;
		public string    name;

		#endregion
	}

	class TriggerComparer : IComparer
	{
		#region IComparer Members

		public int Compare(object x, object y)
		{
			Trigger cell1 = (Trigger)x;
			Trigger cell2 = (Trigger)y;
			if (cell1.X == cell2.X)
				return cell1.Y - cell2.Y;
			else
				return cell1.X - cell2.X;
		}

		#endregion
	}

	namespace Definitions
	{
		#region types

		public enum Building
		{
			UNIT_ATTRIBUTE_COLLECTOR,
			UNIT_ATTRIBUTE_COMMANDER,
			UNIT_ATTRIBUTE_CORE,
			UNIT_ATTRIBUTE_ELECTRO_CANNON,
			UNIT_ATTRIBUTE_GUN_HOWITZER,
			UNIT_ATTRIBUTE_GUN_SUBCHASER,
			UNIT_ATTRIBUTE_LASER_CANNON,
			UNIT_ATTRIBUTE_OFFICER_PLANT,
			UNIT_ATTRIBUTE_ROCKET_LAUNCHER,
			UNIT_ATTRIBUTE_SOLDIER_PLANT,
			UNIT_ATTRIBUTE_TECHNIC_PLANT
		}
		public enum ConditionType
		{
			NORMAL,
			INVERTED
		}
		public enum ControlID
		{
			SQSH_FIELD_OFF_ID,
			SQSH_FIELD_ON_ID,
			SQSH_FRAME_TERRAIN_BUILD1_ID,
			SQSH_FRAME_TERRAIN_BUILD2_ID,
			SQSH_FRAME_TERRAIN_BUILD3_ID,
			SQSH_FRAME_TERRAIN_BUILD4_ID,
			SQSH_FRAME_TERRAIN_BUILD5_ID,
			SQSH_OFFICER_ID,
			SQSH_RELAY_ID,
			SQSH_SELPANEL_BRIG_CHANGE_ID,
			SQSH_SQUAD_UNIT2,
			SQSH_STATION2_ID,
			SQSH_STATION5_ID,
			SQSH_TAB_BUILD_ID,
			SQSH_WORKAREA3_ID,
			SQSH_YADRO_ID
		}
		public enum Difficulty
		{
			DIFFICULTY_EASY,
			DIFFICULTY_HARD,
			DIFFICULTY_NORMAL
		}
		[Flags]
		public enum FrameStateType
		{
			AI_FRAME_STATE_INSTALLED,
			AI_FRAME_STATE_INSTALLING,
			AI_FRAME_STATE_SPIRAL_CHARGING,
			AI_FRAME_STATE_TELEPORTATION_ENABLED,
			AI_FRAME_STATE_TELEPORTATION_STARTED
		}	
		public enum Operator
		{
			COMPARE_GREATER,
			COMPARE_GREATER_EQ,
			COMPARE_LESS,
			COMPARE_LESS_EQ
		}
		public enum PlayerStateType
		{
			PLAYER_STATE_UNABLE_TO_PLACE_BUILDING,
			PLAYER_STATE_UNABLE_TO_PLACE_CORE
		}	
		public enum PlayerType
		{
			AI_PLAYER_TYPE_ANY,
			AI_PLAYER_TYPE_ENEMY,
			AI_PLAYER_TYPE_ME,
			AI_PLAYER_TYPE_WORLD
		}
		public enum Race
		{
			EMPIRE,
			EXODUS,
			HARKBACKHOOD
		}
		public enum SquadID
		{
			CHOOSE_SQUAD_1,
			CHOOSE_SQUAD_2,
			CHOOSE_SQUAD_3,
			CHOOSE_SQUAD_4,
			CHOOSE_SQUAD_5
		}
		[Flags]
		public enum SpotType
		{
			FILTH,
			GEO
		}	
		public enum SuperWeapon
		{
			UNIT_ATTRIBUTE_GUN_BALLISTIC,
			UNIT_ATTRIBUTE_GUN_FILTH_NAVIGATOR,
			UNIT_ATTRIBUTE_GUN_SCUM_DISRUPTOR
		}
		public enum SwitchType
		{
			AND,
			OR
		}
		public enum TeleportationType
		{
			TELEPORTATION_TYPE_ALPHA,
			TELEPORTATION_TYPE_OMEGA
		}
		[Flags]
		public enum UnitClass
		{
			UNIT_CLASS_AIR,
			UNIT_CLASS_AIR_FILTH,
			UNIT_CLASS_AIR_HEAVY,
			UNIT_CLASS_BASE,
			UNIT_CLASS_BLOCK,
			UNIT_CLASS_BUILDER,
			UNIT_CLASS_FRAME,
			UNIT_CLASS_GROUND_FILTH,
			UNIT_CLASS_HEAVY,
			UNIT_CLASS_IGNORE,
			UNIT_CLASS_LIGHT,
			UNIT_CLASS_MEDIUM,
			UNIT_CLASS_MISSILE,
			UNIT_CLASS_NATURE,
			UNIT_CLASS_STRUCTURE,
			UNIT_CLASS_STRUCTURE_CORE,
			UNIT_CLASS_STRUCTURE_ENVIRONMENT,
			UNIT_CLASS_STRUCTURE_GUN,
			UNIT_CLASS_STRUCTURE_SPECIAL,
			UNIT_CLASS_TRUCK,
			UNIT_CLASS_UNDERGROUND,
			UNIT_CLASS_UNDERGROUND_FILTH
		}
		[Flags]
		public enum UnitType
		{
			UNIT_ATTRIBUTE_ANY,
			UNIT_ATTRIBUTE_BLACK_HOLE,
			UNIT_ATTRIBUTE_BOMB_STATION1,
			UNIT_ATTRIBUTE_BOMB_STATION2,
			UNIT_ATTRIBUTE_BOMB_STATION3,
			UNIT_ATTRIBUTE_BOMBER,
			UNIT_ATTRIBUTE_BUILD_MASTER,
			UNIT_ATTRIBUTE_CEPTOR,
			UNIT_ATTRIBUTE_COLLECTOR,
			UNIT_ATTRIBUTE_COMMANDER,
			UNIT_ATTRIBUTE_CONDUCTOR,
			UNIT_ATTRIBUTE_CORE,
			UNIT_ATTRIBUTE_CORRIDOR_ALPHA,
			UNIT_ATTRIBUTE_CORRIDOR_OMEGA,
			UNIT_ATTRIBUTE_DIGGER,
			UNIT_ATTRIBUTE_DISINTEGRATOR,
			UNIT_ATTRIBUTE_EFLAIR,
			UNIT_ATTRIBUTE_ELECTRO_CANNON,
			UNIT_ATTRIBUTE_ELECTRO_STATION1,
			UNIT_ATTRIBUTE_ELECTRO_STATION2,
			UNIT_ATTRIBUTE_ELECTRO_STATION3,
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
			UNIT_ATTRIBUTE_GUN_BALLISTIC,
			UNIT_ATTRIBUTE_GUN_FILTH_NAVIGATOR,
			UNIT_ATTRIBUTE_GUN_HOWITZER,
			UNIT_ATTRIBUTE_GUN_SCUM_DISRUPTOR,
			UNIT_ATTRIBUTE_GUN_SUBCHASER,
			UNIT_ATTRIBUTE_GYROID,
			UNIT_ATTRIBUTE_HARKBACK_STATION1,
			UNIT_ATTRIBUTE_HARKBACK_STATION2,
			UNIT_ATTRIBUTE_HARKBACK_STATION3,
			UNIT_ATTRIBUTE_IMPALER,
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
			UNIT_ATTRIBUTE_STRAFER,
			UNIT_ATTRIBUTE_SUBTERRA_STATION1,
			UNIT_ATTRIBUTE_SUBTERRA_STATION2,
			UNIT_ATTRIBUTE_TECHNIC,
			UNIT_ATTRIBUTE_TECHNIC_PLANT,
			UNIT_ATTRIBUTE_TERRAIN_MASTER,
			UNIT_ATTRIBUTE_UNSEEN,
			UNIT_ATTRIBUTE_WARGON
		}

		#endregion

		public class Condition : ICloneable
		{
			#region ICloneable Members

			/// <summary>
			/// Copy the object's public properties and preconditions.
			/// </summary>
			public object Clone()
			{
				Condition condition = (Condition)Activator.CreateInstance(GetType());
				// copy properties
				PropertyInfo[] properties = GetType().GetProperties();
				foreach (PropertyInfo property in properties)
					if (property.CanRead && property.CanWrite)
						property.SetValue(condition, property.GetValue(this, null), null);
				// copy preconditions
				if (null != preconditions)
				{
					condition.preconditions = new ArrayList(preconditions.Count);
					foreach (Condition precondition in preconditions)
						condition.preconditions.Add(precondition.Clone());
				}
				return condition;
			}

			#endregion
			private bool state__ = true;
			public  bool state_
			{
				get { return state__; }
				set { state__ = value; }
			}
			private ConditionType condition_type_ = ConditionType.NORMAL;
			public  ConditionType condition_type
			{
				get { return condition_type_; }
				set { condition_type_ = value; }
			}
			public ArrayList preconditions;
			[System.ComponentModel.Browsable(false)]
			public string Name
			{
				get
				{
					string name = GetType().ToString();
					name = name.Substring(name.LastIndexOf(Type.Delimiter) + 1);
					return name;
				}
			}
		}
		public class ActivateSpot : Condition
		{
			private SpotType type_;
			public  SpotType type
			{
				get { return type_; }
				set { type_ = value; }
			}
		}
		public class BuildingNearBuilding : Condition
		{
			private float distance_;
			public  float distance
			{
				get { return distance_; }
				set { distance_ = value; }
			}
			private PlayerType playerType1_;
			public  PlayerType playerType1
			{
				get { return playerType1_; }
				set { playerType1_ = value; }
			}
			private PlayerType playerType2_;
			public  PlayerType playerType2
			{
				get { return playerType2_; }
				set { playerType2_ = value; }
			}
		}
		public class CaptureBuilding : Condition
		{
			private UnitType object_;
			public  UnitType @object
			{
				get { return object_; }
				set { object_ = value; }
			}
			private PlayerType playerType_;
			public  PlayerType playerType
			{
				get { return playerType_; }
				set { playerType_ = value; }
			}
		}
		public class CheckBelligerent : Condition
		{
			private Race belligerent_;
			public  Race belligerent
			{
				get { return belligerent_; }
				set { belligerent_ = value; }
			}
		}
		public class ClickOnButton : Condition
		{
			private ControlID controlID_;
			public  ControlID controlID
			{
				get { return controlID_; }
				set { controlID_ = value; }
			}
			private int counter_;
			public  int counter
			{
				get { return counter_; }
				set { counter_ = value; }
			}
		}
		public class CorridorOmegaUpgraded : Condition
		{
		}
		public class CutSceneWasSkipped : Condition
		{
			private int timeMax_;
			public  int timeMax
			{
				get { return timeMax_; }
				set { timeMax_ = value; }
			}
		}
		public class DifficultyLevel : Condition
		{
			private Difficulty difficulty_;
			public  Difficulty difficulty
			{
				get { return difficulty_; }
				set { difficulty_ = value; }
			}
		}
		public class EnegyLevelLowerReserve : Condition
		{
			private float energyReserve_;
			public  float energyReserve
			{
				get { return energyReserve_; }
				set { energyReserve_ = value; }
			}
		}
		public class EnegyLevelUpperReserve : Condition
		{
			private float energyReserve_;
			public  float energyReserve
			{
				get { return energyReserve_; }
				set { energyReserve_ = value; }
			}
		}
		public class FrameState : Condition
		{
			private FrameStateType f_state_;
			public  FrameStateType state
			{
				get { return f_state_; }
				set { f_state_ = value; }
			}
			private PlayerType playerType_;
			public  PlayerType playerType
			{
				get { return playerType_; }
				set { playerType_ = value; }
			}
			private int spiralChargingPercent_;
			public  int spiralChargingPercent
			{
				get { return spiralChargingPercent_; }
				set { spiralChargingPercent_ = value; }
			}
		}
		public class IsFieldOn : Condition
		{
		}
		public class KillObject : Condition
		{
			private UnitType object_;
			public  UnitType @object
			{
				get { return object_; }
				set { object_ = value; }
			}
			private int counter_;
			public  int counter
			{
				get { return counter_; }
				set { counter_ = value; }
			}
			private PlayerType playerType_;
			public  PlayerType playerType
			{
				get { return playerType_; }
				set { playerType_ = value; }
			}
		}
		public class KillObjectByLabel : Condition
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
		}
		public class MutationEnabled : Condition
		{
			private UnitType unitType_;
			public  UnitType unitType
			{
				get { return unitType_; }
				set { unitType_ = value; }
			}
		}
		public class NumberOfBuildingByCoresCapacity : Condition
		{
			private Building building_;
			public  Building building
			{
				get { return building_; }
				set { building_ = value; }
			}
			private Building building2_;
			public  Building building2
			{
				get { return building2_; }
				set { building2_ = value; }
			}
			private float factor_;
			public  float factor
			{
				get { return factor_; }
				set { factor_ = value; }
			}
			private Operator compareOp_;
			public  Operator compareOp
			{
				get { return compareOp_; }
				set { compareOp_ = value; }
			}
			private PlayerType playerType_;
			public  PlayerType playerType
			{
				get { return playerType_; }
				set { playerType_ = value; }
			}
		}
		public class ObjectByLabelExists : Condition
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
		}
		public class ObjectExists : Condition
		{
			private UnitType object_;
			public  UnitType @object
			{
				get { return object_; }
				set { object_ = value; }
			}
			private int counter_;
			public  int counter
			{
				get { return counter_; }
				set { counter_ = value; }
			}
			private PlayerType playerType_;
			public  PlayerType playerType
			{
				get { return playerType_; }
				set { playerType_ = value; }
			}
			private bool constructedAndConstructing_;
			public  bool constructedAndConstructing
			{
				get { return constructedAndConstructing_; }
				set { constructedAndConstructing_ = value; }
			}
		}
		public class ObjectNearObjectByLabel : Condition
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
			private UnitType object_;
			public  UnitType @object
			{
				get { return object_; }
				set { object_ = value; }
			}
			private bool objectConstructed_;
			public  bool objectConstructed
			{
				get { return objectConstructed_; }
				set { objectConstructed_ = value; }
			}
			private PlayerType playerType_;
			public  PlayerType playerType
			{
				get { return playerType_; }
				set { playerType_ = value; }
			}
			private float distance_;
			public  float Distance
			{
				get { return distance_; }
				set { distance_ = value; }
			}
		}
		public class OnlyMyClan : Condition
		{
		}
		public class OutOfEnergyCapacity : Condition
		{
			private int chargingPercent_;
			public  int chargingPercent
			{
				get { return chargingPercent_; }
				set { chargingPercent_ = value; }
			}
		}
		public class PlayerState : Condition
		{
			private PlayerStateType playerState_;
			public  PlayerStateType playerState
			{
				get { return playerState_; }
				set { playerState_ = value; }
			}
		}
		public class SetSquadWayPoint : Condition
		{
		}
		public class SkipCutScene : Condition
		{
		}
		public class SquadGoingToAttack : Condition
		{
			private SquadID chooseSquadID_;
			public  SquadID chooseSquadID
			{
				get { return chooseSquadID_; }
				set { chooseSquadID_ = value; }
			}
		}
		public class SquadSufficientUnits : Condition
		{
			private PlayerType playerType_;
			public  PlayerType playerType
			{
				get { return playerType_; }
				set { playerType_ = value; }
			}
			private SquadID chooseSquadID_;
			public  SquadID chooseSquadID
			{
				get { return chooseSquadID_; }
				set { chooseSquadID_ = value; }
			}
			private UnitType unitType_;
			public  UnitType unitType
			{
				get { return unitType_; }
				set { unitType_ = value; }
			}
			private Operator compareOperator_;
			public  Operator compareOperator
			{
				get { return compareOperator_; }
				set { compareOperator_ = value; }
			}
			private int unitsNumber_;
			public  int unitsNumber
			{
				get { return unitsNumber_; }
				set { unitsNumber_ = value; }
			}
			private int soldiers_;
			public  int soldiers
			{
				get { return soldiers_; }
				set { soldiers_ = value; }
			}
			private int officers_;
			public  int officers
			{
				get { return officers_; }
				set { officers_ = value; }
			}
			private int technics_;
			public  int technics
			{
				get { return technics_; }
				set { technics_ = value; }
			}
		}
		public class Switcher : Condition
		{
			private SwitchType type_;
			public  SwitchType type
			{
				get { return type_; }
				set { type_ = value; }
			}
		}
		public class Teleportation : Condition
		{
			private TeleportationType teleportationType_;
			public  TeleportationType teleportationType
			{
				get { return teleportationType_; }
				set { teleportationType_ = value; }
			}
			private PlayerType playerType_;
			public  PlayerType playerType
			{
				get { return playerType_; }
				set { playerType_ = value; }
			}
		}
		public class TerrainLeveledNearObjectByLabel : Condition
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
			private int radius_;
			public  int radius
			{
				get { return radius_; }
				set { radius_ = value; }
			}
		}
		public class TimeMatched : Condition
		{
			private int time_;
			public  int time
			{
				get { return time_; }
				set { time_ = value; }
			}
		}
		public class ToolzerSelectedNearObjectByLabel : Condition
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
			private int radius_;
			public  int radius
			{
				get { return radius_; }
				set { radius_ = value; }
			}
		}
		public class UnitClassIsGoingToBeAttacked : Condition
		{
			private UnitClass victimUnitClass_;
			public  UnitClass victimUnitClass
			{
				get { return victimUnitClass_; }
				set { victimUnitClass_ = value; }
			}
			private UnitClass agressorUnitClass_;
			public  UnitClass agressorUnitClass
			{
				get { return agressorUnitClass_; }
				set { agressorUnitClass_ = value; }
			}
		}
		public class UnitClassUnderAttack : Condition
		{
			private UnitClass victimUnitClass_;
			public  UnitClass victimUnitClass
			{
				get { return victimUnitClass_; }
				set { victimUnitClass_ = value; }
			}
			private UnitClass agressorUnitClass_;
			public  UnitClass agressorUnitClass
			{
				get { return agressorUnitClass_; }
				set { agressorUnitClass_ = value; }
			}
			private int damagePercent_;
			public  int damagePercent
			{
				get { return damagePercent_; }
				set { damagePercent_ = value; }
			}
			private PlayerType playerType_;
			public  PlayerType playerType
			{
				get { return playerType_; }
				set { playerType_ = value; }
			}
		}
		public class WeaponIsFiring : Condition
		{
			private SuperWeapon gun_;
			public  SuperWeapon gun
			{
				get { return gun_; }
				set { gun_ = value; }
			}
		}
	}
}