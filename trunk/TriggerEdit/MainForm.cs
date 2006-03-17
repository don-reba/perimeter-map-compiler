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

		//----------
		// interface
		//----------

		#region

		public MainForm(string[] args)
		{
			// Required for Windows Form Designer support
			InitializeComponent();
			args_ = args;
			display_pnl_.SelectTrigger
				+= new TriggerDisplay.SelectTriggerEvent(display_pnl__SelectTrigger);
			triggers_       = new TriggerContainer();
			dialog_opacity_ = 1.0f;
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
//			if (m.Msg == (int)Import.WindowsMessages.WM_KEYDOWN)
//			{
//				// toggle animation on "space" key press
//				int vcode = (int)m.WParam;
//				if (vcode == (int)Import.VK.SPACE)
//					display_pnl_.ToggleAnimation();
//				else if (vcode == (int)Import.VK.ALPHA_F)
//					display_pnl_.ShowFrameRate = !display_pnl_.ShowFrameRate;
//			}
			return false;
		}

		#endregion

		//---------------
		// implementation
		//---------------

		#region

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
			// enable the save button
			save_btn_.Enabled = true;
		}

		private void NewFile()
		{
			triggers_.Reset();
			display_pnl_.Reset(ref triggers_);
		}

		private void DisplayMarkerInfo(TriggerContainer.TriggerDescription[] triggers)
		{
			// name field
			if (null == triggers || triggers.Length == 0)
			{
				name_edt_.Text = "";
				name_edt_.Enabled = false;
			}
			else if (triggers.Length == 1)
			{
				name_edt_.Text = triggers[0].name_;
				name_edt_.Enabled = true;
			}
			else
			{
				name_edt_.Text = "[multiple]";
				name_edt_.Enabled = false;
			}
			// property tree
			property_tree_.SuspendLayout();
			property_tree_.Nodes.Clear();
			if (null != triggers)
			{
				for (int i = 0; i != triggers.Length; ++i)
				{
					TreeNode node = new TreeNode();
					if (triggers[i].is_virtual_)
						node.Text = triggers[i].name_ + " [virtual]";
					else
					{
						node.Text = triggers[i].name_;
						if (null != triggers[i].action_)
							AppendActionTreeNode(triggers[i].action_, node.Nodes);
						if (null != triggers[i].condition_)
							AppendConditionTreeNode(triggers[i].condition_, node.Nodes);
					}
					property_tree_.Nodes.Add(node);
				}
				property_tree_.ExpandAll();
			}
			property_tree_.ResumeLayout(true);
		}

		private void SaveProperties()
		{
			string path = Path.ChangeExtension(Application.ExecutablePath, ".ini");
			XmlWriter w = new XmlTextWriter(path, System.Text.UnicodeEncoding.Unicode);
			w.WriteStartElement("preferences");
			w.WriteElementString(
				"animation_speed",
				display_pnl_.AnimationSpeed.ToString());
			w.WriteElementString(
				"marker_spacing",
				display_pnl_.MarkerSpacing.ToString());
			w.WriteElementString(
				"marker_line_count",
				display_pnl_.MarkerLineCount.ToString());
			w.WriteElementString(
				"dialog_opacity",
				dialog_opacity_.ToString());
			w.Close();
		}

		protected void LoadProperties()
		{
			try
			{
				// load the XML file
				string path = Path.ChangeExtension(Application.ExecutablePath, ".ini");
				XmlTextReader reader = new XmlTextReader(path);
				XmlDocument doc = new XmlDocument();
				doc.Load(reader);
				reader.Close();
				// extract data
				try
				{
					display_pnl_.AnimationSpeed = float.Parse(doc.SelectSingleNode(
						"preferences/animation_speed").InnerText);
				}
				catch {}
				try
				{
					display_pnl_.MarkerSpacing = float.Parse(doc.SelectSingleNode(
						"preferences/marker_spacing").InnerText);
				}
				catch {}
				try
				{
					display_pnl_.MarkerLineCount = int.Parse(doc.SelectSingleNode(
						"preferences/marker_line_count").InnerText);
				}
				catch {}
				try
				{
					dialog_opacity_ = float.Parse(doc.SelectSingleNode(
						"preferences/dialog_opacity").InnerText);
				}
				catch {}
			}
			catch {}
		}

		#endregion

		//------------
		// entry point
		//------------

		#region

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args) 
		{
				Application.Run(new MainForm(args));
		}
		#endregion

		//---------------
		// event handlers
		//---------------

		#region

		private void display_pnl__SelectTrigger(object sender, TriggerDisplay.TriggerEventArgs e)
		{
			TriggerDescription[] triggers = new TriggerDescription[e.Count];
			for (int i = 0; i != triggers.Length; ++i)
				triggers[i] = triggers_.descriptions_[e[i]];
			DisplayMarkerInfo(triggers);
			if (1 == e.Count)
			{
				if (null != action_builder_ && !action_builder_.IsDisposed)
					action_builder_.Action = triggers_.descriptions_[e[0]].action_;
				if (null != condition_builder_ && !condition_builder_.IsDisposed)
					condition_builder_.Condition = triggers_.descriptions_[e[0]].condition_;
			}
			else
			{
				if (null != action_builder_ && !action_builder_.IsDisposed)
				{
					action_builder_.Action  = null;
					action_builder_.Enabled = false;
				}
				if (null != condition_builder_ && !condition_builder_.IsDisposed)
				{
					condition_builder_.Condition = null;
					condition_builder_.Enabled   = false;
				}
			}
		}

		private void MainForm_Load(object sender, System.EventArgs e)
		{
			// set priority level
			Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.BelowNormal;
			LoadProperties();
			if (0 != args_.Length)
				LoadFile(args_[0]);
			else
				NewFile();
		}

		private void name_edt__TextChanged(object sender, System.EventArgs e)
		{
			if (display_pnl_.Selection.Count != 1)
				return;
			triggers_.descriptions_[(int)display_pnl_.Selection[0]].name_ = name_edt_.Text;
		}

		private void toolbar__ButtonClick(object sender, System.Windows.Forms.ToolBarButtonClickEventArgs e)
		{
			if (e.Button == load_btn_)
				OnLoadFile();
			else if (e.Button == save_btn_)
				OnSaveFile();
			else if (e.Button == prefs_btn_)
				OnPrefs();
			else if (e.Button == new_btn_)
				OnNewFile();
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

		private void OnNewFile()
		{
			NewFile();
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

		private void OnPrefs()
		{
			TriggerEditProperties pref_dlg = new TriggerEditProperties();
			pref_dlg.AnimationSpeed  = display_pnl_.AnimationSpeed;
			pref_dlg.MarkerSpacing   = display_pnl_.MarkerSpacing;
			pref_dlg.MarkerLineCount = display_pnl_.MarkerLineCount;
			pref_dlg.DialogOpacity   = dialog_opacity_;
			if (DialogResult.OK == pref_dlg.ShowDialog(this))
			{
				display_pnl_.AnimationSpeed  = pref_dlg.AnimationSpeed;
				display_pnl_.MarkerSpacing   = pref_dlg.MarkerSpacing;
				display_pnl_.MarkerLineCount = pref_dlg.MarkerLineCount;
				if (pref_dlg.DialogOpacity != dialog_opacity_)
				{
					dialog_opacity_ = pref_dlg.DialogOpacity;
					if (null != action_builder_ && !action_builder_.IsDisposed)
						action_builder_.Opacity = dialog_opacity_;
					if (null != condition_builder_ && !condition_builder_.IsDisposed)
						condition_builder_.Opacity = dialog_opacity_;
				}
			}
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

		private void action_btn__Click(object sender, System.EventArgs e)
		{
			if (null == action_builder_ || action_builder_.IsDisposed)
			{
				action_builder_         = new ActionBuilder();
				action_builder_.Owner   = this;
				action_builder_.Opacity = dialog_opacity_;
				action_builder_.ActionChanged += new EventHandler(action_builder__ActionChanged);
			}
			if (display_pnl_.Selection.Count == 1)
			{
				action_builder_.Action  = triggers_.descriptions_[(int)display_pnl_.Selection[0]].action_;
				action_builder_.Enabled = true;
			}
			else
			{
				action_builder_.Action  = null;
				action_builder_.Enabled = false;
			}
			action_builder_.Show();
		}

		private void action_builder__ActionChanged(object sender, EventArgs e)
		{
			if (display_pnl_.Selection.Count != 1)
				return;
			triggers_.descriptions_[(int)display_pnl_.Selection[0]].action_
				= action_builder_.Action;
			TriggerDescription[] triggers = new TriggerDescription[1];
			triggers[0] = triggers_.descriptions_[(int)display_pnl_.Selection[0]];
			DisplayMarkerInfo(triggers);
		}

		private void condition_btn__Click(object sender, System.EventArgs e)
		{
			if (null == condition_builder_ || condition_builder_.IsDisposed)
			{
				condition_builder_         = new ConditionBuilder();
				condition_builder_.Owner   = this;
				condition_builder_.Opacity = dialog_opacity_;
				condition_builder_.ConditionChanged += new EventHandler(condition_builder__ConditionChanged);
			}
			condition_builder_.Condition
				= (display_pnl_.Selection.Count != 1)
				? null
				: triggers_.descriptions_[(int)display_pnl_.Selection[0]].condition_;
			condition_builder_.Show();
		}

		private void condition_builder__ConditionChanged(object sender, EventArgs e)
		{
			if (display_pnl_.Selection.Count != 1)
				return;
			triggers_.descriptions_[(int)display_pnl_.Selection[0]].condition_
				= condition_builder_.Condition;
			TriggerDescription[] triggers = new TriggerDescription[1];
			triggers[0] = triggers_.descriptions_[(int)display_pnl_.Selection[0]];
			DisplayMarkerInfo(triggers);
		}

		protected override void OnKeyPress(KeyPressEventArgs e)
		{
			base.OnKeyPress(e);
		}

		protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
		{
			Type active = ActiveControl.GetType();
			if (  active != typeof(TextBox)
				&& active != typeof(ComboBox))
			{
				switch (keyData)
				{
					case Keys.Control | Keys.O: OnLoadFile();               break;
					case Keys.Control | Keys.N: OnNewFile();                break;
					case Keys.Control | Keys.S: OnSaveFile();               break;
					case Keys.Control | Keys.P: OnPrefs();                  break;
					case Keys.F:                OnToggleFrameRateDisplay(); break;
					case Keys.Space:            OnToggleAnimation();        break;
				}
			}
			return base.ProcessCmdKey (ref msg, keyData);
		}

		private void OnToggleAnimation()
		{
			display_pnl_.ToggleAnimation();
		}

		private void OnToggleFrameRateDisplay()
		{
			display_pnl_.ShowFrameRate = !display_pnl_.ShowFrameRate;
		}

		private void zoom_eye_btn__Click(object sender, System.EventArgs e)
		{
			display_pnl_.Zoom = 1.0f;
			zoom_udc_.Value   = new decimal(1.0f);
			display_pnl_.Zoom = 1.0f;
		}

		private void zoom_fit_btn__Click(object sender, System.EventArgs e)
		{
			display_pnl_.ZoomToFit();
		}

		private void zoom_selection_btn__Click(object sender, System.EventArgs e)
		{
			display_pnl_.ZoomToSelection();
		}

		private void MainForm_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			SaveProperties();
		}

		#endregion

		//-----
		// data
		//-----

		#region data

		private string[]                    args_;
		private TriggerContainer            triggers_;
		private ActionBuilder               action_builder_;
		private ConditionBuilder            condition_builder_;
		private System.Windows.Forms.Button zoom_eye_btn_;
		private float                       dialog_opacity_;

		#endregion

		//----------
		// generated
		//----------

		#region code
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
			this.group_box_ = new System.Windows.Forms.GroupBox();
			this.comment_btn_ = new System.Windows.Forms.Button();
			this.group_btn_ = new System.Windows.Forms.Button();
			this.ungroup_btn_ = new System.Windows.Forms.Button();
			this.textBox1 = new System.Windows.Forms.TextBox();
			this.comment_label_ = new System.Windows.Forms.Label();
			this.zoom_box_ = new System.Windows.Forms.GroupBox();
			this.label1 = new System.Windows.Forms.Label();
			this.zoom_udc_ = new System.Windows.Forms.NumericUpDown();
			this.zoom_selection_btn_ = new System.Windows.Forms.Button();
			this.zoom_fit_btn_ = new System.Windows.Forms.Button();
			this.trigger_box_ = new System.Windows.Forms.GroupBox();
			this.name_lbl_ = new System.Windows.Forms.Label();
			this.name_edt_ = new System.Windows.Forms.TextBox();
			this.condition_btn_ = new System.Windows.Forms.Button();
			this.action_btn_ = new System.Windows.Forms.Button();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.toolbar_ = new System.Windows.Forms.ToolBar();
			this.new_btn_ = new System.Windows.Forms.ToolBarButton();
			this.load_btn_ = new System.Windows.Forms.ToolBarButton();
			this.save_btn_ = new System.Windows.Forms.ToolBarButton();
			this.prefs_btn_ = new System.Windows.Forms.ToolBarButton();
			this.toolbar_img_lst_ = new System.Windows.Forms.ImageList(this.components);
			this.zoom_eye_btn_ = new System.Windows.Forms.Button();
			this.property_panel_.SuspendLayout();
			this.property_actions_panel_.SuspendLayout();
			this.group_box_.SuspendLayout();
			this.zoom_box_.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.zoom_udc_)).BeginInit();
			this.trigger_box_.SuspendLayout();
			this.SuspendLayout();
			// 
			// display_pnl_
			// 
			this.display_pnl_.AnimationSpeed = 96F;
			this.display_pnl_.BackColor = System.Drawing.SystemColors.WindowFrame;
			this.display_pnl_.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.display_pnl_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.display_pnl_.Location = new System.Drawing.Point(211, 42);
			this.display_pnl_.MarkerLineCount = 2;
			this.display_pnl_.MarkerSpacing = 96F;
			this.display_pnl_.Name = "display_pnl_";
			this.display_pnl_.RenderingEnabled = true;
			this.display_pnl_.ShowFrameRate = false;
			this.display_pnl_.Size = new System.Drawing.Size(365, 515);
			this.display_pnl_.TabIndex = 0;
			this.display_pnl_.Zoom = 1F;
			this.display_pnl_.ZoomChanged += new TriggerEdit.TriggerDisplay.ZoomChangedEvent(this.display_pnl__ZoomChanged);
			// 
			// property_panel_
			// 
			this.property_panel_.Controls.Add(this.property_tree_);
			this.property_panel_.Controls.Add(this.property_actions_panel_);
			this.property_panel_.Dock = System.Windows.Forms.DockStyle.Left;
			this.property_panel_.DockPadding.Right = 4;
			this.property_panel_.Location = new System.Drawing.Point(8, 42);
			this.property_panel_.Name = "property_panel_";
			this.property_panel_.Size = new System.Drawing.Size(200, 515);
			this.property_panel_.TabIndex = 1;
			// 
			// property_tree_
			// 
			this.property_tree_.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.property_tree_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.property_tree_.ImageIndex = -1;
			this.property_tree_.Location = new System.Drawing.Point(0, 0);
			this.property_tree_.Name = "property_tree_";
			this.property_tree_.SelectedImageIndex = -1;
			this.property_tree_.Size = new System.Drawing.Size(196, 171);
			this.property_tree_.TabIndex = 1;
			// 
			// property_actions_panel_
			// 
			this.property_actions_panel_.Controls.Add(this.group_box_);
			this.property_actions_panel_.Controls.Add(this.zoom_box_);
			this.property_actions_panel_.Controls.Add(this.trigger_box_);
			this.property_actions_panel_.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.property_actions_panel_.Location = new System.Drawing.Point(0, 171);
			this.property_actions_panel_.Name = "property_actions_panel_";
			this.property_actions_panel_.Size = new System.Drawing.Size(196, 344);
			this.property_actions_panel_.TabIndex = 3;
			// 
			// group_box_
			// 
			this.group_box_.Anchor = System.Windows.Forms.AnchorStyles.Top;
			this.group_box_.Controls.Add(this.comment_btn_);
			this.group_box_.Controls.Add(this.group_btn_);
			this.group_box_.Controls.Add(this.ungroup_btn_);
			this.group_box_.Controls.Add(this.textBox1);
			this.group_box_.Controls.Add(this.comment_label_);
			this.group_box_.Enabled = false;
			this.group_box_.Location = new System.Drawing.Point(8, 120);
			this.group_box_.Name = "group_box_";
			this.group_box_.Size = new System.Drawing.Size(176, 104);
			this.group_box_.TabIndex = 16;
			this.group_box_.TabStop = false;
			this.group_box_.Text = "Group";
			// 
			// comment_btn_
			// 
			this.comment_btn_.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.comment_btn_.Location = new System.Drawing.Point(144, 32);
			this.comment_btn_.Name = "comment_btn_";
			this.comment_btn_.Size = new System.Drawing.Size(24, 20);
			this.comment_btn_.TabIndex = 4;
			this.comment_btn_.Text = "...";
			// 
			// group_btn_
			// 
			this.group_btn_.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.group_btn_.Location = new System.Drawing.Point(8, 64);
			this.group_btn_.Name = "group_btn_";
			this.group_btn_.TabIndex = 3;
			this.group_btn_.Text = "Group";
			// 
			// ungroup_btn_
			// 
			this.ungroup_btn_.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.ungroup_btn_.Location = new System.Drawing.Point(96, 64);
			this.ungroup_btn_.Name = "ungroup_btn_";
			this.ungroup_btn_.TabIndex = 2;
			this.ungroup_btn_.Text = "Ungroup";
			// 
			// textBox1
			// 
			this.textBox1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.textBox1.Location = new System.Drawing.Point(8, 32);
			this.textBox1.Name = "textBox1";
			this.textBox1.Size = new System.Drawing.Size(136, 20);
			this.textBox1.TabIndex = 1;
			this.textBox1.Text = "";
			// 
			// comment_label_
			// 
			this.comment_label_.Location = new System.Drawing.Point(8, 16);
			this.comment_label_.Name = "comment_label_";
			this.comment_label_.Size = new System.Drawing.Size(56, 16);
			this.comment_label_.TabIndex = 0;
			this.comment_label_.Text = "Comment";
			// 
			// zoom_box_
			// 
			this.zoom_box_.Anchor = System.Windows.Forms.AnchorStyles.Top;
			this.zoom_box_.Controls.Add(this.zoom_eye_btn_);
			this.zoom_box_.Controls.Add(this.label1);
			this.zoom_box_.Controls.Add(this.zoom_udc_);
			this.zoom_box_.Controls.Add(this.zoom_selection_btn_);
			this.zoom_box_.Controls.Add(this.zoom_fit_btn_);
			this.zoom_box_.Location = new System.Drawing.Point(8, 240);
			this.zoom_box_.Name = "zoom_box_";
			this.zoom_box_.Size = new System.Drawing.Size(176, 96);
			this.zoom_box_.TabIndex = 15;
			this.zoom_box_.TabStop = false;
			this.zoom_box_.Text = "Zoom";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(8, 24);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(40, 16);
			this.label1.TabIndex = 8;
			this.label1.Text = "Ratio";
			// 
			// zoom_udc_
			// 
			this.zoom_udc_.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.zoom_udc_.DecimalPlaces = 2;
			this.zoom_udc_.Increment = new System.Decimal(new int[] {
																						  5,
																						  0,
																						  0,
																						  131072});
			this.zoom_udc_.Location = new System.Drawing.Point(48, 24);
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
			this.zoom_udc_.TabIndex = 9;
			this.zoom_udc_.Value = new System.Decimal(new int[] {
																					 100,
																					 0,
																					 0,
																					 131072});
			this.zoom_udc_.ValueChanged += new System.EventHandler(this.zoom_udc__ValueChanged);
			// 
			// zoom_selection_btn_
			// 
			this.zoom_selection_btn_.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.zoom_selection_btn_.Location = new System.Drawing.Point(56, 56);
			this.zoom_selection_btn_.Name = "zoom_selection_btn_";
			this.zoom_selection_btn_.Size = new System.Drawing.Size(64, 23);
			this.zoom_selection_btn_.TabIndex = 11;
			this.zoom_selection_btn_.Text = "Selection";
			this.zoom_selection_btn_.Click += new System.EventHandler(this.zoom_selection_btn__Click);
			// 
			// zoom_fit_btn_
			// 
			this.zoom_fit_btn_.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.zoom_fit_btn_.Location = new System.Drawing.Point(136, 56);
			this.zoom_fit_btn_.Name = "zoom_fit_btn_";
			this.zoom_fit_btn_.Size = new System.Drawing.Size(32, 23);
			this.zoom_fit_btn_.TabIndex = 10;
			this.zoom_fit_btn_.Text = "All";
			this.zoom_fit_btn_.Click += new System.EventHandler(this.zoom_fit_btn__Click);
			// 
			// trigger_box_
			// 
			this.trigger_box_.Anchor = System.Windows.Forms.AnchorStyles.Top;
			this.trigger_box_.Controls.Add(this.name_lbl_);
			this.trigger_box_.Controls.Add(this.name_edt_);
			this.trigger_box_.Controls.Add(this.condition_btn_);
			this.trigger_box_.Controls.Add(this.action_btn_);
			this.trigger_box_.Location = new System.Drawing.Point(8, 8);
			this.trigger_box_.Name = "trigger_box_";
			this.trigger_box_.Size = new System.Drawing.Size(176, 96);
			this.trigger_box_.TabIndex = 14;
			this.trigger_box_.TabStop = false;
			this.trigger_box_.Text = "Trigger";
			// 
			// name_lbl_
			// 
			this.name_lbl_.Location = new System.Drawing.Point(8, 24);
			this.name_lbl_.Name = "name_lbl_";
			this.name_lbl_.Size = new System.Drawing.Size(40, 16);
			this.name_lbl_.TabIndex = 4;
			this.name_lbl_.Text = "Name";
			// 
			// name_edt_
			// 
			this.name_edt_.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.name_edt_.Enabled = false;
			this.name_edt_.Location = new System.Drawing.Point(48, 24);
			this.name_edt_.Name = "name_edt_";
			this.name_edt_.Size = new System.Drawing.Size(120, 20);
			this.name_edt_.TabIndex = 0;
			this.name_edt_.Text = "";
			this.name_edt_.TextChanged += new System.EventHandler(this.name_edt__TextChanged);
			// 
			// condition_btn_
			// 
			this.condition_btn_.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.condition_btn_.Location = new System.Drawing.Point(8, 56);
			this.condition_btn_.Name = "condition_btn_";
			this.condition_btn_.Size = new System.Drawing.Size(72, 23);
			this.condition_btn_.TabIndex = 12;
			this.condition_btn_.Text = "Conditions";
			this.condition_btn_.Click += new System.EventHandler(this.condition_btn__Click);
			// 
			// action_btn_
			// 
			this.action_btn_.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.action_btn_.Location = new System.Drawing.Point(96, 56);
			this.action_btn_.Name = "action_btn_";
			this.action_btn_.Size = new System.Drawing.Size(72, 23);
			this.action_btn_.TabIndex = 13;
			this.action_btn_.Text = "Action";
			this.action_btn_.Click += new System.EventHandler(this.action_btn__Click);
			// 
			// splitter1
			// 
			this.splitter1.Location = new System.Drawing.Point(208, 42);
			this.splitter1.Name = "splitter1";
			this.splitter1.Size = new System.Drawing.Size(3, 515);
			this.splitter1.TabIndex = 2;
			this.splitter1.TabStop = false;
			// 
			// toolbar_
			// 
			this.toolbar_.Appearance = System.Windows.Forms.ToolBarAppearance.Flat;
			this.toolbar_.Buttons.AddRange(new System.Windows.Forms.ToolBarButton[] {
																												this.new_btn_,
																												this.load_btn_,
																												this.save_btn_,
																												this.prefs_btn_});
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
			// new_btn_
			// 
			this.new_btn_.ImageIndex = 3;
			this.new_btn_.Text = "&New";
			this.new_btn_.ToolTipText = "Ctrl+N";
			// 
			// load_btn_
			// 
			this.load_btn_.ImageIndex = 1;
			this.load_btn_.Text = "&Open";
			this.load_btn_.ToolTipText = "Ctrl+O";
			// 
			// save_btn_
			// 
			this.save_btn_.ImageIndex = 0;
			this.save_btn_.Text = "&Save";
			this.save_btn_.ToolTipText = "Ctrl+S";
			// 
			// prefs_btn_
			// 
			this.prefs_btn_.ImageIndex = 2;
			this.prefs_btn_.Text = "&Preferences";
			this.prefs_btn_.ToolTipText = "Ctrl+P";
			// 
			// toolbar_img_lst_
			// 
			this.toolbar_img_lst_.ColorDepth = System.Windows.Forms.ColorDepth.Depth24Bit;
			this.toolbar_img_lst_.ImageSize = new System.Drawing.Size(24, 24);
			this.toolbar_img_lst_.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("toolbar_img_lst_.ImageStream")));
			this.toolbar_img_lst_.TransparentColor = System.Drawing.Color.Magenta;
			// 
			// zoom_eye_btn_
			// 
			this.zoom_eye_btn_.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.zoom_eye_btn_.Location = new System.Drawing.Point(8, 56);
			this.zoom_eye_btn_.Name = "zoom_eye_btn_";
			this.zoom_eye_btn_.Size = new System.Drawing.Size(32, 23);
			this.zoom_eye_btn_.TabIndex = 12;
			this.zoom_eye_btn_.Text = "1:1";
			this.zoom_eye_btn_.Click += new System.EventHandler(this.zoom_eye_btn__Click);
			// 
			// MainForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(584, 565);
			this.Controls.Add(this.display_pnl_);
			this.Controls.Add(this.splitter1);
			this.Controls.Add(this.property_panel_);
			this.Controls.Add(this.toolbar_);
			this.DockPadding.All = 8;
			this.KeyPreview = true;
			this.Name = "MainForm";
			this.Text = "Trigger Editor";
			this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
			this.Resize += new System.EventHandler(this.MainForm_Resize);
			this.Closing += new System.ComponentModel.CancelEventHandler(this.MainForm_Closing);
			this.Load += new System.EventHandler(this.MainForm_Load);
			this.property_panel_.ResumeLayout(false);
			this.property_actions_panel_.ResumeLayout(false);
			this.group_box_.ResumeLayout(false);
			this.zoom_box_.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.zoom_udc_)).EndInit();
			this.trigger_box_.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region data

		private System.ComponentModel.IContainer   components;
		private System.Windows.Forms.Button        action_btn_;
		private System.Windows.Forms.Button        comment_btn_;
		private System.Windows.Forms.Button        condition_btn_;
		private System.Windows.Forms.Button        group_btn_;
		private System.Windows.Forms.Button        ungroup_btn_;
		private System.Windows.Forms.Button        zoom_fit_btn_;
		private System.Windows.Forms.Button        zoom_selection_btn_;
		private System.Windows.Forms.GroupBox      group_box_;
		private System.Windows.Forms.GroupBox      trigger_box_;
		private System.Windows.Forms.GroupBox      zoom_box_;
		private System.Windows.Forms.ImageList     toolbar_img_lst_;
		private System.Windows.Forms.Label         name_lbl_;
		private System.Windows.Forms.Label         comment_label_;
		private System.Windows.Forms.Label         label1;
		private System.Windows.Forms.NumericUpDown zoom_udc_;
		private System.Windows.Forms.Panel         property_panel_;
		private System.Windows.Forms.Panel         property_actions_panel_;
		private System.Windows.Forms.Splitter      splitter1;
		private System.Windows.Forms.TextBox       name_edt_;
		private System.Windows.Forms.TextBox       textBox1;
		private System.Windows.Forms.ToolBar       toolbar_;
		private System.Windows.Forms.ToolBarButton load_btn_;
		private System.Windows.Forms.ToolBarButton new_btn_;
		private System.Windows.Forms.ToolBarButton prefs_btn_;
		private System.Windows.Forms.ToolBarButton save_btn_;
		private System.Windows.Forms.TreeView      property_tree_;
		private TriggerDisplay                     display_pnl_;

		#endregion
	}
}
