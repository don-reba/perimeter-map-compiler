using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Xml;
using System.Windows.Forms;

namespace TriggerEdit
{
	public class MainForm : System.Windows.Forms.Form
	{
		#region interface

		public MainForm(string[] args)
		{
			// Required for Windows Form Designer support
			InitializeComponent();
			args_      = args;
			selection_ = -1;
			display_pnl_.SelectTrigger +=new TriggerEdit.TriggerDisplay.SelectTriggerEvent(display_pnl__SelectTrigger);
		}

		#endregion

		#region internal implementation

		private void GetStats()
		{
			string dir = Directory.GetCurrentDirectory();
			string[] files = Directory.GetFiles("F:\\Application Data\\PETdemo\\Game\\Scripts\\Triggers\\XML", "*.xml");
			SortedList list = new SortedList();
			foreach (string source in files)
			{
				// load the XML file
				XmlTextReader reader = new XmlTextReader(source);
				XmlDocument doc = new XmlDocument();
				doc.Load(reader);
				reader.Close();
				// extract data
				XmlDocument fusion = new XmlDocument();
				XmlNodeList positions = doc.SelectNodes(
					"descendant::array[@name=\"conditions\"]/set/value[@name=\"type\"]");
				foreach (XmlNode node in positions)
				{
					string str = node.InnerText;
					if (!list.ContainsKey(str))
						list.Add(str, null);
				}
			}
			StreamWriter writer = new StreamWriter("output.txt");
			foreach (DictionaryEntry entry in list)
				writer.WriteLine(entry.Key.ToString());
			writer.Close();
			Close();
		}

		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		private void EnableTriggerControls(bool enable)
		{
			name_edt_.Enabled      = enable;
			state_lst_.Enabled     = enable;
			condition_btn_.Enabled = enable;
			action_btn_.Enabled    = enable;
		}

		#endregion

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.display_pnl_ = new TriggerEdit.TriggerDisplay();
			this.property_panel_ = new System.Windows.Forms.Panel();
			this.property_actions_panel_ = new System.Windows.Forms.Panel();
			this.state_lst_ = new System.Windows.Forms.ComboBox();
			this.state_lbl_ = new System.Windows.Forms.Label();
			this.name_lbl_ = new System.Windows.Forms.Label();
			this.action_btn_ = new System.Windows.Forms.Button();
			this.condition_btn_ = new System.Windows.Forms.Button();
			this.name_edt_ = new System.Windows.Forms.TextBox();
			this.property_tree_ = new System.Windows.Forms.TreeView();
			this.property_label_ = new System.Windows.Forms.Label();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.property_panel_.SuspendLayout();
			this.property_actions_panel_.SuspendLayout();
			this.SuspendLayout();
			// 
			// display_pnl_
			// 
			this.display_pnl_.AutoScroll = true;
			this.display_pnl_.AutoScrollMinSize = new System.Drawing.Size(512, 512);
			this.display_pnl_.BackColor = System.Drawing.SystemColors.WindowFrame;
			this.display_pnl_.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.display_pnl_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.display_pnl_.Location = new System.Drawing.Point(211, 8);
			this.display_pnl_.Name = "display_pnl_";
			this.display_pnl_.Size = new System.Drawing.Size(365, 461);
			this.display_pnl_.TabIndex = 0;
			// 
			// property_panel_
			// 
			this.property_panel_.Controls.Add(this.property_actions_panel_);
			this.property_panel_.Controls.Add(this.property_tree_);
			this.property_panel_.Controls.Add(this.property_label_);
			this.property_panel_.Dock = System.Windows.Forms.DockStyle.Left;
			this.property_panel_.DockPadding.Right = 4;
			this.property_panel_.Location = new System.Drawing.Point(8, 8);
			this.property_panel_.Name = "property_panel_";
			this.property_panel_.Size = new System.Drawing.Size(200, 461);
			this.property_panel_.TabIndex = 1;
			this.property_panel_.Resize += new System.EventHandler(this.property_panel__Resize);
			// 
			// property_actions_panel_
			// 
			this.property_actions_panel_.Controls.Add(this.state_lst_);
			this.property_actions_panel_.Controls.Add(this.state_lbl_);
			this.property_actions_panel_.Controls.Add(this.name_lbl_);
			this.property_actions_panel_.Controls.Add(this.action_btn_);
			this.property_actions_panel_.Controls.Add(this.condition_btn_);
			this.property_actions_panel_.Controls.Add(this.name_edt_);
			this.property_actions_panel_.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.property_actions_panel_.Location = new System.Drawing.Point(0, 357);
			this.property_actions_panel_.Name = "property_actions_panel_";
			this.property_actions_panel_.Size = new System.Drawing.Size(196, 104);
			this.property_actions_panel_.TabIndex = 3;
			// 
			// state_lst_
			// 
			this.state_lst_.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.state_lst_.Enabled = false;
			this.state_lst_.Items.AddRange(new object[] {
																		  "checking",
																		  "done",
																		  "sleeping"});
			this.state_lst_.Location = new System.Drawing.Point(56, 40);
			this.state_lst_.Name = "state_lst_";
			this.state_lst_.Size = new System.Drawing.Size(128, 21);
			this.state_lst_.TabIndex = 6;
			// 
			// state_lbl_
			// 
			this.state_lbl_.Location = new System.Drawing.Point(8, 40);
			this.state_lbl_.Name = "state_lbl_";
			this.state_lbl_.Size = new System.Drawing.Size(40, 16);
			this.state_lbl_.TabIndex = 5;
			this.state_lbl_.Text = "State";
			// 
			// name_lbl_
			// 
			this.name_lbl_.Location = new System.Drawing.Point(8, 8);
			this.name_lbl_.Name = "name_lbl_";
			this.name_lbl_.Size = new System.Drawing.Size(40, 16);
			this.name_lbl_.TabIndex = 4;
			this.name_lbl_.Text = "Name";
			// 
			// action_btn_
			// 
			this.action_btn_.Anchor = System.Windows.Forms.AnchorStyles.Top;
			this.action_btn_.Enabled = false;
			this.action_btn_.Location = new System.Drawing.Point(104, 72);
			this.action_btn_.Name = "action_btn_";
			this.action_btn_.Size = new System.Drawing.Size(72, 23);
			this.action_btn_.TabIndex = 3;
			this.action_btn_.Text = "Action";
			// 
			// condition_btn_
			// 
			this.condition_btn_.Anchor = System.Windows.Forms.AnchorStyles.Top;
			this.condition_btn_.Enabled = false;
			this.condition_btn_.Location = new System.Drawing.Point(24, 72);
			this.condition_btn_.Name = "condition_btn_";
			this.condition_btn_.Size = new System.Drawing.Size(72, 23);
			this.condition_btn_.TabIndex = 2;
			this.condition_btn_.Text = "Condition";
			this.condition_btn_.Click += new System.EventHandler(this.condition_btn__Click);
			// 
			// name_edt_
			// 
			this.name_edt_.Enabled = false;
			this.name_edt_.Location = new System.Drawing.Point(56, 8);
			this.name_edt_.Name = "name_edt_";
			this.name_edt_.Size = new System.Drawing.Size(128, 20);
			this.name_edt_.TabIndex = 0;
			this.name_edt_.Text = "";
			this.name_edt_.TextChanged += new System.EventHandler(this.name_edt__TextChanged);
			// 
			// property_tree_
			// 
			this.property_tree_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.property_tree_.ImageIndex = -1;
			this.property_tree_.Location = new System.Drawing.Point(0, 32);
			this.property_tree_.Name = "property_tree_";
			this.property_tree_.SelectedImageIndex = -1;
			this.property_tree_.Size = new System.Drawing.Size(196, 429);
			this.property_tree_.TabIndex = 1;
			// 
			// property_label_
			// 
			this.property_label_.Dock = System.Windows.Forms.DockStyle.Top;
			this.property_label_.Location = new System.Drawing.Point(0, 0);
			this.property_label_.Name = "property_label_";
			this.property_label_.Size = new System.Drawing.Size(196, 32);
			this.property_label_.TabIndex = 2;
			this.property_label_.Text = "Trigger";
			this.property_label_.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// splitter1
			// 
			this.splitter1.Location = new System.Drawing.Point(208, 8);
			this.splitter1.Name = "splitter1";
			this.splitter1.Size = new System.Drawing.Size(3, 461);
			this.splitter1.TabIndex = 2;
			this.splitter1.TabStop = false;
			// 
			// MainForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(584, 477);
			this.Controls.Add(this.display_pnl_);
			this.Controls.Add(this.splitter1);
			this.Controls.Add(this.property_panel_);
			this.DockPadding.All = 8;
			this.Name = "MainForm";
			this.Text = "TriggerViewer";
			this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
			this.Load += new System.EventHandler(this.MainForm_Load);
			this.property_panel_.ResumeLayout(false);
			this.property_actions_panel_.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region Entry Point
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args) 
		{
				Application.Run(new MainForm(args));
		}
		#endregion

		#region event handlers

		private void condition_btn__Click(object sender, System.EventArgs e)
		{
			if (selection_ < 0)
				return;
			ConditionBuilder condition_builder = new ConditionBuilder();
			condition_builder.Condition = triggers_[selection_].condition;
			condition_builder.ShowDialog(this);
			if (DialogResult.OK == condition_builder.DialogResult)
				triggers_[selection_].condition = condition_builder.Condition;
		}

		private void display_pnl__SelectTrigger(object sender, TriggerDisplay.TriggerEventArgs e)
		{
			if (e.trigger_id_ < 0)
			{
				EnableTriggerControls(false);
				return;
			}
			// display cell info
			Trigger trigger = triggers_[e.trigger_id_];
			property_label_.Text = trigger.name;
			name_edt_.Text       = trigger.name;
			property_tree_.Nodes.Clear();
			switch (trigger.state)
			{
				case Trigger.State.Checking:
					property_tree_.Nodes.Add("state: checking");
					state_lst_.SelectedIndex = 0;
					break;
				case Trigger.State.Done:
					property_tree_.Nodes.Add("state: done");
					state_lst_.SelectedIndex = 1;
					break;
				case Trigger.State.Sleeping:
					property_tree_.Nodes.Add("state: sleeping");
					state_lst_.SelectedIndex = 2;
					break;
			}
			if (null != trigger.action)
				property_tree_.Nodes.Add(trigger.action.GetTreeNode());
			//			if (null != trigger.condition)
			//				property_tree_.Nodes.Add(trigger.condition.GetTreeNode());
			property_tree_.ExpandAll();
			EnableTriggerControls(true);
		}

		private void MainForm_Load(object sender, System.EventArgs e)
		{
//			GetStats();
//			Application.Exit();
//			return;
			// get path
			string path;
			if (0 != args_.Length)
				path = args_[0];
			else
			{
				OpenFileDialog dlg = new OpenFileDialog();
				dlg.Filter = "trigger file (xml made from scr)|*xml";
				if (dlg.ShowDialog() != DialogResult.OK)
				{
					Close();
					return;
				}
				path = dlg.FileName;
			}
			// load the XML file
			XmlTextReader reader = new XmlTextReader(path);
			XmlDocument doc = new XmlDocument();
			doc.Load(reader);
			reader.Close();
			// extract data
			{
				XmlNodeList trigger_nodes = doc.SelectNodes(
					"//script"                     +
					"/set[@name=\"TriggerChain\"]" +
					"/array[@name=\"triggers\"]"   +
					"/set");
				triggers_ = new Trigger[trigger_nodes.Count];
				Hashtable names = new Hashtable();
				for (int i = 0; i != trigger_nodes.Count; ++i)
				{
					XmlNode position = trigger_nodes[i];
					triggers_[i].color_   = Color.Orange;
					triggers_[i].outline_ = Color.Orange;
					triggers_[i].Serialize(position);
					names.Add(triggers_[i].name, i);
				}
				// add links
				for (int i = 0; i != trigger_nodes.Count; ++i)
				{
					XmlNodeList link_nodes = trigger_nodes[i].SelectNodes(
						"array[@name=\"outcomingLinks\"]/set");
					triggers_[i].links = new ArrayList();
					foreach (XmlNode node in link_nodes)
					{
						Trigger.Link link = new Trigger.Link();
						// target
						link.target = (int)names[node.SelectSingleNode("string[@name=\"triggerName\"]").InnerText];
						// color
						XmlNode color_node = node.SelectSingleNode("value[@name=\"color\"]");
						if (null != color_node)
							switch (color_node.InnerText)
							{
								case "STRATEGY_BLUE":    link.color = Color.Blue;   break;
								case "STRATEGY_COLOR_0": link.color = Color.Black;  break;
								case "STRATEGY_GREEN":   link.color = Color.Green;  break;
								case "STRATEGY_RED":     link.color = Color.Red;    break;
								case "STRATEGY_YELLOW":  link.color = Color.Yellow; break;
								default:                 link.color = Color.Cyan;   break;

							}
						else
							link.color = Color.Cyan;
						// active
						XmlNode is_active_node = node.SelectSingleNode("value[@name=\"active_\"]");
						if (null != is_active_node && "true" == is_active_node.InnerText)
							link.is_active = true;
						triggers_[i].links.Add(link);
					}
				}
				Array.Sort(triggers_, new TriggerComparer());
			}
			display_pnl_.SetTriggers(ref triggers_);
		}

		private void name_edt__TextChanged(object sender, System.EventArgs e)
		{
			if (selection_ < 0)
				return;
			triggers_[selection_].name = name_edt_.Text;
			property_label_.Text       = name_edt_.Text;
		}

		private void property_panel__Resize(object sender, System.EventArgs e)
		{
			int width = property_actions_panel_.Width - name_edt_.Left - 8;
			name_edt_.Width  = width;
			state_lst_.Width = width;
		}

		#endregion

		#region data

		private string[]  args_;
		private int       selection_;
		private Trigger[] triggers_;

		#endregion

		#region Component Designer data

		private System.ComponentModel.Container components = null;
		private System.Windows.Forms.Button   action_btn_;
		private System.Windows.Forms.Button   condition_btn_;
		private System.Windows.Forms.ComboBox state_lst_;
		private System.Windows.Forms.Label    name_lbl_;
		private System.Windows.Forms.Label    property_label_;
		private System.Windows.Forms.Label    state_lbl_;
		private System.Windows.Forms.Panel    property_actions_panel_;
		private System.Windows.Forms.Panel    property_panel_;
		private System.Windows.Forms.Splitter splitter1;
		private System.Windows.Forms.TextBox  name_edt_;
		private System.Windows.Forms.TreeView property_tree_;
		private TriggerDisplay                display_pnl_;

		#endregion
	}
}
