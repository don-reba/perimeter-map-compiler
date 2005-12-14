using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace TriggerEdit
{	/// <summary>
	/// Summary description for TriggerDisplay.
	/// </summary>
	public class TriggerDisplay : System.Windows.Forms.Panel
	{
		#region nested types

		public enum Mode
		{
			Grid,
			Free
		}

		#endregion

		#region interface

		public TriggerDisplay()
		{
			// This call is required by the Windows.Forms Form Designer.
			InitializeComponent();
			// initialize variables
			mode_ = Mode.Free;
			Font = new Font(Font.FontFamily, 7);
			cell_size_ = new Size(Font.Height * 6, Font.Height * 2 + 4);
		}

		public void SetTriggers(ref Trigger[] triggers)
		{
			layout_timer_.Stop();
			triggers_ = triggers;
			// initialize the adjacency matrix
			trigger_adj_ = new ArrayList(triggers_.Length);
			for (int i = 0; i != triggers_.Length; ++i)
				trigger_adj_.Add(new BitArray(triggers_.Length));
			for (int i = 0; i != triggers_.Length; ++i)
			{
				foreach (Trigger.Link link in triggers_[i].links)
				{
					((BitArray)trigger_adj_[i])[link.target] = true;
					((BitArray)trigger_adj_[link.target])[i] = true;
				}
			}
			if	(Mode.Grid == mode_)
			{
				// find bounds
				Point min = new Point(int.MaxValue, int.MaxValue);
				Point max = new Point(int.MinValue, int.MinValue);
				foreach (Trigger trigger in triggers_)
				{
					if (trigger.X < min.X)
						min.X = trigger.X;
					if (trigger.X > max.X)
						max.X = trigger.X;
					if (trigger.Y < min.Y)
						min.Y = trigger.Y;
					if (trigger.Y > max.Y)
						max.Y = trigger.Y;
				}
				// normalize positions
				for (int i = 0; i != triggers_.Length; ++i)
				{
					triggers_[i].X -= min.X - 1;
					triggers_[i].Y -= min.Y - 1;
				}
				// calculate grid size
				grid_size_ = new Size(max.X - min.X + 3, max.Y - min.Y + 3);
			} 
			else if (Mode.Free == mode_)
			{
				Random rand = new Random(0);
				bounds_ = new Rectangle(
					0,
					0,
					(cell_size_.Width  + cell_spacing_) * ((int)Math.Sqrt(triggers_.Length) + 1) * 2,
					(cell_size_.Height + cell_spacing_) * ((int)Math.Sqrt(triggers_.Length) + 1) * 2);
				bounds_.Inflate(cell_size_);
				bounds_ = Rectangle.Union(bounds_, ClientRectangle);
				for (int i = 0; i != triggers_.Length; ++i)
				{
					triggers_[i].X = cell_size_.Width  / 2 + rand.Next(bounds_.Width  - cell_size_.Width);
					triggers_[i].Y = cell_size_.Height / 2 + rand.Next(bounds_.Height - cell_size_.Height);
				}
				layout_timer_.Start();
			}
			// draw
			InitGraphics();
			AutoScrollMinSize = bmp_.Size;
			FormImage();
		}

		public void SwithMode(Mode mode)
		{
			mode_ = mode;
		}

		public int GetTriggerAtPoint(Point point)
		{
			Point hit = HitTest(PointToClient(Cursor.Position));
			Trigger dummy = new Trigger();
			dummy.X = hit.X;
			dummy.Y = hit.Y;
			return Array.BinarySearch(triggers_, dummy, new TriggerComparer());
		}

		#endregion

		#region internal implementation

		private void SnapPoints(ref Point pnt1, ref Point pnt2)
		{
			// initialize the rectangles
			Rectangle rect1 = new Rectangle(0, 0, cell_size_.Width, cell_size_.Height);
			Rectangle rect2 = rect1;
			rect2.Offset(pnt2.X - pnt1.X, pnt2.Y - pnt1.Y);
			// snap logic
			if (rect1.Bottom < rect2.Top)
			{
				if (rect1.Right < rect2.Left)
				{
					pnt1.X += cell_size_.Width  / 2;
					pnt1.Y += cell_size_.Height / 2;
					pnt2.X -= cell_size_.Width  / 2;
					pnt2.Y -= cell_size_.Height / 2;
				}
				else if (rect1.Left > rect2.Right)
				{
					pnt1.X -= cell_size_.Width  / 2;
					pnt1.Y += cell_size_.Height / 2;
					pnt2.X += cell_size_.Width  / 2;
					pnt2.Y -= cell_size_.Height / 2;
				}
				else
				{
					pnt1.Y += cell_size_.Height / 2;
					pnt2.Y -= cell_size_.Height / 2;
				}
			}
			else if (rect1.Top > rect2.Bottom)
			{
				if (rect1.Right < rect2.Left)
				{
					pnt1.X += cell_size_.Width  / 2;
					pnt1.Y -= cell_size_.Height / 2;
					pnt2.X -= cell_size_.Width  / 2;
					pnt2.Y += cell_size_.Height / 2;
				}
				else if (rect1.Left > rect2.Right)
				{
					pnt1.X -= cell_size_.Width  / 2;
					pnt1.Y -= cell_size_.Height / 2;
					pnt2.X += cell_size_.Width  / 2;
					pnt2.Y += cell_size_.Height / 2;
				}
				else
				{
					pnt1.Y -= cell_size_.Height / 2;
					pnt2.Y += cell_size_.Height / 2;
				}
			}
			else
			{
				if (rect1.Right < rect2.Left)
				{
					pnt1.X += cell_size_.Width / 2;
					pnt2.X -= cell_size_.Width / 2;
				}
				else if (rect1.Left > rect2.Right)
				{
					pnt1.X -= cell_size_.Width / 2;
					pnt2.X += cell_size_.Width / 2;
				}
			}
		}

		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if( components != null )
					components.Dispose();
			}
			base.Dispose( disposing );
		}

		private void DrawGrid()
		{
			gfx_.SmoothingMode = SmoothingMode.None;
			Pen   grid_pen   = new Pen(Color.Yellow);
			Pen   arrow_pen  = new Pen(Color.Red);
			arrow_pen.StartCap = LineCap.RoundAnchor;
			arrow_pen.CustomEndCap = new AdjustableArrowCap(4, 6, true);
			// fill the surface
			gfx_.Clear(Color.Black);
			// draw cells
			foreach (Trigger trigger in triggers_)
				gfx_.FillRectangle(trigger.color_, GetRectAtCoords(trigger.X, trigger.Y));
			// draw horizontal lines
			if (0 != grid_size_.Height)
			{
				Point s = new Point(0, 0);
				Point f = new Point(bmp_.Width, 0);
				for (int i = 1; i != grid_size_.Height; ++i)
				{
					s.Y = f.Y = bmp_.Height * i / grid_size_.Height;
					gfx_.DrawLine(grid_pen, s, f);
				}
			}
			// draw vertical lines
			if (0 != grid_size_.Width)
			{
				Point s = new Point(0, 0);
				Point f = new Point(0, bmp_.Height);
				for (int i = 1; i != grid_size_.Width; ++i)
				{
					s.X = f.X = bmp_.Width * i / grid_size_.Width;
					gfx_.DrawLine(grid_pen, s, f);
				}
			}
			// draw dependency arrows
			gfx_.SmoothingMode = SmoothingMode.AntiAlias;
			foreach (Trigger trigger in triggers_)
			{
				Point start = Center(GetRectAtCoords(trigger.X, trigger.Y));
				foreach (Trigger.Link link in trigger.links)
				{
					arrow_pen.Color = link.color;
					arrow_pen.Width = link.is_active ? 2 : 0;
					Trigger target = triggers_[link.target];
					gfx_.DrawLine(
						arrow_pen,
						start,
						Center(GetRectAtCoords(target.X, target.Y)));
				}
			}
			gfx_.SmoothingMode = SmoothingMode.None;
			// draw labels
			foreach (Trigger trigger in triggers_)
			{
				Rectangle rect = GetRectAtCoords(trigger.X, trigger.Y);
				rect.Offset(4, 2);
				if (rect.Width > 8)
					rect.Width -= 8;
				if (rect.Height > 4)
					rect.Height -= 4;
				gfx_.DrawString(trigger.name, Font, SystemBrushes.WindowText, rect);
			}
		}

		private void DrawFree()
		{
			Pen   arrow_pen  = new Pen(Color.Red);
			arrow_pen.StartCap = LineCap.RoundAnchor;
			arrow_pen.CustomEndCap = new AdjustableArrowCap(4, 6, true);
			// fill the surface
			gfx_.Clear(Color.Black);
			// draw cells
			foreach (Trigger trigger in triggers_)
				gfx_.FillRectangle(
					Brushes.Orange,
					trigger.X - cell_size_.Width  / 2,
					trigger.Y - cell_size_.Height / 2,
					cell_size_.Width,
					cell_size_.Height);
			// draw dependency arrows
			gfx_.SmoothingMode = SmoothingMode.AntiAlias;
			foreach (Trigger trigger in triggers_)
				foreach (Trigger.Link link in trigger.links)
				{
					arrow_pen.Color = link.color;
					arrow_pen.Width = link.is_active ? 2 : 0;
					Trigger target = triggers_[link.target];
					Point s = new Point(trigger.X, trigger.Y);
					Point f = new Point(target.X,  target.Y);
					SnapPoints(ref s, ref f);
					gfx_.DrawLine(arrow_pen, s, f);
				}
			gfx_.SmoothingMode = SmoothingMode.None;
			// draw labels
			foreach (Trigger trigger in triggers_)
			{
				Rectangle rect = new Rectangle(
					trigger.X - cell_size_.Width  / 2,
					trigger.Y - cell_size_.Height / 2,
					cell_size_.Width,
					cell_size_.Height);
				rect.Inflate(-2, -1);
				gfx_.DrawString(trigger.name, Font, SystemBrushes.WindowText, rect);
			}
		}

		private void FormImage()
		{
			if (null == gfx_)
				InitGraphics();
			switch (mode_)
			{
				case Mode.Free: DrawFree(); break;
				case Mode.Grid: DrawGrid(); break;
			}
		}
		
		private void InitGraphics()
		{
			// calculate the new size
			Size new_size;
			if (null == bmp_ || bounds_.Width > bmp_.Width || bounds_.Height > bmp_.Height)
				new_size = bounds_.Size;
			else
				return;
			// release resources
			if (null != gfx_)
				gfx_.Dispose();
			if (null != bmp_)
				bmp_.Dispose();
			// initialize resources
			bmp_ = new Bitmap(new_size.Width, new_size.Height);
			gfx_ = Graphics.FromImage(bmp_);
		}

		#endregion

		#region event handlers

		private void layout_timer__Tick(object sender, System.EventArgs e)
		{
			float k = cell_spacing_;
			float range = (Math.Max(cell_size_.Width, cell_size_.Height) + k) * 3;
			Rectangle new_bounds = new Rectangle(0, 0, 0, 0);
			bool new_bounds_initialized = false;
			// calculate forces
			for (int i = 0; i != triggers_.Length; ++i)
			{
				float v_x = 0;
				float v_y = 0;
				BitArray adj = (BitArray)trigger_adj_[i];
				for (int j = 0; j != triggers_.Length; ++j)
					if (i != j)
						if (adj[j])
						{
							// attraction
							float delta_x   = triggers_[j].X - triggers_[i].X;
							float delta_y   = triggers_[j].Y - triggers_[i].Y;
							float dx = Math.Max(0, Math.Abs(delta_x) - cell_size_.Width);
							float dy = Math.Max(0, Math.Abs(delta_y) - cell_size_.Height);
							float delta_abs = (float)Math.Sqrt(dx * dx + dy * dy);
							if (delta_abs > k)
							{
								float ratio = delta_abs / k;
								delta_x *= ratio;
								delta_y *= ratio;
								v_x += delta_x;
								v_y += delta_y;
							}
							else if (i < j)
								v_x += cell_size_.Width + k;
						}
						else
						{
							// repulsion
							float delta_x       = triggers_[i].X - triggers_[j].X;
							float delta_y       = triggers_[i].Y - triggers_[j].Y;
							float dx = Math.Max(0, Math.Abs(delta_x) - cell_size_.Width);
							float dy = Math.Max(0, Math.Abs(delta_y) - cell_size_.Height);
							float delta_abs_sqr = dx * dx + dy * dy;
							if (delta_abs_sqr > 0 && delta_abs_sqr < range * range)
							{
								float ratio = k * k / delta_abs_sqr;
								delta_x *= ratio;
								delta_y *= ratio;
								v_x += delta_x;
								v_y += delta_y;
							}
						}
				triggers_[i].v_x_ = v_x;
				triggers_[i].v_y_ = v_y;
			}
			// displace vertices
			float temperature = k * 4 / temperature_;
			for (int i = 0; i != triggers_.Length; ++i)
			{
				float v_x = triggers_[i].v_x_;
				float v_y = triggers_[i].v_y_;
				float v_abs = (float)Math.Sqrt(v_x * v_x + v_y * v_y);
				if (v_abs != 0)
				{
					// calculate new coordinates
					float x = triggers_[i].X + (v_x / v_abs) * Math.Min(v_abs, temperature);
					float y = triggers_[i].Y + (v_y / v_abs) * Math.Min(v_abs, temperature);
					// set the new coordinates
					triggers_[i].X = (int)Math.Round(x);
					triggers_[i].Y = (int)Math.Round(y);
				}
				// adjust the bounding rectangle
				{
					Rectangle point_rect = new Rectangle(triggers_[i].X, triggers_[i].Y, 0, 0);
					if (new_bounds_initialized)
						new_bounds = Rectangle.Union(new_bounds, point_rect);
					else
					{
						new_bounds = point_rect;
						new_bounds_initialized = true;
					}
				}
			}
			// cool down, with simmering
			if (temperature <= 2)
				temperature_ *= 1.01f;
			else if (temperature <= 4)
				temperature_ *= 1.05f;
			else
				temperature_ *= 1.1f;
			// normalize the vertices
			for (int i = 0; i != triggers_.Length; ++i)
			{
				triggers_[i].X -= new_bounds.Left - cell_size_.Width  / 2;
				triggers_[i].Y -= new_bounds.Top  - cell_size_.Height / 2;
			}
			// adjust the new bounding rectangle to enclose the cells
			// assume the cell has even dimensions
			new_bounds.Inflate(cell_size_.Width / 2, cell_size_.Height / 2);
			// reposition the viewport, if necessary
			{
				Point delta = new Point(
					new_bounds.Left - bounds_.Left,
					new_bounds.Top  - bounds_.Top);
				AutoScrollMinSize = bounds_.Size;
				if (AutoScroll)
				{
					Point position = AutoScrollPosition;
					if (delta.X > 0)
						position.Offset(delta.X, 0);
					if (delta.Y > 0)
						position.Offset(0, delta.Y);
					position.X = -position.X;
					position.Y = -position.Y;
					AutoScrollPosition = position;
				}
			}
			// commit the new bounds
			bounds_ = new_bounds;
			// adjust the bitmap
			InitGraphics();
			// redraw
			FormImage();
			Invalidate();
		}

		protected override void OnMouseMove(MouseEventArgs e)
		{
			base.OnMouseMove(e);
			if (Mode.Grid == mode_)
			{
				if (DesignMode || null == triggers_)
					return;
				// find the cell under cursor
				int index = GetTriggerAtPoint(Cursor.Position);
				if (index == selection_)
					return;
				// unhighlight the previous cell and its children
				Rectangle rect;
				if (selection_ >= 0)
				{
					triggers_[selection_].color_ = Brushes.Orange;
					rect = GetRectAtCoords(
						triggers_[selection_].X,
						triggers_[selection_].Y);
					rect.Offset(AutoScrollPosition);
					Invalidate(rect);
					foreach (Trigger.Link link in triggers_[selection_].links)
					{
						triggers_[link.target].color_ = Brushes.Orange;
						rect = GetRectAtCoords(
							triggers_[link.target].X,
							triggers_[link.target].Y);
						rect.Offset(AutoScrollPosition)  ;
						Invalidate(rect);
					}
				}
				selection_ = index;
				if (index < 0)
				{
					FormImage();
					return;
				}
				// highlight the selected trigger and its dependants
				triggers_[index].color_ = Brushes.Yellow;
				rect = GetRectAtCoords(
					triggers_[index].X,
					triggers_[index].Y);
				rect.Offset(AutoScrollPosition);
				Invalidate(rect);
				foreach (Trigger.Link link in triggers_[index].links)
				{
					triggers_[link.target].color_ = Brushes.Gold;
					rect = GetRectAtCoords(
						triggers_[link.target].X,
						triggers_[link.target].Y);
					rect.Offset(AutoScrollPosition);
					Invalidate(rect);
				}
				// redraw
				FormImage();
			}
		}

		protected override void OnPaint(PaintEventArgs e)
		{
			if (DesignMode || null == triggers_)
			{
				base.OnPaint(e);
				return;
			}
			// calculate the difference between the size of the client rectangle and the bounds
			Size delta = new Size(
				ClientRectangle.Width  - bounds_.Width,
				ClientRectangle.Height - bounds_.Height);
			// calculate image placement
			Point origin = new Point(
				(delta.Width  > 0) ? (delta.Width  / 2) : AutoScrollPosition.X,
				(delta.Height > 0) ? (delta.Height / 2) : AutoScrollPosition.Y);
			// draw the image
			e.Graphics.DrawImage(bmp_, origin);
			// draw black borders, if necessary
			if (delta.Width > 0)
			{
				// left
				e.Graphics.FillRectangle(
					Brushes.Black,
					0,
					0,
					origin.X,
					ClientRectangle.Height);
				// right
				e.Graphics.FillRectangle(
					Brushes.Black,
					ClientRectangle.Width - delta.Width / 2,
					0,
					delta.Width / 2,
					ClientRectangle.Height);
			}
			if (delta.Height > 0)
			{
				// top
				e.Graphics.FillRectangle(
					Brushes.Black,
					0,
					0,
					ClientRectangle.Width,
					origin.Y);
				// bottom
				e.Graphics.FillRectangle(
					Brushes.Black,
					0,
					ClientRectangle.Height - delta.Height / 2,
					ClientRectangle.Width,
					delta.Height / 2);
			}
		}

		protected override void OnPaintBackground(PaintEventArgs pevent)
		{
			if (DesignMode || null == triggers_)
				base.OnPaintBackground(pevent);
		}

		protected override void OnSizeChanged(EventArgs e)
		{
			base.OnSizeChanged(e);
			if (Mode.Grid == mode_)
			{
				InitGraphics();
				if (null != triggers_)
					FormImage();
			}
			Invalidate();
		}


		#endregion

		#region utilities

		Point Center(Rectangle rect)
		{
			return new Point(
				rect.Left + (rect.Right  - rect.Left) / 2,
				rect.Top  + (rect.Bottom - rect.Top) / 2);
		}

		private Rectangle GetRectAtCoords(int x, int y)
		{
			Point min = new Point(
				bmp_.Width  * x / grid_size_.Width,
				bmp_.Height * y / grid_size_.Height);
			Point max = new Point(
				bmp_.Width  * (x + 1) / grid_size_.Width,
				bmp_.Height * (y + 1) / grid_size_.Height);
			return new Rectangle(
				min.X,
				min.Y,
				max.X - min.X,
				max.Y - min.Y);
		}

		private Point HitTest(int x, int y)
		{
			x -= AutoScrollPosition.X;
			y -= AutoScrollPosition.Y;
			return new Point(
				x * grid_size_.Width  / bmp_.Width,
				y * grid_size_.Height / bmp_.Height);
		}

		private Point HitTest(Point point)
		{
			return HitTest(point.X, point.Y);
		}

		#endregion

		#region Component Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			this.layout_timer_ = new System.Windows.Forms.Timer(this.components);
			// 
			// layout_timer_
			// 
			this.layout_timer_.Interval = 200;
			this.layout_timer_.Tick += new System.EventHandler(this.layout_timer__Tick);

		}
		#endregion

		private System.ComponentModel.IContainer components;
		
		#region Component Designer data

		private System.Windows.Forms.Timer      layout_timer_;

		#endregion

		#region data

		private Rectangle bounds_;
		private Bitmap    bmp_;
		private Size      cell_size_;
		private int       cell_spacing_ = 64;
		private Graphics  gfx_;
		private Size      grid_size_;
		private Mode      mode_;
		private int       selection_ = -1;
		private float     temperature_ = 1.0f;
		private ArrayList trigger_adj_;
		private Trigger[] triggers_;

		#endregion
	}
}
