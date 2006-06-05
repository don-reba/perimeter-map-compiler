using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.IO;
using System.Xml;

namespace MissionEdit
{
	public class MainForm : System.Windows.Forms.Form
	{
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

		#endregion

		//------------
		// entry point
		//------------

		[STAThread]
		static void Main(string[] args) 
		{
			Application.Run(new MainForm(args[0]));
		}

		//----------
		// interface
		//----------

		#region

		public MainForm(string file_name)
		{
			current_player_ = -1;
			InitializeComponent();
			InitializeControlList();
			InitializePlayers();
			LoadFile(file_name);
			player_cb_.SelectedIndex = 0;
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

		private void add_controls_btn__Click(object sender, System.EventArgs e)
		{
			// stop redrawing the lists
			enabled_controls_lst_.SuspendLayout();
			disabled_controls_lst_.SuspendLayout();
			// get the items
			ListBox.ObjectCollection enabled        = enabled_controls_lst_.Items;
			ListBox.ObjectCollection disabled       = disabled_controls_lst_.Items;
			ListBox.SelectedIndexCollection indices = enabled_controls_lst_.SelectedIndices;
			// get the selection
			int[] selection = new int[indices.Count];
			for (int i = 0; i != selection.Length; ++i)
				selection[i] = (int)indices[i];
			// move items
			int removed_count = 0;
			foreach(int i in selection)
			{
				disabled.Add(enabled[i - removed_count]);
				enabled.RemoveAt(i - removed_count);
				++removed_count;
			}
			// redraw the lists
			disabled_controls_lst_.ResumeLayout();
			enabled_controls_lst_.ResumeLayout();
		}

		private void remove_controls_btn__Click(object sender, System.EventArgs e)
		{
			// stop redrawing the lists
			enabled_controls_lst_.SuspendLayout();
			disabled_controls_lst_.SuspendLayout();
			// get the items
			ListBox.ObjectCollection enabled  = enabled_controls_lst_.Items;
			ListBox.ObjectCollection disabled = disabled_controls_lst_.Items;
			ListBox.SelectedIndexCollection indices = disabled_controls_lst_.SelectedIndices;
			// get the selection
			int[] selection = new int[indices.Count];
			for (int i = 0; i != selection.Length; ++i)
				selection[i] = (int)indices[i];
			// move items
			int removed_count = 0;
			foreach(int i in selection)
			{
				enabled.Add(disabled[i - removed_count]);
				disabled.RemoveAt(i - removed_count);
				++removed_count;
			}
			// redraw the lists
			disabled_controls_lst_.ResumeLayout();
			enabled_controls_lst_.ResumeLayout();
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
			if (player_nodes.Count > player_data_.Length)
				player_data_ = new PlayerData[player_nodes.Count];
			for (int i = 0; i != player_nodes.Count; ++i)
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
				player_data_[i].triggers_ = (null != query);
			}
		}

		private void InitializeControlList() 
		{
			string[] items = 
				{
					#region
					"BACKGRND_ID",
					"BAR_SQUAD1_ID",
					"BAR_SQUAD2_ID",
					"BAR_SQUAD3_ID",
					"BAR_SQUAD4_ID",
					"BAR_SQUAD5_ID",
					"CHAT_INFO_ID",
					"COMMANDER_ID",
					"CORRIDOR_ALPHA_ID",
					"CORRIDOR_OMEGA_ID",
					"EMBLEM_ID",
					"EMPTY_WND",
					"FIELD_OFF_ID",
					"FIELD_ON_ID",
					"FRAME_TERRAIN_BUILD1_ID",
					"FRAME_TERRAIN_BUILD2_ID",
					"FRAME_TERRAIN_BUILD3_ID",
					"FRAME_TERRAIN_BUILD4_ID",
					"FRAME_TERRAIN_BUILD5_ID",
					"GAME_MAX",
					"GAME_SCREEN_ID",
					"GUN_BALLISTIC_ID",
					"GUN_ELECTRO_ID",
					"GUN_FILTH_ID",
					"GUN_GIMLET_ID",
					"GUN_HOWITZER_ID",
					"GUN_LASER_ID",
					"GUN_ROCKET_ID",
					"GUN_SUBCHASER_ID",
					"HINT_ID",
					"INFOWND_ID",
					"INGAME_CHAT_EDIT_ID",
					"MAP_WINDOW_ID",
					"MENU_BUTTON_ID",
					"MM_APPLY_BTN",
					"MM_APPLY_NAME_BTN",
					"MM_BACK",
					"MM_BACK_CREDITS_BTN",
					"MM_BACK_FROM_BATTLE_BTN",
					"MM_BACK_FROM_CREATE_GAME_BTN",
					"MM_BACK_FROM_CREATE_ONLINE_GAME_BTN",
					"MM_BACK_FROM_CUSTOM_BTN",
					"MM_BACK_FROM_GAME_BTN",
					"MM_BACK_FROM_GRAPHICS_BTN",
					"MM_BACK_FROM_INGAME_CUSTOM",
					"MM_BACK_FROM_INGAME_GAME_OPTIONS",
					"MM_BACK_FROM_INGAME_GRAPHICS_BTN",
					"MM_BACK_FROM_INGAME_OPTIONS",
					"MM_BACK_FROM_INGAME_SOUND",
					"MM_BACK_FROM_LAN_BTN",
					"MM_BACK_FROM_LOAD_BTN",
					"MM_BACK_FROM_LOAD_IN_GAME_BTN",
					"MM_BACK_FROM_LOAD_REPLAY_BTN",
					"MM_BACK_FROM_LOBBY_BTN",
					"MM_BACK_FROM_NAME_INPUT_BTN",
					"MM_BACK_FROM_ONLINE_BTN",
					"MM_BACK_FROM_ONLINE_LOBBY_BTN",
					"MM_BACK_FROM_OPTIONS_BTN",
					"MM_BACK_FROM_PROFILE_BTN",
					"MM_BACK_FROM_SAVE_GAME_BTN",
					"MM_BACK_FROM_SAVE_REPLAY_BTN",
					"MM_BACK_FROM_SCENARIO_BTN",
					"MM_BACK_FROM_SINGLE_BTN",
					"MM_BACK_FROM_SOUND_BTN",
					"MM_BACK_FROM_TASK_BTN",
					"MM_BATTLE_BTN",
					"MM_BATTLE_GO_BTN",
					"MM_BATTLE_MAP",
					"MM_BATTLE_MAP_DESCR_TXT",
					"MM_BATTLE_PLAYER1_CLAN_BTN",
					"MM_BATTLE_PLAYER1_CLR_BG",
					"MM_BATTLE_PLAYER1_CLR_BTN",
					"MM_BATTLE_PLAYER1_FRM_BTN",
					"MM_BATTLE_PLAYER1_HC_BTN",
					"MM_BATTLE_PLAYER1_SLOT_BTN",
					"MM_BATTLE_PLAYER2_CLAN_BTN",
					"MM_BATTLE_PLAYER2_CLR_BG",
					"MM_BATTLE_PLAYER2_CLR_BTN",
					"MM_BATTLE_PLAYER2_FRM_BTN",
					"MM_BATTLE_PLAYER2_HC_BTN",
					"MM_BATTLE_PLAYER2_SLOT_BTN",
					"MM_BATTLE_PLAYER3_CLAN_BTN",
					"MM_BATTLE_PLAYER3_CLR_BG",
					"MM_BATTLE_PLAYER3_CLR_BTN",
					"MM_BATTLE_PLAYER3_FRM_BTN",
					"MM_BATTLE_PLAYER3_HC_BTN",
					"MM_BATTLE_PLAYER3_SLOT_BTN",
					"MM_BATTLE_PLAYER4_CLAN_BTN",
					"MM_BATTLE_PLAYER4_CLR_BG",
					"MM_BATTLE_PLAYER4_CLR_BTN",
					"MM_BATTLE_PLAYER4_FRM_BTN",
					"MM_BATTLE_PLAYER4_HC_BTN",
					"MM_BATTLE_PLAYER4_SLOT_BTN",
					"MM_BATTLE_SCR",
					"MM_BATTLE_SURVIVAL_TXT",
					"MM_BRIEFING_ICON",
					"MM_BRIEFING_SCR",
					"MM_BRIEFING_TXT",
					"MM_BRIEFING_YEAR_TXT",
					"MM_CONNECTION_TYPE_COMBO",
					"MM_CONTINUE_BRIEFING_BORDER",
					"MM_CONTINUE_BRIEFING_BTN",
					"MM_CONTINUE_BTN",
					"MM_CONTINUE_FROM_STATS_BORDER",
					"MM_CONTINUE_FROM_STATS_BTN",
					"MM_CREATE_BTN",
					"MM_CREATE_GAME_MAP",
					"MM_CREATE_GAME_MAP_DESCR_TXT",
					"MM_CREATE_GAME_SCR",
					"MM_CREATE_ONLINE_GAME_MAP",
					"MM_CREATE_ONLINE_GAME_MAP_DESCR_TXT",
					"MM_CREATE_ONLINE_GAME_SCR",
					"MM_CREDITS_BTN",
					"MM_CREDITS_SCR",
					"MM_CREDITS_TXT",
					"MM_CUSTOM_GRAPHICS_BTN",
					"MM_CUSTOM_INGAME_GRAPHICS_BTN",
					"MM_CUSTOM_SCR",
					"MM_DEL_PROFILE_BTN",
					"MM_DEL_REPLAY_BTN",
					"MM_DEL_SAVE_BTN",
					"MM_DIFFICULTY_BTN",
					"MM_DIFFICULTY_COMBO",
					"MM_ENDMISSION_SCR",
					"MM_GAME_ANGLESENS",
					"MM_GAME_ANGLESENS_SLIDER",
					"MM_GAME_LIST",
					"MM_GAME_MOUSE_SLIDER",
					"MM_GAME_MOUSESPEED",
					"MM_GAME_SCR",
					"MM_GAME_SCROLLRATE",
					"MM_GAME_SCROLLRATE_SLIDER",
					"MM_GAME_TOOLTIPS",
					"MM_GAME_TOOLTIPS_COMBO",
					"MM_GO_BTN",
					"MM_GRAPHICS_BUMP",
					"MM_GRAPHICS_BUMP_CHAOS",
					"MM_GRAPHICS_BUMP_CHAOS_COMBO",
					"MM_GRAPHICS_BUMP_COMBO",
					"MM_GRAPHICS_COLORDEPTH",
					"MM_GRAPHICS_COLORDEPTH_COMBO",
					"MM_GRAPHICS_COMPRESS",
					"MM_GRAPHICS_COMPRESS_COMBO",
					"MM_GRAPHICS_FURROWS",
					"MM_GRAPHICS_FURROWS_COMBO",
					"MM_GRAPHICS_GAMMA",
					"MM_GRAPHICS_GAMMA_SLIDER",
					"MM_GRAPHICS_LOD",
					"MM_GRAPHICS_LOD_COMBO",
					"MM_GRAPHICS_LOD_SLIDER",
					"MM_GRAPHICS_MODE",
					"MM_GRAPHICS_MODE_COMBO",
					"MM_GRAPHICS_OCCLUSION",
					"MM_GRAPHICS_OCCLUSION_COMBO",
					"MM_GRAPHICS_POINT_LIGHT",
					"MM_GRAPHICS_POINT_LIGHT_COMBO",
					"MM_GRAPHICS_REFLECTION",
					"MM_GRAPHICS_REFLECTION_COMBO",
					"MM_GRAPHICS_RESOLUTION",
					"MM_GRAPHICS_RESOLUTION_COMBO",
					"MM_GRAPHICS_SCR",
					"MM_GRAPHICS_SHADOWS",
					"MM_GRAPHICS_SHADOWS_COMBO",
					"MM_GRAPHICS_SHADOWS_SAMPLES",
					"MM_GRAPHICS_SHADOWS_SAMPLES_COMBO",
					"MM_INGAME_APPLY_BTN",
					"MM_INGAME_CUSTOM_SCR",
					"MM_INMISSION_LOAD_BTN",
					"MM_INMISSION_OPTIONS_BTN",
					"MM_INMISSION_QUIT_BTN",
					"MM_INMISSION_RESTART_BTN",
					"MM_INMISSION_RESUME_BTN",
					"MM_INMISSION_SAVE_BTN",
					"MM_INMISSION_SCR",
					"MM_IP_BTN",
					"MM_IP_INPUT",
					"MM_JOIN_BTN",
					"MM_LAN_BTN",
					"MM_LAN_CREATE_GAME_BTN",
					"MM_LAN_CREATE_GAMESPY_LOGO",
					"MM_LAN_GAME_MAP",
					"MM_LAN_GAME_MAP_DESCR_TXT",
					"MM_LAN_GAME_SPEED_BTN",
					"MM_LAN_GAME_SPEED_SLIDER",
					"MM_LAN_GAMESPY_LOGO",
					"MM_LAN_LOBBY_GAMESPY_LOGO",
					"MM_LAN_MAP_LIST",
					"MM_LAN_PLAYER_NAME_INPUT",
					"MM_LAN_SCR",
					"MM_LOAD_BTN",
					"MM_LOAD_GO_BTN",
					"MM_LOAD_IN_GAME_DEL_BTN",
					"MM_LOAD_IN_GAME_GO_BTN",
					"MM_LOAD_IN_GAME_MAP",
					"MM_LOAD_IN_GAME_MAP_DESCR_TXT",
					"MM_LOAD_IN_GAME_MAP_LIST",
					"MM_LOAD_IN_GAME_SCR",
					"MM_LOAD_MAP",
					"MM_LOAD_MAP_DESCR_TXT",
					"MM_LOAD_MAP_LIST",
					"MM_LOAD_REPLAY_DESCR_TXT",
					"MM_LOAD_REPLAY_GO_BTN",
					"MM_LOAD_REPLAY_LIST",
					"MM_LOAD_REPLAY_MAP",
					"MM_LOAD_REPLAY_SCR",
					"MM_LOAD_SCR",
					"MM_LOADING_MISSION_SCR",
					"MM_LOADING_NOMAD_ICON",
					"MM_LOADING_NOMAD_TXT",
					"MM_LOBBY_CHAT_INPUT",
					"MM_LOBBY_CHAT_TEXT",
					"MM_LOBBY_GAME_MAP",
					"MM_LOBBY_GAME_MAP_DESCR_TXT",
					"MM_LOBBY_GAME_NAME_BTN",
					"MM_LOBBY_HOST_GAME_MAP",
					"MM_LOBBY_MAP_LIST",
					"MM_LOBBY_MAP_LIST_RAMKA1",
					"MM_LOBBY_MAP_LIST_RAMKA2",
					"MM_LOBBY_MAP_LIST_RAMKA3",
					"MM_LOBBY_MAP_LIST_RAMKA4",
					"MM_LOBBY_PLAYER1_CLAN_BTN",
					"MM_LOBBY_PLAYER1_CLR_BG",
					"MM_LOBBY_PLAYER1_CLR_BTN",
					"MM_LOBBY_PLAYER1_FRM_BTN",
					"MM_LOBBY_PLAYER1_HC_BTN",
					"MM_LOBBY_PLAYER1_NAME_BTN",
					"MM_LOBBY_PLAYER1_READY_BTN",
					"MM_LOBBY_PLAYER1_SLOT_BTN",
					"MM_LOBBY_PLAYER2_CLAN_BTN",
					"MM_LOBBY_PLAYER2_CLR_BG",
					"MM_LOBBY_PLAYER2_CLR_BTN",
					"MM_LOBBY_PLAYER2_FRM_BTN",
					"MM_LOBBY_PLAYER2_HC_BTN",
					"MM_LOBBY_PLAYER2_NAME_BTN",
					"MM_LOBBY_PLAYER2_READY_BTN",
					"MM_LOBBY_PLAYER2_SLOT_BTN",
					"MM_LOBBY_PLAYER3_CLAN_BTN",
					"MM_LOBBY_PLAYER3_CLR_BG",
					"MM_LOBBY_PLAYER3_CLR_BTN",
					"MM_LOBBY_PLAYER3_FRM_BTN",
					"MM_LOBBY_PLAYER3_HC_BTN",
					"MM_LOBBY_PLAYER3_NAME_BTN",
					"MM_LOBBY_PLAYER3_READY_BTN",
					"MM_LOBBY_PLAYER3_SLOT_BTN",
					"MM_LOBBY_PLAYER4_CLAN_BTN",
					"MM_LOBBY_PLAYER4_CLR_BG",
					"MM_LOBBY_PLAYER4_CLR_BTN",
					"MM_LOBBY_PLAYER4_FRM_BTN",
					"MM_LOBBY_PLAYER4_HC_BTN",
					"MM_LOBBY_PLAYER4_NAME_BTN",
					"MM_LOBBY_PLAYER4_READY_BTN",
					"MM_LOBBY_PLAYER4_SLOT_BTN",
					"MM_LOBBY_SCR",
					"MM_LOBBY_START_BORDER",
					"MM_LOBBY_START_BTN",
					"MM_MAP_LIST",
					"MM_MAPWINDOW",
					"MM_MISSION_DESCR_TXT",
					"MM_MISSION_LIST",
					"MM_MISSION_TASK_SCR",
					"MM_MISSION_TASK_TXT",
					"MM_NAME_INPUT_SCR",
					"MM_NEW_PROFILE_BTN",
					"MM_NOMAD_ICON",
					"MM_NOMAD_TXT",
					"MM_ONLINE_BTN",
					"MM_ONLINE_CREATE_BTN",
					"MM_ONLINE_CREATE_GAME_BTN",
					"MM_ONLINE_GAME_LIST",
					"MM_ONLINE_GAME_MAP",
					"MM_ONLINE_GAME_MAP_DESCR_TXT",
					"MM_ONLINE_GAME_SPEED_BTN",
					"MM_ONLINE_GAME_SPEED_SLIDER",
					"MM_ONLINE_JOIN_BTN",
					"MM_ONLINE_LOBBY_GAME_MAP",
					"MM_ONLINE_LOBBY_GAME_MAP_DESCR_TXT",
					"MM_ONLINE_LOBBY_GAME_NAME_BTN",
					"MM_ONLINE_LOBBY_PLAYER1_CLR_BG",
					"MM_ONLINE_LOBBY_PLAYER1_CLR_BTN",
					"MM_ONLINE_LOBBY_PLAYER1_FRM_BTN",
					"MM_ONLINE_LOBBY_PLAYER1_NAME_BTN",
					"MM_ONLINE_LOBBY_PLAYER1_SLOT_BTN",
					"MM_ONLINE_LOBBY_PLAYER2_CLR_BG",
					"MM_ONLINE_LOBBY_PLAYER2_CLR_BTN",
					"MM_ONLINE_LOBBY_PLAYER2_FRM_BTN",
					"MM_ONLINE_LOBBY_PLAYER2_NAME_BTN",
					"MM_ONLINE_LOBBY_PLAYER2_SLOT_BTN",
					"MM_ONLINE_LOBBY_PLAYER3_CLR_BG",
					"MM_ONLINE_LOBBY_PLAYER3_CLR_BTN",
					"MM_ONLINE_LOBBY_PLAYER3_FRM_BTN",
					"MM_ONLINE_LOBBY_PLAYER3_NAME_BTN",
					"MM_ONLINE_LOBBY_PLAYER3_SLOT_BTN",
					"MM_ONLINE_LOBBY_PLAYER4_CLR_BG",
					"MM_ONLINE_LOBBY_PLAYER4_CLR_BTN",
					"MM_ONLINE_LOBBY_PLAYER4_FRM_BTN",
					"MM_ONLINE_LOBBY_PLAYER4_NAME_BTN",
					"MM_ONLINE_LOBBY_PLAYER4_SLOT_BTN",
					"MM_ONLINE_LOBBY_SCR",
					"MM_ONLINE_LOBBY_START_BTN",
					"MM_ONLINE_MAP_LIST",
					"MM_ONLINE_PLAYER_NAME_INPUT",
					"MM_ONLINE_SCR",
					"MM_OPTIONS",
					"MM_OPTIONS_BTN",
					"MM_OPTIONS_GAME",
					"MM_OPTIONS_GAME_BTN",
					"MM_OPTIONS_GRAPHICS",
					"MM_OPTIONS_GRAPHICS_BTN",
					"MM_OPTIONS_SCR",
					"MM_OPTIONS_SOUND",
					"MM_OPTIONS_SOUND_BTN",
					"MM_PARTICLE_RATE",
					"MM_PARTICLE_RATE_SLIDER",
					"MM_PLAYER_NAME_INPUT",
					"MM_PROFILE_BTN",
					"MM_PROFILE_LIST",
					"MM_PROFILE_NAME_INPUT",
					"MM_PROFILE_SCR",
					"MM_QUIT_BTN",
					"MM_QUIT_FROM_BRIEFING_BTN",
					"MM_QUIT_FROM_STATS_BTN",
					"MM_RAMKA",
					"MM_REPLAY_BORDER",
					"MM_REPLAY_BTN",
					"MM_REPLAY_LINE",
					"MM_REPLAY_NAME_INPUT",
					"MM_RESTART_BTN",
					"MM_RESULT_TXT",
					"MM_RESUME_BORDER",
					"MM_RESUME_BTN",
					"MM_SAVE_GAME_DEL_BTN",
					"MM_SAVE_GAME_GO_BTN",
					"MM_SAVE_GAME_MAP",
					"MM_SAVE_GAME_MAP_DESCR_TXT",
					"MM_SAVE_GAME_MAP_LIST",
					"MM_SAVE_GAME_SCR",
					"MM_SAVE_NAME_INPUT",
					"MM_SAVE_REPLAY_BORDER",
					"MM_SAVE_REPLAY_BTN",
					"MM_SAVE_REPLAY_DEL_BTN",
					"MM_SAVE_REPLAY_DESCR_TXT",
					"MM_SAVE_REPLAY_GO_BTN",
					"MM_SAVE_REPLAY_LIST",
					"MM_SAVE_REPLAY_MAP",
					"MM_SAVE_REPLAY_SCR",
					"MM_SCENARIO_BTN",
					"MM_SCENARIO_SCR",
					"MM_SCREEN_GAME",
					"MM_SCREEN_GRAPHICS",
					"MM_SCREEN_OPTIONS",
					"MM_SCREEN_SOUND",
					"MM_SELECT_PROFILE_BTN",
					"MM_SETTINGS",
					"MM_SETTINGS_COMBO",
					"MM_SINGLE_BTN",
					"MM_SINGLE_SCR",
					"MM_SKIP_BRIEFING_BORDER",
					"MM_SKIP_BRIEFING_BTN",
					"MM_SKIP_MISSION_BORDER",
					"MM_SKIP_MISSION_BTN",
					"MM_SOUND_MUSIC",
					"MM_SOUND_MUSIC_COMBO",
					"MM_SOUND_MUSICVOLUME",
					"MM_SOUND_MUSICVOLUME_SLIDER",
					"MM_SOUND_SCR",
					"MM_SOUND_SOUNDEFFECTS",
					"MM_SOUND_SOUNDEFFECTS_COMBO",
					"MM_SOUND_SOUNDVOLUME",
					"MM_SOUND_SOUNDVOLUME_SLIDER",
					"MM_SPLASH_LAST",
					"MM_SPLASH1",
					"MM_SPLASH2",
					"MM_SPLASH3",
					"MM_SPLASH4",
					"MM_START_BRIEFING_BORDER",
					"MM_START_MISSION_BTN",
					"MM_START_SCR",
					"MM_STATS_BUILDINGS_BTN",
					"MM_STATS_BUILDINGS_HEAD_LIST",
					"MM_STATS_BUILDINGS_LIST",
					"MM_STATS_BUILDINGS_RAMKA",
					"MM_STATS_GENERAL_BTN",
					"MM_STATS_GENERAL_HEAD_LIST",
					"MM_STATS_GENERAL_LIST",
					"MM_STATS_GENERAL_RAMKA",
					"MM_STATS_SCR",
					"MM_STATS_TOTAL_BTN",
					"MM_STATS_TOTAL_HEAD_LIST",
					"MM_STATS_TOTAL_LIST",
					"MM_STATS_TOTAL_RAMKA",
					"MM_STATS_UNITS_BTN",
					"MM_STATS_UNITS_HEAD_LIST",
					"MM_STATS_UNITS_LIST",
					"MM_STATS_UNITS_RAMKA",
					"MM_SUBMIT_DIALOG_SCR",
					"MM_SUBMIT_NO_BORDER",
					"MM_SUBMIT_NO_BTN",
					"MM_SUBMIT_OK_BORDER",
					"MM_SUBMIT_OK_BTN",
					"MM_SUBMIT_TXT",
					"MM_SUBMIT_YES_BORDER",
					"MM_SUBMIT_YES_BTN",
					"MM_VERSION_TXT",
					"OFFICER_ID",
					"OFFICER_PLANT_ID",
					"PROGRESS_COLLECTED",
					"PROGRESS_ENERGY",
					"RAMKA_ID",
					"RELAY_ID",
					"REPLAY_PLAYER_BUTTON_ID",
					"RESULT_WND",
					"SELPANEL_BRIG_BACK_ID",
					"SELPANEL_BRIG_BUILD_ID",
					"SELPANEL_BRIG_CHANGE_ID",
					"SELPANEL_FIELDOFF_ID",
					"SELPANEL_FIELDON_ID",
					"SELPANEL_FRAME_ALARM_ID",
					"SELPANEL_FRAME_INSTALL_ID",
					"SELPANEL_FRAME_TELEPORTATE_ID",
					"SELPANEL_MOVE_ID",
					"SELPANEL_POWEROFF_ID",
					"SELPANEL_POWERON_ID",
					"SELPANEL_SELL_ID",
					"SELPANEL_SQ_ATTACK_ID",
					"SELPANEL_SQ_BACK_ID",
					"SELPANEL_SQ_OFDEF_ID",
					"SELPANEL_SQ_PATROL_ID",
					"SELPANEL_START_CHARGE_ID",
					"SELPANEL_STOP_CHARGE_ID",
					"SELPANEL_STOP_ID",
					"SELPANEL_STOP2_ID",
					"SELPANEL_UNIT_CHARGE_ID",
					"SELPANEL_UPGRADE_BOMB1_ID  ",
					"SELPANEL_UPGRADE_BOMB2_ID  ",
					"SELPANEL_UPGRADE_ELECTRO1_ID ",
					"SELPANEL_UPGRADE_ELECTRO2_ID ",
					"SELPANEL_UPGRADE_EMPIRE1_ID",
					"SELPANEL_UPGRADE_EMPIRE2_ID ",
					"SELPANEL_UPGRADE_EXODUS1_ID",
					"SELPANEL_UPGRADE_EXODUS2_ID",
					"SELPANEL_UPGRADE_FLY_ID ",
					"SELPANEL_UPGRADE_HARKBACK1_ID ",
					"SELPANEL_UPGRADE_HARKBACK2_ID ",
					"SELPANEL_UPGRADE_ID",
					"SELPANEL_UPGRADE_LASER1_ID ",
					"SELPANEL_UPGRADE_LASER2_ID ",
					"SELPANEL_UPGRADE_OMEGA_ID ",
					"SELPANEL_UPGRADE_ROCKET1_ID",
					"SELPANEL_UPGRADE_ROCKET2_ID",
					"SELPANEL_UPGRADE_SUBTERRA_ID ",
					"SOLDIER_ID",
					"SOLDIER_PLANT_ID",
					"SPEED_100",
					"SPEED_150",
					"SPEED_50",
					"SPEED_PAUSE",
					"SQUAD_DISINTEGRATE_ID",
					"SQUAD_UNIT1",
					"SQUAD_UNIT10",
					"SQUAD_UNIT11",
					"SQUAD_UNIT12",
					"SQUAD_UNIT13",
					"SQUAD_UNIT14",
					"SQUAD_UNIT15",
					"SQUAD_UNIT16",
					"SQUAD_UNIT17",
					"SQUAD_UNIT18",
					"SQUAD_UNIT19",
					"SQUAD_UNIT2",
					"SQUAD_UNIT20",
					"SQUAD_UNIT21",
					"SQUAD_UNIT22",
					"SQUAD_UNIT23",
					"SQUAD_UNIT24",
					"SQUAD_UNIT25",
					"SQUAD_UNIT3",
					"SQUAD_UNIT4",
					"SQUAD_UNIT5",
					"SQUAD_UNIT6",
					"SQUAD_UNIT7",
					"SQUAD_UNIT8",
					"SQUAD_UNIT9",
					"STATIC_BOMB_ID",
					"STATION_ELECTRO_LAB_ID",
					"STATION_EMPIRE_LAB_ID",
					"STATION_EXODUS_LAB_ID",
					"STATION_HARKBACK_LAB_ID",
					"STATION1_ID",
					"STATION2_ID",
					"STATION3_ID",
					"STATION4_ID",
					"STATION5_ID",
					"TAB_BUILD_ID",
					"TAB_SQUAD_ID",
					"TASK_BUTTON_ID",
					"TECHNIC_ID",
					"TECHNIC_PLANT_ID",
					"TOGETHER_ID",
					"WORKAREA2_ID",
					"WORKAREA3_ID",
					"WORKAREA4_ID",
					"YADRO_EX_ID",
					"YADRO_ID"
					#endregion
				};
			enabled_controls_lst_.Items.AddRange(items);
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
		private System.Windows.Forms.Button edit_trigger_btn_;
		private PlayerData[] player_data_;

		#endregion

		//----------
		// generated
		//----------

		#region code

		private void InitializeComponent()
		{
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
			this.remove_controls_btn_ = new System.Windows.Forms.Button();
			this.add_controls_btn_ = new System.Windows.Forms.Button();
			this.label20 = new System.Windows.Forms.Label();
			this.label13 = new System.Windows.Forms.Label();
			this.disabled_controls_lst_ = new System.Windows.Forms.ListBox();
			this.enabled_controls_lst_ = new System.Windows.Forms.ListBox();
			this.edit_trigger_btn_ = new System.Windows.Forms.Button();
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
			this.button_pnl_.Location = new System.Drawing.Point(0, 0);
			this.button_pnl_.Name = "button_pnl_";
			this.button_pnl_.Size = new System.Drawing.Size(80, 239);
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
			this.multipanel.Location = new System.Drawing.Point(80, 0);
			this.multipanel.Name = "multipanel";
			this.multipanel.Pages.Add(this.world_pg_);
			this.multipanel.Pages.Add(this.player_pg_);
			this.multipanel.Pages.Add(this.interface_pg_);
			this.multipanel.Size = new System.Drawing.Size(392, 239);
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
			this.world_pg_.Size = new System.Drawing.Size(392, 239);
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
			this.player_pg_.Size = new System.Drawing.Size(392, 239);
			this.player_pg_.TabIndex = 0;
			this.player_pg_.Text = "Player";
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
																				 "player",
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
			this.interface_pg_.Controls.Add(this.remove_controls_btn_);
			this.interface_pg_.Controls.Add(this.add_controls_btn_);
			this.interface_pg_.Controls.Add(this.label20);
			this.interface_pg_.Controls.Add(this.label13);
			this.interface_pg_.Controls.Add(this.disabled_controls_lst_);
			this.interface_pg_.Controls.Add(this.enabled_controls_lst_);
			this.interface_pg_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.interface_pg_.Location = new System.Drawing.Point(0, 0);
			this.interface_pg_.Name = "interface_pg_";
			this.interface_pg_.Size = new System.Drawing.Size(392, 239);
			this.interface_pg_.TabIndex = 0;
			this.interface_pg_.Text = "Interface";
			// 
			// remove_controls_btn_
			// 
			this.remove_controls_btn_.Location = new System.Drawing.Point(180, 120);
			this.remove_controls_btn_.Name = "remove_controls_btn_";
			this.remove_controls_btn_.Size = new System.Drawing.Size(32, 23);
			this.remove_controls_btn_.TabIndex = 5;
			this.remove_controls_btn_.Text = "<<";
			this.remove_controls_btn_.Click += new System.EventHandler(this.remove_controls_btn__Click);
			// 
			// add_controls_btn_
			// 
			this.add_controls_btn_.Location = new System.Drawing.Point(180, 88);
			this.add_controls_btn_.Name = "add_controls_btn_";
			this.add_controls_btn_.Size = new System.Drawing.Size(32, 23);
			this.add_controls_btn_.TabIndex = 4;
			this.add_controls_btn_.Text = ">>";
			this.add_controls_btn_.Click += new System.EventHandler(this.add_controls_btn__Click);
			// 
			// label20
			// 
			this.label20.Location = new System.Drawing.Point(232, 16);
			this.label20.Name = "label20";
			this.label20.Size = new System.Drawing.Size(144, 16);
			this.label20.TabIndex = 3;
			this.label20.Text = "Disabled";
			// 
			// label13
			// 
			this.label13.Location = new System.Drawing.Point(16, 16);
			this.label13.Name = "label13";
			this.label13.Size = new System.Drawing.Size(136, 16);
			this.label13.TabIndex = 2;
			this.label13.Text = "Enabled";
			// 
			// disabled_controls_lst_
			// 
			this.disabled_controls_lst_.HorizontalScrollbar = true;
			this.disabled_controls_lst_.Location = new System.Drawing.Point(224, 40);
			this.disabled_controls_lst_.Name = "disabled_controls_lst_";
			this.disabled_controls_lst_.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
			this.disabled_controls_lst_.Size = new System.Drawing.Size(152, 186);
			this.disabled_controls_lst_.Sorted = true;
			this.disabled_controls_lst_.TabIndex = 1;
			// 
			// enabled_controls_lst_
			// 
			this.enabled_controls_lst_.HorizontalScrollbar = true;
			this.enabled_controls_lst_.Location = new System.Drawing.Point(16, 40);
			this.enabled_controls_lst_.Name = "enabled_controls_lst_";
			this.enabled_controls_lst_.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
			this.enabled_controls_lst_.Size = new System.Drawing.Size(152, 186);
			this.enabled_controls_lst_.Sorted = true;
			this.enabled_controls_lst_.TabIndex = 0;
			// 
			// edit_trigger_btn_
			// 
			this.edit_trigger_btn_.Location = new System.Drawing.Point(312, 200);
			this.edit_trigger_btn_.Name = "edit_trigger_btn_";
			this.edit_trigger_btn_.Size = new System.Drawing.Size(64, 20);
			this.edit_trigger_btn_.TabIndex = 22;
			this.edit_trigger_btn_.Text = "Edit";
			// 
			// MainForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(472, 239);
			this.Controls.Add(this.multipanel);
			this.Controls.Add(this.button_pnl_);
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
		private System.Windows.Forms.Button        add_controls_btn_;
		private System.Windows.Forms.Button        remove_controls_btn_;
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
		private System.Windows.Forms.Label         label13;
		private System.Windows.Forms.Label         label14;
		private System.Windows.Forms.Label         label15;
		private System.Windows.Forms.Label         label16;
		private System.Windows.Forms.Label         label17;
		private System.Windows.Forms.Label         label18;
		private System.Windows.Forms.Label         label19;
		private System.Windows.Forms.Label         label2;
		private System.Windows.Forms.Label         label20;
		private System.Windows.Forms.Label         label3;
		private System.Windows.Forms.Label         label4;
		private System.Windows.Forms.Label         label5;
		private System.Windows.Forms.Label         label6;
		private System.Windows.Forms.Label         label7;
		private System.Windows.Forms.Label         label8;
		private System.Windows.Forms.Label         label9;
		private System.Windows.Forms.ListBox       disabled_controls_lst_;
		private System.Windows.Forms.ListBox       enabled_controls_lst_;
		private System.Windows.Forms.NumericUpDown alpha_distance_ud_;
		private System.Windows.Forms.NumericUpDown omega_distance_ud_;
		private System.Windows.Forms.NumericUpDown player_handicap_ud_;
		private System.Windows.Forms.NumericUpDown spiral_energy_ud_;
		private System.Windows.Forms.NumericUpDown spiral_priority_ud_;
		private System.Windows.Forms.NumericUpDown spiral_time_ud_;
		private System.Windows.Forms.Panel         button_pnl_;
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
