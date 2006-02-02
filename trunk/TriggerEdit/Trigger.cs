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
			XmlNode action_node = node.SelectSingleNode("set[@name=\"action\"]");
			if (null != action_node)
				action = ReadAction(action_node);
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

		#region implementation

		private Action ReadAction(XmlNode node)
		{
			Action action;
			// get the "code" attribute node
			XmlNode code_node = node.Attributes["code"];
			if (null == code_node)
				return  null;
			// get the action name
			string code   = code_node.InnerText;
			string prefix = "struct ";
			if (code.StartsWith(prefix))
				code = code.Substring(prefix.Length);
			// get type information
			try
			{
				// initialize main object
				Type action_type = Type.GetType("TriggerEdit.Definitions." + code);
				action = (Action)Activator.CreateInstance(action_type);
				// set properties
				PropertyInfo[] properties = action_type.GetProperties();
				foreach (PropertyInfo property in properties)
					if (
						property.Name == "attackTimer"   ||
						property.Name == "delayTimer"    ||
						property.Name == "durationTimer" ||
						property.Name == "timer")
					{
						XmlNode property_node = node.SelectSingleNode(
							"*[@name=\"" + property.Name + "\"]/int[@name=\"time\"]");
						if (null == property_node)
							continue;
						property.SetValue(
							action,
							int.Parse(property_node.InnerText),
							null);
					} 
					else if (property.PropertyType == typeof(TriggerEdit.Definitions.ControlCollection))
					{
						// get the control nodes
						XmlNode property_node = node.SelectSingleNode(
							"*[@name=\"" + property.Name + "\"]");
						if (null == property_node)
							continue;
						XmlNodeList control_nodes = property_node.SelectNodes("set");
						if (null == control_nodes || 0 == control_nodes.Count)
							continue;
						// initialize the collection
						ControlCollection controls = new ControlCollection();
						// set properties
						PropertyInfo[] control_properties = typeof(Control).GetProperties();
						foreach (XmlNode control_node in control_nodes)
						{
							Control control = new Control();
							foreach (PropertyInfo control_property in control_properties)
							{
								XmlNode control_property_node = control_node.SelectSingleNode(
									"*[@name=\"" + control_property.Name + "\"]");
								if (null == control_property_node)
									continue;
								if (control_property.PropertyType.IsEnum)
									control_property.SetValue(
										action,
										Enum.Parse(
											control_property.PropertyType,
											control_property_node.InnerText),
										null);
								else
									control_property.SetValue(
										control,
										Convert.ChangeType(
											control_property_node.InnerText,
											control_property.PropertyType),
										null);
							}
							controls.Add(control);
						}
					}
					else
					{
						XmlNode property_node = node.SelectSingleNode(
							"*[@name=\"" + property.Name + "\"]");
						if (null == property_node)
							continue;
						if (property.PropertyType.IsEnum)
							property.SetValue(
								action,
								Enum.Parse(property.PropertyType, property_node.InnerText),
								null);
						else
							property.SetValue(
								action,
								Convert.ChangeType(property_node.InnerText, property.PropertyType),
								null);
					}
			}
			catch (Exception e)
			{
				Debug.WriteLine("Action property could not be set.");
				Debug.WriteLine(e.ToString());
				return null;
			}
			return action;
		}

		private Condition ReadCondition(XmlNode node)
		{
			Condition condition;
			// get the "code" attribute node
			XmlNode code_node = node.Attributes["code"];
			if (null == code_node)
				return  null;
			// get the condition name
			string code   = code_node.InnerText;
			string prefix = "struct ";
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
				if ("ConditionSwitcher" == code)
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
		public Action    action;
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
			SQSH_BAR_SQUAD1_ID,
			SQSH_COMMANDER_ID,
			SQSH_CORRIDOR_ALPHA_ID,
			SQSH_FIELD_OFF_ID,
			SQSH_FIELD_ON_ID,
			SQSH_FRAME_TERRAIN_BUILD1_ID,
			SQSH_FRAME_TERRAIN_BUILD2_ID,
			SQSH_FRAME_TERRAIN_BUILD3_ID,
			SQSH_FRAME_TERRAIN_BUILD4_ID,
			SQSH_FRAME_TERRAIN_BUILD5_ID,
			SQSH_OFFICER_ID,
			SQSH_OFFICER_PLANT_ID,
			SQSH_PROGRESS_COLLECTED,
			SQSH_RELAY_ID,
			SQSH_SELPANEL_BRIG_CHANGE_ID,
			SQSH_SELPANEL_SELL_ID,
			SQSH_SOLDIER_PLANT_ID,
			SQSH_SQUAD_UNIT2,
			SQSH_STATION_HARKBACK_LAB_ID,
			SQSH_STATION1_ID,
			SQSH_STATION2_ID,
			SQSH_STATION3_ID,
			SQSH_STATION4_ID,
			SQSH_STATION5_ID,
			SQSH_TAB_BUILD_ID,
			SQSH_TECHNIC_PLANT_ID,
			SQSH_WORKAREA1_ID,
			SQSH_WORKAREA2_ID,
			SQSH_WORKAREA3_ID,
			SQSH_WORKAREA4_ID,
			SQSH_YADRO_EX_ID,
			SQSH_YADRO_ID
		}
		public enum Difficulty
		{
			DIFFICULTY_EASY,
			DIFFICULTY_HARD,
			DIFFICULTY_NORMAL
		}
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
		public enum PlacementStrategy
		{
			PLACEMENT_STRATEGY_CORE_CATCHING_CORRIDOR,
			PLACEMENT_STRATEGY_CORE_CATCHING,
			PLACEMENT_STRATEGY_CORE_OFFENSIVE,
			PLACEMENT_STRATEGY_CORE,
			PLACEMENT_STRATEGY_GUN_TO_ENEMY_BUILDING,
			PLACEMENT_STRATEGY_GUN,
			PLACEMENT_STRATEGY_PLANT,
			PLACEMENT_STRATEGY_SPECIAL_WEAPON,
			PLACEMENT_STRATEGY_STATION
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
		public enum SellFactor
		{
			AI_SELL_CLOSEST_TO_FRAME,
			AI_SELL_FAREST_FROM_FRAME,
			AI_SELL_IF_DAMAGE_GREATER,
			AI_SELL_IF_GUN_CANT_REACH_BUILDINGS
		}
		public enum SquadID
		{
			CHOOSE_SQUAD_1,
			CHOOSE_SQUAD_2,
			CHOOSE_SQUAD_3,
			CHOOSE_SQUAD_4,
			CHOOSE_SQUAD_5
		}
		public enum SpotType
		{
			FILTH,
			GEO
		}
		public enum SwitchMode
		{
			ON,
			OFF
		}
		public enum SwitchType
		{
			AND,
			OR
		}
		public enum TaskType
		{
			ASSIGNED,
			COMPLETED,
			FAILED,
			TO_DELETE
		}
		public enum TeleportationType
		{
			TELEPORTATION_TYPE_ALPHA,
			TELEPORTATION_TYPE_OMEGA
		}
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
		public enum Weapon
		{
			UNIT_ATTRIBUTE_GUN_BALLISTIC,
			UNIT_ATTRIBUTE_GUN_FILTH_NAVIGATOR,
			UNIT_ATTRIBUTE_GUN_SCUM_DISRUPTOR,
			UNIT_ATTRIBUTE_SCUM_SPOT
		}

		#endregion

		#region conditions

		public class Condition : ICloneable
		{
			#region interface

			public static Condition CreateInstance(XmlNode node)
			{
				Condition condition;
				// get the "code" attribute node
				XmlNode code_node = node.Attributes["code"];
				if (null == code_node)
					return  null;
				// get the condition name
				string code   = code_node.InnerText;
				string prefix = "struct ";
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
						else if (property.PropertyType == typeof(BitEnum))
						{
							BitEnum bit_enum = (BitEnum)property.GetValue(condition, null);
							if (null == bit_enum)
								continue;
							if (property_node.Name == "disjunction")
							{
								XmlNodeList value_nodes = property_node.SelectNodes("value");
								foreach (XmlNode value_node in value_nodes)
									bit_enum[Enum.Parse(
										bit_enum.GetEnumType(),
										value_node.InnerText)] = true;
							}
							else
							{
								bit_enum[Enum.Parse(
									bit_enum.GetEnumType(),
									property_node.InnerText)] = true;
							}
						}
						else
							property.SetValue(
								condition,
								Convert.ChangeType(property_node.InnerText, property.PropertyType),
								null);
					}
					// set preconditions
					if ("ConditionSwitcher" == code)
					{
						XmlNodeList wrappers = node.SelectNodes(
							"*[@name=\"conditions\"]/set");
						condition.preconditions = new ArrayList(wrappers.Count);
						if (wrappers.Count > 0)
						{
							foreach (XmlNode wrapper in wrappers)
							{
								XmlNode condition_node = wrapper.SelectSingleNode("set[@name=\"condition\"]");
								Condition precondition = Condition.CreateInstance(condition_node);
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
					name = name.Substring(name.LastIndexOf(Type.Delimiter) + "Condition".Length + 1);
					return name;
				}
			}
		}
		public class ConditionActivateSpot : Condition
		{
			private BitEnum type_ = new BitEnum(typeof(SpotType));
			public  BitEnum type
			{
				get { return type_; }
				set { type_ = value; }
			}
		}
		public class ConditionBuildingNearBuilding : Condition
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
		public class ConditionCaptureBuilding : Condition
		{
			private BitEnum object_ = new BitEnum(typeof(UnitType));
			public  BitEnum @object
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
		public class ConditionCheckBelligerent : Condition
		{
			private Race belligerent_;
			public  Race belligerent
			{
				get { return belligerent_; }
				set { belligerent_ = value; }
			}
		}
		public class ConditionClickOnButton : Condition
		{
			private BitEnum controlID_ = new BitEnum(typeof(ControlID));
			public  BitEnum controlID
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
		public class ConditionCorridorOmegaUpgraded : Condition
		{
		}
		public class ConditionCutSceneWasSkipped : Condition
		{
			private int timeMax_;
			public  int timeMax
			{
				get { return timeMax_; }
				set { timeMax_ = value; }
			}
		}
		public class ConditionDifficultyLevel : Condition
		{
			private Difficulty difficulty_;
			public  Difficulty difficulty
			{
				get { return difficulty_; }
				set { difficulty_ = value; }
			}
		}
		public class ConditionEnegyLevelLowerReserve : Condition
		{
			private float energyReserve_;
			public  float energyReserve
			{
				get { return energyReserve_; }
				set { energyReserve_ = value; }
			}
		}
		public class ConditionEnegyLevelUpperReserve : Condition
		{
			private float energyReserve_;
			public  float energyReserve
			{
				get { return energyReserve_; }
				set { energyReserve_ = value; }
			}
		}
		public class ConditionFrameState : Condition
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
		public class ConditionIsFieldOn : Condition
		{
		}
		public class ConditionKillObject : Condition
		{
			private BitEnum object_ = new BitEnum(typeof(UnitType));
			public  BitEnum @object
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
		public class ConditionKillObjectByLabel : Condition
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
		}
		public class ConditionMutationEnabled : Condition
		{
			private BitEnum unitType_ = new BitEnum(typeof(UnitType));
			public  BitEnum unitType
			{
				get { return unitType_; }
				set { unitType_ = value; }
			}
		}
		public class ConditionNumberOfBuildingByCoresCapacity : Condition
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
		public class ConditionObjectByLabelExists : Condition
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
		}
		public class ConditionObjectExists : Condition
		{
			private BitEnum object_ = new BitEnum(typeof(UnitType));
			public  BitEnum @object
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
		public class ConditionObjectNearObjectByLabel : Condition
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
			private BitEnum object_ = new BitEnum(typeof(UnitType));
			public  BitEnum @object
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
		public class ConditionOnlyMyClan : Condition
		{
		}
		public class ConditionOutOfEnergyCapacity : Condition
		{
			private int chargingPercent_;
			public  int chargingPercent
			{
				get { return chargingPercent_; }
				set { chargingPercent_ = value; }
			}
		}
		public class ConditionPlayerState : Condition
		{
			private PlayerStateType playerState_;
			public  PlayerStateType playerState
			{
				get { return playerState_; }
				set { playerState_ = value; }
			}
		}
		public class ConditionSetSquadWayPoint : Condition
		{
		}
		public class ConditionSkipCutScene : Condition
		{
		}
		public class ConditionSquadGoingToAttack : Condition
		{
			private SquadID chooseSquadID_;
			public  SquadID chooseSquadID
			{
				get { return chooseSquadID_; }
				set { chooseSquadID_ = value; }
			}
		}
		public class ConditionSquadSufficientUnits : Condition
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
			private BitEnum unitType_ = new BitEnum(typeof(UnitType));
			public  BitEnum unitType
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
		public class ConditionSwitcher : Condition
		{
			private SwitchType type_;
			public  SwitchType type
			{
				get { return type_; }
				set { type_ = value; }
			}
		}
		public class ConditionTeleportation : Condition
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
		public class ConditionTerrainLeveledNearObjectByLabel : Condition
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
		public class ConditionTimeMatched : Condition
		{
			private int time_;
			public  int time
			{
				get { return time_; }
				set { time_ = value; }
			}
		}
		public class ConditionToolzerSelectedNearObjectByLabel : Condition
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
		public class ConditionUnitClassIsGoingToBeAttacked : Condition
		{
			private BitEnum victimUnitClass_ = new BitEnum(typeof(UnitClass));
			public  BitEnum victimUnitClass
			{
				get { return victimUnitClass_; }
				set { victimUnitClass_ = value; }
			}
			private BitEnum agressorUnitClass_ = new BitEnum(typeof(UnitClass));
			public  BitEnum agressorUnitClass
			{
				get { return agressorUnitClass_; }
				set { agressorUnitClass_ = value; }
			}
		}
		public class ConditionUnitClassUnderAttack : Condition
		{
			private BitEnum victimUnitClass_ = new BitEnum(typeof(UnitClass));
			public  BitEnum victimUnitClass
			{
				get { return victimUnitClass_; }
				set { victimUnitClass_ = value; }
			}
			private BitEnum agressorUnitClass_ = new BitEnum(typeof(UnitClass));
			public  BitEnum agressorUnitClass
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
		public class ConditionWeaponIsFiring : Condition
		{
			private Weapon gun_;
			public  Weapon gun
			{
				get { return gun_; }
				set { gun_ = value; }
			}
		}

		#endregion

		#region actions

		public class Action
		{
			#region interface

			public static Action CreateInstance(XmlNode node)
			{
				Action action;
				// get the "code" attribute node
				XmlNode code_node = node.Attributes["code"];
				if (null == code_node)
					return  null;
				// get the action name
				string code   = code_node.InnerText;
				string prefix = "struct ";
				if (code.StartsWith(prefix))
					code = code.Substring(prefix.Length);
				// get type information
				try
				{
					// initialize main object
					Type action_type = Type.GetType("TriggerEdit.Definitions." + code);
					action = (Action)Activator.CreateInstance(action_type);
					// set properties
					PropertyInfo[] properties = action_type.GetProperties();
					foreach (PropertyInfo property in properties)
						if (
							property.Name == "attackTimer"   ||
							property.Name == "delayTimer"    ||
							property.Name == "durationTimer" ||
							property.Name == "timer")
						{
							XmlNode property_node = node.SelectSingleNode(
								"*[@name=\"" + property.Name + "\"]/int[@name=\"time\"]");
							if (null == property_node)
								continue;
							property.SetValue(
								action,
								int.Parse(property_node.InnerText),
								null);
						} 
						else if (property.PropertyType == typeof(TriggerEdit.Definitions.ControlCollection))
						{
							// get the control nodes
							XmlNode property_node = node.SelectSingleNode(
								"*[@name=\"" + property.Name + "\"]");
							if (null == property_node)
								continue;
							XmlNodeList control_nodes = property_node.SelectNodes("set");
							if (null == control_nodes || 0 == control_nodes.Count)
								continue;
							// initialize the collection
							ControlCollection controls = new ControlCollection();
							// set properties
							PropertyInfo[] control_properties = typeof(Control).GetProperties();
							foreach (XmlNode control_node in control_nodes)
							{
								Control control = new Control();
								foreach (PropertyInfo control_property in control_properties)
								{
									XmlNode control_property_node = control_node.SelectSingleNode(
										"*[@name=\"" + control_property.Name + "\"]");
									if (null == control_property_node)
										continue;
									if (control_property.PropertyType.IsEnum)
										control_property.SetValue(
											action,
											Enum.Parse(
											control_property.PropertyType,
											control_property_node.InnerText),
											null);
									else
										control_property.SetValue(
											control,
											Convert.ChangeType(
											control_property_node.InnerText,
											control_property.PropertyType),
											null);
								}
								controls.Add(control);
							}
						}
						else
						{
							XmlNode property_node = node.SelectSingleNode(
								"*[@name=\"" + property.Name + "\"]");
							if (null == property_node)
								continue;
							if (property.PropertyType.IsEnum)
								property.SetValue(
									action,
									Enum.Parse(property.PropertyType, property_node.InnerText),
									null);
							else
								property.SetValue(
									action,
									Convert.ChangeType(property_node.InnerText, property.PropertyType),
									null);
						}
				}
				catch (Exception e)
				{
					Debug.WriteLine("Action property could not be set.");
					Debug.WriteLine(e.ToString());
					return null;
				}
				return action;
			}

			#endregion

			#region ICloneable Members

			/// <summary>
			/// Copy the object's public properties.
			/// </summary>
			public object Clone()
			{
				Action action = (Action)Activator.CreateInstance(GetType());
				// copy properties
				PropertyInfo[] properties = GetType().GetProperties();
				foreach (PropertyInfo property in properties)
					if (property.CanRead && property.CanWrite)
						property.SetValue(action, property.GetValue(this, null), null);
				return action;
			}

			#endregion

			public ArrayList preconditions;
			[System.ComponentModel.Browsable(false)]
			public string Name
			{
				get
				{
					string name = GetType().ToString();
					name = name.Substring(name.LastIndexOf(Type.Delimiter) + "Action".Length + 1);
					return name;
				}
			}
		}
		public class ActionActivateAllSpots : Action
		{
		}
		public class ActionActivateObjectByLabel : Action
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
		}
		public class ActionAttackBySpecialWeapon : Action
		{
			private bool onlyIfAi_;
			public  bool onlyIfAi
			{
				get { return onlyIfAi_; }
				set { onlyIfAi_ = value; }
			}
			private Weapon weapon_;
			public  Weapon weapon
			{
				get { return weapon_; }
				set { weapon_ = value; }
			}
			private BitEnum unitsToAttack_ = new BitEnum(typeof(UnitType));
			public  BitEnum unitsToAttack
			{
				get { return unitsToAttack_; }
				set { unitsToAttack_ = value; }
			}
			private BitEnum unitClassToAttack_ = new BitEnum(typeof(UnitClass));
			public  BitEnum unitClassToAttack
			{
				get { return unitClassToAttack_; }
				set { unitClassToAttack_ = value; }
			}
		}
		public class ActionDeactivateAllSpots : Action
		{
		}
		public class ActionDeactivateObjectByLabel : Action
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
		}
		public class ActionDefeat : Action
		{
		}
		public class ActionDelay : Action
		{
			private int delay_;
			public  int delay
			{
				get { return delay_; }
				set { delay_ = value; }
			}
			private bool showTimer_;
			public  bool showTimer
			{
				get { return showTimer_; }
				set { showTimer_ = value; }
			}
			private bool scaleByDifficulty_;
			public  bool scaleByDifficulty
			{
				get { return scaleByDifficulty_; }
				set { scaleByDifficulty_ = value; }
			}
			private int timer_;
			public  int timer
			{
				get { return timer_; }
				set { timer_ = value; }
			}
		}
		public class ActionHoldBuilding : Action
		{
			private bool onlyIfAi_;
			public  bool onlyIfAi
			{
				get { return onlyIfAi_; }
				set { onlyIfAi_ = value; }
			}
			private Building building_;
			public  Building building
			{
				get { return building_; }
				set { building_ = value; }
			}
		}
		public class ActionInstallFrame : Action
		{
			private bool onlyIfAi_;
			public  bool onlyIfAi
			{
				get { return onlyIfAi_; }
				set { onlyIfAi_ = value; }
			}
		}
		public class ActionKillObject : Action
		{
			private BitEnum object_ = new BitEnum(typeof(UnitType));
			public  BitEnum @object
			{
				get { return object_; }
				set { object_ = value; }
			}
		}
		public class ActionMessage : Action
		{
			private string messageID_;
			[ System.ComponentModel.Description(
				"Message ID from Texts.btdb. " +
				"For example: \"Mission Tips.Mission 26.Tip 6\".") ]
			public  string messageID
			{
				get { return messageID_; }
				set { messageID_ = value; }
			}
			private string message_;
			public  string message
			{
				get { return message_; }
				set { message_ = value; }
			}
			private int delay_;
			public  int delay
			{
				get { return delay_; }
				set { delay_ = value; }
			}
			private int duration_;
			public  int duration
			{
				get { return duration_; }
				set { duration_ = value; }
			}
			private bool syncroBySound_;
			public  bool syncroBySound
			{
				get { return syncroBySound_; }
				set { syncroBySound_ = value; }
			}
			private int delayTimer_; 
			public  int delayTimer
			{
				get { return delayTimer_; }
				set { delayTimer_ = value; }
			}
			private int durationTimer_;
			public  int durationTimer
			{
				get { return durationTimer_; }
				set { durationTimer_ = value; }
			}
		}
		public class ActionOrderBuilding : Action
		{
			private bool onlyIfAi_;
			public  bool onlyIfAi
			{
				get { return onlyIfAi_; }
				set { onlyIfAi_ = value; }
			}
			private Building building_;
			public  Building building
			{
				get { return building_; }
				set { building_ = value; }
			}
			private PlacementStrategy placementStrategy_;
			public  PlacementStrategy placementStrategy
			{
				get { return placementStrategy_; }
				set { placementStrategy_ = value; }
			}
			private float energyReserve_;
			public  float energyReserve
			{
				get { return energyReserve_; }
				set { energyReserve_ = value; }
			}
			private int buildDurationMax_;
			public  int buildDurationMax
			{
				get { return buildDurationMax_; }
				set { buildDurationMax_ = value; }
			}
			[ System.ComponentModel.Description("[0-3]") ]
			private int priority_;
			public  int priority
			{
				get { return priority_; }
				set { priority_ = value; }
			}
		}
		public class ActionOscillateCamera : Action
		{
			private int duration_;
			public  int duration
			{
				get { return duration_; }
				set { duration_ = value; }
			}
			private float factor_; 
			public  float factor
			{
				get { return factor_; }
				set { factor_ = value; }
			}
		}
		public class ActionRepareObjectByLabel : Action
		{
			private string label_;
			public  string label
			{
				get { return label_; }
				set { label_ = value; }
			}
		}
		public class ActionSellBuilding : Action
		{
			private bool onlyIfAi_;
			public  bool onlyIfAi
			{
				get { return onlyIfAi_; }
				set { onlyIfAi_ = value; }
			}
			private Building building_;
			public  Building building
			{
				get { return building_; }
				set { building_ = value; }
			}
			private float damagePercent_;
			public  float damagePercent
			{
				get { return damagePercent_; }
				set { damagePercent_ = value; }
			}
			private SellFactor sellFactor_;
			public  SellFactor sellFactor
			{
				get { return sellFactor_; }
				set { sellFactor_ = value; }
			}
		}
		public class ActionSetCamera : Action
		{
			private string cameraSplineName_;
			public  string cameraSplineName
			{
				get { return cameraSplineName_; }
				set { cameraSplineName_ = value; }
			}
			private float stepTime_;
			public  float stepTime
			{
				get { return stepTime_; }
				set { stepTime_ = value; }
			}
			private float cycles_;
			public  float cycles
			{
				get { return cycles_; }
				set { cycles_ = value; }
			}
			private bool smoothTransition_;
			public  bool smoothTransition
			{
				get { return smoothTransition_; }
				set { smoothTransition_ = value; }
			}
		}
		public class ActionSetCameraAtObject : Action
		{
			private BitEnum object_ = new BitEnum(typeof(UnitType));
			public  BitEnum @object
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
			private int transitionTime_;
			public  int transitionTime
			{
				get { return transitionTime_; }
				set { transitionTime_ = value; }
			}
			private bool setFollow_;
			public  bool setFollow
			{
				get { return setFollow_; }
				set { setFollow_ = value; }
			}
			private int turnTime_;
			public  int turnTime
			{
				get { return turnTime_; }
				set { turnTime_ = value; }
			}
		}
		public class ActionSetControls : Action
		{
			private ControlCollection controls_;
			public  ControlCollection controls
			{
				get { return controls_; }
				set { controls_ = value; }
			}
		}
		public class ActionSetInterface : Action
		{
			private bool enableInterface_;
			public  bool enableInterface
			{
				get { return enableInterface_; }
				set { enableInterface_ = value; }
			}
		}
		public class ActionSquadAttack : Action
		{
			private bool onlyIfAi_;
			public  bool onlyIfAi
			{
				get { return onlyIfAi_; }
				set { onlyIfAi_ = value; }
			}
			private SquadID chooseSquadID_;
			public  SquadID chooseSquadID
			{
				get { return chooseSquadID_; }
				set { chooseSquadID_ = value; }
			}
			private BitEnum attackByType_ = new BitEnum(typeof(UnitType));
			public  BitEnum attackByType
			{
				get { return attackByType_; }
				set { attackByType_ = value; }
			}
			private BitEnum unitsToAttack_ = new BitEnum(typeof(UnitType));
			public  BitEnum unitsToAttack
			{
				get { return unitsToAttack_; }
				set { unitsToAttack_ = value; }
			}
			private BitEnum unitClassToAttack_ = new BitEnum(typeof(UnitClass));
			public  BitEnum unitClassToAttack
			{
				get { return unitClassToAttack_; }
				set { unitClassToAttack_ = value; }
			}
			private bool offensive_;
			public  bool offensive
			{
				get { return offensive_; }
				set { offensive_ = value; }
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
			private int attackTime_;
			public  int attackTime
			{
				get { return attackTime_; }
				set { attackTime_ = value; }
			}
			[ System.ComponentModel.Description("[0-2]") ]
			private int remutateCounter_;
			public  int remutateCounter
			{
				get { return remutateCounter_; }
				set { remutateCounter_ = value; }
			}
			private bool holdProduction_;
			public  bool holdProduction
			{
				get { return holdProduction_; }
				set { holdProduction_ = value; }
			}
			private float squadFollowDistance_;
			public  float squadFollowDistance
			{
				get { return squadFollowDistance_; }
				set { squadFollowDistance_ = value; }
			}
			private SquadID squadToFollowBy_;
			public  SquadID squadToFollowBy
			{
				get { return squadToFollowBy_; }
				set { squadToFollowBy_ = value; }
			}
			private bool ignoreLastTarget_;
			public  bool ignoreLastTarget
			{
				get { return ignoreLastTarget_; }
				set { ignoreLastTarget_ = value; }
			}
			private bool returnToBase_;
			public  bool returnToBase
			{
				get { return returnToBase_; }
				set { returnToBase_ = value; }
			}
			private bool interruptable_;
			public  bool interruptable
			{
				get { return interruptable_; }
				set { interruptable_ = value; }
			}
			private int attackTimer_;
			public  int attackTimer
			{
				get { return attackTimer_; }
				set { attackTimer_ = value; }
			}
		}
		public class ActionSquadOrderUnits : Action
		{
			private bool onlyIfAi_;
			public  bool onlyIfAi
			{
				get { return onlyIfAi_; }
				set { onlyIfAi_ = value; }
			}
			private SquadID chooseSquadID_;
			public  SquadID chooseSquadID
			{
				get { return chooseSquadID_; }
				set { chooseSquadID_ = value; }
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
			private float energyReserve_;
			public  float energyReserve
			{
				get { return energyReserve_; }
				set { energyReserve_ = value; }
			}
		}
		public class ActionSwitchFieldOn : Action
		{
			private bool onlyIfAi_;
			public  bool onlyIfAi
			{
				get { return onlyIfAi_; }
				set { onlyIfAi_ = value; }
			}
			private int duration_;
			public  int duration
			{
				get { return duration_; }
				set { duration_ = value; }
			}
			private float energyReserve_;
			public  float energyReserve
			{
				get { return energyReserve_; }
				set { energyReserve_ = value; }
			}
			private bool allCores_;
			public  bool allCores
			{
				get { return allCores_; }
				set { allCores_ = value; }
			}
			private bool onlyIfCoreDamaged_;
			public  bool onlyIfCoreDamaged
			{
				get { return onlyIfCoreDamaged_; }
				set { onlyIfCoreDamaged_ = value; }
			}
			private int timer_;
			public  int timer
			{
				get { return timer_; }
				set { timer_ = value; }
			}
		}
		public class ActionSwitchGuns : Action
		{
			private bool onlyIfAi_;
			public  bool onlyIfAi
			{
				get { return onlyIfAi_; }
				set { onlyIfAi_ = value; }
			}
			private SwitchMode mode_;
			public  SwitchMode mode
			{
				get { return mode_; }
				set { mode_ = value; }
			}
			private Weapon gunID_;
			public  Weapon gunID
			{
				get { return gunID_; }
				set { gunID_ = value; }
			}
		}
		public class ActionTask : Action
		{
			private TaskType type_;
			public  TaskType type
			{
				get { return type_; }
				set { type_ = value; }
			}
			[ System.ComponentModel.Description(
				  "Task ID from Texts.btdb. " +
				  "For example: \"Mission Tips.Mission 15.Task 1\".") ]
			private string taskID_;
			public  string taskID
			{
				get { return taskID_; }
				set { taskID_ = value; }
			}
			private int duration_;
			public  int duration
			{
				get { return duration_; }
				set { duration_ = value; }
			}
			private bool syncroBySound_;
			public  bool syncroBySound
			{
				get { return syncroBySound_; }
				set { syncroBySound_ = value; }
			}
			private bool showTips_;
			public  bool showTips
			{
				get { return showTips_; }
				set { showTips_ = value; }
			}
			private int durationTimer_;
			public  int durationTimer
			{
				get { return durationTimer_; }
				set { durationTimer_ = value; }
			}
		}
		public class ActionTeleportationOut : Action
		{
		}
		public class ActionVictory : Action
		{
		}


		#endregion
	}
}