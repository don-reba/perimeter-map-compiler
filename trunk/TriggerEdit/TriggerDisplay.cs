using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Windows.Forms;

namespace TriggerEdit
{
		#region using

		using TriggerDescription = TriggerContainer.TriggerDescription;
		using TriggerState       = TriggerContainer.TriggerDescription.State;
		using TriggerStatus      = TriggerContainer.TriggerDescription.Status;

		#endregion

	/// <summary>
	/// Summary description for TriggerDisplay.
	/// </summary>
	public class TriggerDisplay : System.Windows.Forms.Panel
	{
		#region nested types

		private enum AnimationState
		{
			Off,
			FirstStep,
			SecondStep,
			Normal,
			SecondLastStep,
			LastStep,
			Done
		}

		private enum DragState
		{
			None,
			DisplaceCamera
		}

		#endregion

		#region interface

		public TriggerDisplay()
		{
			// This call is required by the Windows.Forms Form Designer.
			InitializeComponent();
			// initialize variables
			animation_state_     = AnimationState.Off;
			camera_              = new Camera();
			drag_start_          = new Point(0, 0);
			marker_layout_       = new MarkerLayout(Font);
			old_camera_position_ = new Point(0, 0);
			// hook event handlers
			Application.Idle += new EventHandler(OnApplicationIdle);
		}

		public void Reset(ref TriggerContainer triggers)
		{
			triggers_ = triggers;
			PreprocessVertices(ref triggers_.positions_);
			NormalizeVertices(ref triggers_.positions_);
			// center a camera at the average vertex position
			PointF average = PointF.Empty;
			for (int i = 0; i != triggers_.count_; ++i)
			{
				average.X += triggers_.positions_[i].X / triggers.count_;
				average.Y += triggers_.positions_[i].Y / triggers.count_;
			}
			camera_.Position = average;
			// set initial marker positions
			// initalize miscellaneous
			InitGraphics();
			bspline_   = new BSpline(triggers_.history_, triggers_.positions_, 32);
			animation_state_ = AnimationState.FirstStep;
		}

		public int Selection
		{
			get { return selection_; }
		}

		#endregion

		#region implementation

		private void AdvanceAnimation()
		{
			switch (animation_state_)
			{
				case AnimationState.FirstStep:
				{
					// shift
					temperature_ = 1.0f;
					ShiftVertices(ref triggers_.positions_);
					// copy positions into the history array
					for (uint i = 0; i != triggers_.count_; ++i)
						triggers_.history_[i].point1_ = triggers_.positions_[i];
					// shift
					ShiftVertices(ref triggers_.positions_);
					// copy positions into the history array
					for (uint i = 0; i != triggers_.count_; ++i)
						triggers_.history_[i].point2_ = triggers_.positions_[i];
					// move vertices
					bspline_.SegmentMode = BSpline.Mode.First;
					bspline_.Set(0.0f);
					// advance state
					animation_state_ = AnimationState.SecondStep;
					last_step_time_  = Environment.TickCount;
				} break;
				case AnimationState.SecondStep:
				{
					// calculate the current path position
					float t = (Environment.TickCount - last_step_time_) / (float)step_duration_;
					// set new position
					if (t <= 1.0f)
						bspline_.Set(t);
					else
					{
						// get last vertex positions
						for (uint i = 0; i != triggers_.count_; ++i)
							triggers_.positions_[i] = triggers_.history_[i].point2_;
						// shift
						ShiftVertices(ref triggers_.positions_);
						// copy positions into the history array
						for (uint i = 0; i != triggers_.count_; ++i)
							triggers_.history_[i].point3_ = triggers_.positions_[i];
						// move vertices
						bspline_.SegmentMode = BSpline.Mode.Middle;
						bspline_.Set(0.0f);
						// advance state
						animation_state_ = AnimationState.Normal;
						last_step_time_  = Environment.TickCount;
					}
				} break;
				case AnimationState.Normal:
				{
					// calculate the current path position
					float t = (Environment.TickCount - last_step_time_) / (float)step_duration_;
					// set new position
					if (t <= 1.0f)
						bspline_.Set(t);
					else
					{
						// recall the more recent positions
						for (uint i = 0; i != triggers_.count_; ++i)
							triggers_.positions_[i] = triggers_.history_[i].point3_;
						// calculate new positions
						float temperature = ShiftVertices(ref triggers_.positions_);
						if (temperature <= 3.0f)
						{
							while (ShiftVertices(ref triggers_.positions_) >= 1.0f);
							animation_state_ = AnimationState.SecondLastStep;
						}
						else if (temperature <= 24.0f)
							while (ShiftVertices(ref triggers_.positions_) > 3.0f);
						// copy positions into the history array
						for (uint i = 0; i != triggers_.count_; ++i)
							triggers_.history_[i].Add(triggers_.positions_[i]);
						// move vertices
						bspline_.Set(0.0f);
						// advance state
						last_step_time_ = Environment.TickCount;
					}
				} break;
				case AnimationState.SecondLastStep:
				{
					// calculate the current path position
					float t = (Environment.TickCount - last_step_time_) / (float)step_duration_;
					// set new position
					if (t <= 1.0f)
						bspline_.Set(t);
					else
					{
						// move vertices
						bspline_.SegmentMode = BSpline.Mode.Last;
						bspline_.Set(0.0f);
						// advance state
						animation_state_ = AnimationState.LastStep;
						last_step_time_  = Environment.TickCount;
					}
				} break;
				case AnimationState.LastStep:
				{
					// calculate the current path position
					float t = (Environment.TickCount - last_step_time_) / (float)step_duration_;
					// set new position
					if (t <= 1.0f)
						bspline_.Set(t);
					else
						animation_state_ = AnimationState.Done;
				} break;
			}
		}

		private bool AppStillIdle
		{
			get
			{
				Import.Message msg;
				return !Import.PeekMessage(out msg, IntPtr.Zero, 0, 0, 0);
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

		/// <param name="position">in screen coordinates</param>
		protected void HighlightTriggers(Point position)
		{
			// find the cell under cursor
			ScreenToWorld(ref position);
			int index = triggers_.GetIndex(position, marker_layout_);
			if (index == active_)
				return;
			// unhighlight the previous cell and its neighbours
			if (active_ >= 0)
			{
				triggers_.descriptions_[active_].status_ &= ~TriggerStatus.Bright;
				foreach (int link in triggers_.adjacency_.GetList(active_))
					triggers_.descriptions_[link].status_ &= ~TriggerStatus.Dim;
			}
			active_ = index;
			if (index < 0)
				return;
			// highlight the selected trigger and its dependants
			triggers_.descriptions_[index].status_ |= TriggerStatus.Bright;
			foreach (int link in triggers_.adjacency_.GetList(index))
				triggers_.descriptions_[link].status_ |= TriggerStatus.Dim;
		}
		
		private bool InitGraphics()
		{
			// get system specs
			AdapterInformation adapter_info = Manager.Adapters.Default;
			int result;
			int quality_levels = 0;
			MultiSampleType best_msample_t = (MultiSampleType)16;
			for (; best_msample_t >= 0; best_msample_t--)
				if (Manager.CheckDeviceMultiSampleType(
					adapter_info.Adapter,
					DeviceType.Hardware,
					adapter_info.CurrentDisplayMode.Format,
					true,
					best_msample_t,
					out result,
					out quality_levels))
					break;
			// initialize device
			PresentParameters presentParams = new PresentParameters();
			presentParams.AutoDepthStencilFormat = DepthFormat.D16;
			presentParams.BackBufferCount        = 1;
			presentParams.EnableAutoDepthStencil = true;
			presentParams.MultiSample            = best_msample_t;
			presentParams.MultiSampleQuality     = quality_levels - 1;
			presentParams.SwapEffect             = SwapEffect.Discard;
			presentParams.Windowed               = true;
			if (null ==	 device_)
			{
				try
				{
					device_ = new Device(
						0,
						DeviceType.Hardware,
						this, 
						CreateFlags.HardwareVertexProcessing,
						presentParams);
				}
				catch (InvalidCallException)
				{
					MessageBox.Show(
						"Direct 3D device could not be created. The program will now terminate.\n" +
						"Please make sure you have Managed DirectX 9.0c installed.",
						"Error",
						MessageBoxButtons.OK,
						MessageBoxIcon.Error);
					Application.Exit();
				}
				trigger_renderer_ = new TriggerRenderer(device_, marker_layout_, Font);
			}
			else
				device_.Reset(presentParams);
			device_.RenderState.CullMode = Cull.None;
			device_.RenderState.Lighting = false;
			return true;
		}

		private void Render()
		{
			if (device_ == null                             ||
				AnimationState.Off       == animation_state_ ||
				AnimationState.FirstStep == animation_state_)
				return;
			device_.Clear(ClearFlags.Target,  Color.Black, 1.0f, 0);
			device_.Clear(ClearFlags.ZBuffer, 0x0,         1.0f, 0);
			device_.BeginScene();
			// matrices
			device_.Transform.View = camera_.View;
			device_.Transform.Projection = camera_.Projection;
			trigger_renderer_.Render(device_, triggers_);
			device_.EndScene();
			try
			{
				device_.Present();
			}
			catch (DeviceLostException)
			{
				InitGraphics();
			}
		}

		private void NormalizeVertices(ref PointF[] positions)
		{
			float min_x = float.MaxValue;
			float min_y = float.MaxValue;
			for (int i = 0; i != positions.Length; ++i)
			{
				if (positions[i].X < min_x)
					min_x = positions[i].X;
				if (positions[i].Y < min_y)
					min_y = positions[i].Y;
			}
			for (int i = 0; i != positions.Length; ++i)
			{
				positions[i].X -= min_x;
				positions[i].Y -= min_y;
			}
		}

		private void PreprocessVertices(ref PointF[] positions)
		{
			float ka_x = cell_spacing_ * 2 + marker_layout_.Width;
			float ka_y = cell_spacing_ * 2 + marker_layout_.Height;
			// spread the vertices across the initial square
			// TODO: try circle
			int range_x = (int)(100.0f * ka_x * (float)Math.Sqrt(positions.Length));
			int range_y = (int)(100.0f * ka_y * (float)Math.Sqrt(positions.Length));
			Random random = new Random(0);
			for (int i = 0; i != positions.Length; ++i)
			{
				positions[i].X = random.Next(range_x);
				positions[i].Y = random.Next(range_y);
			}
			// phase 1
			PointF[] new_positions = new PointF[positions.Length];
			PointF[] p             = new PointF[positions.Length];
			for (int iteration = 0; iteration != 256; ++iteration)
			{
				// calculate new positions
				for (int i = 0; i != positions.Length; ++i)
				{
					// calculate p_i's
					int p_index = 0;
					for (int j = 0; j != positions.Length; ++j)
					{
						if (triggers_.adjacency_[i, j] || triggers_.adjacency_[j, i])
						{
							float dx = positions[i].X - positions[j].X;
							float dy = positions[i].Y - positions[j].Y;
							float d  = (float)Math.Sqrt(dx * dx + dy * dy);
							if (0 == d)
							{
								p[p_index].X = positions[j].X + ka_x;
								p[p_index].Y = positions[j].Y;
							}
							else
							{
								p[p_index].X = positions[j].X + ka_x * dx / d;
								p[p_index].Y = positions[j].Y + ka_y * dy / d;
							}
							++p_index;
						}
					}
					// average out
					if (0 == p_index)
						new_positions[i] = positions[i];
					else
					{
						float sum_x = 0;
						float sum_y = 0;
						for (int j = 0; j != p_index; ++j)
						{
							sum_x += p[j].X;
							sum_y += p[j].Y;
						}
						new_positions[i].X = sum_x / p_index;
						new_positions[i].Y = sum_y / p_index;
					}
				}
				// commit
				Array.Copy(new_positions, positions, positions.Length);
			}
		}

		private void ShiftVertices_AdjustLength(ref float length, float offset)
		{
			if (length > 0)
				if (length < offset)
					length = 0;
				else
					length -= offset;
			else
				if (length > -offset)
					length = 0;
				else
					length += offset;
		}

		/// <param name="positions">must not be empty</param>
		private float ShiftVertices(ref PointF[] positions)
		{
			float k = cell_spacing_;
			float range = (Math.Max(marker_layout_.Width, marker_layout_.Height) + k) * 3;
			// reset velocities
			for (int i = 0; i != triggers_.count_; ++i)
				triggers_.velocities_[i] = PointF.Empty;	
			// calculate forces
			for (int i = 1; i != triggers_.count_; ++i)
			{
				float v_x = 0;
				float v_y = 0;
				for (int j = 0; j != i; ++j)
				{
					float dx = positions[j].X - positions[i].X;
					float dy = positions[j].Y - positions[i].Y;
					ShiftVertices_AdjustLength(ref dx, marker_layout_.Width);
					ShiftVertices_AdjustLength(ref dy, marker_layout_.Height);
					float d_sqr = dx * dx + dy * dy;
					// repulsion
					if (d_sqr > 0 && d_sqr < range * range)
					{
						float ratio = k * k / d_sqr;
						float r_dx = dx * ratio;
						float r_dy = dy * ratio;
						v_x -= r_dx;
						v_y -= r_dy;
						triggers_.velocities_[j].X += r_dx;
						triggers_.velocities_[j].Y += r_dy;
					}
					// attraction
					if (triggers_.adjacency_[i, j] || triggers_.adjacency_[j, i])
					{
						float d = (float)Math.Sqrt(d_sqr);
						if (d > k)
						{
							float ratio = d / k;
							float a_dx = dx * ratio;
							float a_dy = dy * ratio;
							v_x += a_dx;
							v_y += a_dy;
							triggers_.velocities_[j].X -= a_dx;
							triggers_.velocities_[j].Y -= a_dy;
						}
//						else
//							v_x += marker_layout_.Width + k;
					}
				}
				triggers_.velocities_[i].X += v_x;
				triggers_.velocities_[i].Y += v_y;
			}
			// displace vertices
			float temperature = k / temperature_;
			for (int i = 0; i != triggers_.count_; ++i)
			{
				float v_x = triggers_.velocities_[i].X;
				float v_y = triggers_.velocities_[i].Y;
				float v_abs = (float)Math.Sqrt(v_x * v_x + v_y * v_y);
				if (v_abs != 0)
				{
					// set the new coordinates
//					positions[i].X = positions[i].X + v_x;
//					positions[i].Y= positions[i].Y + v_y;
					positions[i].X = positions[i].X + (v_x / v_abs) * Math.Min(v_abs, temperature);
					positions[i].Y = positions[i].Y + (v_y / v_abs) * Math.Min(v_abs, temperature);
				}
			}
			// cool down, with simmering
			if (temperature <= 2.0f)
				temperature_ *= 1.0005f;
			else if (temperature <= 3.0f)
				temperature_ *= 1.005f;
			else
				temperature_ *= 1.05f;
//			double energy = 0.0;
//			foreach (PointF v in triggers_.velocities_)
//				energy += v.X * v.X + v.Y * v.Y;	
//			energies.WriteLine(energy);
//			energies.Flush();
			return temperature;
		}

//		System.IO.StreamWriter energies = new StreamWriter("energy.txt", false, System.Text.Encoding.ASCII);

		private void SnapPoints(ref Point pnt1, ref Point pnt2)
		{
			// initialize the rectangles
			Rectangle rect1 = new Rectangle(0, 0, marker_layout_.Width, marker_layout_.Height);
			Rectangle rect2 = rect1;
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

		#region events

		public class TriggerEventArgs : EventArgs
		{
			public TriggerEventArgs(int trigger_id)
			{
				trigger_id_ = trigger_id;
			}
			public int trigger_id_;
		}

		public delegate void SelectTriggerEvent(object sender, TriggerEventArgs e);

		public event SelectTriggerEvent SelectTrigger;

		public void OnSelectTrigger(TriggerEventArgs e)
		{
			if (null != SelectTrigger)
				SelectTrigger(this, e);	
		}

		#endregion

		#region event handlers

		private void OnApplicationIdle(object sender, EventArgs e)
		{
			while (AppStillIdle)
			{
				AdvanceAnimation();
				Render();
			}
		}

		protected override void OnMouseDown(MouseEventArgs e)
		{
			// calculate new selection
			Point cursor_location = new Point(e.X, e.Y);
			ScreenToWorld(ref cursor_location);
			int new_selection = triggers_.GetIndex(cursor_location, marker_layout_);
			if ( new_selection < 0)
			{
				drag_start_.X = e.X;
				drag_start_.Y = e.Y;
				old_camera_position_ = camera_.Position;
				drag_state_ = DragState.DisplaceCamera;
				Capture = true;
			}
			else
			{
				Capture = false;
				drag_state_ = DragState.None;
			}
			if (selection_ == new_selection)
				return;
			if (selection_ >= 0)
				triggers_.descriptions_[selection_].status_ &= ~TriggerStatus.Selected;
			if (new_selection >= 0)
				triggers_.descriptions_[new_selection].status_ |= TriggerStatus.Selected;
			selection_ = new_selection;
			// fire the event
			OnSelectTrigger(new TriggerEventArgs(selection_));
			base.OnMouseDown(e);
		}

		protected override void OnMouseMove(MouseEventArgs e)
		{
			if (DesignMode || null == triggers_)
				return;
			HighlightTriggers(new Point(e.X, e.Y));
			if (DragState.DisplaceCamera == drag_state_)
				camera_.SetPosition(
					old_camera_position_.X + (drag_start_.X - e.X) * camera_.Zoom,
					old_camera_position_.Y + (drag_start_.Y - e.Y) * camera_.Zoom);
			base.OnMouseMove(e);
		}

		protected override void OnMouseUp(MouseEventArgs e)
		{
			drag_state_ = DragState.None;
			Capture = false;
			base.OnMouseUp (e);
		}

		protected override void OnMouseWheel(MouseEventArgs e)
		{
			camera_.IncrementZoom(e.Delta / 120);
			base.OnMouseWheel (e);
		}


		protected override void OnPaint(PaintEventArgs e)
		{
			if (DesignMode || null == triggers_)
			{
				base.OnPaint(e);
				return;
			}
			Render();
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
			camera_.ViewportSize = ClientRectangle.Size;
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
			int radius_h = marker_layout_.Width  / 2;
			int radius_v = marker_layout_.Height / 2;
			return Rectangle.FromLTRB(x - radius_h, y - radius_v, x + radius_h, y + radius_v);
		}

		private void ScreenToWorld(ref Point point)
		{
			point.X = (int)(point.X * camera_.Zoom + camera_.Position.X - ClientRectangle.Width  * camera_.Zoom / 2);
			point.Y = (int)(point.Y * camera_.Zoom + camera_.Position.Y - ClientRectangle.Height * camera_.Zoom / 2);
		}

		private void ScreenToWorld(ref PointF point)
		{
			point.X = point.X * camera_.Zoom + camera_.Position.X - ClientRectangle.Width  * camera_.Zoom / 2;
			point.Y = point.Y * camera_.Zoom + camera_.Position.Y - ClientRectangle.Height * camera_.Zoom / 2;
		}

		#endregion

		#region data

		private int              active_ = -1;
		private BSpline          bspline_;
		private int              cell_spacing_ = 64;
		private Point            drag_start_;
		private DragState        drag_state_;
		private MarkerLayout     marker_layout_;
		private int              selection_ = -1;
		private float            temperature_;
		private TriggerContainer triggers_;
		private TriggerRenderer  trigger_renderer_;
		// camera
		private Camera camera_;
		private PointF old_camera_position_;
		// DirectX
		private Device device_;
		// animation
		private const int      step_duration_ = 1000; // milliseconds
		private int            last_step_time_;
		private AnimationState animation_state_;

		#endregion

		#region Component Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{

		}

		#endregion
		
		#region Component Designer data

		private System.ComponentModel.IContainer components = null;

		#endregion
	}
}
