using System;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Xml;
using TriggerEdit.Definitions;

namespace TriggerEdit
{
	namespace Definitions
	{
		#region types

		public enum Building
		{
			UNIT_ATTRIBUTE_ANY,
			UNIT_ATTRIBUTE_BOMB_STATION1,
			UNIT_ATTRIBUTE_BOMB_STATION2,
			UNIT_ATTRIBUTE_BOMB_STATION3,
			UNIT_ATTRIBUTE_COLLECTOR,
			UNIT_ATTRIBUTE_COMMANDER,
			UNIT_ATTRIBUTE_CORE,
			UNIT_ATTRIBUTE_CORRIDOR_ALPHA,
			UNIT_ATTRIBUTE_CORRIDOR_OMEGA,
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
			UNIT_ATTRIBUTE_FLY_STATION1,
			UNIT_ATTRIBUTE_FLY_STATION2,
			UNIT_ATTRIBUTE_FRAME,
			UNIT_ATTRIBUTE_GUN_BALLISTIC,
			UNIT_ATTRIBUTE_GUN_FILTH_NAVIGATOR,
			UNIT_ATTRIBUTE_GUN_HOWITZER,
			UNIT_ATTRIBUTE_GUN_SCUM_DISRUPTOR,
			UNIT_ATTRIBUTE_GUN_SUBCHASER,
			UNIT_ATTRIBUTE_HARKBACK_STATION1,
			UNIT_ATTRIBUTE_HARKBACK_STATION2,
			UNIT_ATTRIBUTE_HARKBACK_STATION3,
			UNIT_ATTRIBUTE_LASER_CANNON,
			UNIT_ATTRIBUTE_LASER_STATION1,
			UNIT_ATTRIBUTE_LASER_STATION2,
			UNIT_ATTRIBUTE_LASER_STATION3,
			UNIT_ATTRIBUTE_OFFICER_PLANT,
			UNIT_ATTRIBUTE_ROCKET_LAUNCHER,
			UNIT_ATTRIBUTE_ROCKET_STATION1,
			UNIT_ATTRIBUTE_ROCKET_STATION2,
			UNIT_ATTRIBUTE_ROCKET_STATION3,
			UNIT_ATTRIBUTE_SOLDIER_PLANT,
			UNIT_ATTRIBUTE_SUBTERRA_STATION1,
			UNIT_ATTRIBUTE_SUBTERRA_STATION2,
			UNIT_ATTRIBUTE_TECHNIC_PLANT
		}
		public enum ControlID
		{
			SQSH_GAME_SCREEN_ID,
			SQSH_BACKGRND_ID,
			SQSH_MAP_WINDOW_ID,
			SQSH_WORKAREA2_ID, //Тулзер рвов
			SQSH_WORKAREA3_ID, //Тулзер выравнивания
			SQSH_WORKAREA4_ID, //Тулзер отмены ровняния и рвов
			SQSH_FIELD_ON_ID, //Общее включения поля
			SQSH_FIELD_OFF_ID, //Общее выключение поля
			SQSH_SOLDIER_ID, //Заказ солдата
			SQSH_OFFICER_ID, //Заказ оффицера
			SQSH_TECHNIC_ID, //Заказ техника
			SQSH_TOGETHER_ID, //Объединение сквадов
			SQSH_YADRO_ID, //Ядро
			SQSH_YADRO_EX_ID, //Усилитель
			SQSH_SOLDIER_PLANT_ID, //Завод солдат
			SQSH_OFFICER_PLANT_ID, //Завод офицеров
			SQSH_TECHNIC_PLANT_ID, //Завод техников
			SQSH_COMMANDER_ID, //Командный центр
			SQSH_RELAY_ID, //Транслятор
			SQSH_STATION1_ID, //Ракетная лаборатория
			SQSH_STATION2_ID, //Лазерная лаборатория
			SQSH_STATION3_ID, //Бомбовая лаборатория
			SQSH_STATION4_ID, //Подземная лаборатория
			SQSH_STATION5_ID, //Антигравитационная лаборатория
			SQSH_STATION_ELECTRO_LAB_ID, //АДДОН Электрическая лаборатория
			SQSH_STATION_EXODUS_LAB_ID, //Лаборатория исходников
			SQSH_STATION_EMPIRE_LAB_ID, //Лаборатория империи
			SQSH_STATION_HARKBACK_LAB_ID, //Лаборатория возвратников
			SQSH_CORRIDOR_ALPHA_ID, //Альфа портал
			SQSH_CORRIDOR_OMEGA_ID, //Омега портал 
			SQSH_STATIC_BOMB_ID, //Бомба
			SQSH_GUN_LASER_ID, //Лазерная пушка
			SQSH_GUN_ELECTRO_ID, //Электрическая пушка
			SQSH_GUN_ROCKET_ID, //Ракетная пушка
			SQSH_GUN_HOWITZER_ID, //Гаубица
			SQSH_GUN_FILTH_ID, //Генератор скверны
			SQSH_GUN_GIMLET_ID, //Скаморазрушитель
			SQSH_GUN_SUBCHASER_ID, //Подземпушка
			SQSH_GUN_BALLISTIC_ID, //Баллистическая ракета

			SQSH_FRAME_TERRAIN_BUILD1_ID, //Заказ в первый слот бригадира или прораба
			SQSH_FRAME_TERRAIN_BUILD2_ID, //Заказ во второй слот бригадира или прораба
			SQSH_FRAME_TERRAIN_BUILD3_ID, //Заказ в третий слот бригадира или прораба
			SQSH_FRAME_TERRAIN_BUILD4_ID, //Заказ в четвертый слот бригадира или прораба
			SQSH_FRAME_TERRAIN_BUILD5_ID, //Заказ в пятый слот бригадира или прораба

			SQSH_SQUAD_DISINTEGRATE_ID, //Разобрать на базовые юниты
			SQSH_SQUAD_UNIT1, //rocker
			SQSH_SQUAD_UNIT2, //sniper
			SQSH_SQUAD_UNIT3, //mortar
			SQSH_SQUAD_UNIT4, //digger
			SQSH_SQUAD_UNIT5, //leech
			SQSH_SQUAD_UNIT6, //r-proj
			SQSH_SQUAD_UNIT7, //scumer
			SQSH_SQUAD_UNIT8, //minotaur
			SQSH_SQUAD_UNIT9, //scum splitter
			SQSH_SQUAD_UNIT10, //piercer
			SQSH_SQUAD_UNIT11, //ceptor
			SQSH_SQUAD_UNIT12, //gyroid
			SQSH_SQUAD_UNIT13, //bomber
			SQSH_SQUAD_UNIT14, //strafer
			SQSH_SQUAD_UNIT15, //unseen
			SQSH_SQUAD_UNIT16, //extirpator
			SQSH_SQUAD_UNIT17, //wargon
			SQSH_SQUAD_UNIT18, //disintegrator
			SQSH_SQUAD_UNIT19, //scum thrower
			SQSH_SQUAD_UNIT20, //spider
			SQSH_SQUAD_UNIT21, //fish
			SQSH_SQUAD_UNIT22, //wasp
			SQSH_SQUAD_UNIT23, //worm
			SQSH_SQUAD_UNIT24, //scum twister
			SQSH_SQUAD_UNIT25, //scum heater

			SQSH_SPEED_PAUSE, //Пауза
			SQSH_SPEED_50, //50% скорость игры
			SQSH_SPEED_100, //100% скорость игры
			SQSH_SPEED_150, //150% скорость игры

			SQSH_MENU_BUTTON_ID,
			SQSH_TASK_BUTTON_ID,
			SQSH_REPLAY_PLAYER_BUTTON_ID,
			SQSH_EMBLEM_ID,

			SQSH_TAB_BUILD_ID, //Папки зданий
			SQSH_TAB_SQUAD_ID, //Папки сквадов

			SQSH_HINT_ID,

			SQSH_SELPANEL_MOVE_ID, // ТВ двигаться
			SQSH_SELPANEL_STOP_ID, // ТВ стоп
			SQSH_SELPANEL_STOP2_ID, // ТВ стоп спец. пушек
			SQSH_SELPANEL_SQ_ATTACK_ID, //ТВ атаковать
			SQSH_SELPANEL_SQ_BACK_ID, //ТВ вернуться на базу
			SQSH_SELPANEL_SQ_OFDEF_ID, //ТВ оффенс-дефенс
			SQSH_SELPANEL_SQ_PATROL_ID, //ТВ патрулировать
			SQSH_SELPANEL_BRIG_BACK_ID, //ТВ бриг-прораб во фрейм
			SQSH_SELPANEL_BRIG_CHANGE_ID, //ТВ бриг-прораб замена
			SQSH_SELPANEL_BRIG_BUILD_ID, //ТВ чинить
			SQSH_SELPANEL_START_CHARGE_ID, //ТВ зарядка
			SQSH_SELPANEL_STOP_CHARGE_ID,
			SQSH_SELPANEL_UNIT_CHARGE_ID,

			SQSH_SELPANEL_POWERON_ID, //ТВ включить здание
			SQSH_SELPANEL_POWEROFF_ID, //ТВ выключить здание
			//	SQSH_SELPANEL_ONOFF_ID,
			SQSH_SELPANEL_SELL_ID, //ТВ продать
			SQSH_SELPANEL_UPGRADE_ID,
			SQSH_SELPANEL_UPGRADE_LASER1_ID,  //апгрейд до лазер. 2 уровня
			SQSH_SELPANEL_UPGRADE_LASER2_ID,  //апгрейд до лазер. 3 уровня
			SQSH_SELPANEL_UPGRADE_ELECTRO1_ID,  //АДДОН апгрейд до электр. 2 уровня
			SQSH_SELPANEL_UPGRADE_ELECTRO2_ID,  //АДДОН апгрейд до электр. 3 уровня
			SQSH_SELPANEL_UPGRADE_BOMB1_ID,   //апгрейд до бомб. 2 уровня
			SQSH_SELPANEL_UPGRADE_BOMB2_ID,   //апгрейд до бомб. 3 уровня
			SQSH_SELPANEL_UPGRADE_ROCKET1_ID, //апгрейд до ракет. 2 уровня
			SQSH_SELPANEL_UPGRADE_ROCKET2_ID, //апгрейд до ракет. 3 уровня

			SQSH_SELPANEL_UPGRADE_EXODUS1_ID, //апгрейд до исход. 2 уровня
			SQSH_SELPANEL_UPGRADE_EXODUS2_ID, //апгрейд до исход. 3 уровня
			SQSH_SELPANEL_UPGRADE_EMPIRE1_ID, //апгрейд до импер. 2 уровня
			SQSH_SELPANEL_UPGRADE_EMPIRE2_ID,  //апгрейд до импер. 3 уровня
			SQSH_SELPANEL_UPGRADE_HARKBACK1_ID,  //апгрейд до возврат. 2 уровня
			SQSH_SELPANEL_UPGRADE_HARKBACK2_ID,  //апгрейд до возврат. 3 уровня

			SQSH_SELPANEL_UPGRADE_FLY_ID,  //апгрейд до летной 2 уровня
			SQSH_SELPANEL_UPGRADE_SUBTERRA_ID,  //апгрейд до подзем. 2 уровня
			SQSH_SELPANEL_UPGRADE_OMEGA_ID,  //апгрейд омеги
			SQSH_SELPANEL_FIELDON_ID, //ТВ поле
			SQSH_SELPANEL_FIELDOFF_ID,
			//	SQSH_SELPANEL_FIELD_ID,

			SQSH_SELPANEL_FRAME_INSTALL_ID, //ТВ инсталлировать-деинсталлировать фрейм
			SQSH_SELPANEL_FRAME_ALARM_ID, //ТВ всем во фрейм
			SQSH_SELPANEL_FRAME_TELEPORTATE_ID, //ТВ кнопка телепортатации фрейма

			SQSH_PROGRESS_ENERGY,
			SQSH_PROGRESS_COLLECTED, //Прогресс энергии
			SQSH_RAMKA_ID,
			SQSH_INFOWND_ID,

			SQSH_BAR_SQUAD1_ID, //Прогресс мутации сквада 1
			SQSH_BAR_SQUAD2_ID, //Прогресс мутации сквада 2
			SQSH_BAR_SQUAD3_ID, //Прогресс мутации сквада 3
			SQSH_BAR_SQUAD4_ID, //Прогресс мутации сквада 4
			SQSH_BAR_SQUAD5_ID, //Прогресс мутации сквада 5

			SQSH_CHAT_INFO_ID,
			SQSH_INGAME_CHAT_EDIT_ID,

			SQSH_GAME_MAX,

			//главное меню
			SQSH_MM_START_SCR,
			SQSH_MM_SINGLE_SCR,
			SQSH_MM_PROFILE_SCR,
			SQSH_MM_SCENARIO_SCR,
			SQSH_MM_BRIEFING_SCR,
			SQSH_MM_LOADING_MISSION_SCR,
			SQSH_MM_ENDMISSION_SCR,
			SQSH_MM_STATS_SCR,
			SQSH_MM_INMISSION_SCR,
			SQSH_MM_BATTLE_SCR,
			SQSH_MM_MISSION_TASK_SCR,
			SQSH_MM_LAN_SCR,
			SQSH_MM_CREATE_GAME_SCR,
			SQSH_MM_LOBBY_SCR,
			SQSH_MM_OPTIONS_SCR,
			SQSH_MM_GRAPHICS_SCR,
			SQSH_MM_CUSTOM_SCR,
			SQSH_MM_INGAME_CUSTOM_SCR,
			SQSH_MM_GAME_SCR,
			SQSH_MM_SOUND_SCR,
			SQSH_MM_ONLINE_SCR,
			SQSH_MM_CREATE_ONLINE_GAME_SCR,
			SQSH_MM_ONLINE_LOBBY_SCR,
			SQSH_MM_LOAD_SCR,
			SQSH_MM_LOAD_IN_GAME_SCR,
			SQSH_MM_SAVE_GAME_SCR,
			SQSH_MM_SAVE_REPLAY_SCR,
			SQSH_MM_SUBMIT_DIALOG_SCR,
			SQSH_MM_LOAD_REPLAY_SCR,
			SQSH_MM_CREDITS_SCR,
			SQSH_MM_NAME_INPUT_SCR,

			SQSH_MM_SCREEN_OPTIONS,
			SQSH_MM_SCREEN_GAME,
			SQSH_MM_SCREEN_GRAPHICS,
			SQSH_MM_SCREEN_SOUND,

			//start menu
			SQSH_MM_SINGLE_BTN,
			SQSH_MM_LAN_BTN,
			SQSH_MM_ONLINE_BTN,
			SQSH_MM_OPTIONS_BTN,
			SQSH_MM_CREDITS_BTN,
			SQSH_MM_QUIT_BTN,
			SQSH_MM_VERSION_TXT,
	
			//single player menu
			SQSH_MM_PROFILE_BTN,
			SQSH_MM_SCENARIO_BTN,
			SQSH_MM_LOAD_BTN,
			SQSH_MM_BATTLE_BTN,
			SQSH_MM_REPLAY_LINE,
			SQSH_MM_REPLAY_BORDER,
			SQSH_MM_REPLAY_BTN,
			SQSH_MM_BACK_FROM_SINGLE_BTN,

			//profile editor
			SQSH_MM_PROFILE_LIST,
			SQSH_MM_PROFILE_NAME_INPUT,
			SQSH_MM_NEW_PROFILE_BTN,
			SQSH_MM_DEL_PROFILE_BTN,
			SQSH_MM_SELECT_PROFILE_BTN,
			SQSH_MM_BACK_FROM_PROFILE_BTN,

			//scenario
			SQSH_MM_MISSION_LIST,
			SQSH_MM_DIFFICULTY_COMBO,
			SQSH_MM_DIFFICULTY_BTN,
			SQSH_MM_GO_BTN,
			SQSH_MM_BACK_FROM_SCENARIO_BTN,

			//briefing
			SQSH_MM_BRIEFING_YEAR_TXT,
			SQSH_MM_NOMAD_TXT,
			SQSH_MM_NOMAD_ICON,
			SQSH_MM_BRIEFING_TXT,
			SQSH_MM_BRIEFING_ICON,
			SQSH_MM_QUIT_FROM_BRIEFING_BTN,
			SQSH_MM_CONTINUE_BRIEFING_BORDER,
			SQSH_MM_CONTINUE_BRIEFING_BTN,
			SQSH_MM_START_BRIEFING_BORDER,
			SQSH_MM_START_MISSION_BTN,
			SQSH_MM_SKIP_BRIEFING_BORDER,
			SQSH_MM_SKIP_BRIEFING_BTN,
			SQSH_MM_SKIP_MISSION_BORDER,
			SQSH_MM_SKIP_MISSION_BTN,

			//loading mission
			SQSH_MM_LOADING_NOMAD_TXT,
			SQSH_MM_LOADING_NOMAD_ICON,
			SQSH_MM_MAPWINDOW,
			SQSH_MM_MISSION_DESCR_TXT,

			//end mission
			SQSH_MM_RESULT_TXT,
			SQSH_MM_RESUME_BORDER,
			SQSH_MM_RESUME_BTN,
			SQSH_MM_CONTINUE_BTN,
			SQSH_RESULT_WND,

			//stats
			SQSH_MM_STATS_TOTAL_HEAD_LIST,
			SQSH_MM_STATS_TOTAL_LIST,
			SQSH_MM_STATS_GENERAL_HEAD_LIST,
			SQSH_MM_STATS_GENERAL_LIST,
			SQSH_MM_STATS_UNITS_HEAD_LIST,
			SQSH_MM_STATS_UNITS_LIST,
			SQSH_MM_STATS_BUILDINGS_HEAD_LIST,
			SQSH_MM_STATS_BUILDINGS_LIST,
			SQSH_MM_RESTART_BTN,
			SQSH_MM_SAVE_REPLAY_BORDER,
			SQSH_MM_SAVE_REPLAY_BTN,
			SQSH_MM_QUIT_FROM_STATS_BTN,
			SQSH_MM_CONTINUE_FROM_STATS_BTN,
			SQSH_MM_CONTINUE_FROM_STATS_BORDER,

			SQSH_MM_STATS_TOTAL_BTN,
			SQSH_MM_STATS_GENERAL_BTN,
			SQSH_MM_STATS_UNITS_BTN,
			SQSH_MM_STATS_BUILDINGS_BTN,
			SQSH_MM_STATS_TOTAL_RAMKA,
			SQSH_MM_STATS_GENERAL_RAMKA,
			SQSH_MM_STATS_UNITS_RAMKA,
			SQSH_MM_STATS_BUILDINGS_RAMKA,


			//inmission
			SQSH_MM_INMISSION_SAVE_BTN,
			SQSH_MM_INMISSION_LOAD_BTN,
			SQSH_MM_INMISSION_OPTIONS_BTN,
			SQSH_MM_INMISSION_RESTART_BTN,
			SQSH_MM_INMISSION_RESUME_BTN,
			SQSH_MM_INMISSION_QUIT_BTN,

			//battle
			SQSH_MM_MAP_LIST,
			SQSH_MM_BATTLE_PLAYER1_FRM_BTN,
			SQSH_MM_BATTLE_PLAYER2_FRM_BTN,
			SQSH_MM_BATTLE_PLAYER3_FRM_BTN,
			SQSH_MM_BATTLE_PLAYER4_FRM_BTN,
			SQSH_MM_BATTLE_PLAYER1_SLOT_BTN,
			SQSH_MM_BATTLE_PLAYER2_SLOT_BTN,
			SQSH_MM_BATTLE_PLAYER3_SLOT_BTN,
			SQSH_MM_BATTLE_PLAYER4_SLOT_BTN,
			SQSH_MM_BATTLE_PLAYER1_CLR_BG,
			SQSH_MM_BATTLE_PLAYER2_CLR_BG,
			SQSH_MM_BATTLE_PLAYER3_CLR_BG,
			SQSH_MM_BATTLE_PLAYER4_CLR_BG,
			SQSH_MM_BATTLE_PLAYER1_CLR_BTN,
			SQSH_MM_BATTLE_PLAYER2_CLR_BTN,
			SQSH_MM_BATTLE_PLAYER3_CLR_BTN,
			SQSH_MM_BATTLE_PLAYER4_CLR_BTN,
			SQSH_MM_BATTLE_PLAYER1_CLAN_BTN,
			SQSH_MM_BATTLE_PLAYER2_CLAN_BTN,
			SQSH_MM_BATTLE_PLAYER3_CLAN_BTN,
			SQSH_MM_BATTLE_PLAYER4_CLAN_BTN,
			SQSH_MM_BATTLE_PLAYER1_HC_BTN,
			SQSH_MM_BATTLE_PLAYER2_HC_BTN,
			SQSH_MM_BATTLE_PLAYER3_HC_BTN,
			SQSH_MM_BATTLE_PLAYER4_HC_BTN,
			SQSH_MM_BATTLE_GO_BTN,
			SQSH_MM_BACK_FROM_BATTLE_BTN,
			SQSH_MM_BATTLE_MAP,
			SQSH_MM_BATTLE_MAP_DESCR_TXT,
			SQSH_MM_BATTLE_SURVIVAL_TXT,

			//load game
			SQSH_MM_LOAD_MAP_LIST,
			SQSH_MM_LOAD_GO_BTN,
			SQSH_MM_BACK_FROM_LOAD_BTN,
			SQSH_MM_LOAD_MAP,
			SQSH_MM_LOAD_MAP_DESCR_TXT,
			SQSH_MM_DEL_SAVE_BTN,

			//load replay
			SQSH_MM_LOAD_REPLAY_LIST,
			SQSH_MM_LOAD_REPLAY_GO_BTN,
			SQSH_MM_BACK_FROM_LOAD_REPLAY_BTN,
			SQSH_MM_LOAD_REPLAY_MAP,
			SQSH_MM_LOAD_REPLAY_DESCR_TXT,
			SQSH_MM_DEL_REPLAY_BTN,

			//load in game
			SQSH_MM_LOAD_IN_GAME_MAP_LIST,
			SQSH_MM_LOAD_IN_GAME_GO_BTN,
			SQSH_MM_BACK_FROM_LOAD_IN_GAME_BTN,
			SQSH_MM_LOAD_IN_GAME_MAP,
			SQSH_MM_LOAD_IN_GAME_MAP_DESCR_TXT,
			SQSH_MM_LOAD_IN_GAME_DEL_BTN,

			//save game
			SQSH_MM_SAVE_GAME_MAP_LIST,
			SQSH_MM_SAVE_GAME_GO_BTN,
			SQSH_MM_BACK_FROM_SAVE_GAME_BTN,
			SQSH_MM_SAVE_GAME_MAP,
			SQSH_MM_SAVE_GAME_MAP_DESCR_TXT,
			SQSH_MM_SAVE_NAME_INPUT,
			SQSH_MM_SAVE_GAME_DEL_BTN,

			//save replay
			SQSH_MM_SAVE_REPLAY_LIST,
			SQSH_MM_SAVE_REPLAY_GO_BTN,
			SQSH_MM_BACK_FROM_SAVE_REPLAY_BTN,
			SQSH_MM_SAVE_REPLAY_MAP,
			SQSH_MM_SAVE_REPLAY_DESCR_TXT,
			SQSH_MM_REPLAY_NAME_INPUT,
			SQSH_MM_SAVE_REPLAY_DEL_BTN,

			//task
			SQSH_MM_MISSION_TASK_TXT,
			SQSH_MM_BACK_FROM_TASK_BTN,

			//name input
			SQSH_MM_PLAYER_NAME_INPUT,
			SQSH_MM_APPLY_NAME_BTN,
			SQSH_MM_BACK_FROM_NAME_INPUT_BTN,
			SQSH_MM_CONNECTION_TYPE_COMBO,
			SQSH_MM_IP_INPUT,
			SQSH_MM_IP_BTN,

			//lan
			SQSH_MM_GAME_LIST,
			SQSH_MM_LAN_GAME_MAP,
			SQSH_MM_LAN_GAME_MAP_DESCR_TXT,
			SQSH_MM_LAN_CREATE_GAME_BTN,
			SQSH_MM_JOIN_BTN,
			SQSH_MM_BACK_FROM_LAN_BTN,
			SQSH_MM_LAN_PLAYER_NAME_INPUT,
			SQSH_MM_LAN_GAMESPY_LOGO,

			//create game
			SQSH_MM_LAN_MAP_LIST,
			SQSH_MM_CREATE_GAME_MAP,
			SQSH_MM_CREATE_GAME_MAP_DESCR_TXT,
			SQSH_MM_LAN_GAME_SPEED_BTN,
			SQSH_MM_LAN_GAME_SPEED_SLIDER,
			SQSH_MM_CREATE_BTN,
			SQSH_MM_BACK_FROM_CREATE_GAME_BTN,
			SQSH_MM_LAN_CREATE_GAMESPY_LOGO,

			//lobby
			SQSH_MM_LOBBY_GAME_NAME_BTN,
			SQSH_MM_LOBBY_PLAYER1_NAME_BTN,
			SQSH_MM_LOBBY_PLAYER2_NAME_BTN,
			SQSH_MM_LOBBY_PLAYER3_NAME_BTN,
			SQSH_MM_LOBBY_PLAYER4_NAME_BTN,
			SQSH_MM_LOBBY_PLAYER1_FRM_BTN,
			SQSH_MM_LOBBY_PLAYER2_FRM_BTN,
			SQSH_MM_LOBBY_PLAYER3_FRM_BTN,
			SQSH_MM_LOBBY_PLAYER4_FRM_BTN,
			SQSH_MM_LOBBY_PLAYER1_SLOT_BTN,
			SQSH_MM_LOBBY_PLAYER2_SLOT_BTN,
			SQSH_MM_LOBBY_PLAYER3_SLOT_BTN,
			SQSH_MM_LOBBY_PLAYER4_SLOT_BTN,
			SQSH_MM_LOBBY_PLAYER1_CLR_BG,
			SQSH_MM_LOBBY_PLAYER2_CLR_BG,
			SQSH_MM_LOBBY_PLAYER3_CLR_BG,
			SQSH_MM_LOBBY_PLAYER4_CLR_BG,
			SQSH_MM_LOBBY_PLAYER1_CLR_BTN,
			SQSH_MM_LOBBY_PLAYER2_CLR_BTN,
			SQSH_MM_LOBBY_PLAYER3_CLR_BTN,
			SQSH_MM_LOBBY_PLAYER4_CLR_BTN,
			SQSH_MM_LOBBY_PLAYER1_CLAN_BTN,
			SQSH_MM_LOBBY_PLAYER2_CLAN_BTN,
			SQSH_MM_LOBBY_PLAYER3_CLAN_BTN,
			SQSH_MM_LOBBY_PLAYER4_CLAN_BTN,
			SQSH_MM_LOBBY_PLAYER1_HC_BTN,
			SQSH_MM_LOBBY_PLAYER2_HC_BTN,
			SQSH_MM_LOBBY_PLAYER3_HC_BTN,
			SQSH_MM_LOBBY_PLAYER4_HC_BTN,
			SQSH_MM_LOBBY_PLAYER1_READY_BTN,
			SQSH_MM_LOBBY_PLAYER2_READY_BTN,
			SQSH_MM_LOBBY_PLAYER3_READY_BTN,
			SQSH_MM_LOBBY_PLAYER4_READY_BTN,
			SQSH_MM_LOBBY_GAME_MAP,
			SQSH_MM_LOBBY_GAME_MAP_DESCR_TXT,
			SQSH_MM_LOBBY_START_BORDER,
			SQSH_MM_LOBBY_START_BTN,
			SQSH_MM_BACK_FROM_LOBBY_BTN,
			SQSH_MM_LAN_LOBBY_GAMESPY_LOGO,

			SQSH_MM_LOBBY_CHAT_TEXT,
			SQSH_MM_LOBBY_CHAT_INPUT,

			SQSH_MM_LOBBY_MAP_LIST,
			SQSH_MM_LOBBY_HOST_GAME_MAP,
			SQSH_MM_LOBBY_MAP_LIST_RAMKA1,
			SQSH_MM_LOBBY_MAP_LIST_RAMKA2,
			SQSH_MM_LOBBY_MAP_LIST_RAMKA3,
			SQSH_MM_LOBBY_MAP_LIST_RAMKA4,

			//online
			SQSH_MM_ONLINE_GAME_LIST,
			SQSH_MM_ONLINE_GAME_MAP,
			SQSH_MM_ONLINE_GAME_MAP_DESCR_TXT,
			SQSH_MM_ONLINE_CREATE_GAME_BTN,
			SQSH_MM_ONLINE_JOIN_BTN,
			SQSH_MM_BACK_FROM_ONLINE_BTN,
			SQSH_MM_ONLINE_PLAYER_NAME_INPUT,

			//create online game
			SQSH_MM_ONLINE_MAP_LIST,
			SQSH_MM_CREATE_ONLINE_GAME_MAP,
			SQSH_MM_CREATE_ONLINE_GAME_MAP_DESCR_TXT,
			SQSH_MM_ONLINE_GAME_SPEED_BTN,
			SQSH_MM_ONLINE_GAME_SPEED_SLIDER,
			SQSH_MM_ONLINE_CREATE_BTN,
			SQSH_MM_BACK_FROM_CREATE_ONLINE_GAME_BTN,

			//online lobby
			SQSH_MM_ONLINE_LOBBY_GAME_NAME_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER1_NAME_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER2_NAME_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER3_NAME_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER4_NAME_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER1_FRM_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER2_FRM_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER3_FRM_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER4_FRM_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER1_SLOT_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER2_SLOT_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER3_SLOT_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER4_SLOT_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER1_CLR_BG,
			SQSH_MM_ONLINE_LOBBY_PLAYER2_CLR_BG,
			SQSH_MM_ONLINE_LOBBY_PLAYER3_CLR_BG,
			SQSH_MM_ONLINE_LOBBY_PLAYER4_CLR_BG,
			SQSH_MM_ONLINE_LOBBY_PLAYER1_CLR_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER2_CLR_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER3_CLR_BTN,
			SQSH_MM_ONLINE_LOBBY_PLAYER4_CLR_BTN,
			SQSH_MM_ONLINE_LOBBY_GAME_MAP,
			SQSH_MM_ONLINE_LOBBY_GAME_MAP_DESCR_TXT,
			SQSH_MM_ONLINE_LOBBY_START_BTN,
			SQSH_MM_BACK_FROM_ONLINE_LOBBY_BTN,

			//options
			SQSH_MM_OPTIONS_GAME_BTN,
			SQSH_MM_OPTIONS_GRAPHICS_BTN,
			SQSH_MM_OPTIONS_SOUND_BTN,
			SQSH_MM_BACK_FROM_OPTIONS_BTN,

			//custom
			SQSH_MM_GRAPHICS_LOD,
			SQSH_MM_GRAPHICS_LOD_COMBO,
			SQSH_MM_PARTICLE_RATE,
			SQSH_MM_PARTICLE_RATE_SLIDER,
			SQSH_MM_GRAPHICS_OCCLUSION,
			SQSH_MM_GRAPHICS_OCCLUSION_COMBO,
			SQSH_MM_GRAPHICS_POINT_LIGHT,
			SQSH_MM_GRAPHICS_POINT_LIGHT_COMBO,
			SQSH_MM_GRAPHICS_SHADOWS_SAMPLES,
			SQSH_MM_GRAPHICS_SHADOWS_SAMPLES_COMBO,
			SQSH_MM_GRAPHICS_BUMP,
			SQSH_MM_GRAPHICS_BUMP_COMBO,
			SQSH_MM_GRAPHICS_BUMP_CHAOS,
			SQSH_MM_GRAPHICS_BUMP_CHAOS_COMBO,
			SQSH_MM_GRAPHICS_COMPRESS,
			SQSH_MM_GRAPHICS_COMPRESS_COMBO,
			SQSH_MM_BACK_FROM_CUSTOM_BTN,

			//game
			SQSH_MM_BACK_FROM_GAME_BTN,

			//graphics
			SQSH_MM_SETTINGS,
			SQSH_MM_SETTINGS_COMBO,
			SQSH_MM_CUSTOM_GRAPHICS_BTN,
			SQSH_MM_APPLY_BTN,
			SQSH_MM_BACK_FROM_GRAPHICS_BTN,

			//sound
			SQSH_MM_BACK_FROM_SOUND_BTN,
	
			//submit dialog
			SQSH_MM_SUBMIT_TXT,
			SQSH_MM_SUBMIT_YES_BTN,
			SQSH_MM_SUBMIT_YES_BORDER,
			SQSH_MM_SUBMIT_NO_BTN,
			SQSH_MM_SUBMIT_NO_BORDER,
			SQSH_MM_SUBMIT_OK_BTN,
			SQSH_MM_SUBMIT_OK_BORDER,

			//ingame graphics
			SQSH_MM_CUSTOM_INGAME_GRAPHICS_BTN,
			SQSH_MM_INGAME_APPLY_BTN,
			SQSH_MM_BACK_FROM_INGAME_GRAPHICS_BTN,

			//credits
			SQSH_MM_CREDITS_TXT,
			SQSH_MM_BACK_CREDITS_BTN,

			SQSH_MM_BACK,
			SQSH_MM_OPTIONS,
			SQSH_MM_OPTIONS_GAME,
			SQSH_MM_OPTIONS_GRAPHICS,
			SQSH_MM_OPTIONS_SOUND,
			SQSH_MM_RAMKA,

			SQSH_MM_BACK_FROM_INGAME_OPTIONS,
			SQSH_MM_BACK_FROM_INGAME_GAME_OPTIONS,
			SQSH_MM_BACK_FROM_INGAME_CUSTOM,
			SQSH_MM_BACK_FROM_INGAME_SOUND,

			SQSH_MM_GAME_ANGLESENS,
			SQSH_MM_GAME_ANGLESENS_SLIDER,
			SQSH_MM_GAME_SCROLLRATE,
			SQSH_MM_GAME_SCROLLRATE_SLIDER,
			SQSH_MM_GAME_MOUSESPEED,
			SQSH_MM_GAME_MOUSE_SLIDER,
			SQSH_MM_GAME_TOOLTIPS,
			SQSH_MM_GAME_TOOLTIPS_COMBO,

			SQSH_MM_GRAPHICS_RESOLUTION,
			SQSH_MM_GRAPHICS_RESOLUTION_COMBO,
			SQSH_MM_GRAPHICS_COLORDEPTH,
			SQSH_MM_GRAPHICS_COLORDEPTH_COMBO,
			SQSH_MM_GRAPHICS_MODE,
			SQSH_MM_GRAPHICS_MODE_COMBO,
			SQSH_MM_GRAPHICS_GAMMA,
			SQSH_MM_GRAPHICS_GAMMA_SLIDER,
			//	SQSH_MM_GRAPHICS_LOD,
			SQSH_MM_GRAPHICS_LOD_SLIDER,
			SQSH_MM_GRAPHICS_SHADOWS,
			SQSH_MM_GRAPHICS_SHADOWS_COMBO,
			SQSH_MM_GRAPHICS_FURROWS,
			SQSH_MM_GRAPHICS_FURROWS_COMBO,
			SQSH_MM_GRAPHICS_REFLECTION,
			SQSH_MM_GRAPHICS_REFLECTION_COMBO,

			SQSH_MM_SOUND_SOUNDEFFECTS,
			SQSH_MM_SOUND_SOUNDEFFECTS_COMBO,
			SQSH_MM_SOUND_SOUNDVOLUME,
			SQSH_MM_SOUND_SOUNDVOLUME_SLIDER,
			SQSH_MM_SOUND_MUSIC,
			SQSH_MM_SOUND_MUSIC_COMBO,
			SQSH_MM_SOUND_MUSICVOLUME,
			SQSH_MM_SOUND_MUSICVOLUME_SLIDER,

			SQSH_EMPTY_WND,

			SQSH_MM_SPLASH1,
			SQSH_MM_SPLASH2,
			SQSH_MM_SPLASH3,
			SQSH_MM_SPLASH4,
			SQSH_MM_SPLASH_LAST
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
			UNIT_ATTRIBUTE_ELECTRO_CANNON,
			UNIT_ATTRIBUTE_GUN_BALLISTIC,
			UNIT_ATTRIBUTE_GUN_FILTH_NAVIGATOR,
			UNIT_ATTRIBUTE_GUN_HOWITZER,
			UNIT_ATTRIBUTE_GUN_SCUM_DISRUPTOR,
			UNIT_ATTRIBUTE_GUN_SUBCHASER,
			UNIT_ATTRIBUTE_LASER_CANNON,
			UNIT_ATTRIBUTE_ROCKET_LAUNCHER
		}

		#endregion

		#region conditions

		public class Precondition : ICloneable
		{
			#region interface

			public enum Type
			{
				NORMAL,
				INVERTED
			}

			public Precondition(Type type, Condition condition)
			{
				type_      = type;
				condition_ = condition;
			}

			public void Serialize(ScriptXmlWriter w)
			{
				condition_.Serialize(w);
			}

			#endregion
			
			#region ICloneable Members

			public object Clone()
			{
				return new Precondition(type_, (Condition)condition_.Clone());
			}

			#endregion

			#region data

			public Type      type_;
			public Condition condition_;

			#endregion
		}

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
				string code   = code_node.InnerText.Trim();
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
								Enum.Parse(property.PropertyType, property_node.InnerText.Trim()),
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
										value_node.InnerText.Trim())] = true;
							}
							else
							{
								bit_enum[Enum.Parse(
									bit_enum.GetEnumType(),
									property_node.InnerText.Trim())] = true;
							}
						}
						else
							property.SetValue(
								condition,
								Convert.ChangeType(property_node.InnerText.Trim(), property.PropertyType),
								null);
					}
					// set preconditions
					if ("ConditionSwitcher" == code)
					{
						XmlNodeList precondition_nodes = node.SelectNodes(
							"*[@name=\"conditions\"]/set");
						condition.preconditions_ = new ArrayList(precondition_nodes.Count);
						if (precondition_nodes.Count > 0)
						{
							foreach (XmlNode precondition_node in precondition_nodes)
							{
								// extract condition
								Condition condition2 = Condition.CreateInstance(
									precondition_node.SelectSingleNode(
										"set[@name=\"condition\"]"));
								if (null == condition2)
									continue;
								// extract precondition type
								XmlNode type_node = precondition_node.SelectSingleNode("value[@name=\"type\"]");
								Precondition.Type type = Precondition.Type.NORMAL;
								if (null != type_node)
									type = (Precondition.Type)Enum.Parse(typeof(Precondition.Type), type_node.InnerText.Trim());
								// add the precondition
								condition.preconditions_.Add(new Precondition(type, condition2));
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

			public void Serialize(ScriptXmlWriter w)
			{
				Type type = GetType();
				// begin condition
				w.WriteStartNamedElement("set", "condition");
				w.WriteAttributeString("code", "struct Condition" + Name);
				w.WriteElement("value", "state_", state_ ? "true" : "false");
				w.WriteElement("int", "internalColor_", "0");
				// properties
				PropertyInfo[] properties = type.GetProperties();
				foreach (PropertyInfo property in properties)
				{
					// skip the "state_" property
					if (property.Name == "state_")
						continue;
					// skip non-browsable
					object[] browsable = property.GetCustomAttributes(
						typeof(BrowsableAttribute),
						false);
					if (browsable.Length != 0 && !((BrowsableAttribute)browsable[0]).Browsable)
						continue;
					// serialize the property
					if (property.PropertyType == typeof(int))
						w.WriteElement(
							"int",
							property.Name,
							property.GetValue(this, null).ToString());
					else if (property.PropertyType == typeof(float))
						w.WriteElement(
							"float",
							property.Name,
							property.GetValue(this, null).ToString());
					else if (property.PropertyType == typeof(string))
						w.WriteElement(
							"string",
							property.Name,
							(string)property.GetValue(this, null));
					else if (property.PropertyType == typeof(bool))
						w.WriteElement(
							"value",
							property.Name,
							(bool)property.GetValue(this, null) ? "true" : "false");
					else if (property.PropertyType.IsEnum)
						w.WriteElement(
							"value",
							property.Name,
							Enum.GetName(property.PropertyType, property.GetValue(this, null)));
					else if (property.PropertyType == typeof(BitEnum))
					{
						w.WriteStartNamedElement("disjunction", property.Name);
						ArrayList fields = ((BitEnum)property.GetValue(this, null)).GetList();
						foreach (object field in fields)
							w.WriteElementString("value", field.ToString	());
						w.WriteEndElement();
					}
					else
						throw new Exception("unknown condition property type");
				}
				// preconditions
				if (preconditions_ != null)
				{
					w.WriteStartNamedElement("array", "conditions");
					foreach (Precondition precondition in preconditions_)
					{
						w.WriteStartElement("set");
						w.WriteElement("value", "type", precondition.type_.ToString());
						precondition.Serialize(w);
						w.WriteEndElement();
					}
					w.WriteEndElement();
				}
				// end condition
				w.WriteEndElement();
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
				if (null != preconditions_)
				{
					condition.preconditions_ = new ArrayList(preconditions_.Count);
					foreach (Precondition precondition in preconditions_)
						condition.preconditions_.Add(precondition.Clone());
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
			public ArrayList preconditions_;
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
				string code   = code_node.InnerText.Trim();
				string prefix = "struct ";
				if (code.StartsWith(prefix))
					code = code.Substring(prefix.Length);
				// get type information
				try
				{
					// initialize main object
					Type action_type = Type.GetType("TriggerEdit.Definitions." + code);
					action = (Action)Activator.CreateInstance(action_type);
					Debug.Assert(action.GetType() != typeof(ActionSetCameraAtObject));
					// set properties
					PropertyInfo[] properties = action_type.GetProperties();
					foreach (PropertyInfo property in properties)
					{
						XmlNode property_node = node.SelectSingleNode(
							"*[@name=\"" + property.Name + "\"]");
						if (null == property_node)
								continue;
						if (
							property.Name == "attackTimer"   ||
							property.Name == "delayTimer"    ||
							property.Name == "durationTimer" ||
							property.Name == "timer")
						{
							property_node = node.SelectSingleNode(
								"*[@name=\"" + property.Name + "\"]/int[@name=\"time\"]");
							if (null == property_node)
								continue;
							property.SetValue(
								action,
								int.Parse(property_node.InnerText.Trim()),
								null);
						} 
						else if (property.PropertyType == typeof(TriggerEdit.Definitions.ControlCollection))
						{
							// get the control nodes
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
											control,
											Enum.Parse(
												control_property.PropertyType,
												control_property_node.InnerText.Trim()),
											null);
									else
										control_property.SetValue(
											control,
											Convert.ChangeType(
												control_property_node.InnerText.Trim(),
												control_property.PropertyType),
											null);
								}
								controls.Add(control);
							}
							property.SetValue(
								action,
								controls,
								null);
						}
						else if (property.PropertyType == typeof(BitEnum))
							property.SetValue(
								action,
								BitEnum.Parse(
									((BitEnum)property.GetValue(action, null)).GetEnumType(),
									property_node.InnerText.Trim()),
								null);
						else if (property.PropertyType.IsEnum)
							property.SetValue(
								action,
								Enum.Parse(
									property.PropertyType,
									property_node.InnerText.Trim()),
								null);
						else
							property.SetValue(
								action,
								Convert.ChangeType(
									property_node.InnerText.Trim(),
									property.PropertyType),
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

			public void Serialize(ScriptXmlWriter w)
			{
				Type type = GetType();
				// begin condition
				w.WriteStartNamedElement("set", "action");
				w.WriteAttributeString("code", "struct Action" + Name);
				w.WriteElement("int", "internalColor_", "0");
				// properties
				PropertyInfo[] properties = type.GetProperties();
				foreach (PropertyInfo property in properties)
				{
					// skip the "state_" property
					if (property.Name == "state_")
						continue;
					// skip non-browsable
					object[] browsable = property.GetCustomAttributes(
						typeof(BrowsableAttribute),
						false);
					if (browsable.Length != 0 && !((BrowsableAttribute)browsable[0]).Browsable)
						continue;
					// serialize the property
					if (property.PropertyType == typeof(int))
						w.WriteElement(
							"int",
							property.Name,
							property.GetValue(this, null).ToString());
					else if (property.PropertyType == typeof(float))
						w.WriteElement(
							"float",
							property.Name,
							property.GetValue(this, null).ToString());
					else if (property.PropertyType == typeof(string))
						w.WriteElement(
							"string",
							property.Name,
							(string)property.GetValue(this, null));
					else if (property.PropertyType == typeof(bool))
						w.WriteElement(
							"value",
							property.Name,
							(bool)property.GetValue(this, null) ? "true" : "false");
					else if (property.PropertyType.IsEnum)
						w.WriteElement(
							"value",
							property.Name,
							Enum.GetName(property.PropertyType, property.GetValue(this, null)));
					else if (property.PropertyType == typeof(BitEnum))
					{
						w.WriteStartNamedElement("disjunction", property.Name);
						ArrayList fields = ((BitEnum)property.GetValue(this, null)).GetList();
						foreach (object field in fields)
							w.WriteElementString("value", field.ToString());
						w.WriteEndElement();
					}
					else
						throw new Exception("unknown condition property type");
				}
				// end condition
				w.WriteEndElement();
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
			private int cycles_;
			public  int cycles
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