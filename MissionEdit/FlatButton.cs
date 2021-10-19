using System;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms.Design;
using System.Drawing;
using System.Data;
using System.Windows.Forms;

namespace MissionEdit
{
	[Designer(typeof(ControlDesigner))]
	public class FlatButton : System.Windows.Forms.UserControl
	{
		//---------------
		// nested classes
		//---------------

		[Flags]
			enum State
		{
			highlighted = 0x1,
			pressed     = 0x2,
			selected    = 0x4
		}

		//----------
		// interface
		//----------

		public FlatButton()
		{
			InitializeComponent();

			back_brush_    = SystemBrushes.Window;
			fore_brush_    = SystemBrushes.WindowText;
			string_format_ = new StringFormat();

			string_format_.Alignment     = StringAlignment.Center;
			string_format_.LineAlignment = StringAlignment.Center;

			Color highlight1 = SystemColors.Window;
			Color highlight2 = SystemColors.ControlLight;
			highligh_brush_ = new SolidBrush(
				Color.FromArgb(
					(highlight1.R + highlight2.R) / 2,
					(highlight1.G + highlight2.G) / 2,
					(highlight1.B + highlight2.B) / 2
				)
			);
		}


		//----------
		// overrides
		//----------

		protected override void OnBackColorChanged(EventArgs e)
		{
			base.OnBackColorChanged (e);
			back_brush_ = new SolidBrush(BackColor);
		}

		protected override void OnForeColorChanged(EventArgs e)
		{
			base.OnForeColorChanged (e);
			fore_brush_ = new SolidBrush(ForeColor);
		}

		protected override void OnPaint(PaintEventArgs e)
		{
			Graphics gfx = e.Graphics;

			if (DesignMode)
			{
				if (0 != (state_ & State.selected))
					PaintSelected(gfx);
				else
					PaintDesign(gfx);
			}
			else
			{
				if (0 != (state_ & State.pressed))
					PaintPressed(gfx);
				else if (0 != (state_ & State.selected))
					PaintSelected(gfx);
				else if (0 != (state_ & State.highlighted))
					PaintHighlighted(gfx);
				else
					PaintPlain(gfx);
			}
		}

		protected override void OnMouseEnter(EventArgs e)
		{
			base.OnMouseEnter(e);
			state_ |= State.highlighted;
			Invalidate();
		}

		protected override void OnMouseLeave(EventArgs e)
		{
			base.OnMouseLeave(e);
			state_ &= ~State.highlighted;
			Invalidate();
		}

		protected override void OnMouseDown(MouseEventArgs e)
		{
			base.OnMouseDown(e);
			state_ |= State.pressed;
			Invalidate();
		}

		protected override void OnMouseUp(MouseEventArgs e)
		{
			base.OnMouseUp(e);
			state_ &= ~State.pressed;
			Invalidate();
		}


		protected override void OnResize(EventArgs e)
		{
			base.OnResize(e);
			Invalidate();
		}

		//---------------
		// implementation
		//---------------

		private void PaintPressed(Graphics gfx)
		{
			gfx.FillRectangle(
				SystemBrushes.ControlLight,
				ClientRectangle);
			gfx.DrawRectangle(
				SystemPens.ControlText,
				0,
				0,
				Width - 1,
				Height - 1);
			gfx.DrawString(
				Text,
				Font,
				SystemBrushes.ControlText,
				ClientRectangle,
				string_format_);
		}

		private void PaintSelected(Graphics gfx)
		{
			gfx.FillRectangle(
				SystemBrushes.Control,
				ClientRectangle);
			gfx.DrawString(
				Text,
				Font,
				SystemBrushes.ControlText,
				ClientRectangle,
				string_format_);
		}

		private void PaintHighlighted(Graphics gfx)
		{
			gfx.FillRectangle(
				highligh_brush_,
				ClientRectangle);
			gfx.DrawString(
				Text,
				Font,
				SystemBrushes.ControlText,
				ClientRectangle,
				string_format_);
		}

		private void PaintPlain(Graphics gfx)
		{
			gfx.FillRectangle(
				back_brush_,
				ClientRectangle);
			gfx.DrawString(Text,
				Font,
				SystemBrushes.ControlText,
				ClientRectangle,
				string_format_);
		}

		private void PaintDesign(Graphics gfx)
		{
			gfx.FillRectangle(
				back_brush_,
				ClientRectangle);
			gfx.DrawRectangle(
				SystemPens.ControlText,
				0,
				0,
				Width - 1,
				Height - 1);
			gfx.DrawString(Text,
				Font,
				SystemBrushes.ControlText,
				ClientRectangle,
				string_format_);
		}

		//-----------
		// properties
		//-----------
		
		[
		Browsable(true),
		DesignerSerializationVisibility(DesignerSerializationVisibility.Visible)
		]
		public new string Text
		{
			get
			{
				return base.Text;
			}
			set
			{
				base.Text = value;
				Invalidate();
			}
		}
		
		[
		Browsable(true),
		Category("Appearance"),
		DesignerSerializationVisibility(DesignerSerializationVisibility.Visible)
		]
		public bool Checked
		{
			get
			{
				return (0 != (state_ & State.selected));
			}
			set
			{
				if (value)
					state_ |= State.selected;
				else
					state_ &= ~State.selected;
				Invalidate();
			}
		}

		//-----
		// data
		//-----

		private Brush        back_brush_;
		private Brush        fore_brush_;
		private Brush        highligh_brush_;
		private State        state_;
		private StringFormat string_format_;

		//----------
		// generated
		//----------

		#region code

		private void InitializeComponent()
		{
		}

		#endregion
	}
}
