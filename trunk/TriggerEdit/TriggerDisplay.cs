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
using System.Threading;
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
			CopyGhost,
			DisplaceCamera,
			DisplaceMarker
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

		public new void Dispose()
		{
			device_.Dispose();
		}

		public void Reset(ref TriggerContainer triggers)
		{
			triggers_ = triggers;
			int t1 = Environment.TickCount;
			PreprocessVertices(ref triggers_.positions_);
			NormalizeVertices(ref triggers_.positions_);
			FitVerticesToGrid(ref triggers_.positions_);
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
			animation_state_  = AnimationState.FirstStep;
			animation_active_ = true;
		}

		public int Selection
		{
			get { return selection_; }
		}

		public void ToggleAnimation()
		{
			ToggleAnimation(!animation_active_);
		}

		public void ToggleAnimation(bool on)
		{
			animation_active_ = on;
			if (on)
				last_step_time_ = Environment.TickCount;
		}

		#endregion

		#region implementation

		private void AdvanceAnimation()
		{
			if (!animation_active_)
				return;
			switch (animation_state_)
			{
				case AnimationState.FirstStep:
					for (int i = 0; i != triggers_.count_; ++i)
						triggers_.history_[i].Set(triggers_.positions_[i]);
					triggers_.InterpolationRatio = 0.0f;
					animation_state_ = AnimationState.Normal;
					temperature_     = 1.0f;
					last_step_time_  = Environment.TickCount;
					break;
				case AnimationState.Normal:
					float d = (Environment.TickCount - last_step_time_) * animation_speed_;
					animation_remainder_ -= d;
					if (animation_remainder_ < 0)
					{
						while (animation_remainder_ < 0)
						{
							animation_remainder_step_ = ShiftVertices(ref triggers_.positions_);
							animation_remainder_ += animation_remainder_step_;
							if (animation_remainder_step_ <= 0.1f)
								animation_state_ = AnimationState.Done;
							// cap the time allowed for computing animation
							if (Environment.TickCount - last_step_time_ > 200)
								break;
						}
						for (int i = 0; i != triggers_.count_; ++i)
							triggers_.history_[i].Add(triggers_.positions_[i]);
					}
					triggers_.InterpolationRatio = Math.Min(
						1.0f,
						1.0f - animation_remainder_ / animation_remainder_step_);
					last_step_time_ = Environment.TickCount;
					break;
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
			if (active_ >= 0 && active_ < triggers_.count_)
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
			Device.IsUsingEventHandlers = false;
			// get system specs
			AdapterInformation adapter_info = Manager.Adapters.Default;
			int result;
			int quality_levels = 0;
			MultiSampleType multisample_type = (MultiSampleType)4;
			for (; multisample_type >= 0; multisample_type--)
				if (Manager.CheckDeviceMultiSampleType(
					adapter_info.Adapter,
					DeviceType.Hardware,
					adapter_info.CurrentDisplayMode.Format,
					true,
					multisample_type,
					out result,
					out quality_levels))
					break;
			// get device caps
			Caps caps = Manager.GetDeviceCaps(adapter_info.Adapter, DeviceType.Hardware);
			CreateFlags create_flags = 0;
			if (caps.DeviceCaps.SupportsHardwareTransformAndLight)
				create_flags |= CreateFlags.HardwareVertexProcessing;
			else
				create_flags |= CreateFlags.SoftwareVertexProcessing;
			// initialize device
			PresentParameters presentParams = new PresentParameters();
			presentParams.AutoDepthStencilFormat = DepthFormat.D16;
			presentParams.BackBufferCount        = 1;
			presentParams.EnableAutoDepthStencil = true;
			presentParams.MultiSample            = multisample_type;
			presentParams.MultiSampleQuality     = 0;
			presentParams.SwapEffect             = SwapEffect.Discard;
			presentParams.Windowed               = true;
			if (null == device_)
			{
				try
				{
					device_ = new Device(
						0,
						DeviceType.Hardware,
						this, 
						create_flags,
						presentParams);
					device_.EvictManagedResources();
				}
				catch (InvalidCallException)
				{
					MessageBox.Show(
						"Direct 3D device could not be created.\n"
						+ "The program will now terminate.",
						"Error",
						MessageBoxButtons.OK,
						MessageBoxIcon.Error);
					Application.Exit();
				}
				trigger_renderer_ = new TriggerRenderer(device_, marker_layout_, Font);
			}
			else
			{
				OnLostDevice();
				while (!device_.CheckCooperativeLevel())
					Thread.Sleep(250);
				device_.Reset(presentParams);
				OnResetDevice();
			}
			device_.RenderState.CullMode = Cull.None;
			device_.RenderState.Lighting = false;
			return true;
		}

		private void OnLostDevice()
		{
			trigger_renderer_.OnLostDevice();
		}

		private void OnResetDevice()
		{
			trigger_renderer_.OnResetDevice();
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

		private void FitVerticesToGrid(ref PointF[] positions)
		{
			float ka_x = cell_spacing_ * 2 + marker_layout_.Width;
			float ka_y = cell_spacing_ * 2 + marker_layout_.Height;
			Hashtable hash = new Hashtable(positions.Length);
			// snap positions to grid
			for (int i = 0; i != positions.Length; ++i)
			{
				Point result = new Point(
					(int)(positions[i].X / ka_x),
					(int)(positions[i].Y / ka_y));
				if (positions[i].X % ka_x > ka_x / 2.0f)
					++result.X;
				if (positions[i].Y % ka_y > ka_y / 2.0f)
					++result.Y;
				// check if the position is already occupied
				// NOTE: it is unlikely that the data are hashed efficiently
				if (hash.Contains(result))
				{
					int n = 1;
					Point neighbour = Point.Empty;
					bool slot_found = false;
					do
					{
						RandomPermutation permutation = new RandomPermutation(8 * n);
						foreach (int j in permutation)
						{
							int mode   = j % 4;
							int offset = j / 4;
							switch (mode)
							{
								case 0:
									neighbour.X = result.X - n + offset;
									neighbour.Y = result.Y - n;
									break;
								case 1:
									neighbour.X = result.X - n + offset + 1;
									neighbour.Y = result.Y + n;
									break;
								case 2:
									neighbour.X = result.X - n;
									neighbour.Y = result.Y - n + offset + 1;
									break;
								case 3:
									neighbour.X = result.X + n;
									neighbour.Y = result.Y - n + offset;
									break;
							}
							slot_found = !hash.Contains(neighbour);
							if (slot_found)
								break;
						}
						++n;
					} while (!slot_found);
					result = neighbour;
				}
				hash.Add(result, null);
				positions[i].X = result.X * ka_x;
				positions[i].Y = result.Y * ka_y;
			}
		}

		private void PreprocessVertices(ref PointF[] positions)
		{
			float ka_x = cell_spacing_ * 2 + marker_layout_.Width;
			float ka_y = cell_spacing_ * 2 + marker_layout_.Height;
			// spread the vertices across the initial square
			// TODO: try circle
			int range_x = (int)(64.0f * ka_x * (float)Math.Sqrt(positions.Length));
			int range_y = (int)(64.0f * ka_y * (float)Math.Sqrt(positions.Length));
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
					if (d_sqr > 0)
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
			float temperature = 4 / temperature_;
			for (int i = 0; i != triggers_.count_; ++i)
			{
				float v_x = triggers_.velocities_[i].X;
				float v_y = triggers_.velocities_[i].Y;
				float v_abs = (float)Math.Sqrt(v_x * v_x + v_y * v_y);
				if (v_abs != 0)
				{
					// set the new coordinates
					positions[i].X = positions[i].X + (v_x / v_abs) * Math.Min(v_abs, temperature);
					positions[i].Y = positions[i].Y + (v_y / v_abs) * Math.Min(v_abs, temperature);
				}
			}
			// cool down
			if (temperature > 0.5f)
			{
				temperature_ *= 1.002f;
				if (temperature < 0.5)
					temperature = 0.5f;
			}
			return temperature;
		}

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

		private void SetSelection(int index)
		{
			if (selection_ == index)
				return;
			if (selection_ >= 0 && selection_ < triggers_.count_)
				triggers_.descriptions_[selection_].status_ &= ~TriggerStatus.Selected;
			if (index >= 0 && index < triggers_.count_)
				triggers_.descriptions_[index].status_ |= TriggerStatus.Selected;
			selection_ = index;
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
			if (new_selection < 0)
			{
				drag_start_.X = e.X;
				drag_start_.Y = e.Y;
				old_camera_position_ = camera_.Position;
				drag_state_ = DragState.DisplaceCamera;
				Capture = true;
			}
			else
			{
				PointF position = PointF.Empty;
				triggers_.GetInterpolatedPosition(new_selection, ref position);
				float tear_off_x = position.X - marker_layout_.Width / 2 + marker_layout_.TearOffWidth;
				if (cursor_location.X < tear_off_x)
					drag_state_ = DragState.DisplaceMarker;
				else
					drag_state_ = DragState.CopyGhost;
				Capture = true;
			}
			SetSelection(new_selection);
			// fire the event
			OnSelectTrigger(new TriggerEventArgs(selection_));
			base.OnMouseDown(e);
		}

		protected override void OnMouseMove(MouseEventArgs e)
		{
			if (DesignMode || null == triggers_)
				return;
			HighlightTriggers(new Point(e.X, e.Y));
			switch (drag_state_)
			{
				case DragState.DisplaceCamera:
					camera_.SetPosition(
						old_camera_position_.X + (drag_start_.X - e.X) * camera_.Zoom,
						old_camera_position_.Y + (drag_start_.Y - e.Y) * camera_.Zoom);
					break;
				case DragState.DisplaceMarker:
					if (selection_ < 0)
						break;
					triggers_.positions_[selection_] = new Point(e.X, e.Y);
					ScreenToWorld(ref triggers_.positions_[selection_]);
					triggers_.history_[selection_].Set(triggers_.positions_[selection_]);
					break;
				case DragState.CopyGhost:
					if (selection_ < 0)
						break;
					if (triggers_.ghost_index_ < 0)
						triggers_.ghost_index_ = selection_;
					triggers_.ghost_position_ = new Point(e.X, e.Y);
					ScreenToWorld(ref triggers_.ghost_position_);
					break;
			}
			base.OnMouseMove(e);
		}

		protected override void OnMouseUp(MouseEventArgs e)
		{
			if (DragState.CopyGhost == drag_state_)
			{
				// calculate new selection
				Point cursor_location = new Point(e.X, e.Y);
				ScreenToWorld(ref cursor_location);
				int new_selection = triggers_.GetIndex(cursor_location, marker_layout_);
				if (new_selection < 0)
				{
					triggers_.ghost_index_ = -1;
					// create a copy of the selected node
					triggers_.Duplicate(selection_);
					triggers_.history_[triggers_.count_ - 1].Set(cursor_location);
					triggers_.positions_[triggers_.count_ - 1] = cursor_location;
					SetSelection(triggers_.count_ - 1);
				}
				else if (selection_ != new_selection)
				{
					if (triggers_.adjacency_[selection_, new_selection])
					{
						triggers_.DeleteLink(selection_, new_selection);
						SetSelection(-1);
					}
					else if (triggers_.adjacency_[new_selection, selection_])
					{
						triggers_.DeleteLink(new_selection, selection_);
						SetSelection(-1);
					}
					else
						triggers_.adjacency_[selection_, new_selection] = true;
				}
			}
			triggers_.ghost_index_ = -1;
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
			if (DesignMode)
				return;
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
		private int              cell_spacing_ = 96;
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
		private bool           animation_active_;
		private float          animation_remainder_;
		private float          animation_remainder_step_;
		private const float    animation_speed_ = 96.0f / 1000.0f; // pixels per millisecond
		private AnimationState animation_state_;
		private int            last_step_time_;

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
