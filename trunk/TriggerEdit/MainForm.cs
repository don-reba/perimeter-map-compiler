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
	#region using

	using TriggerDescription = TriggerContainer.TriggerDescription;
	using TriggerState       = TriggerContainer.TriggerDescription.State;
	using TriggerStatus      = TriggerContainer.TriggerDescription.Status;

	#endregion

	public class MainForm : System.Windows.Forms.Form, IMessageFilter
	{
		#region interface

		public MainForm(string[] args)
		{
			// Required for Windows Form Designer support
			InitializeComponent();
			args_      = args;
			display_pnl_.SelectTrigger
				+= new TriggerDisplay.SelectTriggerEvent(display_pnl__SelectTrigger);
			display_pnl_.ActionSelected += new EventHandler(OnAction);
			display_pnl_.ConditionSelected += new EventHandler(OnCondition);
			triggers_  = new TriggerContainer();
			Application.AddMessageFilter(this);
		}

		#endregion

		#region IMessageFilter Members

		public bool PreFilterMessage(ref Message m)
		{
			if (m.Msg == (int)Import.WindowsMessages.WM_MOUSEWHEEL)
			{
				Import.POINT point = new TriggerEdit.Import.POINT(
					(int)(short)((uint)m.LParam & 0xFFFF),
					(int)(short)(((uint)m.LParam & 0xFFFF0000) >> 16));
				m.HWnd = Import.WindowFromPoint(point);
				Import.SendMessage(m.HWnd, (uint)m.Msg, m.WParam, m.LParam);
				return true;
			}
			if (m.Msg == (int)Import.WindowsMessages.WM_KEYDOWN)
			{
				// toggle animation on "space" key press
				int vcode = (int)m.WParam;
				if (vcode == (int)Import.VK.SPACE)
					display_pnl_.ToggleAnimation();
				else if (vcode == (int)Import.VK.ALPHA_F)
					display_pnl_.ShowFrameRate = !display_pnl_.ShowFrameRate;
			}
			return false;
		}

		#endregion

		#region implementation

		private void AppendActionTreeNode(Definitions.Action action, TreeNodeCollection nodes)
		{
			nodes.Add("Action: " + action.Name);
		}

		private void AppendConditionTreeNode(Definitions.Condition condition, TreeNodeCollection nodes)
		{
			if (null == condition.preconditions_ || 0 == condition.preconditions_.Count)
			{
				nodes.Add("Condition: " + condition.Name);
				return;
			}
			TreeNode root = new TreeNode("Conditions:");
			AppendConditionTreeNodeRecursive(condition, root.Nodes);
			nodes.Add(root);
		}

		private void AppendConditionTreeNodeRecursive(Definitions.Condition condition, TreeNodeCollection nodes)
		{
			TreeNode root = new TreeNode();
			if (condition.GetType() == typeof(Definitions.ConditionSwitcher))
				root.Text = ((Definitions.ConditionSwitcher)condition).type.ToString();
			else
				root.Text = condition.Name;
			if (null != condition.preconditions_)
				foreach (Definitions.Precondition precondition in condition.preconditions_)
					AppendConditionTreeNodeRecursive(precondition.condition_, root.Nodes);
			nodes.Add(root);
		}

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
//				XmlNodeList positions = doc.SelectNodes(
//					"script"                                      +
//					"/set[@name=\"TriggerChain\"]"                +
//					"/array[@name=\"triggers\"]"                  +
//					"/set"                                        +
//					"/set[@code=\"struct ActionOrderBuilding\"]" +
//					"/*[@name=\"placementStrategy\"]");
				XmlNodeList positions = doc.SelectNodes(
					"descendant::*[@name=\"controlID\"]");
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
			name_edt_.Enabled = enable;
		}

		private void LoadFile(string path)
		{
			// load the XML file
			XmlTextReader reader = new XmlTextReader(path);
			XmlDocument doc = new XmlDocument();
			doc.Load(reader);
			reader.Close();
			// extract data
			if (!triggers_.Serialize(doc))
				return;
			display_pnl_.Reset(ref triggers_);
			// set priority level
			Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.BelowNormal;
			// enable the save button
			save_btn_.Enabled = true;
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

		private void OnAction(object sender, System.EventArgs e)
		{
			if (display_pnl_.Selection < 0)
				return;
			ActionBuilder action_builder = new ActionBuilder();
			action_builder.Action = triggers_.descriptions_[display_pnl_.Selection].action_;
			action_builder.ShowDialog(this);
			if (DialogResult.OK == action_builder.DialogResult)
				triggers_.descriptions_[display_pnl_.Selection].action_ = action_builder.Action;
		}

		private void OnCondition(object sender, System.EventArgs e)
		{
			if (display_pnl_.Selection < 0)
				return;
			ConditionBuilder condition_builder = new ConditionBuilder();
			condition_builder.Condition = triggers_.descriptions_[display_pnl_.Selection].condition_;
			condition_builder.ShowDialog(this);
			if (DialogResult.OK == condition_builder.DialogResult)
				triggers_.descriptions_[display_pnl_.Selection].condition_ = condition_builder.Condition;
		}

		private void display_pnl__SelectTrigger(object sender, TriggerDisplay.TriggerEventArgs e)
		{
			if (e.trigger_id_ < 0)
			{
				EnableTriggerControls(false);
				return;
			}
			// display cell info
			TriggerContainer.TriggerDescription trigger = triggers_.descriptions_[e.trigger_id_];
			property_label_.Text = trigger.name_;
			name_edt_.Text       = trigger.name_;
			property_tree_.SuspendLayout();
			property_tree_.Nodes.Clear();
			if (trigger.is_virtual_)
				property_tree_.Nodes.Add("virtual");
			else
			{
				if (null != trigger.action_)
					AppendActionTreeNode(trigger.action_, property_tree_.Nodes);
				if (null != trigger.condition_)
					AppendConditionTreeNode(trigger.condition_, property_tree_.Nodes);
				property_tree_.ExpandAll();
			}
			property_tree_.ResumeLayout(true);
			EnableTriggerControls(true);
			// enable/disable buttons
			EnableTriggerControls(!trigger.is_virtual_);
		}

		private void MainForm_Load(object sender, System.EventArgs e)
		{
//			GetStats();
//			Application.Exit();
//			return;
			if (0 != args_.Length)
				LoadFile(args_[0]);
		}

		private void name_edt__TextChanged(object sender, System.EventArgs e)
		{
			if (display_pnl_.Selection < 0)
				return;
			triggers_.descriptions_[display_pnl_.Selection].name_ = name_edt_.Text;
			property_label_.Text                   = name_edt_.Text;
		}

		private void property_panel__Resize(object sender, System.EventArgs e)
		{
			int width = property_actions_panel_.Width - name_edt_.Left - 8;
			name_edt_.Width = width;
			zoom_udc_.Width = width;
		}

		private void toolbar__ButtonClick(object sender, System.Windows.Forms.ToolBarButtonClickEventArgs e)
		{
			if (e.Button == load_btn_)
				OnLoadFile();
			else if (e.Button == save_btn_)
				OnSaveFile();
		}

		private void MainForm_Resize(object sender, System.EventArgs e)
		{
			display_pnl_.RenderingEnabled = (WindowState != FormWindowState.Minimized);
		}

		private void OnLoadFile()
		{
			OpenFileDialog dlg = new OpenFileDialog();
			dlg.Filter = "trigger file (xml made from scr)|*xml";
			if (dlg.ShowDialog() != DialogResult.OK)
				return;
			LoadFile(dlg.FileName);
		}

		private void OnSaveFile()
		{
			SaveFileDialog dlg = new SaveFileDialog();
			dlg.Filter = "trigger file (xml made from scr)|*xml";
			if (dlg.ShowDialog() != DialogResult.OK)
				return;
			ScriptXmlWriter w = new ScriptXmlWriter(
				dlg.FileName,
				System.Text.ASCIIEncoding.GetEncoding(1251));
			w.WriteStartDocument(true);
			triggers_.Serialize(w);
			w.WriteEndDocument();
			w.Close();
		}

		private void display_pnl__ZoomChanged(object sender, TriggerEdit.TriggerDisplay.ZoomEventArgs e)
		{
			zoom_udc_.DecimalPlaces = 3;
			zoom_udc_.Value = new decimal(1 / e.zoom_);
		}

		private void zoom_udc__ValueChanged(object sender, System.EventArgs e)
		{
			if ((float)zoom_udc_.Value == 0.0f)
				zoom_udc_.Value = new decimal(1.0f);
			display_pnl_.Zoom = 1.0f / (float)zoom_udc_.Value;
		}

		#endregion

		#region data

		private string[]  args_;
		private System.Windows.Forms.Panel property_actions_panel_;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.NumericUpDown zoom_udc_;
		private TriggerContainer triggers_;

		#endregion

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(MainForm));
			this.display_pnl_ = new TriggerEdit.TriggerDisplay();
			this.property_panel_ = new System.Windows.Forms.Panel();
			this.property_tree_ = new System.Windows.Forms.TreeView();
			this.property_actions_panel_ = new System.Windows.Forms.Panel();
			this.zoom_udc_ = new System.Windows.Forms.NumericUpDown();
			this.label1 = new System.Windows.Forms.Label();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.name_lbl_ = new System.Windows.Forms.Label();
			this.name_edt_ = new System.Windows.Forms.TextBox();
			this.property_label_ = new System.Windows.Forms.Label();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.toolbar_ = new System.Windows.Forms.ToolBar();
			this.save_btn_ = new System.Windows.Forms.ToolBarButton();
			this.load_btn_ = new System.Windows.Forms.ToolBarButton();
			this.toolbar_img_lst_ = new System.Windows.Forms.ImageList(this.components);
			this.property_panel_.SuspendLayout();
			this.property_actions_panel_.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.zoom_udc_)).BeginInit();
			this.SuspendLayout();
			// 
			// display_pnl_
			// 
			this.display_pnl_.BackColor = System.Drawing.SystemColors.WindowFrame;
			this.display_pnl_.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.display_pnl_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.display_pnl_.Location = new System.Drawing.Point(211, 42);
			this.display_pnl_.Name = "display_pnl_";
			this.display_pnl_.RenderingEnabled = true;
			this.display_pnl_.ShowFrameRate = false;
			this.display_pnl_.Size = new System.Drawing.Size(365, 427);
			this.display_pnl_.TabIndex = 0;
			this.display_pnl_.Zoom = 1F;
			this.display_pnl_.ZoomChanged += new TriggerEdit.TriggerDisplay.ZoomChangedEvent(this.display_pnl__ZoomChanged);
			// 
			// property_panel_
			// 
			this.property_panel_.Controls.Add(this.property_tree_);
			this.property_panel_.Controls.Add(this.property_actions_panel_);
			this.property_panel_.Controls.Add(this.property_label_);
			this.property_panel_.Dock = System.Windows.Forms.DockStyle.Left;
			this.property_panel_.DockPadding.Right = 4;
			this.property_panel_.Location = new System.Drawing.Point(8, 42);
			this.property_panel_.Name = "property_panel_";
			this.property_panel_.Size = new System.Drawing.Size(200, 427);
			this.property_panel_.TabIndex = 1;
			this.property_panel_.Resize += new System.EventHandler(this.property_panel__Resize);
			// 
			// property_tree_
			// 
			this.property_tree_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.property_tree_.ImageIndex = -1;
			this.property_tree_.Location = new System.Drawing.Point(0, 32);
			this.property_tree_.Name = "property_tree_";
			this.property_tree_.SelectedImageIndex = -1;
			this.property_tree_.Size = new System.Drawing.Size(196, 291);
			this.property_tree_.TabIndex = 1;
			// 
			// property_actions_panel_
			// 
			this.property_actions_panel_.Controls.Add(this.zoom_udc_);
			this.property_actions_panel_.Controls.Add(this.label1);
			this.property_actions_panel_.Controls.Add(this.groupBox1);
			this.property_actions_panel_.Controls.Add(this.name_lbl_);
			this.property_actions_panel_.Controls.Add(this.name_edt_);
			this.property_actions_panel_.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.property_actions_panel_.Location = new System.Drawing.Point(0, 323);
			this.property_actions_panel_.Name = "property_actions_panel_";
			this.property_actions_panel_.Size = new System.Drawing.Size(196, 104);
			this.property_actions_panel_.TabIndex = 3;
			// 
			// zoom_udc_
			// 
			this.zoom_udc_.DecimalPlaces = 2;
			this.zoom_udc_.Increment = new System.Decimal(new int[] {
																						  5,
																						  0,
																						  0,
																						  131072});
			this.zoom_udc_.Location = new System.Drawing.Point(56, 72);
			this.zoom_udc_.Maximum = new System.Decimal(new int[] {
																						-1,
																						-1,
																						-1,
																						0});
			this.zoom_udc_.Minimum = new System.Decimal(new int[] {
																						1,
																						0,
																						0,
																						262144});
			this.zoom_udc_.Name = "zoom_udc_";
			this.zoom_udc_.Size = new System.Drawing.Size(128, 20);
			this.zoom_udc_.TabIndex = 9;
			this.zoom_udc_.Value = new System.Decimal(new int[] {
																					 1,
																					 0,
																					 0,
																					 0});
			this.zoom_udc_.ValueChanged += new System.EventHandler(this.zoom_udc__ValueChanged);
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(8, 72);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(40, 16);
			this.label1.TabIndex = 8;
			this.label1.Text = "Zoom";
			// 
			// groupBox1
			// 
			this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.Top;
			this.groupBox1.Location = new System.Drawing.Point(16, 48);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(168, 8);
			this.groupBox1.TabIndex = 7;
			this.groupBox1.TabStop = false;
			// 
			// name_lbl_
			// 
			this.name_lbl_.Location = new System.Drawing.Point(8, 16);
			this.name_lbl_.Name = "name_lbl_";
			this.name_lbl_.Size = new System.Drawing.Size(40, 16);
			this.name_lbl_.TabIndex = 4;
			this.name_lbl_.Text = "Name";
			// 
			// name_edt_
			// 
			this.name_edt_.Enabled = false;
			this.name_edt_.Location = new System.Drawing.Point(56, 16);
			this.name_edt_.Name = "name_edt_";
			this.name_edt_.Size = new System.Drawing.Size(128, 20);
			this.name_edt_.TabIndex = 0;
			this.name_edt_.Text = "";
			this.name_edt_.TextChanged += new System.EventHandler(this.name_edt__TextChanged);
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
			this.splitter1.Location = new System.Drawing.Point(208, 42);
			this.splitter1.Name = "splitter1";
			this.splitter1.Size = new System.Drawing.Size(3, 427);
			this.splitter1.TabIndex = 2;
			this.splitter1.TabStop = false;
			// 
			// toolbar_
			// 
			this.toolbar_.Appearance = System.Windows.Forms.ToolBarAppearance.Flat;
			this.toolbar_.Buttons.AddRange(new System.Windows.Forms.ToolBarButton[] {
																												this.save_btn_,
																												this.load_btn_});
			this.toolbar_.Divider = false;
			this.toolbar_.DropDownArrows = true;
			this.toolbar_.ImageList = this.toolbar_img_lst_;
			this.toolbar_.Location = new System.Drawing.Point(8, 8);
			this.toolbar_.Name = "toolbar_";
			this.toolbar_.ShowToolTips = true;
			this.toolbar_.Size = new System.Drawing.Size(568, 34);
			this.toolbar_.TabIndex = 3;
			this.toolbar_.TextAlign = System.Windows.Forms.ToolBarTextAlign.Right;
			this.toolbar_.ButtonClick += new System.Windows.Forms.ToolBarButtonClickEventHandler(this.toolbar__ButtonClick);
			// 
			// save_btn_
			// 
			this.save_btn_.Enabled = false;
			this.save_btn_.ImageIndex = 0;
			this.save_btn_.Text = "&Save";
			// 
			// load_btn_
			// 
			this.load_btn_.ImageIndex = 1;
			this.load_btn_.Text = "&Open";
			// 
			// toolbar_img_lst_
			// 
			this.toolbar_img_lst_.ColorDepth = System.Windows.Forms.ColorDepth.Depth24Bit;
			this.toolbar_img_lst_.ImageSize = new System.Drawing.Size(24, 24);
			this.toolbar_img_lst_.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("toolbar_img_lst_.ImageStream")));
			this.toolbar_img_lst_.TransparentColor = System.Drawing.Color.Magenta;
			// 
			// MainForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(584, 477);
			this.Controls.Add(this.display_pnl_);
			this.Controls.Add(this.splitter1);
			this.Controls.Add(this.property_panel_);
			this.Controls.Add(this.toolbar_);
			this.DockPadding.All = 8;
			this.Name = "MainForm";
			this.Text = "Trigger Editor";
			this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
			this.Resize += new System.EventHandler(this.MainForm_Resize);
			this.Load += new System.EventHandler(this.MainForm_Load);
			this.property_panel_.ResumeLayout(false);
			this.property_actions_panel_.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.zoom_udc_)).EndInit();
			this.ResumeLayout(false);

		}
		#endregion

		private System.ComponentModel.IContainer components;

		#region Component Designer data

		private System.Windows.Forms.Label    name_lbl_;
		private System.Windows.Forms.Label    property_label_;
		private System.Windows.Forms.Panel    property_panel_;
		private System.Windows.Forms.Splitter splitter1;
		private System.Windows.Forms.TextBox  name_edt_;
		private System.Windows.Forms.TreeView property_tree_;
		private TriggerDisplay                display_pnl_;
		private System.Windows.Forms.ToolBar toolbar_;
		private System.Windows.Forms.ImageList toolbar_img_lst_;
		private System.Windows.Forms.ToolBarButton save_btn_;
		private System.Windows.Forms.ToolBarButton load_btn_;

		#endregion
	}
}
