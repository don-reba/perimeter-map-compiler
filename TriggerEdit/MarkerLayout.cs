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
			line_count_   = 2;
			line_height_  = font.Height;
			text_padding_ = new Size(2, 1);
			text_offset_  = new Size(0, 0);
		}

		public int LineCount
		{
			get { return line_count_; }
			set { line_count_ = value; }
		}

		public Size TextOffset
		{
			get
			{
				text_offset_.Width  = text_padding_.Width;
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
			get { return (int)(Height * 3.236f); } // 2 * golden ratio
		}

		public int Height
		{
			get { return line_height_ * line_count_ + 2 * text_padding_.Height; }
		}

		public Size Size
		{
			get { return new Size(Width, Height); }
		}

		#endregion

		#region data

		private int  line_count_;
		private int  line_height_;
		private Size text_padding_;
		private Size text_offset_;

		#endregion
	}
}
