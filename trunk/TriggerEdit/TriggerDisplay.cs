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
	#region nested types

	public enum Mode
	{
		Grid,
		Free
	}

	#endregion

	/// <summary>
	/// Summary description for TriggerDisplay.
	/// </summary>
	public class TriggerDisplay : System.Windows.Forms.Panel
	{
		#region interface

		public TriggerDisplay()
		{
			// This call is required by the Windows.Forms Form Designer.
			InitializeComponent();
			// initialize variables
			mode_ = Mode.Grid;
			cell_size_ = new Size(Font.Height * 6, Font.Height + 4);
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
				triggers_[i].X -= min.X - 1;
				triggers_[i].Y -= min.Y - 1;
			}
			// calculate grid size
			grid_size_ = new Size(max.X - min.X + 3, max.Y - min.Y + 3);
			// draw
			InitGraphics();
			AutoScrollMinSize = bmp_.Size;
			FormImage();
		}

		public void SwithMode(Mode mode)
		{
			mode_ = mode;
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

		private void DrawGrid()
		{
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
			Size new_size = new Size(
				Math.Max(ClientSize.Width,  grid_size_.Width  * cell_size_.Width),
				Math.Max(ClientSize.Height, grid_size_.Height * cell_size_.Height));
			if (null != bmp_ && bmp_.Size == new_size)
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

		#region overrides

		protected override void OnMouseMove(MouseEventArgs e)
		{
			base.OnMouseMove(e);
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

		protected override void OnPaint(PaintEventArgs e)
		{
			if (DesignMode || null == triggers_)
			{
				base.OnPaint(e);
				return;
			}
			e.Graphics.DrawImage(bmp_, AutoScrollPosition);
		}

		protected override void OnPaintBackground(PaintEventArgs pevent)
		{
			if (DesignMode || null == triggers_)
				base.OnPaintBackground(pevent);
		}

		protected override void OnSizeChanged(EventArgs e)
		{
			base.OnSizeChanged(e);
			InitGraphics();
			if (null != triggers_)
				FormImage();
			// set scrollbars
			if (null != bmp_)
			{
				Size new_scroll_size = bmp_.Size;
				if (ClientSize.Width >= bmp_.Width)
					new_scroll_size.Width = 0;
				if (ClientSize.Height >= bmp_.Height)
					new_scroll_size.Height = 0;
				AutoScrollMinSize = new_scroll_size;
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
			components = new System.ComponentModel.Container();
		}
		#endregion
		
		#region Component Designer data

		private System.ComponentModel.Container components = null;

		#endregion

		#region data

		private Bitmap    bmp_;
		private Size      cell_size_;
		private Graphics  gfx_;
		private Size      grid_size_;
		private Mode      mode_;
		private int       selection_ = -1;
		private Trigger[] triggers_;

		#endregion
	}
}
