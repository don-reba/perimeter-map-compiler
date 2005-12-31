using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using TriggerEdit.Definitions;

namespace TriggerEdit
{
	public class ActionBuilder : System.Windows.Forms.Form
	{
		#region interface

		public ActionBuilder()
		{
			InitializeComponent();
			InitializeActionsList();
		}

		public Action Action
		{
			get
			{
				return (Action)action_.Clone();
			}
			set
			{
				if (null == value)
				{
					action_                       = null;
					property_grid_.SelectedObject = null;
					actions_lst_.SelectedIndex    = -1;
				}
				else
				{
					action_                       = (Action)value.Clone();
					property_grid_.SelectedObject = value;
					actions_lst_.SelectedIndex    = actions_lst_.FindString(value.Name);
				}
			}
		}

		#endregion

		#region implementation

		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
					components.Dispose();
			}
			base.Dispose( disposing );
		}

		private void InitializeActionsList()
		{
			actions_lst_.Items.AddRange(new string[] {
				"ActivateAllSpots",
				"ActivateObjectByLabel",
				"AttackBySpecialWeapon",
				"DeactivateAllSpots",
				"DeactivateObjectByLabel",
				"Defeat",
				"Delay",
				"HoldBuilding",
				"InstallFrame",
				"KillObject",
				"Message",
				"OrderBuilding",
				"OscillateCamera",
				"RepareObjectByLabel",
				"SellBuilding",
				"SetCamera",
				"SetCameraAtObject",
				"SetControls",
				"SetInterface",
				"SquadAttack",
				"SquadOrderUnits",
				"SwitchFieldOn",
				"SwitchGuns",
				"Task",
				"TeleportationOut",
				"Victory"});
		}

		#endregion

		#region event handlers

		private void actions_lst__SelectedValueChanged(object sender, System.EventArgs e)
		{
			if (null == actions_lst_.SelectedItem)
				return;
			string selection_name = actions_lst_.SelectedItem.ToString();
			if (null != action_ && selection_name == action_.Name)
				return;
			Type action_type = Type.GetType("TriggerEdit.Definitions.Action" + selection_name);
			if (null == action_type)
				return;
			action_ = (Action)Activator.CreateInstance(action_type);
			property_grid_.SelectedObject = action_;
		}

		#endregion

		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.Button ok_btn_;
		private System.Windows.Forms.Button cancel_btn_;

		#region data

		Action action_;

		#endregion

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.property_grid_ = new System.Windows.Forms.PropertyGrid();
			this.actions_lst_ = new System.Windows.Forms.ComboBox();
			this.panel1 = new System.Windows.Forms.Panel();
			this.ok_btn_ = new System.Windows.Forms.Button();
			this.cancel_btn_ = new System.Windows.Forms.Button();
			this.panel1.SuspendLayout();
			this.SuspendLayout();
			// 
			// property_grid_
			// 
			this.property_grid_.CommandsVisibleIfAvailable = true;
			this.property_grid_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.property_grid_.LargeButtons = false;
			this.property_grid_.LineColor = System.Drawing.SystemColors.ScrollBar;
			this.property_grid_.Location = new System.Drawing.Point(8, 29);
			this.property_grid_.Name = "property_grid_";
			this.property_grid_.PropertySort = System.Windows.Forms.PropertySort.Alphabetical;
			this.property_grid_.Size = new System.Drawing.Size(276, 196);
			this.property_grid_.TabIndex = 0;
			this.property_grid_.Text = "propertyGrid1";
			this.property_grid_.ToolbarVisible = false;
			this.property_grid_.ViewBackColor = System.Drawing.SystemColors.Window;
			this.property_grid_.ViewForeColor = System.Drawing.SystemColors.WindowText;
			// 
			// actions_lst_
			// 
			this.actions_lst_.Dock = System.Windows.Forms.DockStyle.Top;
			this.actions_lst_.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.actions_lst_.Location = new System.Drawing.Point(8, 8);
			this.actions_lst_.Name = "actions_lst_";
			this.actions_lst_.Size = new System.Drawing.Size(276, 21);
			this.actions_lst_.TabIndex = 1;
			this.actions_lst_.SelectedValueChanged += new System.EventHandler(this.actions_lst__SelectedValueChanged);
			// 
			// panel1
			// 
			this.panel1.Controls.Add(this.cancel_btn_);
			this.panel1.Controls.Add(this.ok_btn_);
			this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.panel1.Location = new System.Drawing.Point(8, 225);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(276, 40);
			this.panel1.TabIndex = 2;
			// 
			// ok_btn_
			// 
			this.ok_btn_.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.ok_btn_.Location = new System.Drawing.Point(192, 8);
			this.ok_btn_.Name = "ok_btn_";
			this.ok_btn_.Size = new System.Drawing.Size(72, 23);
			this.ok_btn_.TabIndex = 0;
			this.ok_btn_.Text = "OK";
			// 
			// cancel_btn_
			// 
			this.cancel_btn_.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.cancel_btn_.Location = new System.Drawing.Point(104, 8);
			this.cancel_btn_.Name = "cancel_btn_";
			this.cancel_btn_.Size = new System.Drawing.Size(72, 23);
			this.cancel_btn_.TabIndex = 1;
			this.cancel_btn_.Text = "Cancel";
			// 
			// ActionBuilder
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(292, 273);
			this.Controls.Add(this.property_grid_);
			this.Controls.Add(this.panel1);
			this.Controls.Add(this.actions_lst_);
			this.DockPadding.All = 8;
			this.Name = "ActionBuilder";
			this.Text = "ActionBuilder";
			this.panel1.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region Windows Form Deigner data
		
		private System.ComponentModel.Container components = null;
		private System.Windows.Forms.ComboBox actions_lst_;
		private System.Windows.Forms.PropertyGrid property_grid_;

		#endregion
	}
}
