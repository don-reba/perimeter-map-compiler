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
using System.Reflection;
using System.Threading;
using System.Windows.Forms;

namespace TriggerEdit
{
		#region using

		using TriggerDescription = TriggerContainer.TriggerDescription;
		using TriggerState       = TriggerContainer.TriggerDescription.State;
		using TriggerStatus      = TriggerContainer.TriggerDescription.Status;

		#endregion

	public class TriggerDisplay : System.Windows.Forms.Panel
	{
		//-------------
		// nested types
		//-------------

		#region

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
			Bud,
			Clone,
			Link,
			MoveCamera,
			MoveMarker
		}

		#endregion

		//----------
		// interface
		//----------

		#region

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
			rendering_enabled_   = false;
			// initialize the disc control
			disk_control_        = new DiscControl(
				(float)marker_layout_.Width,
				(float)marker_layout_.Height);
			Assembly a = Assembly.GetExecutingAssembly();
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.link.png")),
				new EventHandler(BeginTriggerLink));
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.collapse.png")),
				new EventHandler(CollapseTrigger));
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.condition.png")),
				new EventHandler(OnConditionSelected));
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.action.png")),
				new EventHandler(OnActionSelected));
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.clone.png")),
				new EventHandler(BeginTriggerClone));
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.bud.png")),
				new EventHandler(BeginTriggerBud));
			// hook event handlers
			if (!DesignMode)
				Application.Idle += new EventHandler(OnApplicationIdle);
		}

		public new void Dispose()
		{
			device_.Dispose();
		}

		public bool RenderingEnabled
		{
			get { return rendering_enabled_; }
			set { rendering_enabled_ = value; }
		}

		public void Reset(ref TriggerContainer triggers)
		{
			triggers_ = triggers;
			int t1 = Environment.TickCount;
			PreprocessVertices(ref triggers_.positions_);
			NormalizeVertices(ref triggers_.positions_);
			FitVerticesToGrid(ref triggers_.positions_);
			// center camera at the root node
			camera_.Position = triggers_.positions_[0];
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

		public bool ShowFrameRate
		{
			get { return show_frame_rate_; }	
			set { show_frame_rate_ = value; }
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

		public float Zoom
		{
			get
			{
				return camera_.Zoom;
			}
			set
			{
				while (camera_.Zoom > value)
					camera_.DecrementZoom();
				while (camera_.Zoom < value)
					camera_.IncrementZoom();
			}
		}

		#endregion

		//---------------
		// implementation
		//---------------

		#region

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
				try
				{
					OnResetDevice();
				}
				catch (DriverInternalErrorException)
				{}
			}
			device_.RenderState.CullMode = Cull.None;
			device_.RenderState.Lighting = false;
			// initialize FPS data
			fps_sprite_ = new Sprite(device_);
			fps_font_   = new Microsoft.DirectX.Direct3D.Font(device_, this.Font);
			// lay out the disk control
			disk_control_.Layout(device_);
			return true;
		}

		private void OnLostDevice()
		{
			trigger_renderer_.OnLostDevice();
			fps_font_.OnLostDevice();
			fps_sprite_.OnLostDevice();
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
			device_.Transform.View       = camera_.View;
			device_.Transform.Projection = camera_.Projection;
			// render disk
			if (selection_ >= 0 && disk_control_.Visible)
			{
				float x, y;
				triggers_.GetInterpolatedPosition(selection_, out x, out y);
				device_.Transform.World = Microsoft.DirectX.Matrix.Translation(x, y, 0);
				disk_control_.Render(device_);
				device_.Transform.World = Microsoft.DirectX.Matrix.Identity;
			}
			// render the trigger graph
			trigger_renderer_.Render(device_, triggers_);
			// render fps
			if (show_frame_rate_)
			{
				int frame_time = Environment.TickCount;
				if (frame_time - last_frame_time_ >= 128)
				{
					float fps        = frames_passed_ * 1000.0f / (frame_time - last_frame_time_);
					float ratio      = 0.1f;
					last_frame_time_ = frame_time;
					fps_             = (1 - ratio) * fps_ + ratio * fps;
					frames_passed_   = 0;
				}
				else
					++frames_passed_;
				string fps_text = string.Format("{0:.0} FPS", fps_);
				fps_sprite_.Begin(SpriteFlags.AlphaBlend | SpriteFlags.AlphaBlend);
				fps_font_.DrawText(fps_sprite_, fps_text, 2, 2, Color.Gold);
				fps_sprite_.End();
			}
			// end the scene
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
			disk_control_.Visible = (index > 0 && index < triggers_.count_);
			if (selection_ >= 0 && selection_ < triggers_.count_)
				triggers_.descriptions_[selection_].status_ &= ~TriggerStatus.Selected;
			if (index >= 0 && index < triggers_.count_)
				triggers_.descriptions_[index].status_ |= TriggerStatus.Selected;
			selection_ = index;
		}

		private void BeginCameraMove(object sender, EventArgs e)
		{
			drag_start_ = Cursor.Position;
			old_camera_position_ = camera_.Position;
			drag_state_ = DragState.MoveCamera;
			Capture = true;
		}

		private void BeginTriggerBud(object sender, EventArgs e)
		{
			// get cursor location
			Point int_cursor = Cursor.Position;
			int_cursor = PointToClient(int_cursor);
			PointF cursor = new PointF(
				(float)int_cursor.X,
				(float)int_cursor.Y);
			ScreenToWorld(ref cursor);
			// create a new marker ghost
			TriggerContainer.TriggerDescription description = new TriggerContainer.TriggerDescription();
			description.name_ = "[new]";
			triggers_.AddMarkerGhost(description, cursor);
			// create the link between the source marker and the ghost
			triggers_.AddLinkGhost(triggers_.GetInterpolatedPosition(selection_), cursor);
			// modify environment
			drag_state_ = DragState.Bud;
			Capture = true;
		}

		private void MoveTriggerBud()
		{
			// get cursor location
			Point int_cursor = Cursor.Position;
			int_cursor = PointToClient(int_cursor);
			PointF cursor = new PointF(
				(float)int_cursor.X,
				(float)int_cursor.Y);
			ScreenToWorld(ref cursor);
			// move the marker ghost
			triggers_.g_marker_positions_[0] = cursor;
			// move the ghost link
			triggers_.SetLinkGhost(0, triggers_.GetInterpolatedPosition(selection_), cursor);
		}

		private void EndTriggerBud(object sender, EventArgs e)
		{
			// modify environment
			drag_state_ = DragState.None;
			// create a new node
			triggers_.AddNew();
			triggers_.descriptions_[triggers_.count_ - 1]
				= (TriggerContainer.TriggerDescription)triggers_.g_marker_descriptions_[0];
			triggers_.adjacency_[selection_, triggers_.count_ - 1] = true;
			triggers_.history_[triggers_.count_ - 1].Set((PointF)triggers_.g_marker_positions_[0]);
			triggers_.positions_[triggers_.count_ - 1] = (PointF)triggers_.g_marker_positions_[0];
			// remove the ghosts
			triggers_.RemoveMarkerGhosts();
			triggers_.RemoveLinkGhosts();
		}

		private void BeginTriggerClone(object sender, EventArgs e)
		{
			// get cursor location
			Point int_cursor = Cursor.Position;
			int_cursor = PointToClient(int_cursor);
			PointF cursor = new PointF(
				(float)int_cursor.X,
				(float)int_cursor.Y);
			ScreenToWorld(ref cursor);
			// create a new marker ghost
			triggers_.AddMarkerGhost(triggers_.descriptions_[selection_], cursor);
			// copy the links
			foreach (int tail in triggers_.adjacency_.GetList(selection_))
				triggers_.AddLinkGhost(cursor, triggers_.positions_[tail]);
			for (int tail = 0; tail != triggers_.count_; ++tail)
				if (triggers_.adjacency_[tail, selection_])
					triggers_.AddLinkGhost(triggers_.GetInterpolatedPosition(tail), cursor);
			// modify environment
			drag_state_ = DragState.Clone;
			Capture = true;
		}

		private void MoveTriggerClone()
		{
			// get cursor location
			Point int_cursor = Cursor.Position;
			int_cursor = PointToClient(int_cursor);
			PointF cursor = new PointF(
				(float)int_cursor.X,
				(float)int_cursor.Y);
			ScreenToWorld(ref cursor);
			// move the marker ghost
			triggers_.g_marker_positions_[0] = cursor;
			// move the links
			int i = 0;
			foreach (int tail in triggers_.adjacency_.GetList(selection_))
				triggers_.SetLinkGhost(
					i++,
					cursor,
					triggers_.positions_[tail]);
			for (int tail = 0; tail != triggers_.count_; ++tail)
				if (triggers_.adjacency_[tail, selection_])
					triggers_.SetLinkGhost(
						i++,
						triggers_.GetInterpolatedPosition(tail),
						cursor);
		}

		private void EndTriggerClone(object sender, EventArgs e)
		{
			// modify environment
			drag_state_ = DragState.None;
			// create a copy of the selected node
			triggers_.Duplicate(selection_);
			triggers_.history_[triggers_.count_ - 1].Set((PointF)triggers_.g_marker_positions_[0]);
			triggers_.positions_[triggers_.count_ - 1] = (PointF)triggers_.g_marker_positions_[0];
			// remove the ghosts
			triggers_.RemoveMarkerGhosts();
			triggers_.RemoveLinkGhosts();
		}

		private void BeginTriggerLink(object sender, EventArgs e)
		{
			// get cursor location
			Point int_cursor = Cursor.Position;
			int_cursor = PointToClient(int_cursor);
			PointF cursor = new PointF(
				(float)int_cursor.X,
				(float)int_cursor.Y);
			ScreenToWorld(ref cursor);
			// create the link ghost connecting the source with the cursor
			TriggerContainer.Link link;
			link.head_      = triggers_.GetInterpolatedPosition(selection_);
			link.tail_      = cursor;
			link.snap_head_ = true;
			link.snap_tail_ = false;
			triggers_.AddLinkGhost(link);
			// modify environment
			drag_state_ = DragState.Link;
		}

		private void MoveTriggerLink()
		{
			// get cursor location
			Point int_cursor = Cursor.Position;
			int_cursor = PointToClient(int_cursor);
			PointF cursor = new PointF(
				(float)int_cursor.X,
				(float)int_cursor.Y);
			ScreenToWorld(ref cursor);
			// create the link ghost connecting the source with the cursor
			TriggerContainer.Link link;
			link.head_      = triggers_.GetInterpolatedPosition(selection_);
			link.tail_      = cursor;
			link.snap_head_ = true;
			link.snap_tail_ = false;
			triggers_.SetLinkGhost(0, link);
		}

		private void EndTriggerLink(object sender, EventArgs e)
		{
			// modify environment
			drag_state_ = DragState.None;
			triggers_.RemoveLinkGhosts();
			// get cursor location
			Point int_cursor = Cursor.Position;
			int_cursor = PointToClient(int_cursor);
			PointF cursor = new PointF(
				(float)int_cursor.X,
				(float)int_cursor.Y);
			ScreenToWorld(ref cursor);
			// find the target trigger
			int new_selection = triggers_.GetIndex(cursor, marker_layout_);
			if (new_selection < 0)
				return;
			// carry out the link operation
			if (0 == new_selection)
			{
				triggers_.adjacency_[0, selection_] = !triggers_.adjacency_[0, selection_];
				triggers_.adjacency_[selection_, 0] = false;
			}
			else
				triggers_.adjacency_[selection_, new_selection]
					= !triggers_.adjacency_[selection_, new_selection];
		}

		private void BeginTriggerMove(object sender, EventArgs e)
		{
			drag_state_ = DragState.MoveMarker;
			Capture = true;
		}

		private void CollapseTrigger(object sender, EventArgs e)
		{
			triggers_.Collapse(selection_);
			selection_ = -1;
		}

		#endregion

		//-------
		// events
		//-------

		#region

		#region trigger event

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

		#region zoom event

		public class ZoomEventArgs : EventArgs
		{
			public ZoomEventArgs(float zoom)
			{
				zoom_ = zoom;
			}
			public float zoom_;
		}

		public delegate void ZoomChangedEvent(object sender, ZoomEventArgs e);
		
		public event ZoomChangedEvent ZoomChanged;

		public void OnZoomChanged(ZoomEventArgs e)
		{
			if (null != ZoomChanged)
				ZoomChanged(this, e);
		}

		#endregion

		#region action event

		public event EventHandler ActionSelected;

		public void OnActionSelected(object sender, EventArgs e)
		{
			if (null != ActionSelected)
				ActionSelected(this, e);
		}

		#endregion

		#region condition event

		public event EventHandler ConditionSelected;

		public void OnConditionSelected(object sender, EventArgs e)
		{
			if (null != ConditionSelected)
				ConditionSelected(this, e);
		}

		#endregion

		#endregion

		//---------------
		// event handlers
		//---------------

		#region

		private void OnApplicationIdle(object sender, EventArgs e)
		{
			while (AppStillIdle)
			{
				if (!rendering_enabled_)
					continue;
				AdvanceAnimation();
				Render();
			}
		}

		protected override void OnMouseDown(MouseEventArgs e)
		{
			if (!rendering_enabled_ || triggers_ == null)
				return;
			// calculate new selection
			Point cursor_location = new Point(e.X, e.Y);
			ScreenToWorld(ref cursor_location);
			int new_selection = triggers_.GetIndex(cursor_location, marker_layout_);
			if (new_selection < 0)
			{
				bool drag_empty = true;
				// call the disk control event handler
				if (-1 != selection_)
				{
					float x, y;
					triggers_.GetInterpolatedPosition(selection_, out x, out y);
					drag_empty = !disk_control_.OnMouseClick(cursor_location.X - x, cursor_location.Y - y);
				}
				// begin dragging the camera
				if (drag_empty && DragState.None == drag_state_)
					BeginCameraMove(this, EventArgs.Empty);
			}
			else
			{
				SetSelection(new_selection);
				BeginTriggerMove(this, EventArgs.Empty);
			}
			// fire the event
			OnSelectTrigger(new TriggerEventArgs(selection_));
			base.OnMouseDown(e);
		}

		protected override void OnMouseMove(MouseEventArgs e)
		{
			if (!rendering_enabled_ || triggers_ == null)
				return;
			if (DesignMode || null == triggers_)
				return;
			HighlightTriggers(new Point(e.X, e.Y));
			if (-1 != selection_)
			{
				float ex = e.X;
				float ey = e.Y;
				ScreenToWorld(ref ex, ref ey);
				float x, y;
				triggers_.GetInterpolatedPosition(selection_, out x, out y);
				disk_control_.OnMouseMove(ex - x, ey - y);
			}
			switch (drag_state_)
			{
				case DragState.MoveCamera:
					camera_.SetPosition(
						old_camera_position_.X + (drag_start_.X - Cursor.Position.X) * camera_.Zoom,
						old_camera_position_.Y + (drag_start_.Y - Cursor.Position.Y) * camera_.Zoom);
					break;
				case DragState.MoveMarker:
					if (selection_ < 0)
						break;
					triggers_.positions_[selection_] = new Point(e.X, e.Y);
					ScreenToWorld(ref triggers_.positions_[selection_]);
					triggers_.history_[selection_].Set(triggers_.positions_[selection_]);
					break;
				case DragState.Bud:
					MoveTriggerBud();
					break;
				case DragState.Clone:
					MoveTriggerClone();
					break;
				case DragState.Link:
					MoveTriggerLink();
					break;
			}
			base.OnMouseMove(e);
		}

		protected override void OnMouseUp(MouseEventArgs e)
		{
			if (!rendering_enabled_ || triggers_ == null)
				return;
			switch (drag_state_)
			{
				case DragState.MoveCamera:
					drag_state_ = DragState.None;
					break;
				case DragState.MoveMarker:
					drag_state_ = DragState.None;
					break;
				case DragState.Bud:
					EndTriggerBud(this, EventArgs.Empty);
					break;
				case DragState.Clone:
					EndTriggerClone(this, EventArgs.Empty);
					break;
				case DragState.Link:
					EndTriggerLink(this, EventArgs.Empty);
					break;
			}
			Capture = false;
			base.OnMouseUp(e);
		}

		protected override void OnMouseWheel(MouseEventArgs e)
		{
			if (!rendering_enabled_)
				return;
			camera_.IncrementZoom(e.Delta / 120);
			OnZoomChanged(new ZoomEventArgs(camera_.Zoom));
			base.OnMouseWheel (e);
		}


		protected override void OnPaint(PaintEventArgs e)
		{
			if (DesignMode || null == triggers_)
			{
				base.OnPaint(e);
				return;
			}
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
//			InitGraphics();
			camera_.ViewportSize = ClientRectangle.Size;
			Invalidate();
		}

		#endregion

		//--------
		// utility
		//--------

		#region

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

		private void ScreenToWorld(ref float x, ref float y)
		{
			x = x * camera_.Zoom + camera_.Position.X - ClientRectangle.Width  * camera_.Zoom / 2;
			y = y * camera_.Zoom + camera_.Position.Y - ClientRectangle.Height * camera_.Zoom / 2;
		}

		private void ScreenToWorld(ref Point point)
		{
			float x = (float)point.X;
			float y = (float)point.Y;
			ScreenToWorld(ref x, ref y);
			point.X = (int)x;
			point.Y = (int)y;
		}

		private void ScreenToWorld(ref PointF point)
		{
			float x = point.X;
			float y = point.Y;
			ScreenToWorld(ref x, ref y);
			point.X = x;
			point.Y = y;
		}

		#endregion

		//-----
		// data
		//-----

		#region

		private int              active_ = -1;
		private int              cell_spacing_ = 96;
		private DiscControl      disk_control_;
		private Point            drag_start_;
		private DragState        drag_state_;
		private MarkerLayout     marker_layout_;
		private int              selection_ = -1;
		private float            temperature_;
		private TriggerContainer triggers_;
		private TriggerRenderer  trigger_renderer_;
		private bool             rendering_enabled_;
		// camera
		private Camera camera_;
		private PointF old_camera_position_;
		// DirectX
		private Device device_;
		// FPS
		float                           fps_;
		Microsoft.DirectX.Direct3D.Font fps_font_;
		Sprite                          fps_sprite_;
		int                             frames_passed_;
		int                             last_frame_time_;
		bool                            show_frame_rate_;
		// animation
		private bool           animation_active_;
		private float          animation_remainder_;
		private float          animation_remainder_step_;
		private const float    animation_speed_ = 96.0f / 1000.0f; // pixels per millisecond
		private AnimationState animation_state_;
		private int            last_step_time_;

		#endregion

		//----------------------------------
		// Component Designer generated code
		//----------------------------------

		#region

		/// <summary>
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{

		}

		#endregion

		//------------------------
		// Component Designer data
		//------------------------
		
		#region

		private System.ComponentModel.IContainer components = null;

		#endregion
	}
}
