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

		//----------
		// interface
		//----------

		#region

		public TriggerRenderer(Device device, MarkerLayout marker_layout, SizeF link_box_size, System.Drawing.Font font)
		{
			font_            = new Microsoft.DirectX.Direct3D.Font(device, font);
			link_box_radius_ = link_box_size;
			marker_layout_   = marker_layout;
			sprite_          = new Sprite(device);
			vertices_        = new CustomVertex.PositionColored[64];
			link_box_radius_.Width  /= 2.0f;
			link_box_radius_.Height /= 2.0f;
		}
		
		/// <summary>
		/// This method must be called from inside of a Device.BeginScene ... Device.EndScene block.
		/// </summary>
		public void Render(Device device, TriggerContainer triggers)
		{
			// create geometry
			StartVertices();
			PointF head = PointF.Empty;
			PointF tail = PointF.Empty;
			for (int i = 0; i != triggers.count_; ++i)
			{
				triggers.GetInterpolatedPosition(i, ref head);
				z_ = -0.17f;
				PushMarker(
					head,
					triggers.descriptions_[i].Fill,
					triggers.descriptions_[i].Outline);
			}
			for (int i = 0; i != triggers.adjacency_list_.Count; ++i)
			{
				TriggerContainer.Link link = triggers.adjacency_list_[i];
				triggers.GetInterpolatedPosition(link.head_, ref head);
				triggers.GetInterpolatedPosition(link.tail_, ref tail);
				z_ = (0 != (link.status_ & TriggerContainer.Link.Status.Selected)) ? -0.05f : -0.1f;
				PushSnappedArrow(
					head,
					tail,
					link.group_,
					link.persistent_,
					link.Fill,
					link.HeadFill);
			}
			for (int i = 0; i != triggers.g_marker_positions_.Count; ++i)
			{
				PointF point
					= (PointF)triggers.g_marker_positions_[i];
				TriggerContainer.TriggerDescription description
					= (TriggerContainer.TriggerDescription)triggers.g_marker_descriptions_[i];
				z_ = -0.13f;
				PushMarker(point, description.Fill, description.Outline);
			}
			for (int i = 0; i != triggers.g_links_.Count; ++i)
			{
				TriggerContainer.GhostLink link = triggers.GetLinkGhost(i);
				PushArrow(
					link.head_,
					link.tail_,
					link.snap_head_,
					link.snap_tail_,
					Color.Red,
					Color.Red);
			}
			EndVertices();
			if (0 == current_vertex_)
				return;
			// create the vertex buffer
			VertexBuffer vb = new VertexBuffer(
				typeof(CustomVertex.PositionColored),
				vertices_.Length,
				device,
				Usage.WriteOnly,
				CustomVertex.PositionColored.Format,
				Pool.Managed);
			GraphicsStream vb_stream = vb.Lock(0, 0, LockFlags.None);
			vb_stream.Write(vertices_);
			vb.Unlock();
			device.SetStreamSource(0, vb, 0);
			device.VertexFormat = CustomVertex.PositionColored.Format;
			// output geometry
			device.DrawPrimitives(PrimitiveType.TriangleList, 0, current_vertex_ / 3);
			vb.Dispose();
			// draw text
			sprite_.Begin(SpriteFlags.AlphaBlend | SpriteFlags.SortTexture | SpriteFlags.ObjectSpace);
			device.SamplerState[0].MinFilter = TextureFilter.Point;
			device.SamplerState[0].MagFilter = TextureFilter.Point;
			// triggers
			for (int i = 0; i != triggers.count_; ++i)
			{
				triggers.GetInterpolatedPosition(i, ref head);
				Rectangle rect = new Rectangle(
					(int)(head.X - marker_layout_.Width  / 2),
					(int)(head.Y - marker_layout_.Height / 2),
					marker_layout_.Width,
					marker_layout_.Height);
				rect.Inflate(
					-marker_layout_.TextOffset.Width,
					-marker_layout_.TextOffset.Height);
				font_.DrawText(
					sprite_,
					(triggers.descriptions_[i].action_ != null)
						? string.Format(
							"{0}:\n{1}",
							triggers.descriptions_[i].name_,
							triggers.descriptions_[i].action_.Name)
						: triggers.descriptions_[i].name_,
					rect,
					DrawTextFormat.WordBreak,
					Color.Black);
			}
			// trigger ghosts
			for (int i = 0; i != triggers.g_marker_positions_.Count; ++i)
			{
				PointF point
					= (PointF)triggers.g_marker_positions_[i];
				TriggerContainer.TriggerDescription description
					= (TriggerContainer.TriggerDescription)triggers.g_marker_descriptions_[i];
				Rectangle rect = new Rectangle(
					(int)(point.X - marker_layout_.Width  / 2.0f),
					(int)(point.Y - marker_layout_.Height / 2.0f),
					marker_layout_.Width,
					marker_layout_.Height);
				rect.Inflate(
					-marker_layout_.TextOffset.Width,
					-marker_layout_.TextOffset.Height);
				font_.DrawText(
					sprite_,
					(description.action_ != null)
						? string.Format(
							"{0}:\n{1}",
							description.name_,
							description.action_.Name)
						: description.name_,
					rect,
					DrawTextFormat.WordBreak,
					Color.Black);
			}
			sprite_.End();
			vb.Dispose();
		}

		public void OnLostDevice()
		{
			sprite_.OnLostDevice();
			font_.OnLostDevice();
		}

		public void OnResetDevice()
		{
			sprite_.OnResetDevice();
			font_.OnResetDevice();
		}

		public bool DisplayAction
		{
			get { return display_action_; }
			set { display_action_ = value; }
		}

		#endregion		

		//---------------
		// implementation
		//---------------

		#region

		private void PushRectangle(PointF position, SizeF radius, Color color)
		{
			CustomVertex.PositionColored vertex = new CustomVertex.PositionColored();
			vertex.Color = color.ToArgb();
			vertex.Z = z_;
			vertex.X = position.X - radius.Width;
			vertex.Y = position.Y + radius.Height;
			AddVertex(vertex);
			vertex.X = position.X + radius.Width;
			vertex.Y = position.Y + radius.Height;
			AddVertex(vertex);
			vertex.X = position.X -radius.Width;
			vertex.Y = position.Y + -radius.Height;
			AddVertex(vertex);
			AddVertex(vertex);
			vertex.X = position.X + radius.Width;
			vertex.Y = position.Y + -radius.Height;
			AddVertex(vertex);
			vertex.X = position.X + radius.Width;
			vertex.Y = position.Y + radius.Height;
			AddVertex(vertex);
		}

		// [-0.1,0)
		private void PushSnappedArrow(
			PointF head,
			PointF tail,
			int    group,
			bool   persistent,
			Color  line_color,
			Color  head_color)
		{
			SnapPoints(ref head, ref tail);
			PushArrow(head, tail, persistent, line_color, head_color);
		}

		private void PushArrow(
			PointF head,
			PointF tail,
			bool   snap_head,
			bool   snap_tail,
			Color  line_color,
			Color  head_color)
		{
			if (snap_head && snap_tail)
			{
				PushSnappedArrow(head, tail, 0, false, line_color, head_color);
				return;
			}
			if (snap_head)
			{
				PointF temp = tail;
				SnapPoints(ref head, ref temp);
				PushArrow(head, tail, false, line_color, head_color);
				return;
			}
			if (snap_tail)
			{
				PointF temp = head;
				SnapPoints(ref temp, ref tail);
				PushArrow(head, tail, false, line_color, head_color);
				return;
			}
		}

		// [-0.1,0)
		private void PushArrow(
			PointF head,
			PointF tail,
			bool   persistent,
			Color  line_color,
			Color  head_color)
		{
			// calculate new coordinate system
			Vector2 dx = new Vector2(
				head.X - tail.X,
				head.Y - tail.Y);
			dx.Normalize();
			Vector2 dy = dx;
			float new_y = -dy.X;
			dy.X = dy.Y;
			dy.Y = new_y;
			// calculate offset
			Vector2 offset = dy;
			offset *= persistent ? 1.5f : 0.5f; // 1-3 units thick
			// build the line
			CustomVertex.PositionColored vertex = new CustomVertex.PositionColored();
			vertex.Color = line_color.ToArgb();
			vertex.Z = z_ - 0.01f;
			vertex.X = tail.X - offset.X;
			vertex.Y = tail.Y - offset.Y;
			AddVertex(vertex);
			vertex.X = head.X - offset.X - dx.X * 8.0f;
			vertex.Y = head.Y - offset.Y - dx.Y * 8.0f;
			AddVertex(vertex);
			vertex.X = head.X + offset.X - dx.X * 8.0f;
			vertex.Y = head.Y + offset.Y - dx.Y * 8.0f;
			AddVertex(vertex);
			AddVertex(vertex);
			vertex.X = tail.X - offset.X;
			vertex.Y = tail.Y - offset.Y;
			AddVertex(vertex);
			vertex.X = tail.X + offset.X;
			vertex.Y = tail.Y + offset.Y;
			AddVertex(vertex);
			// build the arrow
			vertex.Color = head_color.ToArgb();
			Vector2 corner1 = dx * -12.0f + dy * 4.0f;
			Vector2 corner2 = dx * -12.0f - dy * 4.0f;
			vertex.X = head.X;
			vertex.Y = head.Y;
			AddVertex(vertex);
			vertex.X = head.X + corner1.X;
			vertex.Y = head.Y + corner1.Y;
			AddVertex(vertex);
			vertex.X = head.X + corner2.X;
			vertex.Y = head.Y + corner2.Y;
			AddVertex(vertex);
		}

		private void PushMarker(PointF position, Color color, Color outline)
		{
			SizeF radius = new SizeF(
				marker_layout_.Width  / 2.0f,
				marker_layout_.Height / 2.0f);
			PushRectangle(position, radius, color);
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

		#region vertex management

		//---------------------------------------------------------------------
		// Memory policy:
		// Buffer size is equal to the maximum amount of memory used.
		//
		// Keep data in a fixed-size array.
		// In case of overflow, measure the amount of space needed
		// by using a variable-size array, then commit to the fixed-size array.
		//---------------------------------------------------------------------

		private void StartVertices()
		{
			current_vertex_ = 0;
		}

		private void AddVertex(CustomVertex.PositionColored vertex)
		{
			if (null != vertices_temp_)
			{
				vertices_temp_.Add(vertex);
				return;
			}
			if (current_vertex_ < vertices_.Length)
			{
				vertices_[current_vertex_] = vertex;
				++current_vertex_;
				return;
			}
			Debug.WriteLine("! hit: " + vertices_.Length.ToString());
			vertices_temp_ = new ArrayList(vertices_);
			vertices_temp_.Add(vertex);
		}

		private void EndVertices()
		{
			if (null != vertices_temp_)
			{
				vertices_
					= (CustomVertex.PositionColored[])
					vertices_temp_.ToArray(typeof(CustomVertex.PositionColored));
				current_vertex_ = vertices_.Length;
				vertices_temp_ = null;
			}
		}

		#endregion

		#endregion

		//-----
		// data
		//-----

		#region

		// DX
		private Microsoft.DirectX.Direct3D.Font font_;
		private SizeF                           link_box_radius_;
		private MarkerLayout                    marker_layout_;
		private Sprite                          sprite_;
		private float                           z_;
		// vertex management
		private CustomVertex.PositionColored[] vertices_;
		private ArrayList                      vertices_temp_;
		private int                            current_vertex_;
		// preferences
		private bool display_action_;

		#endregion
	}
}
