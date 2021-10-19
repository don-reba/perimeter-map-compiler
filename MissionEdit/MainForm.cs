using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using System.Xml;

namespace MissionEdit
{
	public class MainForm : System.Windows.Forms.Form
	{

		//----------
		// constants
		//----------

		const string default_trigger_name = "default";
		private System.Windows.Forms.ToolBar toolbar_;
		private System.Windows.Forms.ImageList toolbar_images_;
		private System.Windows.Forms.ToolBarButton open_btn_;
		private System.Windows.Forms.ToolBarButton save_btn_;
		private System.Windows.Forms.ToolBarButton about_btn_;
		private System.ComponentModel.IContainer components;
		private System.Windows.Forms.Button control_new_btn_;
		private System.Windows.Forms.Button remove_all_btn_;
		const string control_id_prefix    = "SQSH_";

		//-------------
		// nested types
		//-------------

		#region

		private struct PlayerData
		{
			public enum Belligerent
			{
				BELLIGERENT_EMPIRE_VICE,
				BELLIGERENT_EMPIRE0,
				BELLIGERENT_EMPIRE1,
				BELLIGERENT_EXODUS0,
				BELLIGERENT_EXODUS1,
				BELLIGERENT_HARKBACKHOOD0,
				BELLIGERENT_HARKBACKHOOD1,
				BELLIGERENT_NONE
			}
			public Belligerent belligerent_;
			public bool        enabled_;
			public bool        triggers_;
			public int         clan_;
			public int         color_;
			public int         difficulty_;
			public int         handicap_;
			public int         type_;
			public string      name_;
		}

		private class ControlData
		{
			public ControlData()
			{
				id_         = "BACKGRND_ID";
				enabled_    = true;
				visible_    = true;
				flashing_   = false;
				tab_number_ = 0;
			}

			public ControlData(
				string id,
				bool   enabled,
				bool   visible,
				bool   flashing,
				int    tab_number)
			{
				id_         = id;
				enabled_    = enabled;
				visible_    = visible;
				flashing_   = flashing;
				tab_number_ = tab_number;
			}

			[TypeConverter(typeof(ControlIdConverter))]
			public string ID
			{
				get { return id_; }
				set { id_ = value; }
			}

			public bool enabled
			{
				get { return enabled_; }
				set { enabled_ = value; }
			}

			public bool visible
			{
				get { return visible_; }
				set { visible_ = value; }
			}

			public bool flashing
			{
				get { return flashing_; }
				set { flashing_ = value; }
			}

			public int tab_number
			{
				get { return tab_number_; }
				set { tab_number_ = value; }
			}

			public override string ToString()
			{
				return id_;
			}


			private string id_;
			private bool   enabled_;
			private bool   visible_;
			private bool   flashing_;
			private int    tab_number_;
		}

		#endregion

		//------------
		// entry point
		//------------

		[STAThread]
		static void Main(string[] args) 
		{
			Application.Run(new MainForm(
				(args.Length > 0)
				? args[0]
				: ""));
		}

		//----------
		// interface
		//----------

		#region

		public MainForm(string file_name)
		{
			current_player_ = -1;
			InitializeComponent();
//			InitializeControlList();
			InitializePlayers();
			player_cb_.SelectedIndex = 0;

			if (File.Exists(file_name))
				LoadFile(file_name);
		}

		#endregion

		//---------------
		// event handlers
		//---------------

		#region

		private void world_btn__Click(object sender, System.EventArgs e)
		{
			multipanel.CurrentPage = world_pg_;
			CheckButton(sender);
		}

		private void player_btn__Click(object sender, System.EventArgs e)
		{
			multipanel.CurrentPage = player_pg_;
			CheckButton(sender);
		}

		private void interface_btn__Click(object sender, System.EventArgs e)
		{
			multipanel.CurrentPage = interface_pg_;
			CheckButton(sender);
		}

		private void player_cb__SelectedIndexChanged(object sender, System.EventArgs e)
		{
			if (null != player_data_)
				SetPlayer(player_cb_.SelectedIndex);
		}

		private void player_triggers_chk__CheckedChanged(object sender, System.EventArgs e)
		{
			edit_trigger_btn_.Enabled = player_triggers_chk_.Checked;
		}

		private void controls_lst__SelectedIndexChanged(object sender, System.EventArgs e)
		{
			ControlData data = (ControlData)controls_lst_.SelectedItem;
			if (null != data)
				control_properties_.SelectedObject = data;
			control_remove_btn_.Enabled = (null != data);
		}

		private void control_new_btn__Click(object sender, System.EventArgs e)
		{
			ControlData new_data = new ControlData();
			int index = controls_lst_.Items.Add(new_data);
			controls_lst_.SelectedIndex = index;
			control_properties_.SelectedObject = new_data;
		}

		private void control_properties__PropertyValueChanged(
			object s,
			PropertyValueChangedEventArgs e)
		{
			if (e.ChangedItem.Label == "ID")
			{
				object data  = control_properties_.SelectedObject;
				controls_lst_.Items.Remove(data);
				int index = controls_lst_.Items.Add(data);
				controls_lst_.SelectedIndex = index;
			}

		}

		private void control_remove_btn__Click(object sender, System.EventArgs e)
		{
			object data = control_properties_.SelectedObject;
			control_properties_.SelectedObject = null;
			controls_lst_.Items.Remove(data);
		}

		private void remove_all_btn__Click(object sender, System.EventArgs e)
		{
			control_properties_.SelectedObject = null;
			controls_lst_.Items.Clear();
		}

		private void toolbar__ButtonClick(
			object sender,
			System.Windows.Forms.ToolBarButtonClickEventArgs e)
		{
			if (e.Button == open_btn_)
			{
				OpenFileDialog dlg = new OpenFileDialog();
				dlg.Multiselect = false;
				dlg.ReadOnlyChecked = false;
				dlg.Filter = "Mission File (*.xml)|*.xml";
				DialogResult result = dlg.ShowDialog(this);
				if (DialogResult.OK == result)
					LoadFile(dlg.FileName);
			}
			else if (e.Button == save_btn_)
			{
				SaveFile(file_name_);
			}
		}

		#endregion

		//---------------
		// implementation
		//---------------

		#region

		private void LoadFile(string file_name)
		{
			file_name_ = file_name;
			XmlNode query;
			// load the document itself
			XmlDocument doc = new XmlDocument();
			doc.PreserveWhitespace = false;
			doc.Load(file_name);
			// set world name
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"MissionDescriptionPrm\"]"
				+ "/string[@name=\"worldName\"]"
				);
			world_name_edt_.Text = query.InnerText;
			// set world difficulty
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"MissionDescriptionPrm\"]"
				+ "/value[@name=\"difficulty\"]"
				);
			switch (query.InnerText.Trim())
			{
				case "DIFFICULTY_EASY":   world_dificulty_cb_.SelectedIndex = 0; break;
				case "DIFFICULTY_NORMAL": world_dificulty_cb_.SelectedIndex = 1; break;
				case "DIFFICULTY_HARD":   world_dificulty_cb_.SelectedIndex = 2; break;
			}
			// set world description
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"MissionDescriptionPrm\"]"
				+ "/string[@name=\"missionDescription\"]"
				);
			world_description_edt_.Text = query.InnerText;
			// set soundtrack "battle"
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/array[@name=\"soundTracks\"]"
				+ "/set[string[@name=\"trackName\"]=\"battle\"]"
				+ "/string[@name=\"fileName\"]"
				);
			if (null != query)
				battle_soundtrack_edt_.Text = query.InnerText;
			// set soundtrack "construction"
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/array[@name=\"soundTracks\"]"
				+ "/set[string[@name=\"trackName\"]=\"construction\"]"
				+ "/string[@name=\"fileName\"]"
				);
			if (null != query)
				construction_soundtrack_edt_.Text = query.InnerText;
			// set soundtrack "regular"
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/array[@name=\"soundTracks\"]"
				+ "/set[string[@name=\"trackName\"]=\"regular\"]"
				+ "/string[@name=\"fileName\"]"
				);
			if (null != query)
				regular_soundtrack_edt_.Text = query.InnerText;
			// set alpha activation distance
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/*[@name=\"alphaActivationDistance\"]"
				);
			alpha_distance_ud_.Value = Decimal.Parse(query.InnerText.Trim());
			// set omega activation distance
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/*[@name=\"omegaActivationDistance\"]"
				);
			omega_distance_ud_.Value = Decimal.Parse(query.InnerText.Trim());
			// set spiral energy
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/int[@name=\"spiralChargingTime\"]"
				);
			spiral_energy_ud_.Value = Decimal.Parse(query.InnerText.Trim());
			// set spiral time
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/int[@name=\"spiralChargingTime\"]"
				);
			spiral_time_ud_.Value = Decimal.Parse(query.InnerText.Trim());
			// set spiral priority
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/int[@name=\"spiralChargingPriority\"]"
				);
			spiral_priority_ud_.Value = Decimal.Parse(query.InnerText.Trim());
			// set player data
			XmlNodeList player_nodes = doc.SelectNodes(
				"/script"
				+ "/set[@name=\"MissionDescriptionPrm\"]"
				+ "/array[@name=\"playersData\"]"
				+ "/set"
				);
			Trace.Assert(4 == player_nodes.Count, "Expected to find 4 player nodes.");
			Debug.Assert(4 == player_data_.Length);
			for (int i = 0; i != 4; ++i)
			{
				// set belligerent
				query = player_nodes[i].SelectSingleNode("value[@name=\"belligerent\"]");
				player_data_[i].belligerent_ = (PlayerData.Belligerent)Enum.Parse(
					typeof(PlayerData.Belligerent),
					query.InnerText, false);
				// set name
				query = player_nodes[i].SelectSingleNode("*[@name=\"playerName\"]");
				player_data_[i].name_ = query.InnerText;
				// set type
				query = player_nodes[i].SelectSingleNode("*[@name=\"realPlayerType\"]");
				switch (query.InnerText.Trim())
				{
					case "REAL_PLAYER_TYPE_PLAYER": player_data_[i].type_ = 0; break;
					case "REAL_PLAYER_TYPE_AI":     player_data_[i].type_ = 1; break;
					default:
						player_data_[i].type_    = 0;
						player_data_[i].enabled_ = false;
						break;
				}
				// set colour
				query = player_nodes[i].SelectSingleNode("*[@name=\"colorIndex\"]");
				player_data_[i].color_ = int.Parse(query.InnerText.Trim());
				// set clan
				query = player_nodes[i].SelectSingleNode("*[@name=\"clan\"]");
				player_data_[i].clan_ = int.Parse(query.InnerText.Trim()) + 1;
				// set difficulty
				query = player_nodes[i].SelectSingleNode("*[@name=\"difficulty\"]");
				switch (query.InnerText)
				{
					case "DIFFICULTY_EASY":   player_data_[i].difficulty_ = 0; break;
					case "DIFFICULTY_NORMAL": player_data_[i].difficulty_ = 1; break;
					case "DIFFICULTY_HARD":   player_data_[i].difficulty_ = 2; break;
				}
				// set handicap
				query = player_nodes[i].SelectSingleNode("*[@name=\"handicap\"]");
				player_data_[i].handicap_ = int.Parse(query.InnerText.Trim());
			}
			// set trigger settings
			player_nodes = doc.SelectNodes(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/array[@name=\"players\"]"
				+ "/set"
				);
			for (int i = 0; i != player_nodes.Count; ++i)
			{
				query = player_nodes[i].SelectSingleNode(
					"array[@name=\"TriggerChainNames\"]"
					);
				player_data_[i].triggers_ = (
					null != query
					&& query.InnerText != default_trigger_name);
			}
			player_nodes = null;
			// set control settings
			XmlNodeList control_nodes = doc.SelectNodes(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/array[@name=\"controls\"]"
				+ "/set");
			try
			{
				controls_lst_.SuspendLayout();
				controls_lst_.Items.Clear();
				controls_lst_.SelectedItem = null;
				control_properties_.SelectedObject = null;
				foreach (XmlNode node in control_nodes)
				{
					ControlData data = new ControlData(
						node.SelectSingleNode(
							"*[@name=\"controlID\"]").InnerText.Trim().Substring(control_id_prefix.Length),
						node.SelectSingleNode(
							"*[@name=\"enabled\"]").InnerText.Trim() == "true",
						node.SelectSingleNode(
							"*[@name=\"visible\"]").InnerText.Trim() == "true",
						node.SelectSingleNode(
							"*[@name=\"flashing\"]").InnerText.Trim() == "true",
						int.Parse(node.SelectSingleNode(
							"*[@name=\"tabNumber\"]").InnerText.Trim())
						);
					controls_lst_.Items.Add(data);
				}
			}
			finally
			{
				controls_lst_.ResumeLayout();
			}
		}

		private void SaveFile(string file_name)
		{
			file_name_ = file_name;
			XmlNode query;
			// load the document itself
			XmlDocument doc = new XmlDocument();
			doc.PreserveWhitespace = false;
			doc.Load(file_name);
			// set world name
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"MissionDescriptionPrm\"]"
				+ "/string[@name=\"worldName\"]"
				);
			query.InnerText = world_name_edt_.Text;
			// set world difficulty
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"MissionDescriptionPrm\"]"
				+ "/value[@name=\"difficulty\"]"
				);
			switch (world_dificulty_cb_.SelectedIndex)
			{
				case 0: query.InnerText = "DIFFICULTY_EASY";   break;
				case 1: query.InnerText = "DIFFICULTY_NORMAL"; break;
				case 2: query.InnerText = "DIFFICULTY_HARD";   break;
			}
			// set world description
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"MissionDescriptionPrm\"]"
				+ "/string[@name=\"missionDescription\"]"
				);
			query.InnerText = world_description_edt_.Text;
			// set soundtracks
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/array[@name=\"soundTracks\"]"
				);
			query.RemoveAll();
			((XmlElement)query).SetAttribute("name", "soundTracks");
			if (construction_soundtrack_edt_.Text.Length > 0)
				query.AppendChild(CreateSoundtrackNode(
					"construction",
					construction_soundtrack_edt_.Text,
					doc));
			if (battle_soundtrack_edt_.Text.Length > 0)
				query.AppendChild(CreateSoundtrackNode(
					"battle",
					battle_soundtrack_edt_.Text,
					doc));
			if (regular_soundtrack_edt_.Text.Length > 0)
				query.AppendChild(CreateSoundtrackNode(
					"regular",
					regular_soundtrack_edt_.Text,
					doc));
			// set alpha activation distance
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/*[@name=\"alphaActivationDistance\"]"
				);
			query.InnerText = alpha_distance_ud_.Value.ToString();
			// set omega activation distance
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/*[@name=\"omegaActivationDistance\"]"
				);
			query.InnerText = omega_distance_ud_.Value.ToString();
			// set spiral energy
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/int[@name=\"spiralChargingTime\"]"
				);
			query.InnerText = spiral_energy_ud_.Value.ToString();
			// set spiral time
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/int[@name=\"spiralChargingTime\"]"
				);
			query.InnerText = spiral_time_ud_.Value.ToString();
			// set spiral priority
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/int[@name=\"spiralChargingPriority\"]"
				);
			query.InnerText = spiral_priority_ud_.Value.ToString();
			// set player data
			XmlNodeList player_nodes = doc.SelectNodes(
				"/script"
				+ "/set[@name=\"MissionDescriptionPrm\"]"
				+ "/array[@name=\"playersData\"]"
				+ "/set"
				);
			Trace.Assert(4 == player_nodes.Count, "Expected to find 4 player nodes.");
			Debug.Assert(4 == player_data_.Length);
			for (int i = 0; i != player_nodes.Count; ++i)
			{
				// set belligerent
				query = player_nodes[i].SelectSingleNode("value[@name=\"belligerent\"]");
				query.InnerText = player_data_[i].belligerent_.ToString();
				// set name
				query = player_nodes[i].SelectSingleNode("*[@name=\"playerName\"]");
				query.InnerText = player_data_[i].name_;
				// set type
				query = player_nodes[i].SelectSingleNode("*[@name=\"realPlayerType\"]");
				if (player_enabled_chk_.Checked)
					switch (player_data_[i].type_)
					{
						case 0: query.InnerText = "REAL_PLAYER_TYPE_PLAYER"; break;
						case 1: query.InnerText = "REAL_PLAYER_TYPE_AI";     break;
					}
				else
					query.InnerText = "REAL_PLAYER_TYPE_CLOSE";
				// set colour
				query = player_nodes[i].SelectSingleNode("*[@name=\"colorIndex\"]");
				query.InnerText = player_data_[i].color_.ToString();
				// set clan
				query = player_nodes[i].SelectSingleNode("*[@name=\"clan\"]");
				query.InnerText = player_data_[i].clan_.ToString();
				// set difficulty
				query = player_nodes[i].SelectSingleNode("*[@name=\"difficulty\"]");
				switch (player_data_[i].difficulty_)
				{
					case 0: query.InnerText = "DIFFICULTY_EASY"; break;
					case 1: query.InnerText = "DIFFICULTY_NORMAL"; break;
					case 2: query.InnerText = "DIFFICULTY_HARD"; break;
				}
				// set handicap
				query = player_nodes[i].SelectSingleNode("*[@name=\"handicap\"]");
				query.InnerText = player_data_[i].handicap_.ToString();
			}
			// set trigger settings
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/array[@name=\"players\"]"
				);
			query.RemoveAll();
			((XmlElement)query).SetAttribute("name", "players");
			for (int i = 0; i != player_data_.Length; ++i)
				query.AppendChild(CreateTriggerNode(
					player_data_[i].triggers_
					? default_trigger_name
					: i.ToString(),
					doc
					));
			for (int i = 0; i != player_nodes.Count; ++i)
			{
				query = player_nodes[i].SelectSingleNode(
					"array[@name=\"TriggerChainNames\"]"
					);
				player_data_[i].triggers_ = (null != query);
			}
			player_nodes = null;
			// set control settings
			query = doc.SelectSingleNode(
				"/script"
				+ "/set[@name=\"SavePrm\"]"
				+ "/set[@name=\"manualData\"]"
				+ "/array[@name=\"controls\"]"
				);
			query.RemoveAll();
			((XmlElement)query).SetAttribute("name", "controls");
			foreach (ControlData data in controls_lst_.Items)
				query.AppendChild(CreateControlNode(data, doc));
			// save
			doc.Save(file_name);
		}

		private XmlElement CreateControlNode(ControlData data, XmlDocument doc)
		{
			XmlElement root = doc.CreateElement("set");
			root.AppendChild(SetElementContent(
				CreateNamedElement(
					"value",
					"controlID",
					doc),
				control_id_prefix + data.ID));
			root.AppendChild(SetElementContent(
				CreateNamedElement(
					"value",
					"enabled",
					doc),
				data.enabled ? "true" : "false"));
			root.AppendChild(SetElementContent(
				CreateNamedElement(
				"value",
				"visible",
				doc),
				data.visible ? "true" : "false"));
			root.AppendChild(SetElementContent(
				CreateNamedElement(
					"value",
					"flashing",
					doc),
				data.flashing ? "true" : "false"));
			root.AppendChild(SetElementContent(
				CreateNamedElement(
					"int",
					"tabNumber",
					doc),
				data.tab_number.ToString()));
			return root;
		}

		private XmlElement CreateSoundtrackNode(string track_name, string file_name, XmlDocument doc) 
		{
			XmlElement root = doc.CreateElement("set");
			root.AppendChild(SetElementContent(
				CreateNamedElement(
					"string",
					"trackName",
					doc),
				track_name));
			root.AppendChild(SetElementContent(
				CreateNamedElement(
					"string",
					"fileName",
					doc),
				file_name));
			return root;
		}

		private XmlElement CreateTriggerNode(string file_name, XmlDocument doc)
		{
			XmlElement name_node = CreateNamedElement("string", "name", doc);
			name_node.InnerText = file_name;
			XmlElement names_container_node = doc.CreateElement("set");
			names_container_node.AppendChild(name_node);
			XmlElement names_node = CreateNamedElement("array", "TriggerChainNames", doc);
			names_node.AppendChild(names_container_node);
			names_node.AppendChild(CreateNamedElement("array", "triggerChainNames", doc));
			XmlElement root = doc.CreateElement("set");
			root.AppendChild(names_node);
			return root;

		}

		private XmlElement CreateNamedElement(string type, string name, XmlDocument doc)
		{
			XmlElement element = doc.CreateElement(type);
			element.SetAttribute("name", name);
			return element;
		}

		private XmlElement SetElementContent(XmlElement element, string content)
		{
			element.InnerText = content;
			return element;
		}

		private void InitializePlayers()
		{
			player_data_ = new PlayerData[player_cb_.Items.Count];

			for (int i = 0; i != player_data_.Length; ++i)
			{
				player_data_[i].belligerent_ = PlayerData.Belligerent.BELLIGERENT_HARKBACKHOOD0;
				player_data_[i].clan_        = 0;
				player_data_[i].color_       = 0;
				player_data_[i].difficulty_  = 0;
				player_data_[i].enabled_     = true;
				player_data_[i].handicap_    = 100;
				player_data_[i].name_        = "Nomad";
				player_data_[i].triggers_    = false;
				player_data_[i].type_        = 0;
			}
		}

		private void SetPlayer(int i)
		{
			PlayerData data = new PlayerData();
			// save current settings
			if (current_player_ >= 0)
			{
				if (empire_rb_.Checked)
					data.belligerent_ = PlayerData.Belligerent.BELLIGERENT_EMPIRE0;
				else if (exodus_rb_.Checked)
					data.belligerent_ = PlayerData.Belligerent.BELLIGERENT_EXODUS0;
				else
					data.belligerent_ = PlayerData.Belligerent.BELLIGERENT_HARKBACKHOOD0;

				data.clan_       = player_clan_cb_.SelectedIndex;
				data.color_      = player_color_cb_.SelectedIndex;
				data.difficulty_ = player_difficulty_cb_.SelectedIndex;
				data.enabled_    = player_enabled_chk_.Checked;
				data.handicap_   = Convert.ToInt32(player_handicap_ud_.Value);
				data.name_       = player_name_edt_.Text;
				data.triggers_   = player_triggers_chk_.Checked;
				data.type_       = player_type_cb_.SelectedIndex;

				player_data_[current_player_] = data;
			}
			// set new settings
			if (i >= 0)
			{
				data = player_data_[i];

				switch (data.belligerent_)
				{
					case PlayerData.Belligerent.BELLIGERENT_EMPIRE0:
						empire_rb_.Checked = true;
						break;
					case PlayerData.Belligerent.BELLIGERENT_EMPIRE1:
						empire_rb_.Checked = true;
						break;
					case PlayerData.Belligerent.BELLIGERENT_EXODUS0:
						exodus_rb_.Checked = true;
						break;
					case PlayerData.Belligerent.BELLIGERENT_EXODUS1:
						exodus_rb_.Checked = true;
						break;
					case PlayerData.Belligerent.BELLIGERENT_HARKBACKHOOD0:
						harckback_rb_.Checked = true;
						break;
					case PlayerData.Belligerent.BELLIGERENT_HARKBACKHOOD1:
						harckback_rb_.Checked = true;
						break;
				}
				player_clan_cb_.SelectedIndex       = data.clan_;
				player_color_cb_.SelectedIndex      = data.color_;
				player_difficulty_cb_.SelectedIndex = data.difficulty_;
				player_enabled_chk_.Checked         = data.enabled_;
				player_handicap_ud_.Value           = Convert.ToDecimal(data.handicap_);
				player_name_edt_.Text               = data.name_;
				player_triggers_chk_.Checked        = data.triggers_;
				player_type_cb_.SelectedIndex       = data.type_;
			}

			current_player_ = i;
		}

		private void CheckButton(object button) 
		{
			FlatButton[] buttons =
				{
					world_btn_,
					player_btn_,
					interface_btn_
				};
			foreach (FlatButton b in buttons)
				b.Checked = (b == button);
		}

		#endregion

		//-----
		// data
		//-----

		#region

		private int          current_player_; // for change through SetPlayer only
		private string       file_name_;
		private PlayerData[] player_data_;

		#endregion

		//----------
		// generated
		//----------

		#region code

		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(MainForm));
			this.button_pnl_ = new System.Windows.Forms.Panel();
			this.interface_btn_ = new MissionEdit.FlatButton();
			this.player_btn_ = new MissionEdit.FlatButton();
			this.world_btn_ = new MissionEdit.FlatButton();
			this.multipanel = new MultipanelLib.Multipanel();
			this.world_pg_ = new MultipanelLib.Page();
			this.groupBox3 = new System.Windows.Forms.GroupBox();
			this.omega_distance_ud_ = new System.Windows.Forms.NumericUpDown();
			this.label11 = new System.Windows.Forms.Label();
			this.alpha_distance_ud_ = new System.Windows.Forms.NumericUpDown();
			this.label12 = new System.Windows.Forms.Label();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.label10 = new System.Windows.Forms.Label();
			this.label9 = new System.Windows.Forms.Label();
			this.label8 = new System.Windows.Forms.Label();
			this.spiral_priority_ud_ = new System.Windows.Forms.NumericUpDown();
			this.spiral_time_ud_ = new System.Windows.Forms.NumericUpDown();
			this.spiral_energy_ud_ = new System.Windows.Forms.NumericUpDown();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.regular_soundtrack_edt_ = new System.Windows.Forms.TextBox();
			this.label6 = new System.Windows.Forms.Label();
			this.construction_soundtrack_edt_ = new System.Windows.Forms.TextBox();
			this.label5 = new System.Windows.Forms.Label();
			this.battle_soundtrack_edt_ = new System.Windows.Forms.TextBox();
			this.label4 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.world_dificulty_cb_ = new System.Windows.Forms.ComboBox();
			this.world_description_edt_ = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.world_name_edt_ = new System.Windows.Forms.TextBox();
			this.label1 = new System.Windows.Forms.Label();
			this.player_pg_ = new MultipanelLib.Page();
			this.edit_trigger_btn_ = new System.Windows.Forms.Button();
			this.player_enabled_chk_ = new System.Windows.Forms.CheckBox();
			this.player_cb_ = new System.Windows.Forms.ComboBox();
			this.empire_rb_ = new System.Windows.Forms.RadioButton();
			this.exodus_rb_ = new System.Windows.Forms.RadioButton();
			this.harckback_rb_ = new System.Windows.Forms.RadioButton();
			this.label19 = new System.Windows.Forms.Label();
			this.player_triggers_chk_ = new System.Windows.Forms.CheckBox();
			this.player_name_edt_ = new System.Windows.Forms.TextBox();
			this.label18 = new System.Windows.Forms.Label();
			this.player_handicap_ud_ = new System.Windows.Forms.NumericUpDown();
			this.label17 = new System.Windows.Forms.Label();
			this.player_difficulty_cb_ = new System.Windows.Forms.ComboBox();
			this.label16 = new System.Windows.Forms.Label();
			this.player_clan_cb_ = new System.Windows.Forms.ComboBox();
			this.label15 = new System.Windows.Forms.Label();
			this.label14 = new System.Windows.Forms.Label();
			this.player_color_cb_ = new System.Windows.Forms.ComboBox();
			this.player_type_cb_ = new System.Windows.Forms.ComboBox();
			this.label7 = new System.Windows.Forms.Label();
			this.interface_pg_ = new MultipanelLib.Page();
			this.control_remove_btn_ = new System.Windows.Forms.Button();
			this.control_new_btn_ = new System.Windows.Forms.Button();
			this.control_properties_ = new System.Windows.Forms.PropertyGrid();
			this.controls_lst_ = new System.Windows.Forms.ListBox();
			this.toolbar_ = new System.Windows.Forms.ToolBar();
			this.open_btn_ = new System.Windows.Forms.ToolBarButton();
			this.save_btn_ = new System.Windows.Forms.ToolBarButton();
			this.about_btn_ = new System.Windows.Forms.ToolBarButton();
			this.toolbar_images_ = new System.Windows.Forms.ImageList(this.components);
			this.remove_all_btn_ = new System.Windows.Forms.Button();
			this.button_pnl_.SuspendLayout();
			this.multipanel.SuspendLayout();
			this.world_pg_.SuspendLayout();
			this.groupBox3.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.omega_distance_ud_)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.alpha_distance_ud_)).BeginInit();
			this.groupBox2.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.spiral_priority_ud_)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.spiral_time_ud_)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.spiral_energy_ud_)).BeginInit();
			this.groupBox1.SuspendLayout();
			this.player_pg_.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.player_handicap_ud_)).BeginInit();
			this.interface_pg_.SuspendLayout();
			this.SuspendLayout();
			// 
			// button_pnl_
			// 
			this.button_pnl_.BackColor = System.Drawing.SystemColors.Window;
			this.button_pnl_.Controls.Add(this.interface_btn_);
			this.button_pnl_.Controls.Add(this.player_btn_);
			this.button_pnl_.Controls.Add(this.world_btn_);
			this.button_pnl_.Dock = System.Windows.Forms.DockStyle.Left;
			this.button_pnl_.Location = new System.Drawing.Point(0, 34);
			this.button_pnl_.Name = "button_pnl_";
			this.button_pnl_.Size = new System.Drawing.Size(80, 245);
			this.button_pnl_.TabIndex = 1;
			// 
			// interface_btn_
			// 
			this.interface_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.interface_btn_.Checked = false;
			this.interface_btn_.Location = new System.Drawing.Point(0, 112);
			this.interface_btn_.Name = "interface_btn_";
			this.interface_btn_.Size = new System.Drawing.Size(80, 56);
			this.interface_btn_.TabIndex = 2;
			this.interface_btn_.Text = "Interface";
			this.interface_btn_.Click += new System.EventHandler(this.interface_btn__Click);
			// 
			// player_btn_
			// 
			this.player_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.player_btn_.Checked = false;
			this.player_btn_.Location = new System.Drawing.Point(0, 56);
			this.player_btn_.Name = "player_btn_";
			this.player_btn_.Size = new System.Drawing.Size(80, 56);
			this.player_btn_.TabIndex = 1;
			this.player_btn_.Text = "Player";
			this.player_btn_.Click += new System.EventHandler(this.player_btn__Click);
			// 
			// world_btn_
			// 
			this.world_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.world_btn_.Checked = true;
			this.world_btn_.Location = new System.Drawing.Point(0, 0);
			this.world_btn_.Name = "world_btn_";
			this.world_btn_.Size = new System.Drawing.Size(80, 56);
			this.world_btn_.TabIndex = 0;
			this.world_btn_.Text = "World";
			this.world_btn_.Click += new System.EventHandler(this.world_btn__Click);
			// 
			// multipanel
			// 
			this.multipanel.Controls.Add(this.world_pg_);
			this.multipanel.Controls.Add(this.player_pg_);
			this.multipanel.Controls.Add(this.interface_pg_);
			this.multipanel.Dock = System.Windows.Forms.DockStyle.Fill;
			this.multipanel.Location = new System.Drawing.Point(80, 34);
			this.multipanel.Name = "multipanel";
			this.multipanel.Pages.Add(this.world_pg_);
			this.multipanel.Pages.Add(this.player_pg_);
			this.multipanel.Pages.Add(this.interface_pg_);
			this.multipanel.Size = new System.Drawing.Size(392, 245);
			this.multipanel.TabIndex = 0;
			// 
			// world_pg_
			// 
			this.world_pg_.Controls.Add(this.groupBox3);
			this.world_pg_.Controls.Add(this.groupBox2);
			this.world_pg_.Controls.Add(this.groupBox1);
			this.world_pg_.Controls.Add(this.label3);
			this.world_pg_.Controls.Add(this.world_dificulty_cb_);
			this.world_pg_.Controls.Add(this.world_description_edt_);
			this.world_pg_.Controls.Add(this.label2);
			this.world_pg_.Controls.Add(this.world_name_edt_);
			this.world_pg_.Controls.Add(this.label1);
			this.world_pg_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.world_pg_.Location = new System.Drawing.Point(0, 0);
			this.world_pg_.Name = "world_pg_";
			this.world_pg_.Size = new System.Drawing.Size(392, 245);
			this.world_pg_.TabIndex = 0;
			this.world_pg_.Text = "World";
			// 
			// groupBox3
			// 
			this.groupBox3.Controls.Add(this.omega_distance_ud_);
			this.groupBox3.Controls.Add(this.label11);
			this.groupBox3.Controls.Add(this.alpha_distance_ud_);
			this.groupBox3.Controls.Add(this.label12);
			this.groupBox3.Location = new System.Drawing.Point(224, 24);
			this.groupBox3.Name = "groupBox3";
			this.groupBox3.Size = new System.Drawing.Size(152, 80);
			this.groupBox3.TabIndex = 14;
			this.groupBox3.TabStop = false;
			this.groupBox3.Text = "Portal Activation Distance";
			// 
			// omega_distance_ud_
			// 
			this.omega_distance_ud_.Increment = new System.Decimal(new int[] {
																									  100,
																									  0,
																									  0,
																									  0});
			this.omega_distance_ud_.Location = new System.Drawing.Point(64, 48);
			this.omega_distance_ud_.Maximum = new System.Decimal(new int[] {
																									6000,
																									0,
																									0,
																									0});
			this.omega_distance_ud_.Name = "omega_distance_ud_";
			this.omega_distance_ud_.Size = new System.Drawing.Size(72, 20);
			this.omega_distance_ud_.TabIndex = 11;
			this.omega_distance_ud_.Value = new System.Decimal(new int[] {
																								 1000,
																								 0,
																								 0,
																								 0});
			// 
			// label11
			// 
			this.label11.Location = new System.Drawing.Point(16, 24);
			this.label11.Name = "label11";
			this.label11.Size = new System.Drawing.Size(40, 16);
			this.label11.TabIndex = 12;
			this.label11.Text = "Alpha";
			// 
			// alpha_distance_ud_
			// 
			this.alpha_distance_ud_.Increment = new System.Decimal(new int[] {
																									  100,
																									  0,
																									  0,
																									  0});
			this.alpha_distance_ud_.Location = new System.Drawing.Point(64, 24);
			this.alpha_distance_ud_.Maximum = new System.Decimal(new int[] {
																									6000,
																									0,
																									0,
																									0});
			this.alpha_distance_ud_.Name = "alpha_distance_ud_";
			this.alpha_distance_ud_.Size = new System.Drawing.Size(72, 20);
			this.alpha_distance_ud_.TabIndex = 10;
			this.alpha_distance_ud_.Value = new System.Decimal(new int[] {
																								 1000,
																								 0,
																								 0,
																								 0});
			// 
			// label12
			// 
			this.label12.Location = new System.Drawing.Point(16, 48);
			this.label12.Name = "label12";
			this.label12.Size = new System.Drawing.Size(48, 16);
			this.label12.TabIndex = 13;
			this.label12.Text = "Omega";
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.label10);
			this.groupBox2.Controls.Add(this.label9);
			this.groupBox2.Controls.Add(this.label8);
			this.groupBox2.Controls.Add(this.spiral_priority_ud_);
			this.groupBox2.Controls.Add(this.spiral_time_ud_);
			this.groupBox2.Controls.Add(this.spiral_energy_ud_);
			this.groupBox2.Location = new System.Drawing.Point(224, 120);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(152, 104);
			this.groupBox2.TabIndex = 9;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Spiral";
			// 
			// label10
			// 
			this.label10.Location = new System.Drawing.Point(16, 72);
			this.label10.Name = "label10";
			this.label10.Size = new System.Drawing.Size(48, 16);
			this.label10.TabIndex = 5;
			this.label10.Text = "Priority";
			// 
			// label9
			// 
			this.label9.Location = new System.Drawing.Point(16, 48);
			this.label9.Name = "label9";
			this.label9.Size = new System.Drawing.Size(48, 16);
			this.label9.TabIndex = 4;
			this.label9.Text = "Time";
			// 
			// label8
			// 
			this.label8.Location = new System.Drawing.Point(16, 24);
			this.label8.Name = "label8";
			this.label8.Size = new System.Drawing.Size(48, 16);
			this.label8.TabIndex = 3;
			this.label8.Text = "Energy";
			// 
			// spiral_priority_ud_
			// 
			this.spiral_priority_ud_.Location = new System.Drawing.Point(64, 72);
			this.spiral_priority_ud_.Maximum = new System.Decimal(new int[] {
																									 -1,
																									 -1,
																									 -1,
																									 0});
			this.spiral_priority_ud_.Name = "spiral_priority_ud_";
			this.spiral_priority_ud_.Size = new System.Drawing.Size(72, 20);
			this.spiral_priority_ud_.TabIndex = 2;
			this.spiral_priority_ud_.Value = new System.Decimal(new int[] {
																								  170,
																								  0,
																								  0,
																								  0});
			// 
			// spiral_time_ud_
			// 
			this.spiral_time_ud_.Location = new System.Drawing.Point(64, 48);
			this.spiral_time_ud_.Maximum = new System.Decimal(new int[] {
																								-1,
																								-1,
																								-1,
																								0});
			this.spiral_time_ud_.Name = "spiral_time_ud_";
			this.spiral_time_ud_.Size = new System.Drawing.Size(72, 20);
			this.spiral_time_ud_.TabIndex = 1;
			this.spiral_time_ud_.Value = new System.Decimal(new int[] {
																							 100,
																							 0,
																							 0,
																							 0});
			// 
			// spiral_energy_ud_
			// 
			this.spiral_energy_ud_.DecimalPlaces = 1;
			this.spiral_energy_ud_.Location = new System.Drawing.Point(64, 24);
			this.spiral_energy_ud_.Maximum = new System.Decimal(new int[] {
																								  -1,
																								  -1,
																								  -1,
																								  0});
			this.spiral_energy_ud_.Name = "spiral_energy_ud_";
			this.spiral_energy_ud_.Size = new System.Drawing.Size(72, 20);
			this.spiral_energy_ud_.TabIndex = 0;
			this.spiral_energy_ud_.Value = new System.Decimal(new int[] {
																								2,
																								0,
																								0,
																								0});
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.regular_soundtrack_edt_);
			this.groupBox1.Controls.Add(this.label6);
			this.groupBox1.Controls.Add(this.construction_soundtrack_edt_);
			this.groupBox1.Controls.Add(this.label5);
			this.groupBox1.Controls.Add(this.battle_soundtrack_edt_);
			this.groupBox1.Controls.Add(this.label4);
			this.groupBox1.Location = new System.Drawing.Point(16, 120);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(192, 104);
			this.groupBox1.TabIndex = 6;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Soundtracks";
			// 
			// regular_soundtrack_edt_
			// 
			this.regular_soundtrack_edt_.Location = new System.Drawing.Point(88, 72);
			this.regular_soundtrack_edt_.Name = "regular_soundtrack_edt_";
			this.regular_soundtrack_edt_.Size = new System.Drawing.Size(88, 20);
			this.regular_soundtrack_edt_.TabIndex = 7;
			this.regular_soundtrack_edt_.Text = "";
			// 
			// label6
			// 
			this.label6.Location = new System.Drawing.Point(16, 72);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(72, 16);
			this.label6.TabIndex = 6;
			this.label6.Text = "regular";
			// 
			// construction_soundtrack_edt_
			// 
			this.construction_soundtrack_edt_.Location = new System.Drawing.Point(88, 48);
			this.construction_soundtrack_edt_.Name = "construction_soundtrack_edt_";
			this.construction_soundtrack_edt_.Size = new System.Drawing.Size(88, 20);
			this.construction_soundtrack_edt_.TabIndex = 4;
			this.construction_soundtrack_edt_.Text = "";
			// 
			// label5
			// 
			this.label5.Location = new System.Drawing.Point(16, 48);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(72, 16);
			this.label5.TabIndex = 3;
			this.label5.Text = "cosntruction";
			// 
			// battle_soundtrack_edt_
			// 
			this.battle_soundtrack_edt_.Location = new System.Drawing.Point(88, 24);
			this.battle_soundtrack_edt_.Name = "battle_soundtrack_edt_";
			this.battle_soundtrack_edt_.Size = new System.Drawing.Size(88, 20);
			this.battle_soundtrack_edt_.TabIndex = 1;
			this.battle_soundtrack_edt_.Text = "";
			// 
			// label4
			// 
			this.label4.Location = new System.Drawing.Point(16, 24);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(72, 16);
			this.label4.TabIndex = 0;
			this.label4.Text = "battle";
			// 
			// label3
			// 
			this.label3.Location = new System.Drawing.Point(16, 40);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(56, 16);
			this.label3.TabIndex = 5;
			this.label3.Text = "Difficulty";
			// 
			// world_dificulty_cb_
			// 
			this.world_dificulty_cb_.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.world_dificulty_cb_.Items.AddRange(new object[] {
																					  "easy",
																					  "normal",
																					  "hard"});
			this.world_dificulty_cb_.Location = new System.Drawing.Point(80, 40);
			this.world_dificulty_cb_.Name = "world_dificulty_cb_";
			this.world_dificulty_cb_.Size = new System.Drawing.Size(88, 21);
			this.world_dificulty_cb_.TabIndex = 4;
			// 
			// world_description_edt_
			// 
			this.world_description_edt_.Location = new System.Drawing.Point(80, 64);
			this.world_description_edt_.Multiline = true;
			this.world_description_edt_.Name = "world_description_edt_";
			this.world_description_edt_.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
			this.world_description_edt_.Size = new System.Drawing.Size(128, 40);
			this.world_description_edt_.TabIndex = 3;
			this.world_description_edt_.Text = "";
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(16, 64);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(64, 16);
			this.label2.TabIndex = 2;
			this.label2.Text = "Description";
			// 
			// world_name_edt_
			// 
			this.world_name_edt_.Location = new System.Drawing.Point(80, 16);
			this.world_name_edt_.Name = "world_name_edt_";
			this.world_name_edt_.Size = new System.Drawing.Size(88, 20);
			this.world_name_edt_.TabIndex = 1;
			this.world_name_edt_.Text = "";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(16, 16);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(56, 16);
			this.label1.TabIndex = 0;
			this.label1.Text = "Name";
			// 
			// player_pg_
			// 
			this.player_pg_.Controls.Add(this.edit_trigger_btn_);
			this.player_pg_.Controls.Add(this.player_enabled_chk_);
			this.player_pg_.Controls.Add(this.player_cb_);
			this.player_pg_.Controls.Add(this.empire_rb_);
			this.player_pg_.Controls.Add(this.exodus_rb_);
			this.player_pg_.Controls.Add(this.harckback_rb_);
			this.player_pg_.Controls.Add(this.label19);
			this.player_pg_.Controls.Add(this.player_triggers_chk_);
			this.player_pg_.Controls.Add(this.player_name_edt_);
			this.player_pg_.Controls.Add(this.label18);
			this.player_pg_.Controls.Add(this.player_handicap_ud_);
			this.player_pg_.Controls.Add(this.label17);
			this.player_pg_.Controls.Add(this.player_difficulty_cb_);
			this.player_pg_.Controls.Add(this.label16);
			this.player_pg_.Controls.Add(this.player_clan_cb_);
			this.player_pg_.Controls.Add(this.label15);
			this.player_pg_.Controls.Add(this.label14);
			this.player_pg_.Controls.Add(this.player_color_cb_);
			this.player_pg_.Controls.Add(this.player_type_cb_);
			this.player_pg_.Controls.Add(this.label7);
			this.player_pg_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.player_pg_.Location = new System.Drawing.Point(0, 0);
			this.player_pg_.Name = "player_pg_";
			this.player_pg_.Size = new System.Drawing.Size(392, 245);
			this.player_pg_.TabIndex = 0;
			this.player_pg_.Text = "Player";
			// 
			// edit_trigger_btn_
			// 
			this.edit_trigger_btn_.Location = new System.Drawing.Point(312, 200);
			this.edit_trigger_btn_.Name = "edit_trigger_btn_";
			this.edit_trigger_btn_.Size = new System.Drawing.Size(64, 20);
			this.edit_trigger_btn_.TabIndex = 22;
			this.edit_trigger_btn_.Text = "Edit";
			// 
			// player_enabled_chk_
			// 
			this.player_enabled_chk_.Location = new System.Drawing.Point(112, 16);
			this.player_enabled_chk_.Name = "player_enabled_chk_";
			this.player_enabled_chk_.Size = new System.Drawing.Size(64, 24);
			this.player_enabled_chk_.TabIndex = 21;
			this.player_enabled_chk_.Text = "enabled";
			// 
			// player_cb_
			// 
			this.player_cb_.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.player_cb_.Items.AddRange(new object[] {
																		  "Player 1",
																		  "Player 2",
																		  "Player 3",
																		  "Player 4"});
			this.player_cb_.Location = new System.Drawing.Point(16, 16);
			this.player_cb_.Name = "player_cb_";
			this.player_cb_.Size = new System.Drawing.Size(80, 21);
			this.player_cb_.TabIndex = 20;
			this.player_cb_.SelectedIndexChanged += new System.EventHandler(this.player_cb__SelectedIndexChanged);
			// 
			// empire_rb_
			// 
			this.empire_rb_.Image = ((System.Drawing.Image)(resources.GetObject("empire_rb_.Image")));
			this.empire_rb_.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
			this.empire_rb_.Location = new System.Drawing.Point(24, 56);
			this.empire_rb_.Name = "empire_rb_";
			this.empire_rb_.Size = new System.Drawing.Size(160, 56);
			this.empire_rb_.TabIndex = 19;
			this.empire_rb_.Text = "Empire";
			// 
			// exodus_rb_
			// 
			this.exodus_rb_.Image = ((System.Drawing.Image)(resources.GetObject("exodus_rb_.Image")));
			this.exodus_rb_.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
			this.exodus_rb_.Location = new System.Drawing.Point(24, 112);
			this.exodus_rb_.Name = "exodus_rb_";
			this.exodus_rb_.Size = new System.Drawing.Size(160, 64);
			this.exodus_rb_.TabIndex = 18;
			this.exodus_rb_.Text = "Exodus";
			// 
			// harckback_rb_
			// 
			this.harckback_rb_.Image = ((System.Drawing.Image)(resources.GetObject("harckback_rb_.Image")));
			this.harckback_rb_.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
			this.harckback_rb_.Location = new System.Drawing.Point(24, 176);
			this.harckback_rb_.Name = "harckback_rb_";
			this.harckback_rb_.Size = new System.Drawing.Size(160, 40);
			this.harckback_rb_.TabIndex = 17;
			this.harckback_rb_.Text = "Harckbackhood";
			// 
			// label19
			// 
			this.label19.Location = new System.Drawing.Point(224, 200);
			this.label19.Name = "label19";
			this.label19.Size = new System.Drawing.Size(56, 16);
			this.label19.TabIndex = 15;
			this.label19.Text = "Triggers";
			// 
			// player_triggers_chk_
			// 
			this.player_triggers_chk_.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
			this.player_triggers_chk_.Location = new System.Drawing.Point(288, 200);
			this.player_triggers_chk_.Name = "player_triggers_chk_";
			this.player_triggers_chk_.Size = new System.Drawing.Size(13, 16);
			this.player_triggers_chk_.TabIndex = 14;
			this.player_triggers_chk_.CheckedChanged += new System.EventHandler(this.player_triggers_chk__CheckedChanged);
			// 
			// player_name_edt_
			// 
			this.player_name_edt_.Location = new System.Drawing.Point(288, 56);
			this.player_name_edt_.Name = "player_name_edt_";
			this.player_name_edt_.Size = new System.Drawing.Size(88, 20);
			this.player_name_edt_.TabIndex = 13;
			this.player_name_edt_.Text = "";
			// 
			// label18
			// 
			this.label18.Location = new System.Drawing.Point(224, 56);
			this.label18.Name = "label18";
			this.label18.Size = new System.Drawing.Size(56, 16);
			this.label18.TabIndex = 12;
			this.label18.Text = "Name";
			// 
			// player_handicap_ud_
			// 
			this.player_handicap_ud_.Increment = new System.Decimal(new int[] {
																										100,
																										0,
																										0,
																										0});
			this.player_handicap_ud_.Location = new System.Drawing.Point(288, 176);
			this.player_handicap_ud_.Maximum = new System.Decimal(new int[] {
																									 1000,
																									 0,
																									 0,
																									 0});
			this.player_handicap_ud_.Name = "player_handicap_ud_";
			this.player_handicap_ud_.Size = new System.Drawing.Size(88, 20);
			this.player_handicap_ud_.TabIndex = 11;
			this.player_handicap_ud_.Value = new System.Decimal(new int[] {
																								  100,
																								  0,
																								  0,
																								  0});
			// 
			// label17
			// 
			this.label17.Location = new System.Drawing.Point(224, 176);
			this.label17.Name = "label17";
			this.label17.Size = new System.Drawing.Size(56, 16);
			this.label17.TabIndex = 10;
			this.label17.Text = "Handicap";
			// 
			// player_difficulty_cb_
			// 
			this.player_difficulty_cb_.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.player_difficulty_cb_.Items.AddRange(new object[] {
																						 "easy",
																						 "normal",
																						 "difficult"});
			this.player_difficulty_cb_.Location = new System.Drawing.Point(288, 152);
			this.player_difficulty_cb_.Name = "player_difficulty_cb_";
			this.player_difficulty_cb_.Size = new System.Drawing.Size(88, 21);
			this.player_difficulty_cb_.TabIndex = 9;
			// 
			// label16
			// 
			this.label16.Location = new System.Drawing.Point(224, 152);
			this.label16.Name = "label16";
			this.label16.Size = new System.Drawing.Size(48, 16);
			this.label16.TabIndex = 8;
			this.label16.Text = "Difficulty";
			// 
			// player_clan_cb_
			// 
			this.player_clan_cb_.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.player_clan_cb_.Items.AddRange(new object[] {
																				 "none",
																				 "1",
																				 "2",
																				 "3",
																				 "4"});
			this.player_clan_cb_.Location = new System.Drawing.Point(288, 128);
			this.player_clan_cb_.Name = "player_clan_cb_";
			this.player_clan_cb_.Size = new System.Drawing.Size(88, 21);
			this.player_clan_cb_.TabIndex = 7;
			// 
			// label15
			// 
			this.label15.Location = new System.Drawing.Point(224, 128);
			this.label15.Name = "label15";
			this.label15.Size = new System.Drawing.Size(48, 16);
			this.label15.TabIndex = 6;
			this.label15.Text = "Clan";
			// 
			// label14
			// 
			this.label14.Location = new System.Drawing.Point(224, 104);
			this.label14.Name = "label14";
			this.label14.Size = new System.Drawing.Size(48, 16);
			this.label14.TabIndex = 5;
			this.label14.Text = "Colour";
			// 
			// player_color_cb_
			// 
			this.player_color_cb_.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.player_color_cb_.Items.AddRange(new object[] {
																				  "blue",
																				  "red",
																				  "orange",
																				  "green",
																				  "yellow",
																				  "dark blue"});
			this.player_color_cb_.Location = new System.Drawing.Point(288, 104);
			this.player_color_cb_.Name = "player_color_cb_";
			this.player_color_cb_.Size = new System.Drawing.Size(88, 21);
			this.player_color_cb_.TabIndex = 4;
			// 
			// player_type_cb_
			// 
			this.player_type_cb_.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.player_type_cb_.Items.AddRange(new object[] {
																				 "human",
																				 "AI"});
			this.player_type_cb_.Location = new System.Drawing.Point(288, 80);
			this.player_type_cb_.Name = "player_type_cb_";
			this.player_type_cb_.Size = new System.Drawing.Size(88, 21);
			this.player_type_cb_.TabIndex = 1;
			// 
			// label7
			// 
			this.label7.Location = new System.Drawing.Point(224, 80);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(40, 16);
			this.label7.TabIndex = 0;
			this.label7.Text = "Type";
			// 
			// interface_pg_
			// 
			this.interface_pg_.Controls.Add(this.remove_all_btn_);
			this.interface_pg_.Controls.Add(this.control_remove_btn_);
			this.interface_pg_.Controls.Add(this.control_new_btn_);
			this.interface_pg_.Controls.Add(this.control_properties_);
			this.interface_pg_.Controls.Add(this.controls_lst_);
			this.interface_pg_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.interface_pg_.Location = new System.Drawing.Point(0, 0);
			this.interface_pg_.Name = "interface_pg_";
			this.interface_pg_.Size = new System.Drawing.Size(392, 245);
			this.interface_pg_.TabIndex = 0;
			this.interface_pg_.Text = "Interface";
			// 
			// control_remove_btn_
			// 
			this.control_remove_btn_.Enabled = false;
			this.control_remove_btn_.Location = new System.Drawing.Point(104, 200);
			this.control_remove_btn_.Name = "control_remove_btn_";
			this.control_remove_btn_.Size = new System.Drawing.Size(72, 23);
			this.control_remove_btn_.TabIndex = 5;
			this.control_remove_btn_.Text = "&Remove";
			this.control_remove_btn_.Click += new System.EventHandler(this.control_remove_btn__Click);
			// 
			// control_new_btn_
			// 
			this.control_new_btn_.Location = new System.Drawing.Point(16, 200);
			this.control_new_btn_.Name = "control_new_btn_";
			this.control_new_btn_.Size = new System.Drawing.Size(72, 23);
			this.control_new_btn_.TabIndex = 4;
			this.control_new_btn_.Text = "&New";
			this.control_new_btn_.Click += new System.EventHandler(this.control_new_btn__Click);
			// 
			// control_properties_
			// 
			this.control_properties_.CommandsVisibleIfAvailable = true;
			this.control_properties_.HelpVisible = false;
			this.control_properties_.LargeButtons = false;
			this.control_properties_.LineColor = System.Drawing.SystemColors.ScrollBar;
			this.control_properties_.Location = new System.Drawing.Point(168, 16);
			this.control_properties_.Name = "control_properties_";
			this.control_properties_.PropertySort = System.Windows.Forms.PropertySort.Alphabetical;
			this.control_properties_.Size = new System.Drawing.Size(208, 168);
			this.control_properties_.TabIndex = 3;
			this.control_properties_.Text = "PropertyGrid";
			this.control_properties_.ToolbarVisible = false;
			this.control_properties_.ViewBackColor = System.Drawing.SystemColors.Window;
			this.control_properties_.ViewForeColor = System.Drawing.SystemColors.WindowText;
			this.control_properties_.PropertyValueChanged += new System.Windows.Forms.PropertyValueChangedEventHandler(this.control_properties__PropertyValueChanged);
			// 
			// controls_lst_
			// 
			this.controls_lst_.HorizontalScrollbar = true;
			this.controls_lst_.IntegralHeight = false;
			this.controls_lst_.Location = new System.Drawing.Point(16, 16);
			this.controls_lst_.Name = "controls_lst_";
			this.controls_lst_.Size = new System.Drawing.Size(136, 168);
			this.controls_lst_.Sorted = true;
			this.controls_lst_.TabIndex = 0;
			this.controls_lst_.SelectedIndexChanged += new System.EventHandler(this.controls_lst__SelectedIndexChanged);
			// 
			// toolbar_
			// 
			this.toolbar_.Appearance = System.Windows.Forms.ToolBarAppearance.Flat;
			this.toolbar_.Buttons.AddRange(new System.Windows.Forms.ToolBarButton[] {
																												this.open_btn_,
																												this.save_btn_,
																												this.about_btn_});
			this.toolbar_.Divider = false;
			this.toolbar_.DropDownArrows = true;
			this.toolbar_.ImageList = this.toolbar_images_;
			this.toolbar_.Location = new System.Drawing.Point(0, 0);
			this.toolbar_.Name = "toolbar_";
			this.toolbar_.ShowToolTips = true;
			this.toolbar_.Size = new System.Drawing.Size(472, 34);
			this.toolbar_.TabIndex = 2;
			this.toolbar_.TextAlign = System.Windows.Forms.ToolBarTextAlign.Right;
			this.toolbar_.ButtonClick += new System.Windows.Forms.ToolBarButtonClickEventHandler(this.toolbar__ButtonClick);
			// 
			// open_btn_
			// 
			this.open_btn_.ImageIndex = 0;
			this.open_btn_.Text = "&Open";
			// 
			// save_btn_
			// 
			this.save_btn_.ImageIndex = 1;
			this.save_btn_.Text = "&Save";
			// 
			// about_btn_
			// 
			this.about_btn_.ImageIndex = 2;
			this.about_btn_.Text = "About";
			// 
			// toolbar_images_
			// 
			this.toolbar_images_.ColorDepth = System.Windows.Forms.ColorDepth.Depth24Bit;
			this.toolbar_images_.ImageSize = new System.Drawing.Size(24, 24);
			this.toolbar_images_.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("toolbar_images_.ImageStream")));
			this.toolbar_images_.TransparentColor = System.Drawing.Color.Magenta;
			// 
			// remove_all_btn_
			// 
			this.remove_all_btn_.Location = new System.Drawing.Point(192, 200);
			this.remove_all_btn_.Name = "remove_all_btn_";
			this.remove_all_btn_.Size = new System.Drawing.Size(72, 23);
			this.remove_all_btn_.TabIndex = 6;
			this.remove_all_btn_.Text = "Remove All";
			this.remove_all_btn_.Click += new System.EventHandler(this.remove_all_btn__Click);
			// 
			// MainForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(472, 279);
			this.Controls.Add(this.multipanel);
			this.Controls.Add(this.button_pnl_);
			this.Controls.Add(this.toolbar_);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.Name = "MainForm";
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
			this.Text = "Mission Editor";
			this.button_pnl_.ResumeLayout(false);
			this.multipanel.ResumeLayout(false);
			this.world_pg_.ResumeLayout(false);
			this.groupBox3.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.omega_distance_ud_)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.alpha_distance_ud_)).EndInit();
			this.groupBox2.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.spiral_priority_ud_)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.spiral_time_ud_)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.spiral_energy_ud_)).EndInit();
			this.groupBox1.ResumeLayout(false);
			this.player_pg_.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.player_handicap_ud_)).EndInit();
			this.interface_pg_.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		#region data

		private MissionEdit.FlatButton             interface_btn_;
		private MissionEdit.FlatButton             player_btn_;
		private MissionEdit.FlatButton             world_btn_;
		private MultipanelLib.Multipanel           multipanel;
		private MultipanelLib.Page                 interface_pg_;
		private MultipanelLib.Page                 player_pg_;
		private MultipanelLib.Page                 world_pg_;
		private System.Windows.Forms.Button        control_remove_btn_;
		private System.Windows.Forms.Button        edit_trigger_btn_;
		private System.Windows.Forms.CheckBox      player_enabled_chk_;
		private System.Windows.Forms.CheckBox      player_triggers_chk_;
		private System.Windows.Forms.ComboBox      player_cb_;
		private System.Windows.Forms.ComboBox      player_clan_cb_;
		private System.Windows.Forms.ComboBox      player_color_cb_;
		private System.Windows.Forms.ComboBox      player_difficulty_cb_;
		private System.Windows.Forms.ComboBox      player_type_cb_;
		private System.Windows.Forms.ComboBox      world_dificulty_cb_;
		private System.Windows.Forms.GroupBox      groupBox1;
		private System.Windows.Forms.GroupBox      groupBox2;
		private System.Windows.Forms.GroupBox      groupBox3;
		private System.Windows.Forms.Label         label1;
		private System.Windows.Forms.Label         label10;
		private System.Windows.Forms.Label         label11;
		private System.Windows.Forms.Label         label12;
		private System.Windows.Forms.Label         label14;
		private System.Windows.Forms.Label         label15;
		private System.Windows.Forms.Label         label16;
		private System.Windows.Forms.Label         label17;
		private System.Windows.Forms.Label         label18;
		private System.Windows.Forms.Label         label19;
		private System.Windows.Forms.Label         label2;
		private System.Windows.Forms.Label         label3;
		private System.Windows.Forms.Label         label4;
		private System.Windows.Forms.Label         label5;
		private System.Windows.Forms.Label         label6;
		private System.Windows.Forms.Label         label7;
		private System.Windows.Forms.Label         label8;
		private System.Windows.Forms.Label         label9;
		private System.Windows.Forms.ListBox       controls_lst_;
		private System.Windows.Forms.NumericUpDown alpha_distance_ud_;
		private System.Windows.Forms.NumericUpDown omega_distance_ud_;
		private System.Windows.Forms.NumericUpDown player_handicap_ud_;
		private System.Windows.Forms.NumericUpDown spiral_energy_ud_;
		private System.Windows.Forms.NumericUpDown spiral_priority_ud_;
		private System.Windows.Forms.NumericUpDown spiral_time_ud_;
		private System.Windows.Forms.Panel         button_pnl_;
		private System.Windows.Forms.PropertyGrid  control_properties_;
		private System.Windows.Forms.RadioButton   empire_rb_;
		private System.Windows.Forms.RadioButton   exodus_rb_;
		private System.Windows.Forms.RadioButton   harckback_rb_;
		private System.Windows.Forms.TextBox       battle_soundtrack_edt_;
		private System.Windows.Forms.TextBox       construction_soundtrack_edt_;
		private System.Windows.Forms.TextBox       player_name_edt_;
		private System.Windows.Forms.TextBox       regular_soundtrack_edt_;
		private System.Windows.Forms.TextBox       world_description_edt_;
		private System.Windows.Forms.TextBox       world_name_edt_;

		#endregion

	}
}
