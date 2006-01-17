using System;
using System.Drawing;

namespace TriggerEdit
{
	/// <summary>
	/// Summary description for MarkerLayout.
	/// </summary>
	public class MarkerLayout
	{
		#region interface

		public MarkerLayout(Font font)
		{
			line_height_  = font.Height;
			text_padding_ = new Size(4, 2);
			text_offset_  = new Size(0, 0);
		}

		public Size TextOffset
		{
			get
			{
				text_offset_.Width  = text_padding_.Width + TearOffWidth;
				text_offset_.Height = text_padding_.Height;
				return text_offset_;
			}
		}

		public Size TextPadding
		{
			get { return text_padding_; }
		}

		public int Width
		{
			get { return line_height_ * 6 + 2 * text_padding_.Width; }
		}

		public int Height
		{
			get { return line_height_ * 2 + 2 * text_padding_.Height; }
		}

		public Size Size
		{
			get { return new Size(Width, Height); }
		}

		public int TearOffWidth
		{
			get { return 8; }
		}

		#endregion

		#region data

		private int line_height_;
		private Size text_padding_;
		private Size text_offset_;

		#endregion
	}
}
