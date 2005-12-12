using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace TriggerEdit
{
	/// <summary>
	/// Summary description for TriggerDisplay.
	/// </summary>
	public class TriggerDisplay : System.Windows.Forms.Panel
	{
		#region interface

		public TriggerDisplay(/*Trigger[] triggers*/)
		{
			// This call is required by the Windows.Forms Form Designer.
			InitializeComponent();
		}

		public void SetTriggers(ref Trigger[] triggers)
		{
			triggers_ = triggers;
			// find bounds
			Point min = new Point(int.MaxValue, int.MaxValue);
			Point max = new Point(int.MinValue, int.MinValue);
			foreach (Trigger cell in triggers_)
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
			for (int i = 0; i != triggers_.Length; ++i)
			{
				triggers_[i].X -= min.X;
				triggers_[i].Y -= min.Y;
			}
			// calculate grid size
			grid_size_ = new Size(max.X - min.X + 1, max.Y - min.Y + 1);
		}

		public enum Mode
		{
			Grid,
			Free
		}

		public void SwithMode(Mode mode)
		{
		}

		/// <summary>
		/// Returns index of the trigger to which the point in screen coordinates belongs.
		/// </summary>
		/// <param name="point">In screen corrdinates.</param>
		/// <returns>
		/// Trigger ID, or a negative value.
		/// See Array.BinarySearch for details.
		/// </returns>
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

		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if( components != null )
					components.Dispose();
			}
			base.Dispose( disposing );
		}

		#endregion

		#region overrides

		protected override void OnMouseMove(MouseEventArgs e)
		{
			base.OnMouseMove (e);
			if (DesignMode || null == triggers_)
				return;
			int index = GetTriggerAtPoint(Cursor.Position);
			if (index == selection_)
				return;
			// unhighlight the previous cell and its children
			if (selection_ >= 0)
			{
				triggers_[selection_].color_ = Brushes.Orange;
				Invalidate(GetRectAtCoords(triggers_[selection_].X, triggers_[selection_].Y));
				foreach (Trigger.Link link in triggers_[selection_].links)
				{
					triggers_[link.target].color_ = Brushes.Orange;
					Invalidate(GetRectAtCoords(triggers_[link.target].X, triggers_[link.target].Y));
				}
			}
			if (index < 0)
				return;
			Trigger trigger = triggers_[index];
			// highlight the selected trigger and its dependants
			foreach (Trigger.Link link in trigger.links)
			{
				triggers_[link.target].color_ = Brushes.Gold;
				Invalidate(GetRectAtCoords(triggers_[link.target].X, triggers_[link.target].Y));
			}
			triggers_[index].color_ = Brushes.Yellow;
			Invalidate(GetRectAtCoords(trigger.X, trigger.Y));
			selection_ = index;
		}

		protected override void OnPaint(PaintEventArgs e)
		{
			if (DesignMode || null == triggers_)
			{
				base.OnPaint(e);
				return;
			}
			if (null == gfx_)
				InitGraphics();
			gfx_.SmoothingMode = SmoothingMode.None;
			Pen   grid_pen   = new Pen(Color.Yellow);
			Pen   arrow_pen  = new Pen(Color.Red);
			arrow_pen.StartCap = LineCap.RoundAnchor;
			arrow_pen.CustomEndCap = new AdjustableArrowCap(4, 6, true);
			// gill the surface
			gfx_.Clear(Color.Black);
			// draw cells
			foreach (Trigger trigger in triggers_)
				gfx_.FillRectangle(trigger.color_, GetRectAtCoords(trigger.X, trigger.Y));
			// draw horizontal lines
			if (0 != grid_size_.Height)
			{
				Point s = new Point(0, 0);
				Point f = new Point(Width, 0);
				for (int i = 1; i != grid_size_.Height; ++i)
				{
					s.Y = f.Y = Height * i / grid_size_.Height;
					gfx_.DrawLine(grid_pen, s, f);
				}
			}
			// draw vertical lines
			if (0 != grid_size_.Width)
			{
				Point s = new Point(0, 0);
				Point f = new Point(0, Height);
				for (int i = 1; i != grid_size_.Width; ++i)
				{
					s.X = f.X = Width * i / grid_size_.Width;
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
				gfx_.DrawString(
					trigger.name,
					Font,
					SystemBrushes.WindowText,
					GetRectAtCoords(trigger.X, trigger.Y).Location);
			e.Graphics.DrawImage(bmp_, 0, 0);
		}

		protected override void OnPaintBackground(PaintEventArgs pevent)
		{
			if (DesignMode || null == triggers_)
				base.OnPaintBackground(pevent);
		}

		#endregion

		#region utilities

		Point Center(Rectangle rect)
		{
			return new Point(
				rect.Left + (rect.Right  - rect.Left) / 2,
				rect.Top  + (rect.Bottom - rect.Top) / 2);
		}
		
		private void InitGraphics()
		{
			bmp_ = new Bitmap(Width, Height);
			gfx_ = Graphics.FromImage(bmp_);
		}

		private Rectangle GetRectAtCoords(int x, int y)
		{
			Point min = new Point(
				Width  * x / grid_size_.Width,
				Height * y / grid_size_.Height);
			Point max = new Point(
				Width  * (x + 1) / grid_size_.Width,
				Height * (y + 1) / grid_size_.Height);
			return new Rectangle(
				min.X,
				min.Y,
				max.X - min.X,
				max.Y - min.Y);
		}

		private Point HitTest(int x, int y)
		{
			return new Point(
				x * grid_size_.Width  / Width,
				y * grid_size_.Height / Height);
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
			components = new System.ComponentModel.Container();
		}
		#endregion
		
		#region Component Designer data

		private System.ComponentModel.Container components = null;

		#endregion

		#region data

		private Trigger[] triggers_;
		private Size      grid_size_;
		private Bitmap    bmp_;
		private Graphics  gfx_;
		private int       selection_ = -1;
		Mode              mode_;

		#endregion
	}
}
