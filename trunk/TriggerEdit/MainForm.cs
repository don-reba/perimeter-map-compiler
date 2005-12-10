using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Xml;
using System.Windows.Forms;

namespace TriggerEdit
{
	public class MainForm : System.Windows.Forms.Form
	{
		/// <summary>
		// nested types
		/// </summary>

		struct Cell
		{
			public int       X;
			public int       Y;
			public string    name;
			public Property  action;
			public Property  condition;
			public ArrayList links;
			public struct Link
			{
				public int   target;
				public Color color;
			};
		}

		class CellComparer : IComparer
		{
			#region IComparer Members

			public int Compare(object x, object y)
			{
				Cell cell1 = (Cell)x;
				Cell cell2 = (Cell)y;
				if (cell1.X == cell2.X)
					return cell1.Y - cell2.Y;
				else
					return cell1.X - cell2.X;
			}

			#endregion

		}


		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
		private System.Windows.Forms.Panel display_pnl_;
		private System.Windows.Forms.Splitter splitter1;
		private System.Windows.Forms.TreeView property_tree_;
		private System.Windows.Forms.Panel property_panel_;
		private System.Windows.Forms.Label property_label_;

		/// <summary>
		/// data
		/// </summary>
		private Cell[] cells_;
		private Size   grid_size_;

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

		public MainForm(string[] args)
		{
			// Required for Windows Form Designer support
			InitializeComponent();
			//GetStats();
			// get path
			string path;
			if (0 != args.Length)
				path = args[0];
			else
			{
				OpenFileDialog dlg = new OpenFileDialog();
				dlg.Filter = "trigger file (xml made from scr)|*xml";
				if (dlg.ShowDialog() != DialogResult.OK)
					throw new Exception();
				this.BringToFront();
				this.Focus();
				path = dlg.FileName;
			}
			// load the XML file
			XmlTextReader reader = new XmlTextReader(path);
			XmlDocument doc = new XmlDocument();
			doc.Load(reader);
			reader.Close();
			// extract data
			{
				XmlNodeList positions = doc.SelectNodes(
					"//script"                     +
					"/set[@name=\"TriggerChain\"]" +
					"/array[@name=\"triggers\"]"   +
					"/set");
				cells_ = new Cell[positions.Count];
				Hashtable names = new Hashtable();
				for (int i = 0; i != positions.Count; ++i)
				{
					XmlNode position = positions[i];
					// read coordinates
					cells_[i].X = int.Parse(position.SelectSingleNode(
						"set[@name=\"cellIndex\"]/int[@name=\"x\"]").InnerText);
					cells_[i].Y = int.Parse(position.SelectSingleNode(
						"set[@name=\"cellIndex\"]/int[@name=\"y\"]").InnerText);
					// reda name
					cells_[i].name = position.SelectSingleNode(
						"string[@name=\"name\"]").InnerText;
					// read action
					XmlNode action = position.SelectSingleNode("set[@name=\"action\"]");
					if (null != action)
						cells_[i].action = new Property(action);
					// read condition
					XmlNode condition = position.SelectSingleNode("set[@name=\"condition\"]");
					if (null != condition)
						cells_[i].condition = new Property(condition);
					names.Add(cells_[i].name, i);
				}
				// add links
				for (int i = 0; i != positions.Count; ++i)
				{
					XmlNodeList link_nodes = positions[i].SelectNodes(
						"array[@name=\"outcomingLinks\"]/set");
					cells_[i].links = new ArrayList();
					foreach (XmlNode node in link_nodes)
					{
						Cell.Link link;
						link.target = (int)names[node.SelectSingleNode("string[@name=\"triggerName\"]").InnerText];
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
						cells_[i].links.Add(link);
					}
				}
				Array.Sort(cells_, new CellComparer());
			}
			// find bounds
			Point min = new Point(int.MaxValue, int.MaxValue);
			Point max = new Point(int.MinValue, int.MinValue);
			foreach (Cell cell in cells_)
			{
				if (cell.X < min.X)
					min.X = cell.X;
				if (cell.X > max.X)
					max.X = cell.X;
				if (cell.Y < min.Y)
					min.Y = cell.Y;
				if (cell.Y > max.Y)
					max.Y = cell.Y;
			}
			// normalize positions
			for (int i = 0; i != cells_.Length; ++i)
			{
				cells_[i].X -= min.X;
				cells_[i].Y -= min.Y;
			}
			// calculate grid size
			grid_size_ = new Size(max.X - min.X + 1, max.Y - min.Y + 1);
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

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.display_pnl_ = new System.Windows.Forms.Panel();
			this.property_panel_ = new System.Windows.Forms.Panel();
			this.property_tree_ = new System.Windows.Forms.TreeView();
			this.property_label_ = new System.Windows.Forms.Label();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.property_panel_.SuspendLayout();
			this.SuspendLayout();
			// 
			// display_pnl_
			// 
			this.display_pnl_.BackColor = System.Drawing.SystemColors.WindowFrame;
			this.display_pnl_.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.display_pnl_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.display_pnl_.Location = new System.Drawing.Point(211, 8);
			this.display_pnl_.Name = "display_pnl_";
			this.display_pnl_.Size = new System.Drawing.Size(365, 461);
			this.display_pnl_.TabIndex = 0;
			this.display_pnl_.Click += new System.EventHandler(this.display_pnl_Click);
			this.display_pnl_.Resize += new System.EventHandler(this.display_pnl_Resize);
			this.display_pnl_.Paint += new System.Windows.Forms.PaintEventHandler(this.display_pnl_Paint);
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
			this.property_label_.Text = "Label";
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
			try
			{
				Application.Run(new MainForm(args));
			}
			catch (Exception e)
			{
				Debug.WriteLine("Error:");
				Debug.WriteLine(e.Message);
			}
		}
		#endregion

		private Rectangle GetRectAtCoords(int x, int y)
		{
			Size dimensions = display_pnl_.Size;
			Point min = new Point(
				dimensions.Width  * x / grid_size_.Width,
				dimensions.Height * y / grid_size_.Height);
			Point max = new Point(
				dimensions.Width  * (x + 1) / grid_size_.Width,
				dimensions.Height * (y + 1) / grid_size_.Height);
			return new Rectangle(
				min.X,
				min.Y,
				max.X - min.X,
				max.Y - min.Y);
		}

		private Point HitTest(int x, int y)
		{
			return new Point(
				x * grid_size_.Width  / display_pnl_.Width,
				y * grid_size_.Height / display_pnl_.Height);
		}

		private Point HitTest(Point point)
		{
			return HitTest(point.X, point.Y);
		}

		Point Center(Rectangle rect)
		{
			return new Point(
				rect.Left + (rect.Right  - rect.Left) / 2,
				rect.Top  + (rect.Bottom - rect.Top) / 2);
		}

		private void display_pnl_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
		{
			e.Graphics.SmoothingMode = SmoothingMode.None;
			Pen   grid_pen   = new Pen(Color.Yellow);
			Pen   arrow_pen  = new Pen(Color.Red);
			Brush cell_brush = new SolidBrush(Color.Orange);
			arrow_pen.StartCap = LineCap.RoundAnchor;
			arrow_pen.CustomEndCap = new AdjustableArrowCap(4, 6, true);
			// gill the surface
			e.Graphics.Clear(Color.Black);
			// draw cells
			foreach (Cell cell in cells_)
				e.Graphics.FillRectangle(cell_brush, GetRectAtCoords(cell.X, cell.Y));
			// draw horizontal lines
			if (0 != grid_size_.Height)
			{
				Point s = new Point(0, 0);
				Point f = new Point(display_pnl_.Width, 0);
				for (int i = 1; i != grid_size_.Height; ++i)
				{
					s.Y = f.Y = display_pnl_.Height * i / grid_size_.Height;
					e.Graphics.DrawLine(grid_pen, s, f);
				}
			}
			// draw vertical lines
			if (0 != grid_size_.Width)
			{
				Point s = new Point(0, 0);
				Point f = new Point(0, display_pnl_.Height);
				for (int i = 1; i != grid_size_.Width; ++i)
				{
					s.X = f.X = display_pnl_.Width * i / grid_size_.Width;
					e.Graphics.DrawLine(grid_pen, s, f);
				}
			}
			// draw dependency arrows
			e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
			foreach (Cell cell in cells_)
			{
				Point start = Center(GetRectAtCoords(cell.X, cell.Y));
				foreach (Cell.Link link in cell.links)
				{
					arrow_pen.Color = link.color;
					Cell target = cells_[link.target];
					e.Graphics.DrawLine(
						arrow_pen,
						start,
						Center(GetRectAtCoords(target.X, target.Y)));
				}
			}
			e.Graphics.SmoothingMode = SmoothingMode.None;
			// draw labels
			foreach (Cell cell in cells_)
				e.Graphics.DrawString(
					cell.name,
					display_pnl_.Font,
					SystemBrushes.WindowText,
					GetRectAtCoords(cell.X, cell.Y).Location);
		}

		private void display_pnl_Resize(object sender, System.EventArgs e)
		{
			display_pnl_.Invalidate();
		}

		private void display_pnl_Click(object sender, System.EventArgs e)
		{
			Point hit = HitTest(display_pnl_.PointToClient(Cursor.Position));
			Cell dummy = new Cell();
			dummy.X = hit.X;
			dummy.Y = hit.Y;
			int index = Array.BinarySearch(cells_, dummy, new CellComparer());
			if (index < 0)
				return;
			Cell cell = cells_[index];
			property_label_.Text = cell.name;
			property_tree_.Nodes.Clear();
			if (null != cell.action)
				property_tree_.Nodes.Add(cell.action.GetTreeNode());
			if (null != cell.condition)
				property_tree_.Nodes.Add(cell.condition.GetTreeNode());
			property_tree_.ExpandAll();
		}
	}
}
