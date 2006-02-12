using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using TriggerEdit.Definitions;

namespace TriggerEdit
{
	public class ConditionBuilder : System.Windows.Forms.Form
	{
		#region interface

		public ConditionBuilder()
		{
			InitializeComponent();
			InitializeConditionList();
			display_pnl_.SelectCondition += new ConditionDisplay.SelectConditionEventHandler(display_pnl__SelectCondition);
			display_pnl_.NewCondition    += new ConditionDisplay.NewConditionEventHandler   (display_pnl__NewCondition);
		}

		public Condition Condition
		{
			get
			{
				return (Condition)condition_.Clone();
			}
			set
			{
				if (null == value)
					condition_ = new ConditionSwitcher();
				else
					condition_ = (Condition)value.Clone();
				selection_                    = condition_;
				new_condition_                = false;
				display_pnl_.Condition        = condition_;
				property_grid_.SelectedObject = condition_;
			}
		}

		#endregion

		#region event handlers

		private void display_pnl__NewCondition(object sender, ConditionDisplay.ConditionEventArgs e)
		{
			selection_parent_             = e.parent_;
			new_condition_                = true;
			condition_lst_.SelectedIndex  = -1;
			property_grid_.SelectedObject = null;
		}
		private void display_pnl__SelectCondition(object sender, ConditionDisplay.ConditionEventArgs e)
		{
			selection_                    = e.condition_;
			selection_parent_             = e.parent_;
			new_condition_                = false;
			condition_lst_.SelectedIndex  = condition_lst_.FindString(e.condition_.Name);
			property_grid_.SelectedObject = e.condition_;
		}
		private void condition_lst__SelectedValueChanged(object sender, System.EventArgs e)
		{
			if (null == selection_)
				return;
			if (null == condition_lst_.SelectedItem)
				return;
			string selection_name = condition_lst_.SelectedItem.ToString();
			if (!new_condition_ && null != selection_ && selection_name == selection_.Name)
				return;
			// create a new condition
			Type condition_type = Type.GetType("TriggerEdit.Definitions.Condition" + selection_name);
			if (null == condition_type)
				return;
			Condition condition = (Condition)Activator.CreateInstance(condition_type);
			if ("Switcher" == condition.Name)
				condition.preconditions_ = new ArrayList();
			if (new_condition_)
			{
				selection_parent_.preconditions_.Add(
					new Precondition(Precondition.Type.NORMAL, condition));
				new_condition_ = false;
			}
			else if (null == selection_parent_)
				condition_ = condition;
			else
			{
				// find the current selection in the parent's preconditions
				int i = 0;
				for (; i != selection_parent_.preconditions_.Count; ++i)
					if (((Precondition)selection_parent_.preconditions_[i]).condition_ == selection_)
						break;
				if (i != selection_parent_.preconditions_.Count)
					selection_parent_.preconditions_[i] = new Precondition(
						Precondition.Type.NORMAL,
						condition);
			}
			selection_ = condition;
			display_pnl_.Condition = condition_;
			property_grid_.SelectedObject = selection_;
		}

		#endregion

		#region implementation

		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		private void InitializeConditionList()
		{
			condition_lst_.Items.AddRange(new string[] {
				"ActivateSpot",
				"BuildingNearBuilding",
				"CaptureBuilding",
				"CheckBelligerent",
				"ClickOnButton",
				"CorridorOmegaUpgraded",
				"CutSceneWasSkipped",
				"DifficultyLevel",
				"EnegyLevelLowerReserve",
				"EnegyLevelUpperReserve",
				"FrameState",
				"IsFieldOn",
				"KillObject",
				"KillObjectByLabel",
				"MutationEnabled",
				"NumberOfBuildingByCoresCapacity",
				"ObjectByLabelExists",
				"ObjectExists",
				"ObjectNearObjectByLabel",
				"OnlyMyClan",
				"OutOfEnergyCapacity",
				"PlayerState",
				"SetSquadWayPoint",
				"SkipCutScene",
				"SquadGoingToAttack",
				"SquadSufficientUnits",
				"Switcher",
				"Teleportation",
				"TerrainLeveledNearObjectByLabel",
				"TimeMatched",
				"ToolzerSelectedNearObjectByLabel",
				"UnitClassIsGoingToBeAttacked",
				"UnitClassUnderAttack",
				"WeaponIsFiring"});
		}

		#endregion

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.panel1 = new System.Windows.Forms.Panel();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.display_pnl_ = new TriggerEdit.ConditionDisplay();
			this.panel3 = new System.Windows.Forms.Panel();
			this.property_grid_ = new System.Windows.Forms.PropertyGrid();
			this.condition_lst_ = new System.Windows.Forms.ComboBox();
			this.ok_btn_ = new System.Windows.Forms.Button();
			this.cancel_btn_ = new System.Windows.Forms.Button();
			this.panel2 = new System.Windows.Forms.Panel();
			this.panel1.SuspendLayout();
			this.panel3.SuspendLayout();
			this.SuspendLayout();
			// 
			// panel1
			// 
			this.panel1.Controls.Add(this.splitter1);
			this.panel1.Controls.Add(this.display_pnl_);
			this.panel1.Controls.Add(this.panel3);
			this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel1.Location = new System.Drawing.Point(8, 8);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(576, 189);
			this.panel1.TabIndex = 0;
			// 
			// splitter1
			// 
			this.splitter1.Location = new System.Drawing.Point(248, 0);
			this.splitter1.Name = "splitter1";
			this.splitter1.Size = new System.Drawing.Size(3, 189);
			this.splitter1.TabIndex = 4;
			this.splitter1.TabStop = false;
			// 
			// display_pnl_
			// 
			this.display_pnl_.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.display_pnl_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.display_pnl_.Location = new System.Drawing.Point(248, 0);
			this.display_pnl_.Name = "display_pnl_";
			this.display_pnl_.Size = new System.Drawing.Size(328, 189);
			this.display_pnl_.TabIndex = 2;
			// 
			// panel3
			// 
			this.panel3.Controls.Add(this.property_grid_);
			this.panel3.Controls.Add(this.condition_lst_);
			this.panel3.Dock = System.Windows.Forms.DockStyle.Left;
			this.panel3.Location = new System.Drawing.Point(0, 0);
			this.panel3.Name = "panel3";
			this.panel3.Size = new System.Drawing.Size(248, 189);
			this.panel3.TabIndex = 3;
			// 
			// property_grid_
			// 
			this.property_grid_.CommandsVisibleIfAvailable = true;
			this.property_grid_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.property_grid_.LargeButtons = false;
			this.property_grid_.LineColor = System.Drawing.SystemColors.ScrollBar;
			this.property_grid_.Location = new System.Drawing.Point(0, 21);
			this.property_grid_.Name = "property_grid_";
			this.property_grid_.PropertySort = System.Windows.Forms.PropertySort.Alphabetical;
			this.property_grid_.Size = new System.Drawing.Size(248, 168);
			this.property_grid_.TabIndex = 0;
			this.property_grid_.Text = "PropertyGrid";
			this.property_grid_.ToolbarVisible = false;
			this.property_grid_.ViewBackColor = System.Drawing.SystemColors.Window;
			this.property_grid_.ViewForeColor = System.Drawing.SystemColors.WindowText;
			// 
			// condition_lst_
			// 
			this.condition_lst_.Dock = System.Windows.Forms.DockStyle.Top;
			this.condition_lst_.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.condition_lst_.Location = new System.Drawing.Point(0, 0);
			this.condition_lst_.Name = "condition_lst_";
			this.condition_lst_.Size = new System.Drawing.Size(248, 21);
			this.condition_lst_.TabIndex = 1;
			this.condition_lst_.SelectedValueChanged += new System.EventHandler(this.condition_lst__SelectedValueChanged);
			// 
			// ok_btn_
			// 
			this.ok_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.ok_btn_.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.ok_btn_.Location = new System.Drawing.Point(416, 208);
			this.ok_btn_.Name = "ok_btn_";
			this.ok_btn_.TabIndex = 1;
			this.ok_btn_.Text = "OK";
			// 
			// cancel_btn_
			// 
			this.cancel_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.cancel_btn_.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.cancel_btn_.Location = new System.Drawing.Point(504, 208);
			this.cancel_btn_.Name = "cancel_btn_";
			this.cancel_btn_.TabIndex = 2;
			this.cancel_btn_.Text = "Cancel";
			// 
			// panel2
			// 
			this.panel2.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.panel2.Location = new System.Drawing.Point(8, 197);
			this.panel2.Name = "panel2";
			this.panel2.Size = new System.Drawing.Size(576, 40);
			this.panel2.TabIndex = 3;
			// 
			// ConditionBuilder
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(592, 245);
			this.Controls.Add(this.cancel_btn_);
			this.Controls.Add(this.ok_btn_);
			this.Controls.Add(this.panel1);
			this.Controls.Add(this.panel2);
			this.DockPadding.All = 8;
			this.Name = "ConditionBuilder";
			this.ShowInTaskbar = false;
			this.Text = "ConditionBuilder";
			this.panel1.ResumeLayout(false);
			this.panel3.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region Component Designer data

		private System.ComponentModel.Container components = null;
		private ConditionDisplay display_pnl_;
		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.Button ok_btn_;
		private System.Windows.Forms.PropertyGrid property_grid_;
		private System.Windows.Forms.Panel panel2;
		private System.Windows.Forms.Panel panel3;
		private System.Windows.Forms.ComboBox condition_lst_;
		private System.Windows.Forms.Splitter splitter1;
		private System.Windows.Forms.Button cancel_btn_;

		#endregion

		#region data

		bool      new_condition_;
		Condition condition_;
		Condition selection_;
		Condition selection_parent_;

		#endregion
	}
}
