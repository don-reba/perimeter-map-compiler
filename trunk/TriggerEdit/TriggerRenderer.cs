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

		public TriggerRenderer(Device device, MarkerLayout marker_layout, System.Drawing.Font font)
		{
			font_          = new Microsoft.DirectX.Direct3D.Font(device, font);
			marker_layout_ = marker_layout;
			sprite_        = new Sprite(device);
		}
		
		/// <summary>
		/// This method must be called from inside of a Device.BeginScene ... Device.EndScene block.
		/// </summary>
		public void Render(Device device, TriggerContainer triggers)
		{
			// create geometry
			ArrayList vertices = new ArrayList();
			PointF rounded_start = PointF.Empty;
			PointF rounded_end   = PointF.Empty;
			for (int i = 0; i != triggers.count_; ++i)
			{
				rounded_start.X = (float)Math.Round(triggers.positions_[i].X);
				rounded_start.Y = (float)Math.Round(triggers.positions_[i].Y);
				PushMarker(
					ref vertices,
					rounded_start,
					triggers.descriptions_[i].Fill,
					triggers.descriptions_[i].Outline);
				foreach (int j in triggers.adjacency_.GetList(i))
				{
					rounded_end.X = (float)Math.Round(triggers.positions_[j].X);
					rounded_end.Y = (float)Math.Round(triggers.positions_[j].Y);
					PushLine(ref vertices, rounded_start, rounded_end, Color.Red);
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
			for (int i = 0; i != triggers.count_; ++i)
			{
				Rectangle rect = new Rectangle(
					(int)Math.Round(triggers.positions_[i].X - marker_layout_.Width  / 2),
					(int)Math.Round(triggers.positions_[i].Y - marker_layout_.Height / 2),
					marker_layout_.Width,
					marker_layout_.Height);
				rect.Inflate(
					-marker_layout_.TextOffset.Width,
					-marker_layout_.TextOffset.Height);
				font_.DrawText(
					sprite_,
					triggers.descriptions_[i].name_,
					rect,
					DrawTextFormat.WordBreak,
					Color.Black);
			}
			sprite_.End();
		}

		#endregion

		#region internal implementation

		private void MakeRectangle(ref ArrayList vertices, PointF position, SizeF radius, float z, Color color)
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

		// [-0.1,0)
		private void PushLine(ref ArrayList vertices, PointF start, PointF end, Color color)
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

		// [-0.2,-0.1)
		private void PushMarker(ref ArrayList vertices, PointF position, Color color, Color outline)
		{
			SizeF radius = new SizeF(
				marker_layout_.Width  / 2.0f,
				marker_layout_.Height / 2.0f);
			MakeRectangle(ref vertices, position, radius, -0.20f, outline);
			radius.Width  -= 1.0f;
			radius.Height -= 1.0f;
			MakeRectangle(ref vertices, position, radius, -0.17f, color);
			MakeRectangle(
				ref vertices,
				new PointF(
					position.X - radius.Width + marker_layout_.TearOffWidth,
					position.Y),
				new SizeF(
					0.5f,
					marker_layout_.Height / 2.0f - 1.0f),
				-0.13f,
				Color.Black);
		}

		private void SnapPoints(ref PointF pnt1, ref PointF pnt2)
		{
			// initialize the rectangles
			RectangleF rect1 = new RectangleF(0, 0, marker_layout_.Width, marker_layout_.Height);
			RectangleF rect2 = rect1;
			rect2.Offset(pnt2.X - pnt1.X, pnt2.Y - pnt1.Y);
			// snap logic
			if (rect1.Bottom < rect2.Top)
			{
				if (rect1.Right < rect2.Left)
				{
					pnt1.X += marker_layout_.Width  / 2;
					pnt1.Y += marker_layout_.Height / 2;
					pnt2.X -= marker_layout_.Width  / 2;
					pnt2.Y -= marker_layout_.Height / 2;
				}
				else if (rect1.Left > rect2.Right)
				{
					pnt1.X -= marker_layout_.Width  / 2;
					pnt1.Y += marker_layout_.Height / 2;
					pnt2.X += marker_layout_.Width  / 2;
					pnt2.Y -= marker_layout_.Height / 2;
				}
				else
				{
					pnt1.Y += marker_layout_.Height / 2;
					pnt2.Y -= marker_layout_.Height / 2;
				}
			}
			else if (rect1.Top > rect2.Bottom)
			{
				if (rect1.Right < rect2.Left)
				{
					pnt1.X += marker_layout_.Width  / 2;
					pnt1.Y -= marker_layout_.Height / 2;
					pnt2.X -= marker_layout_.Width  / 2;
					pnt2.Y += marker_layout_.Height / 2;
				}
				else if (rect1.Left > rect2.Right)
				{
					pnt1.X -= marker_layout_.Width  / 2;
					pnt1.Y -= marker_layout_.Height / 2;
					pnt2.X += marker_layout_.Width  / 2;
					pnt2.Y += marker_layout_.Height / 2;
				}
				else
				{
					pnt1.Y -= marker_layout_.Height / 2;
					pnt2.Y += marker_layout_.Height / 2;
				}
			}
			else
			{
				if (rect1.Right < rect2.Left)
				{
					pnt1.X += marker_layout_.Width / 2;
					pnt2.X -= marker_layout_.Width / 2;
				}
				else if (rect1.Left > rect2.Right)
				{
					pnt1.X -= marker_layout_.Width / 2;
					pnt2.X += marker_layout_.Width / 2;
				}
			}
		}

		#endregion

		#region data

		Microsoft.DirectX.Direct3D.Font font_;
		MarkerLayout                    marker_layout_;
		Sprite                          sprite_;

		#endregion
	}
}
