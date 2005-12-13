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
			args_ = args;
		}

		#endregion

		#region internal implementation

		private void GetStats()
		{
			string dir = Directory.GetCurrentDirectory();
			string[] files = Directory.GetFiles("F:\\Application Data\\PETdemo\\Game\\Scripts\\Triggers", "*.scr");
			SortedList list = new SortedList();
			foreach (string source in files)
			{
				// create the destination path
				string destination = Path.ChangeExtension(source, ".xml");
				// make sure the conversion of the SCR file into XML exists
				if (!File.Exists(destination))
				{
					Process process = new Process();
					process.StartInfo.FileName  = "ScriptConverter.exe";
					process.StartInfo.Arguments = string.Format("-s \"{0}\" \"{1}\"", source, destination);
					process.StartInfo.RedirectStandardError = true;
					process.StartInfo.RedirectStandardOutput = true;
					process.StartInfo.UseShellExecute = false;
					process.Start();
					process.WaitForExit();
					if (0 != process.ExitCode)
						return;
				}
				// load the XML file
				XmlTextReader reader = new XmlTextReader(destination);
				XmlDocument doc = new XmlDocument();
				doc.Load(reader);
				reader.Close();
				// extract data
				XmlDocument fusion = new XmlDocument();
				XmlNodeList positions = doc.SelectNodes(
					"//script"                                           +
					"/set[@name=\"TriggerChain\"]"                       +
					"/array[@name=\"triggers\"]"                         +
					"/set"                                               +
					"/set[@code=\"struct ActionAttackBySpecialWeapon\"]" +
					"/value[@name=\"unitClassToAttack\"]");
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

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
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
			this.property_tree_ = new System.Windows.Forms.TreeView();
			this.property_label_ = new System.Windows.Forms.Label();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.property_panel_.SuspendLayout();
			this.SuspendLayout();
			// 
			// display_pnl_
			// 
			this.display_pnl_.AutoScroll = true;
			this.display_pnl_.BackColor = System.Drawing.SystemColors.WindowFrame;
			this.display_pnl_.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.display_pnl_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.display_pnl_.Location = new System.Drawing.Point(211, 8);
			this.display_pnl_.Name = "display_pnl_";
			this.display_pnl_.Size = new System.Drawing.Size(365, 461);
			this.display_pnl_.TabIndex = 0;
			this.display_pnl_.Click += new System.EventHandler(this.display_pnl_Click);
			// 
			// property_panel_
			// 
			this.property_panel_.Controls.Add(this.property_tree_);
			this.property_panel_.Controls.Add(this.property_label_);
			this.property_panel_.Dock = System.Windows.Forms.DockStyle.Left;
			this.property_panel_.Location = new System.Drawing.Point(8, 8);
			this.property_panel_.Name = "property_panel_";
			this.property_panel_.Size = new System.Drawing.Size(200, 461);
			this.property_panel_.TabIndex = 1;
			// 
			// property_tree_
			// 
			this.property_tree_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.property_tree_.ImageIndex = -1;
			this.property_tree_.Location = new System.Drawing.Point(0, 32);
			this.property_tree_.Name = "property_tree_";
			this.property_tree_.SelectedImageIndex = -1;
			this.property_tree_.Size = new System.Drawing.Size(200, 429);
			this.property_tree_.TabIndex = 1;
			// 
			// property_label_
			// 
			this.property_label_.Dock = System.Windows.Forms.DockStyle.Top;
			this.property_label_.Location = new System.Drawing.Point(0, 0);
			this.property_label_.Name = "property_label_";
			this.property_label_.Size = new System.Drawing.Size(200, 32);
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
			this.Load += new System.EventHandler(this.MainForm_Load);
			this.property_panel_.ResumeLayout(false);
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
		
		private void display_pnl_Click(object sender, System.EventArgs e)
		{
			int index = display_pnl_.GetTriggerAtPoint(Cursor.Position);
			if (index < 0)
				return;
			Trigger cell = triggers_[index];
			// display cell info in the property tree
			property_label_.Text = cell.name;
			property_tree_.Nodes.Clear();
			if (null != cell.action)
				property_tree_.Nodes.Add(cell.action.GetTreeNode());
			if (null != cell.condition)
				property_tree_.Nodes.Add(cell.condition.GetTreeNode());
			property_tree_.ExpandAll();
		}

		private void MainForm_Load(object sender, System.EventArgs e)
		{
			//GetStats();
			// get path
			string path;
			if (0 != args_.Length)
				path = args_[0];
			else
			{
				OpenFileDialog dlg = new OpenFileDialog();
				dlg.Filter = "trigger file (xml made from scr)|*xml";
				if (dlg.ShowDialog() != DialogResult.OK)
					throw new Exception();
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
					triggers_[i].color_ = Brushes.Orange;
					// read coordinates
					triggers_[i].X = int.Parse(position.SelectSingleNode(
						"set[@name=\"cellIndex\"]/int[@name=\"x\"]").InnerText);
					triggers_[i].Y = int.Parse(position.SelectSingleNode(
						"set[@name=\"cellIndex\"]/int[@name=\"y\"]").InnerText);
					// reda name
					triggers_[i].name = position.SelectSingleNode(
						"string[@name=\"name\"]").InnerText;
					// read action
					XmlNode action = position.SelectSingleNode("set[@name=\"action\"]");
					if (null != action)
						triggers_[i].action = new Property(action);
					// read condition
					XmlNode condition = position.SelectSingleNode("set[@name=\"condition\"]");
					if (null != condition)
						triggers_[i].condition = new Property(condition);
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

		#endregion

		#region data

		private Trigger[] triggers_;
		private string[]    args_;

		#endregion

		#region Component Designer data

		private System.ComponentModel.Container components = null;
		private TriggerDisplay                display_pnl_;
		private System.Windows.Forms.Splitter splitter1;
		private System.Windows.Forms.TreeView property_tree_;
		private System.Windows.Forms.Panel    property_panel_;
		private System.Windows.Forms.Label    property_label_;

		#endregion
	}
}
