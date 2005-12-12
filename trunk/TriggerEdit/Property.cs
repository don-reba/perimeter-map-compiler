using System;
using System.Collections;
using System.Drawing;
using System.Windows.Forms;
using System.Xml;

namespace TriggerEdit
{
	public class Property
	{
		public Property(XmlNode node)
		{
			if (null == strings_)
				InitializeStrings();
			// get the "code" attribute node
			XmlNode code_node = node.Attributes["code"];
			if (null == code_node)
				return;
			// get the "code" string
			string code = code_node.InnerText;
			if (code.StartsWith("struct "))
				code = code.Substring("struct ".Length);
			if (!strings_.ContainsKey(code))
				throw new Exception("unknown property");
			// initialize the object
			name_ = code;
			ReadFields(node);
		}

		public TreeNode GetTreeNode()
		{
			TreeNode node = new TreeNode(name_);
			ArrayList children = (ArrayList)strings_[name_];
			for (int i = 0; i != fields_.Length; ++i)
				if (null != fields_[i])
					node.Nodes.Add(children[i] + ": " + fields_[i]);
			if (switch_)
			{
				TreeNode conditions = new TreeNode("conditions:");
				if (conditions_.Count != 0)
				{
					foreach (Property condition in conditions_)
						conditions.Nodes.Add(condition.GetTreeNode());
				}
				else
					conditions.Nodes.Add("none");
				node.Nodes.Add(conditions);
			}
			return node;
		}

		private void ReadFields(XmlNode node)
		{
			ArrayList children = (ArrayList)strings_[name_];
			fields_ = new string[children.Count];
			for (int i = 0; i != fields_.Length; ++i)
			{
				if ("internalColor_" == (string)children[i])
					continue;
				string name = node.SelectSingleNode("*[@name=\"" + children[i] + "\"]").InnerText;
				if ((string)children[i] != "conditions")
					fields_[i] = name;
				else
				{
					switch_ = true;
					XmlNodeList conditions = node.SelectNodes(
						"array[@name=\"conditions\"]" +
						"/set"                        +
						"/set[@name=\"condition\"]");
					foreach (XmlNode condition in conditions)
						conditions_.Add(new Property(condition));
				}
			}
		}

		private void InitializeStrings()
		{
			string    key;
			ArrayList val;

			strings_ = new Hashtable();

			#region conditions

			// ConditionActivateSpot
			key = "ConditionActivateSpot";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("type");
			strings_.Add(key, val);
			// ConditionBuildingNearBuilding
			key = "ConditionBuildingNearBuilding";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("distance");
			val.Add("playerType1");
			val.Add("playerType2");
			strings_.Add(key, val);
			// ConditionCaptureBuilding
			key = "ConditionCaptureBuilding";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("object");
			val.Add("playerType");
			strings_.Add(key, val);
			// ConditionCheckBelligerent
			key = "ConditionCheckBelligerent";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("belligerent");
			strings_.Add(key, val);
			// ConditionClickOnButton
			key = "ConditionClickOnButton";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("controlID");
			val.Add("counter");
			strings_.Add(key, val);
			// ConditionCorridorOmegaUpgraded
			key = "ConditionCorridorOmegaUpgraded";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ConditionCutSceneWasSkipped
			key = "ConditionCutSceneWasSkipped";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("timeMax");
			strings_.Add(key, val);
			// ConditionDifficultyLevel
			key = "ConditionDifficultyLevel";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("difficulty");
			strings_.Add(key, val);
			// ConditionEnegyLevelLowerReserve
			key = "ConditionEnegyLevelLowerReserve";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("energyReserve");
			strings_.Add(key, val);
			// ConditionEnegyLevelUpperReserve
			key = "ConditionEnegyLevelUpperReserve";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("energyReserve");
			strings_.Add(key, val);
			// ConditionFrameState
			key = "ConditionFrameState";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("state");
			val.Add("playerType");
			val.Add("spiralChargingPercent");
			strings_.Add(key, val);
			// ConditionIsFieldOn
			key = "ConditionIsFieldOn";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ConditionKillObject
			key = "ConditionKillObject";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("object");
			val.Add("counter");
			val.Add("playerType");
			strings_.Add(key, val);
			// ConditionKillObjectByLabel
			key = "ConditionKillObjectByLabel";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("label");
			strings_.Add(key, val);
			// ConditionMutationEnabled
			key = "ConditionMutationEnabled";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("unitType");
			strings_.Add(key, val);
			// ConditionNumberOfBuildingByCoresCapacity
			key = "ConditionNumberOfBuildingByCoresCapacity";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("building");
			val.Add("factor");
			val.Add("compareOp");
			val.Add("building2");
			val.Add("playerType");
			strings_.Add(key, val);
			// ConditionObjectByLabelExists
			key = "ConditionObjectByLabelExists";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("label");
			strings_.Add(key, val);
			// ConditionObjectExists
			key = "ConditionObjectExists";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("object");
			val.Add("counter");
			val.Add("playerType");
			val.Add("constructedAndConstructing");
			strings_.Add(key, val);
			// ConditionObjectNearObjectByLabel
			key = "ConditionObjectNearObjectByLabel";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("label");
			val.Add("object");
			val.Add("objectConstructed");
			val.Add("playerType");
			val.Add("distance");
			strings_.Add(key, val);
			// ConditionOnlyMyClan
			key = "ConditionOnlyMyClan";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ConditionOutOfEnergyCapacity
			key = "ConditionOutOfEnergyCapacity";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("chargingPercent");
			strings_.Add(key, val);
			// ConditionPlayerState
			key = "ConditionPlayerState";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("playerState");
			strings_.Add(key, val);
			// ConditionSetSquadWayPoint
			key = "ConditionSetSquadWayPoint";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ConditionSkipCutScene
			key = "ConditionSkipCutScene";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ConditionSquadGoingToAttack
			key = "ConditionSquadGoingToAttack";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("chooseSquadID");
			strings_.Add(key, val);
			// ConditionSquadSufficientUnits
			key = "ConditionSquadSufficientUnits";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("playerType");
			val.Add("chooseSquadID");
			val.Add("unitType");
			val.Add("compareOperator");
			val.Add("unitsNumber");
			val.Add("soldiers");
			val.Add("officers");
			val.Add("technics");
			strings_.Add(key, val);
			// ConditionSwitcher
			key = "ConditionSwitcher";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("type");
			val.Add("conditions");
			strings_.Add(key, val);
			// ConditionTeleportation
			key = "ConditionTeleportation";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("teleportationType");
			val.Add("playerType");
			strings_.Add(key, val);
			// ConditionTerrainLeveledNearObjectByLabel
			key = "ConditionTerrainLeveledNearObjectByLabel";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("label");
			val.Add("radius");
			strings_.Add(key, val);
			// ConditionTimeMatched
			key = "ConditionTimeMatched";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("time");
			strings_.Add(key, val);
			// ConditionToolzerSelectedNearObjectByLabel
			key = "ConditionToolzerSelectedNearObjectByLabel";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("label");
			val.Add("radius");
			strings_.Add(key, val);
			// ConditionUnitClassIsGoingToBeAttacked
			key = "ConditionUnitClassIsGoingToBeAttacked";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("victimUnitClass");
			val.Add("agressorUnitClass");
			strings_.Add(key, val);
			// ConditionUnitClassUnderAttack
			key = "ConditionUnitClassUnderAttack";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("victimUnitClass");
			val.Add("damagePercent");
			val.Add("agressorUnitClass");
			val.Add("playerType");
			strings_.Add(key, val);
			// ConditionWeaponIsFiring
			key = "ConditionWeaponIsFiring";
			val = new ArrayList();
			val.Add("state_");
			val.Add("internalColor_");
			val.Add("gun");
			strings_.Add(key, val);

			#endregion

			#region actions

			// Action
			key = "Action";
			val = new ArrayList();
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ActionActivateAllSpots
			key = "ActionActivateAllSpots";
			val = new ArrayList();
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ActionActivateObjectByLabel
			key = "ActionActivateObjectByLabel";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("label");
			strings_.Add(key, val);
			// ActionAttackBySpecialWeapon
			key = "ActionAttackBySpecialWeapon";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("onlyIfAi");
			val.Add("weapon");
			val.Add("unitsToAttack");
			val.Add("unitClassToAttack");
			strings_.Add(key, val);
			// ActionDeactivateAllSpots
			key = "ActionDeactivateAllSpots";
			val = new ArrayList();
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ActionDeactivateObjectByLabel
			key = "ActionDeactivateObjectByLabel";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("label");
			strings_.Add(key, val);
			// ActionDefeat
			key = "ActionDefeat";
			val = new ArrayList();
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ActionDelay
			key = "ActionDelay";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("delay");
			val.Add("showTimer");
			val.Add("scaleByDifficulty");
			val.Add("timer");
			strings_.Add(key, val);
			// ActionHoldBuilding
			key = "ActionHoldBuilding";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("onlyIfAi");
			val.Add("building");
			strings_.Add(key, val);
			// ActionInstallFrame
			key = "ActionInstallFrame";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("onlyIfAi");
			strings_.Add(key, val);
			// ActionKillObject
			key = "ActionKillObject";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("object");
			strings_.Add(key, val);
			// ActionMessage
			key = "ActionMessage";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("messageID");
			val.Add("message");
			val.Add("delay");
			val.Add("duration");
			val.Add("syncroBySound");
			val.Add("delayTimer");
			val.Add("durationTimer");
			strings_.Add(key, val);
			// ActionOrderBuilding
			key = "ActionOrderBuilding";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("onlyIfAi");
			val.Add("building");
			val.Add("placementStrategy");
			val.Add("energyReserve");
			val.Add("buildDurationMax");
			val.Add("priority");
			strings_.Add(key, val);
			// ActionOscillateCamera
			key = "ActionOscillateCamera";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("duration");
			val.Add("factor");
			strings_.Add(key, val);
			// ActionRepareObjectByLabel
			key = "ActionRepareObjectByLabel";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("label");
			strings_.Add(key, val);
			// ActionSellBuilding
			key = "ActionSellBuilding";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("onlyIfAi");
			val.Add("building");
			val.Add("sellFactor");
			val.Add("damagePercent");
			strings_.Add(key, val);
			// ActionSetCamera
			key = "ActionSetCamera";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("cameraSplineName");
			val.Add("stepTime");
			val.Add("cycles");
			val.Add("smoothTransition");
			strings_.Add(key, val);
			// ActionSetCameraAtObject
			key = "ActionSetCameraAtObject";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("object");
			val.Add("playerType");
			val.Add("transitionTime");
			val.Add("setFollow");
			val.Add("turnTime");
			strings_.Add(key, val);
			// ActionSetControls
			key = "ActionSetControls";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("controls");
			strings_.Add(key, val);
			// ActionSetInterface
			key = "ActionSetInterface";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("enableInterface");
			strings_.Add(key, val);
			// ActionSquadAttack
			key = "ActionSquadAttack";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("onlyIfAi");
			val.Add("chooseSquadID");
			val.Add("attackByType");
			val.Add("unitsToAttack");
			val.Add("unitClassToAttack");
			val.Add("offensive");
			val.Add("unitsNumber");
			val.Add("soldiers");
			val.Add("officers");
			val.Add("technics");
			val.Add("attackTime");
			val.Add("remutateCounter");
			val.Add("holdProduction");
			val.Add("squadFollowDistance");
			val.Add("squadToFollowBy");
			val.Add("ignoreLastTarget");
			val.Add("returnToBase");
			val.Add("interruptable");
			val.Add("attackTimer");
			strings_.Add(key, val);
			// ActionSquadOrderUnits
			key = "ActionSquadOrderUnits";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("onlyIfAi");
			val.Add("chooseSquadID");
			val.Add("soldiers");
			val.Add("officers");
			val.Add("technics");
			val.Add("energyReserve");
			strings_.Add(key, val);
			// ActionSwitchFieldOn
			key = "ActionSwitchFieldOn";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("onlyIfAi");
			val.Add("duration");
			val.Add("energyReserve");
			val.Add("allCores");
			val.Add("onlyIfCoreDamaged");
			val.Add("timer");
			strings_.Add(key, val);
			// ActionSwitchGuns
			key = "ActionSwitchGuns";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("onlyIfAi");
			val.Add("mode");
			val.Add("gunID");
			strings_.Add(key, val);
			// ActionTask
			key = "ActionTask";
			val = new ArrayList();
			val.Add("internalColor_");
			val.Add("type");
			val.Add("taskID");
			val.Add("duration");
			val.Add("syncroBySound");
			val.Add("showTips");
			val.Add("durationTimer");
			strings_.Add(key, val);
			// ActionTeleportationOut
			key = "ActionTeleportationOut";
			val = new ArrayList();
			val.Add("internalColor_");
			strings_.Add(key, val);
			// ActionVictory
			key = "ActionVictory";
			val = new ArrayList();
			val.Add("internalColor_");
			strings_.Add(key, val);

			#endregion
		}

		// data
		private static Hashtable strings_;
		private string    name_;
		private string[]  fields_;
		private ArrayList conditions_ = new ArrayList();
		private bool      switch_;
	}
}
