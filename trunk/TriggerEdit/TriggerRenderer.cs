using System;
using System.Collections;
using System.Diagnostics;
using System.Drawing;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

namespace TriggerEdit
{
	public class TriggerRenderer
	{
		#region interface

		public TriggerRenderer(Device device, System.Drawing.Font font)
		{
			System.Drawing.Font new_font = new System.Drawing.Font("Bitstream Vera Sans", 12);
			new_font = font;//new System.Drawing.Font(new_font, FontStyle.Regular);
			font_ = new Microsoft.DirectX.Direct3D.Font(device, new_font);
			sprite_ = new Sprite(device);
			cell_size_ = new Size(new_font.Height * 6, new_font.Height * 2 + 4);
		}
		
		/// <summary>
		/// This method must be called from inside of a Device.BeginScene ... Device.EndScene block.
		/// </summary>
		public void Render(Device device, Trigger[] triggers)
		{
			// create geometry
			ArrayList vertices = new ArrayList();
			foreach (Trigger trigger in triggers)
			{
				PushMarker(ref vertices, new Point(trigger.X, trigger.Y), trigger.color_, trigger.outline_);
				foreach(Trigger.Link link in trigger.links)
				{
					Trigger target = (Trigger)triggers[link.target];
					PushLine(
						ref vertices,
						new Point(trigger.X, trigger.Y),
						new Point(target.X, target.Y),
						link.color);
				}
			}
			Debug.Assert(vertices.Count % 3 == 0);
			// create the vertex buffer
			VertexBuffer vb = new VertexBuffer(
				typeof(CustomVertex.PositionColored),
				vertices.Count,
				device,
				Usage.WriteOnly,
				CustomVertex.PositionColored.Format,
				Pool.Managed);
			device.SetStreamSource(0, vb, 0);
			device.VertexFormat = CustomVertex.PositionColored.Format;
			GraphicsStream vb_stream = vb.Lock(0, 0, LockFlags.None);
			vb_stream.Write(vertices.ToArray(typeof(CustomVertex.PositionColored)));
			vb.Unlock();
			// output geometry
			device.DrawPrimitives(PrimitiveType.TriangleList, 0, vertices.Count / 3);
			vb.Dispose();
			// draw text
			sprite_.Begin(SpriteFlags.AlphaBlend | SpriteFlags.SortTexture | SpriteFlags.ObjectSpace);
			device.SamplerState[0].MinFilter = TextureFilter.Point;
			foreach (Trigger trigger in triggers)
			{
				Rectangle rect = new Rectangle(
					trigger.X - cell_size_.Width  / 2,
					trigger.Y - cell_size_.Height / 2,
					cell_size_.Width,
					cell_size_.Height);
				rect.Inflate(-3, -2);
				font_.DrawText(sprite_, trigger.name, rect, DrawTextFormat.WordBreak, Color.Black);
			}
			sprite_.End();
		}

		#endregion

		#region internal implementation

		private void MakeRectangle(ref ArrayList vertices, Point position, SizeF radius, float z, Color color)
		{
			CustomVertex.PositionColored vertex = new CustomVertex.PositionColored();
			vertex.Color = color.ToArgb();
			vertex.Z = z;
			vertex.X = position.X - radius.Width;
			vertex.Y = position.Y + radius.Height;
			vertices.Add(vertex);
			vertex.X = position.X + radius.Width;
			vertex.Y = position.Y + radius.Height;
			vertices.Add(vertex);
			vertex.X = position.X -radius.Width;
			vertex.Y = position.Y + -radius.Height;
			vertices.Add(vertex);
			vertices.Add(vertex);
			vertex.X = position.X + radius.Width;
			vertex.Y = position.Y + -radius.Height;
			vertices.Add(vertex);
			vertex.X = position.X + radius.Width;
			vertex.Y = position.Y + radius.Height;
			vertices.Add(vertex);
		}

		private void PushLine(ref ArrayList vertices, Point start, Point end, Color color)
		{
			SnapPoints(ref start, ref end);
			// calculate new coordinate system
			Vector2 dx = new Vector2(
				end.X - start.X,
				end.Y - start.Y);
			dx.Normalize();
			Vector2 dy = dx;
			float new_y = -dy.X;
			dy.X = dy.Y;
			dy.Y = new_y;
			// calculate offset
			Vector2 offset = dy;
			offset *= 0.5f;
			// build the line
			CustomVertex.PositionColored vertex = new CustomVertex.PositionColored();
			vertex.Color = color.ToArgb();
			vertex.Z = -0.1f;
			vertex.X = start.X - offset.X;
			vertex.Y = start.Y - offset.Y;
			vertices.Add(vertex);
			vertex.X = end.X - offset.X;
			vertex.Y = end.Y - offset.Y;
			vertices.Add(vertex);
			vertex.X = end.X + offset.X;
			vertex.Y = end.Y + offset.Y;
			vertices.Add(vertex);
			vertices.Add(vertex);
			vertex.X = start.X - offset.X;
			vertex.Y = start.Y - offset.Y;
			vertices.Add(vertex);
			vertex.X = start.X + offset.X;
			vertex.Y = start.Y + offset.Y;
			vertices.Add(vertex);
			// build the arrow
			Vector2 corner1 = dx * -12.0f + dy * 4.0f;
			Vector2 corner2 = dx * -12.0f - dy * 4.0f;
			vertex.X = end.X;
			vertex.Y = end.Y;
			vertices.Add(vertex);
			vertex.X = end.X + corner1.X;
			vertex.Y = end.Y + corner1.Y;
			vertices.Add(vertex);
			vertex.X = end.X + corner2.X;
			vertex.Y = end.Y + corner2.Y;
			vertices.Add(vertex);
		}

		private void PushMarker(ref ArrayList vertices, Point position, Color color, Color outline)
		{
			SizeF radius = new SizeF(
				cell_size_.Width  / 2.0f,
				cell_size_.Height / 2.0f);
			MakeRectangle(ref vertices, position, radius, -0.3f, outline);
			radius.Width  -= 1.0f;
			radius.Height -= 1.0f;
			MakeRectangle(ref vertices, position, radius, -0.2f, color);
		}

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

		#endregion

		#region utility
//
//		private float MakeAngle(float dx, float dy)
//		{
//			f
//		}

		#endregion

		#region data

		Size cell_size_;
		Microsoft.DirectX.Direct3D.Font font_;
		Sprite                          sprite_;

		#endregion
	}
}
