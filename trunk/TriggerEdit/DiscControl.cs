using System;
using System.Collections;
using System.Diagnostics;
using System.Drawing;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

namespace TriggerEdit
{
	public class DiscControl
	{
		//---------------
		// nested classes
		//---------------

		#region

		private struct ButtonInfo
		{
			public ButtonInfo(Image image, EventHandler click_event)
			{
				image_       = image;
				click_event_ = click_event;
			}
			public Image        image_;
			public EventHandler click_event_;
		}

		private struct ButtonPlacement
		{
			public float angle_;
			public float angle_start_;
			public int   highlight_vb_start_;
			public int   highlight_vb_end_;
			public float length_;
		}

		#endregion

		//----------
		// interface
		//----------

		#region

		public DiscControl(SizeF inside)
			:this(inside.Width, inside.Height)
		{}

		public DiscControl(float w, float h)
			:this((float)Math.Sqrt(w * w + h * h) / 2.0f)
		{}
		
		public DiscControl(float radius)
		{
			button_info_ = new ArrayList();
			is_visible_  = false;
			radius_      = radius;
			ring_        = 8.0f;
			h_space_     = 2.0f;
			v_space_     = 2.0f;
			selection_   = -1;
		}

		public void AddButton(Image image, EventHandler click_event)
		{
			button_info_.Add(new ButtonInfo(image, click_event));
		}

		/// <summary>
		/// Find the button at the coordinates.
		/// </summary>
		/// <returns>Number of the button or -1.</returns>
		public int HitTest(float x, float y)
		{
			// box test
			if (x > extent_ || x < -extent_ || y > extent_ || y < -extent_)
				return -1;
			// convert to polar
			float a, r;
			RectToPolar(x, y, out r, out a);
			// radius test
			if (r > extent_ || r < radius_)
				return -1;
			// per-button test
			r -= radius_ + ring_;
			for (int i = 0; i != button_placement_.Length; ++i)
			{
				ButtonPlacement p = button_placement_[i];
				if (r > p.length_)
					continue;
				float angle_end   = p.angle_start_ + p.angle_;
				if (a < p.angle_start_)
				{
					float two_pi = 2.0f * (float)Math.PI;
					if (angle_end <= two_pi || a > angle_end - two_pi)
						continue;
				}
				if (a > angle_end)
					continue;
				return i;
			}
			return -1;
		}

		public void Layout(Device device)
		{
			const float pi      = (float)Math.PI;
			const float max_arc = 2.0f;
			// store the number of buttons, for briefness
			int count = button_info_.Count;
			// calculate button sizes
			float occupied_angle = 0.0f;
			cell_w_ = 0;
			cell_h_ = 0;
			button_placement_ = new ButtonPlacement[count];
			for (int i = 0; i != count; ++i)
			{
				ButtonInfo info = (ButtonInfo)button_info_[i];
				// calculate maximum icon size
				if (info.image_.Width > cell_w_)
					cell_w_ = info.image_.Width;
				if (info.image_.Height > cell_h_)
					cell_h_ = info.image_.Height;
				// calculate button size
				float w = (float)info.image_.Width + 2.0f * h_space_;
				float h = (float)info.image_.Height + v_space_;
				float a = 2.0f * (float)Math.Atan2(w / 2.0f, radius_);
				float l = (float)Math.Sqrt((radius_ + h) * (radius_ + h) + w * w / 4.0f) - radius_;
				button_placement_[i].angle_ = a;
				button_placement_[i].length_ = l;
				if (l + radius_ + ring_ > extent_)
					extent_ = l + radius_ + ring_;
				occupied_angle += a;
			}
			// calculate the layout of the button icon atlas texture
			// wx = hy
			// xy = count
			// -> x = sqrt(count * h/w)
			cell_row_sz_ = (int)(Math.Round(Math.Sqrt((float)count * cell_h_ / cell_w_)) + 0.1);
			// initialize the texture bitmap
			int texture_side = 64;
			while (
				   texture_side < count / cell_row_sz_ * cell_h_
				|| texture_side < cell_row_sz_ * cell_w_
				)
				texture_side *= 2;
			Bitmap icon_bmp = new Bitmap(texture_side, texture_side);
			Graphics icon_gfx = Graphics.FromImage(icon_bmp);
			icon_gfx.Clear(Color.Orange);
			// initialize the vertex containers
			ArrayList colored     = new ArrayList();
			ArrayList highlighted = new ArrayList();
			ArrayList textured    = new ArrayList();
			// initialize the vertex structures
			CustomVertex.PositionColored vc = new CustomVertex.PositionColored();
			vc.Z = -0.4f;
			vc.Color = Color.Orange.ToArgb();
			CustomVertex.PositionTextured vt = new CustomVertex.PositionTextured();
			vt.Z = -0.36f;
			// calculate button placement,
			// draw the texture,
			// and create geometry
			int icon_x = 0;
			int icon_y = 0;
			if (occupied_angle < 2.0f * pi)
			{
				// calculate unoccupied space properties
				float space = (2 * pi - occupied_angle) / button_placement_.Length;
				float angle = (pi + space) / 2.0f;
				float space_step = pi * max_arc / (radius_ + ring_);
				// iterate through the buttons
				for(int i = 0; i != button_placement_.Length; ++i)
				{
					// calculate placement
					button_placement_[i].angle_start_ = angle % (2.0f * (float)Math.PI);
					angle += button_placement_[i].angle_ + space;
					// calculate button properties
					float begin  = button_placement_[i].angle_start_;
					float end    = begin + button_placement_[i].angle_;
					float extent = button_placement_[i].length_ + radius_ + ring_;
					float step   = pi * max_arc / extent;
					// generate button color vertices
					button_placement_[i].highlight_vb_start_ = highlighted.Count;
					for (float t = begin; t < end; t += step)
					{
						PolarToRect(radius_, t, out vc.X, out vc.Y);
						colored.Add(vc);
						highlighted.Add(vc);
						PolarToRect(extent,  t, out vc.X, out vc.Y);
						colored.Add(vc);
						highlighted.Add(vc);
					}
					PolarToRect(radius_, end, out vc.X, out vc.Y);
					colored.Add(vc);
					highlighted.Add(vc);
					PolarToRect(extent,  end, out vc.X, out vc.Y);
					colored.Add(vc);
					highlighted.Add(vc);
					button_placement_[i].highlight_vb_end_ = highlighted.Count;
					// generate button texture vertices
				{
					ButtonInfo info = (ButtonInfo)button_info_[i];
					float w         = (float)info.image_.Width;
					float h         = (float)info.image_.Height;
					float half_w    = w / 2.0f;
					// calculate the origin and the basis
					Vector2 dy;
					PolarToRect(
						radius_ + ring_,
						(begin + end) / 2.0f,
						out dy.X,
						out dy.Y);
					Vector2 o = dy;
					dy.Normalize();
					Vector2 dx = new Vector2(dy.Y, -dy.X);
					// bl
					vt.X = o.X + dx.X * half_w;
					vt.Y = o.Y + dx.Y * half_w;
					vt.Tu = (float)icon_x / texture_side;
					vt.Tv = (icon_y + h) / texture_side;
					textured.Add(vt);
					// br
					vt.X = o.X - dx.X * half_w;
					vt.Y = o.Y - dx.Y * half_w;
					vt.Tu = (icon_x + w) / texture_side;
					textured.Add(vt);
					// tr
					vt.X += dy.X * h;
					vt.Y += dy.Y * h;
					vt.Tv = (float)icon_y / texture_side;
					textured.Add(vt);
					textured.Add(vt);
					// tl
					vt.X += dx.X * w;
					vt.Y += dx.Y * w;
					vt.Tu = (float)icon_x / texture_side;
					textured.Add(vt);
					// bl
					vt.X = o.X + dx.X * half_w;
					vt.Y = o.Y + dx.Y * half_w;
					vt.Tv = (icon_y + h) / texture_side;
					textured.Add(vt);
					textured_count_ += 6;
				}
					// calculate space properties
					begin  = end;
					end    = begin + space;
					extent = ring_ + radius_;
					// generate space vertices
					for (float t = begin; t < end; t += space_step)
					{
						PolarToRect(radius_, t, out vc.X, out vc.Y);
						colored.Add(vc);
						PolarToRect(extent,  t, out vc.X, out vc.Y);
						colored.Add(vc);
					}
					PolarToRect(radius_, end, out vc.X, out vc.Y);
					colored.Add(vc);
					PolarToRect(extent,  end, out vc.X, out vc.Y);
					colored.Add(vc);
					// draw the icon
					icon_gfx.DrawImage(((ButtonInfo)button_info_[i]).image_, icon_x, icon_y);
					// calculate coordinates for the next icon
					icon_x += cell_w_;
					if (icon_x + cell_w_ > texture_side)
					{
						icon_x = 0;
						icon_y += cell_h_;
					}
				}
			}
			else
			{
			}
			colored_count_ = colored.Count;
			// change highlighted vertex color
			for (int i = 0; i != highlighted.Count; ++i)
			{
				CustomVertex.PositionColored v = (CustomVertex.PositionColored)highlighted[i];
				highlighted[i] = new CustomVertex.PositionColored(
					v.X,
					v.Y,
					-0.33f,
					0x00505A00);
			}
			// initialize the texture
			icon_texture_ = Texture.FromBitmap(device, icon_bmp, Usage.None, Pool.Managed);
			// set up the colored vertex buffer
			if (colored_vb_ != null)
				colored_vb_.Dispose();
			colored_vb_ = new VertexBuffer(
				typeof(CustomVertex.PositionColored),
				colored.Count + highlighted.Count,
				device,
				Usage.WriteOnly,
				CustomVertex.PositionColored.Format,
				Pool.Managed);
			GraphicsStream vb_stream = colored_vb_.Lock(0, 0, LockFlags.None);
			vb_stream.Write(colored.ToArray(typeof(CustomVertex.PositionColored)));
			vb_stream.Write(highlighted.ToArray(typeof(CustomVertex.PositionColored)));
			colored_vb_.Unlock();
			// set up the textured vertex buffer
			if (textured_vb_ != null)
				textured_vb_.Dispose();
			textured_vb_ = new VertexBuffer(
				typeof(CustomVertex.PositionTextured),
				textured.Count,
				device,
				Usage.WriteOnly,
				CustomVertex.PositionColored.Format,
				Pool.Managed);
			vb_stream = textured_vb_.Lock(0, 0, LockFlags.None);
			vb_stream.Write(textured.ToArray(typeof(CustomVertex.PositionTextured)));
			textured_vb_.Unlock();
			// make visible
			is_visible_ = false;
		}

		public void Render(Device device)
		{
			if (!is_visible_)
				return;
			// render the disk
//			device.SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
			device.VertexFormat = CustomVertex.PositionColored.Format;
			device.SetStreamSource(0, colored_vb_, 0);
			device.DrawPrimitives(PrimitiveType.TriangleStrip, 0, colored_count_ - 2);
			// render the icons
			device.SetTexture(0, icon_texture_);
			device.VertexFormat = CustomVertex.PositionTextured.Format;
			device.SetStreamSource(0, textured_vb_, 0);
			device.SamplerState[0].MinFilter = TextureFilter.Anisotropic;
			device.SamplerState[0].MagFilter = TextureFilter.Anisotropic;
			device.DrawPrimitives(PrimitiveType.TriangleList, 0, textured_count_ / 3);
			device.SetTexture(0, null);
			// render the highlight
			device.VertexFormat = CustomVertex.PositionColored.Format;
			device.SetStreamSource(0, colored_vb_, 0);
			if(selection_ >= 0)
			{
				ButtonPlacement button = button_placement_[selection_];
				// additive overlay on
				device.SetRenderState(RenderStates.AlphaBlendEnable, 1);
				device.SetRenderState(RenderStates.DestinationBlend, (int)Blend.One);
				// draw overlay
				device.DrawPrimitives(
					PrimitiveType.TriangleStrip,
					colored_count_ + button.highlight_vb_start_,
					button.highlight_vb_end_ - button.highlight_vb_start_ - 2);
				// additive overlay off
				device.SetRenderState(RenderStates.DestinationBlend, (int)Blend.Zero);
				device.SetRenderState(RenderStates.AlphaBlendEnable, 0);
			}
		}

		public bool Visible
		{
			get { return is_visible_; }
			set { is_visible_ = value; }
		}

		#endregion

		//---------------
		// event handlers
		//---------------

		#region

		/// <summary>
		/// Mouse event handler. Coordinates must be with respect to the object.
		/// </summary>
		public void OnMouseMove(float x, float y)
		{
			if (!is_visible_)
				return;
			selection_ = HitTest(x, y);
		}

		/// <summary>
		/// Mouse event handler. Coordinates must be with respect to the object.
		/// </summary>
		/// <returns>Returns true if a button was clicked.</returns>
		public bool OnMouseClick(float x, float y)
		{
			if (!is_visible_)
				return false;
			selection_ = HitTest(x, y);
			if (selection_ >= 0)
			{
				EventHandler e = ((ButtonInfo)button_info_[selection_]).click_event_;
				if (e != null)
					e(this,  EventArgs.Empty);
			}
			return selection_ >= 0;
		}

		#endregion

		//---------------
		// implementation
		//---------------

		#region

		void PolarToRect(float radius, float angle, out float x, out float y)
		{
			x = radius * (float)Math.Cos(angle);
			y = radius * (float)Math.Sin(angle);
		}

		void RectToPolar(float x, float y, out float radius, out float angle)
		{
			radius = (float)Math.Sqrt(x * x + y * y);
			angle  = (float)Math.Atan2(y, x);
			if (angle < 0)
				angle += 2.0f * (float)Math.PI;
		}

		#endregion

		//-----
		// data
		//-----

		#region

		// button info
		private ArrayList         button_info_;
		private ButtonPlacement[] button_placement_;
		private float             extent_;
		private float             radius_;
		private float             ring_;
		private float             h_space_;
		private float             v_space_;
		private int               selection_;

		// control info
		private bool is_visible_;

		// geometry
		private VertexBuffer colored_vb_;
		private VertexBuffer textured_vb_;

		private int colored_count_;
		private int textured_count_;

		// texture
		private Texture icon_texture_;
		private int     cell_w_;
		private int     cell_h_;
		private int     cell_row_sz_;

		#endregion
	}
}
