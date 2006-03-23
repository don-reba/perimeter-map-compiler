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
	//------
	// using
	//------

	#region

	using TriggerDescription = TriggerContainer.TriggerDescription;
	using TriggerGroup       = TriggerContainer.Group;
	using TriggerLink        = TriggerContainer.Link;
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
			MoveMarker,
			Selection
		}

		private enum SelectionState
		{
			None,
			Select,
			Zoom
		}

		private enum SelectionMode
		{
			Replace,
			Add,
			Substract
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
			drag_offset_         = PointF.Empty;
			drag_start_          = new Point(0, 0);
			marker_layout_       = new MarkerLayout(Font);
			old_camera_position_ = new Point(0, 0);
			rendering_enabled_   = false;
			selection_           = new ArrayList();
			link_selection_      = new ArrayList();
			selection_rect_      = RectangleF.Empty;
			// initialize the disc control
			InitializeDiskControl();
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
			if (!ArePositionsSane(triggers_.positions_))
			{
				PreprocessVertices(triggers_.positions_);
				NormalizeVertices(triggers_.positions_);
				FitVerticesToGrid(triggers_.positions_);
			}
			// remove selection
			active_    = -1;
			selection_.Clear();
			// center camera at the root node
			camera_.Position = triggers_.positions_[0];
			// set initial marker positions
			// initalize miscellaneous
			InitGraphics();
			animation_state_  = AnimationState.FirstStep;
			animation_active_ = true;
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

		public void ZoomToFit()
		{
			// initialize bounds
			float min_x = float.MaxValue;
			float max_x = float.MinValue;
			float min_y = float.MaxValue;
			float max_y = float.MinValue;
			// calculate bounds around all trigger positions
			for (int i = 0; i != triggers_.positions_.Length; ++i)
			{
				if (triggers_.positions_[i].X < min_x) min_x = triggers_.positions_[i].X;
				if (triggers_.positions_[i].X > max_x) max_x = triggers_.positions_[i].X;
				if (triggers_.positions_[i].Y < min_y) min_y = triggers_.positions_[i].Y;
				if (triggers_.positions_[i].Y > max_y) max_y = triggers_.positions_[i].Y;
			}
			// offset bounds to include trigger size
			min_x -= marker_layout_.Width  / 2;
			max_x += marker_layout_.Width  / 2;
			min_y -= marker_layout_.Height / 2;
			max_y += marker_layout_.Height / 2;
			// set zoom
			ZoomRect = RectangleF.FromLTRB(min_x, min_y, max_x, max_y);
		}

		public void ZoomToSelection()
		{
			selection_state_ = SelectionState.Zoom;
		}

		public void GroupSelectedLinks()
		{
			// partition the selected links by the head trigger
			Hashtable partitions = new Hashtable(link_selection_.Count);
			for (int i = 0; i != link_selection_.Count; ++i)
			{
				int head = triggers_.adjacency_list_[(int)link_selection_[i]].head_;
				if (!partitions.ContainsKey(head))
					partitions.Add(head, new ArrayList());
				((ArrayList)partitions[head]).Add(link_selection_[i]);
				// ungroup the link
				triggers_.adjacency_list_.SetGroup((int)link_selection_[i], 0);
			}
			// work with each partition separately
			foreach (DictionaryEntry entry in partitions)
			{
				int       trigger = (int)      entry.Key;
				ArrayList list    = (ArrayList)entry.Value;
				// count the groups for this trigger
				int[] groups = new int[10];
				for (int i = 0; i != triggers_.adjacency_list_.Count; ++i)
					if (triggers_.adjacency_list_[i].head_ == trigger)
						++groups[triggers_.adjacency_list_[i].group_];
				// find the minimum unoccupied group
				int group = 1;
				for (;group != groups.Length; ++group)
					if (0 == groups[group])
						break;
				if (group == groups.Length)
					continue;
				// set the links to this group
				for (int i = 0; i != list.Count; ++i)
					triggers_.adjacency_list_.SetGroup((int)list[i], group);
			}
		}

		public void UngroupSelectedLinks()
		{
			for (int i = 0; i != link_selection_.Count; ++i)
				triggers_.adjacency_list_.SetGroup((int)link_selection_[i], 0);
		}

		/// <summary>
		/// Determine whether the selected links "die" after activation.
		/// </summary>
		/// <param name="persist">If false, the link will not activate more than once.</param>
		public void SetSelectedLinksPersistence(bool persist)
		{
			for (int i = 0; i != link_selection_.Count; ++i)
				triggers_.adjacency_list_.SetPersistence((int)link_selection_[i], persist);
		}

		//-----------
		// properties
		//-----------

		#region

		/// <summary>
		/// Animation speed in pixels per second.
		/// </summary>
		public float AnimationSpeed
		{
			get { return animation_speed_ * 1000.0f; }
			set { animation_speed_ = value / 1000.0f; }
		}

		public int MarkerLineCount
		{
			get { return marker_layout_.LineCount; }
			set
			{
				if (marker_layout_.LineCount != value)
				{
					marker_layout_.LineCount = value;
					InitializeDiskControl();
				}
			}
		}

		public float MarkerSpacing
		{
			get { return cell_spacing_; }
			set { cell_spacing_ = value; }
		}

		public ArrayList Selection
		{
			get { return ArrayList.ReadOnly(selection_); }
			set
			{
				if (selection_.IsReadOnly || selection_.IsFixedSize)
					throw new ArgumentException("Selection is read only or of fixed size.");
				SetSelection(value);
				OnSelectTrigger(new TriggerEventArgs(selection_));
			}
		}

		public bool ShowFrameRate
		{
			get { return show_frame_rate_; }	
			set { show_frame_rate_ = value; }
		}

		public float Zoom
		{
			get
			{
				return camera_.Zoom;
			}
			set
			{
				if (value < 1.0f)
					value = 1.0f;
				while (camera_.Zoom > value)
					camera_.DecrementZoom();
				while (camera_.Zoom < value)
					camera_.IncrementZoom();
			}
		}

		public RectangleF ZoomRect
		{
			set
			{
				// calculate ratio
				float ratio = (float)Math.Max(
					value.Width  / ClientRectangle.Width,
					value.Height / ClientRectangle.Height);
				if (ratio < 1.0f)
					ratio = 1.0f;
				// set camera
				camera_.Position = new PointF(
					value.Left / 2.0f + value.Right  / 2.0f,
					value.Top  / 2.0f + value.Bottom / 2.0f);
				while (camera_.Zoom > ratio)
					camera_.DecrementZoom();
				while (camera_.Zoom < ratio)
					camera_.IncrementZoom();
				OnZoomChanged(new ZoomEventArgs(camera_.Zoom));
			}
		}

		#endregion

		#endregion

		//---------------
		// implementation
		//---------------

		#region

		private void AdvanceAnimation()
		{
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
					if (d > 0.0f)
					{
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
								if (Environment.TickCount - last_step_time_ > 100)
									break;
							}
							for (int i = 0; i != triggers_.count_; ++i)
								triggers_.history_[i].Add(triggers_.positions_[i]);
						}
						triggers_.InterpolationRatio = Math.Min(
							1.0f,
							1.0f - animation_remainder_ / animation_remainder_step_);
						last_step_time_ = Environment.TickCount;
					}
					break;
			}
		}

		private void InitializeDiskControl()
		{
			// remember visibility of the disk control
			bool visible = false;
			if (null != disk_control_ && disk_control_.Visible)
				visible = true;
			// create a new disk control
			disk_control_ = new DiscControl(
				(float)marker_layout_.Width,
				(float)marker_layout_.Height);
			Assembly a = Assembly.GetExecutingAssembly();
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.link.png")),
				new EventHandler(BeginTriggerLink));
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.collapse.png")),
				new EventHandler(CollapseTrigger));
//			disk_control_.AddButton(
//				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.condition.png")),
//				new EventHandler(OnConditionSelected));
//			disk_control_.AddButton(
//				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.action.png")),
//				new EventHandler(OnActionSelected));
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.clone.png")),
				new EventHandler(BeginTriggerClone));
			disk_control_.AddButton(
				new Bitmap(a.GetManifestResourceStream("TriggerEdit.img.bud.png")),
				new EventHandler(BeginTriggerBud));
			// lay out the disk control
			if (null != device_ && device_.CheckCooperativeLevel())
				disk_control_.Layout(device_);
			// set visibility
			disk_control_.Visible = visible;
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
		protected bool HighlightTriggers(PointF position)
		{
			// find the trigger under the cursor
			ScreenToWorld(ref position);
			int index = triggers_.GetIndex(position, marker_layout_);
			if (disk_control_.Selection >= 0)
				index = -1;
			else if (index == active_)
				return (index >= 0);
			// unhighlight the previous cell and its neighbours
			if (active_ >= 0)
				UnhighlightTrigger(active_);
			active_ = index;
			if (index < 0)
				return false;
			// highlight the selected trigger and its dependants
			HighlightTrigger(index);
			return true;
		}

		protected void HighlightTrigger(int index)
		{
			triggers_.descriptions_[index].status_ |= TriggerStatus.Bright;
			for (int link = 0; link != triggers_.count_; ++link)
				if (triggers_.adjacency_matrix_[index, link])
					triggers_.descriptions_[link].status_ |= TriggerStatus.Dim;
		}

		protected void UnhighlightTrigger(int index)
		{
			triggers_.descriptions_[index].status_ &= ~TriggerStatus.Bright;
			for (int link = 0; link != triggers_.count_; ++link)
				if (triggers_.adjacency_matrix_[index, link])
					triggers_.descriptions_[link].status_ &= ~TriggerStatus.Dim;
		}

		/// <param name="position">in screen coordinates</param>
		protected bool HighlightLinks(PointF position)
		{
			// find the link under the cursor
			ScreenToWorld(ref position);
			if (active_link_ >= 0)
				triggers_.adjacency_list_.RemoveStatus(active_link_, TriggerLink.Status.Dim);
			int index = triggers_.GetLinkIndex(position, marker_layout_.Width, marker_layout_.Height);
			if (disk_control_.Selection >= 0)
				index = -1;
			else if (index == active_)
				return (index >= 0);
			active_link_ = index;
			if (index < 0)
				return false;
			triggers_.adjacency_list_.AddStatus(active_link_, TriggerLink.Status.Dim);
			return true;
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
				trigger_renderer_ = new TriggerRenderer(
					device_,
					marker_layout_,
					new SizeF(Font.Height, Font.Height),
					Font);
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
			// render the trigger graph
			device_.Transform.World = Microsoft.DirectX.Matrix.Translation(0.0f, 0.0f, -1.0f);
			trigger_renderer_.Render(device_, triggers_);
			// render disk
			if (selection_.Count == 1 && disk_control_.Visible)
			{
				float x, y;
				triggers_.GetInterpolatedPosition((int)selection_[0], out x, out y);
				device_.Transform.World = Microsoft.DirectX.Matrix.Translation(x, y, 0.0f);
				disk_control_.Render(device_);
				device_.Transform.World = Microsoft.DirectX.Matrix.Identity;
			}
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
			// render the selection rectangle
			if (DragState.Selection == drag_state_
				&& selection_rect_.Width  > 0.0f
				&& selection_rect_.Height > 0.0f)
				RenderSelectioRect();
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

		private void RenderSelectioRect()
		{
			float offset = 0.5f * camera_.Zoom;
			// initialize vertices
			CustomVertex.PositionColored[] vertices = new CustomVertex.PositionColored[10];
			for (int i = 0; i != 10; ++i)
			{
				vertices[i].Z     = 0.0f;
				vertices[i].Color = Color.Yellow.ToArgb();
			}
			// 0
			vertices[0].X = selection_rect_.Left - offset;
			vertices[0].Y = selection_rect_.Top  - offset;
			// 1
			vertices[1].X = selection_rect_.Left + offset;
			vertices[1].Y = selection_rect_.Top  + offset;
			// 2
			vertices[2].X = selection_rect_.Right + offset;
			vertices[2].Y = selection_rect_.Top   - offset;
			// 3
			vertices[3].X = selection_rect_.Right - offset;
			vertices[3].Y = selection_rect_.Top   + offset;
			// 4
			vertices[4].X = selection_rect_.Right  + offset;
			vertices[4].Y = selection_rect_.Bottom + offset;
			// 5
			vertices[5].X = selection_rect_.Right  - offset;
			vertices[5].Y = selection_rect_.Bottom - offset;
			// 6
			vertices[6].X = selection_rect_.Left   - offset;
			vertices[6].Y = selection_rect_.Bottom + offset;
			// 7
			vertices[7].X = selection_rect_.Left   + offset;
			vertices[7].Y = selection_rect_.Bottom - offset;
			// 8
			vertices[8].X = selection_rect_.Left - offset;
			vertices[8].Y = selection_rect_.Top  - offset;
			// 9
			vertices[9].X = selection_rect_.Left + offset;
			vertices[9].Y = selection_rect_.Top  + offset;
			// create the vertex buffer
			VertexBuffer vb = new VertexBuffer(
				typeof(CustomVertex.PositionColored),
				10,
				device_,
				Usage.WriteOnly,
				CustomVertex.PositionColored.Format,
				Pool.Managed);
			GraphicsStream vb_stream = vb.Lock(0, 0, LockFlags.None);
			vb_stream.Write(vertices);
			vb.Unlock();
			device_.SetStreamSource(0, vb, 0);
			device_.VertexFormat = CustomVertex.PositionColored.Format;
			// output geometry
			device_.DrawPrimitives(PrimitiveType.TriangleStrip, 0, 8);
			vb.Dispose();
		}

		private bool ArePositionsSane(PointF[] positions)
		{
			const int sample_size    = 8;
			const int faults_allowed = 2;
			// if there are few entries, we are good
			if (positions.Length < 8)
				return true;
			// filter out duplicate elements among the first sample_size entries
			SortedList list = new SortedList(new PointFComparer());
			for (int i = 0; i != sample_size; ++i)
				if (!list.ContainsKey(positions[i]))
					list.Add(positions[i], null);
			// return true if there were not more than faults_allowed duplicates
			return list.Count >= sample_size - faults_allowed;
		}

		private void NormalizeVertices(PointF[] positions)
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

		private void FitVerticesToGrid(PointF[] positions)
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

		private void PreprocessVertices(PointF[] positions)
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
						if (triggers_.adjacency_matrix_[i, j] || triggers_.adjacency_matrix_[j, i])
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

		/// <summary>
		/// Shortens length to the distance from the offset.
		/// </summary>
		/// <param name="length">Distance from zero.</param>
		/// <param name="offset">Radius of a ball around zero.</param>
		private void ShiftVertices_AdjustLength(ref float length, float offset)
		{
			if (length > 0)
				if (length < offset)
					length = 0.1f;
				else
					length -= offset;
			else
				if (length > -offset)
					length = -0.1f;
				else
					length += offset;
		}

		/// <param name="positions">must not be empty</param>
		private float ShiftVertices(ref PointF[] positions)
		{
			float k = cell_spacing_;
			float marker_width  = marker_layout_.Width;
			float marker_height = marker_layout_.Height;
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
					// manual inlining (performance-critical)
					// ShiftVertices_AdjustLength(ref dx, marker_width);
					if (dx > 0)
						if (dx < marker_width)
							dx = 0.1f;
						else
							dx -= marker_width;
					else
						if (dx > -marker_width)
						dx = -0.1f;
					else
						dx += marker_width;
					// manual inlining (performance-critical)
					// ShiftVertices_AdjustLength(ref dy, marker_height);
					if (dy > 0)
						if (dy < marker_height)
							dy = 0.1f;
						else
							dy -= marker_height;
					else
						if (dy > -marker_height)
						dy = -0.1f;
					else
						dy += marker_height;
					// calculate distance squared
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
					TriggerGroup group_i = triggers_.descriptions_[i].group_;
					TriggerGroup group_j = triggers_.descriptions_[j].group_;
					if (triggers_.adjacency_matrix_[i, j] || triggers_.adjacency_matrix_[j, i]
						|| (null != group_i && group_i == group_j))
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
					// calculate the offset as the minimum between v_abs and temperature
					float offset;
					if (v_abs < temperature)
						offset = v_abs;
					else
						offset = temperature;
					// set the new coordinates
					positions[i].X = positions[i].X + (v_x / v_abs) * offset;
					positions[i].Y = positions[i].Y + (v_y / v_abs) * offset;
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

		private void SetSelection(int index)
		{
			if (SelectionMode.Replace == selection_mode_)
			{
				// clear selected flags for previuos selection
				for (int i = 0; i != selection_.Count; ++i)
					triggers_.descriptions_[(int)selection_[i]].status_ &= ~TriggerStatus.Selected;
				selection_.Clear();
			}
			if (index < 0)
				return;
			if (  SelectionMode.Replace == selection_mode_
				|| SelectionMode.Add     == selection_mode_)
			{
				// add selection
				triggers_.descriptions_[index].status_ |= TriggerStatus.Selected;
				selection_.Add(index);
			} 
			else if (SelectionMode.Substract == selection_mode_)
			{
				// remove selection
				if (selection_.Contains(index))
				{
					triggers_.descriptions_[index].status_ &= ~TriggerStatus.Selected;
					selection_.Remove(index);
				}
			}
		}

		private void SetLinkSelection(int index)
		{
			if (SelectionMode.Replace == selection_mode_)
			{
				// clear selected flags for previuos selection
				for (int i = 0; i != link_selection_.Count; ++i)
					triggers_.adjacency_list_.RemoveStatus((int)link_selection_[i], TriggerLink.Status.Selected);
				link_selection_.Clear();
			}
			if (index < 0)
				return;
			if (  SelectionMode.Replace == selection_mode_
				|| SelectionMode.Add     == selection_mode_)
			{
				// add selection
				triggers_.adjacency_list_.AddStatus(index, TriggerLink.Status.Selected);
				link_selection_.Add(index);
			} 
			else if (SelectionMode.Substract == selection_mode_)
			{
				// remove selection
				if (link_selection_.Contains(index))
				{
					triggers_.adjacency_list_.RemoveStatus(index, TriggerLink.Status.Selected);
					link_selection_.Remove(index);
				}
			}
		}

		private void SetSelection(ArrayList list)
		{
			if (SelectionMode.Replace == selection_mode_)
			{
				// clear selected flags for previuos selection
				for (int i = 0; i != selection_.Count; ++i)
					triggers_.descriptions_[(int)selection_[i]].status_ &= ~TriggerStatus.Selected;
				// set the new selection
				selection_ = list;
				for (int i = 0; i != selection_.Count; ++i)
					triggers_.descriptions_[(int)selection_[i]].status_ |= TriggerStatus.Selected;
			}
			if (SelectionMode.Add == selection_mode_)
			{
				for (int i = 0; i != list.Count; ++i)
					triggers_.descriptions_[(int)list[i]].status_ |= TriggerStatus.Selected;
				for (int i = 0; i != list.Count; ++i)
					if (!selection_.Contains(list[i]))
						selection_.Add(list[i]);
			}
			if (SelectionMode.Substract == selection_mode_)
			{
				// create a list of nodes not in "list", and deselect others
				list.Sort();
				ArrayList not_list = new ArrayList();
				for (int i = 0; i != selection_.Count; ++i)
				{
					if (list.BinarySearch(selection_[i]) < 0)
						not_list.Add(selection_[i]);
					else
						triggers_.descriptions_[(int)selection_[i]].status_ &= ~TriggerStatus.Selected;
				}
				// make this list the current selection
				selection_ = not_list;
			}
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
			triggers_.AddLinkGhost(triggers_.GetInterpolatedPosition((int)selection_[0]), cursor);
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
			triggers_.SetLinkGhost(0, triggers_.GetInterpolatedPosition((int)selection_[0]), cursor);
		}

		private void EndTriggerBud(object sender, EventArgs e)
		{
			// modify environment
			drag_state_ = DragState.None;
			// create a new node
			triggers_.AddNew();
			triggers_.descriptions_[triggers_.count_ - 1]
				= (TriggerContainer.TriggerDescription)triggers_.g_marker_descriptions_[0];
			triggers_.AddLink((int)selection_[0], triggers_.count_ - 1);
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
			triggers_.AddMarkerGhost(triggers_.descriptions_[(int)selection_[0]], cursor);
			// copy the links
			for (int tail = 0; tail != triggers_.count_; ++tail)
				if (triggers_.adjacency_matrix_[(int)selection_[0], tail])
					triggers_.AddLinkGhost(cursor, triggers_.positions_[tail]);
			for (int tail = 0; tail != triggers_.count_; ++tail)
				if (triggers_.adjacency_matrix_[tail, (int)selection_[0]])
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
			for (int tail = 0; tail != triggers_.count_; ++tail)
				if (triggers_.adjacency_matrix_[(int)selection_[0], tail])
					triggers_.SetLinkGhost(
						i++,
						cursor,
						triggers_.positions_[tail]);
			for (int tail = 0; tail != triggers_.count_; ++tail)
				if (triggers_.adjacency_matrix_[tail, (int)selection_[0]])
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
			triggers_.Duplicate((int)selection_[0]);
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
			TriggerContainer.GhostLink link;
			link.head_      = cursor;
			link.tail_      = triggers_.GetInterpolatedPosition((int)selection_[0]);
			link.snap_head_ = false;
			link.snap_tail_ = true;
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
			TriggerContainer.GhostLink link;
			link.head_      = cursor;
			link.tail_      = triggers_.GetInterpolatedPosition((int)selection_[0]);
			link.snap_head_ = false;
			link.snap_tail_ = true;
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
				if (triggers_.adjacency_matrix_[0, (int)selection_[0]])
					triggers_.RemoveLink(0, (int)selection_[0]);
				else
					triggers_.AddLink(0, (int)selection_[0]);
			}
			else
			{
				if (triggers_.adjacency_matrix_[(int)selection_[0], new_selection])
					triggers_.RemoveLink((int)selection_[0], new_selection);
				else
					triggers_.AddLink((int)selection_[0], new_selection);
			}
		}

		private void BeginTriggerMove(object sender, EventArgs e)
		{
			// get cursor location
			Point cursor = Cursor.Position;
			cursor = PointToClient(cursor);
			ScreenToWorld(ref cursor);
			// get trigger location
			PointF trigger = triggers_.GetInterpolatedPosition((int)selection_[0]);
			// set drag offset
			drag_offset_.X = trigger.X - cursor.X;
			drag_offset_.Y = trigger.Y - cursor.Y;
			// set state
			drag_state_ = DragState.MoveMarker;
			Capture = true;
		}

		private void MoveTrigger()
		{
			// get cursor location
			Point cursor = Cursor.Position;
			cursor = PointToClient(cursor);
			ScreenToWorld(ref cursor);
			// change positoin
			triggers_.positions_[(int)selection_[0]] = new PointF(
				cursor.X + drag_offset_.X,
				cursor.Y + drag_offset_.Y);
			triggers_.history_[(int)selection_[0]].Set(triggers_.positions_[(int)selection_[0]]);
		}

		private void CollapseTrigger(object sender, EventArgs e)
		{
			disk_control_.Visible = false;
			triggers_.Collapse((int)selection_[0]);
			selection_.Clear();
		}

		private void BeginTriggerSelection(object sender, EventArgs e)
		{
			// get cursor location
			Point cursor = Cursor.Position;
			cursor = PointToClient(cursor);
			ScreenToWorld(ref cursor);
			// remember the coordinates
			drag_start_ = cursor;
			// set state
			drag_state_      = DragState.Selection;
			selection_state_ = SelectionState.Select;
			SetSelection(new ArrayList());
		}

		private void MoveTriggerSelection()
		{
			// get cursor location
			Point cursor = Cursor.Position;
			cursor = PointToClient(cursor);
			ScreenToWorld(ref cursor);
			// set the rectangle
			selection_rect_ = RectangleF.FromLTRB(
				(float)Math.Min(drag_start_.X, cursor.X),
				(float)Math.Min(drag_start_.Y, cursor.Y),
				(float)Math.Max(drag_start_.X, cursor.X),
				(float)Math.Max(drag_start_.Y, cursor.Y));
			// find out which triggers are within the rectangle
			ArrayList list = new ArrayList();
			float w = marker_layout_.Width  / 2;
			float h = marker_layout_.Height / 2;
			for (int i = 0; i != triggers_.count_; ++i)
			{
				PointF p = triggers_.GetInterpolatedPosition(i);
				if (  p.X - w >= selection_rect_.Left
					&& p.X + w <= selection_rect_.Right
					&& p.Y - h >= selection_rect_.Top
					&& p.Y + h <= selection_rect_.Bottom)
					list.Add(i);
			}
			SetSelection(list);
		}

		private void EndTriggerSelection(object sender, EventArgs e)
		{
			selection_rect_       = RectangleF.Empty;
			drag_state_           = DragState.None;
			selection_state_      = SelectionState.None;
			disk_control_.Visible = selection_.Count == 1;
			OnSelectTrigger(new TriggerEventArgs(selection_));
			
		}

		#endregion

		//-------
		// events
		//-------

		#region

		#region trigger event

		public class TriggerEventArgs : EventArgs
		{
			public TriggerEventArgs(ArrayList ids)
			{
				ids_ = ids;
			}
			public int this[int index]
			{
				get { return (int)ids_[index]; }
			}
			public int Count
			{
				get { return ids_.Count; }
			}
			private ArrayList ids_;
		}

		public delegate void SelectTriggerEvent(object sender, TriggerEventArgs e);

		public event SelectTriggerEvent SelectTrigger;

		public void OnSelectTrigger(TriggerEventArgs e)
		{
			if (null != SelectTrigger)
				SelectTrigger(this, e);	
		}

		#endregion

		#region link event

		public class LinkEventArgs : EventArgs
		{
			public LinkEventArgs(ArrayList ids)
			{
				ids_ = ids;
			}
			public int this[int index]
			{
				get { return (int)ids_[index]; }
			}
			public int Count
			{
				get { return ids_.Count; }
			}
			private ArrayList ids_;
		}

		public delegate void SelectLinkEvent(object sender, LinkEventArgs e);

		public event SelectLinkEvent SelectLink;

		public void OnSelectLink(LinkEventArgs e)
		{
			if (null != SelectLink)
				SelectLink(this, e);	
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
				if (!rendering_enabled_ || !animation_active_)
					continue;
				AdvanceAnimation();
				Render();
			}
		}

		protected override void OnMouseDown(MouseEventArgs e)
		{
			if (!rendering_enabled_ || triggers_ == null)
				return;
			switch (e.Button)
			{
				case MouseButtons.Left:   OnMouseLeftDown(e);   break;
				case MouseButtons.Middle: OnMouseMiddleDown(e); break;
				case MouseButtons.Right:  OnMouseRightDown(e);  break;
			}
			base.OnMouseDown(e);
			return;
		}

		private void OnMouseLeftDown(MouseEventArgs e)
		{
			// get cursor location
			Point cursor_location = new Point(e.X, e.Y);
			ScreenToWorld(ref cursor_location);
			// handle zoom selection
			if (selection_state_ == SelectionState.Zoom)
			{
				drag_start_     = cursor_location;
				selection_rect_ = RectangleF.Empty;
				drag_state_     = DragState.Selection;
				return;
			}
			// handle disk click
			if (disk_control_.Visible)
			{
				float x, y;
				triggers_.GetInterpolatedPosition((int)selection_[0], out x, out y);
				if (disk_control_.OnMouseClick(
					cursor_location.X - x,
					cursor_location.Y - y))
				{
					return;
				}
			}
			// set selection mode, depending on the modifier key pressed
			switch (Control.ModifierKeys)
			{
				case Keys.Shift:   selection_mode_ = SelectionMode.Add;       break;
				case Keys.Control: selection_mode_ = SelectionMode.Substract; break;
				default:           selection_mode_ = SelectionMode.Replace;   break;
			}
			// find the marker under the cursor
			int new_selection = triggers_.GetIndex(cursor_location, marker_layout_);
			// deselect the link, if a marker is selected
			if (new_selection >= 0)
				SetLinkSelection(-1);
			// handle marker dragging
			if (new_selection >= 0
				&& selection_.Count == 1
				&& (int)selection_[0] == new_selection)
			{
				BeginTriggerMove(this, EventArgs.Empty);
				return;
			}
			// handle selection change
			if (new_selection >= 0)
			{
				SetSelection(new_selection);
				disk_control_.Visible
					= (1 == selection_.Count)
					&& !triggers_.descriptions_[new_selection].is_virtual_;
				OnSelectTrigger(new TriggerEventArgs(selection_));
				BeginTriggerMove(this, EventArgs.Empty);
				return;
			}
			// handle marker selection
			if (new_selection < 0)
			{
				disk_control_.Visible = false;
				// find the link under the cursor
				new_selection = triggers_.GetLinkIndex(cursor_location, marker_layout_.Width, marker_layout_.Height);
				if (new_selection >= 0)
				{
					SetLinkSelection(new_selection);
					OnSelectLink(new LinkEventArgs(link_selection_));
				}
				else
				{
					SetLinkSelection(-1);
					OnSelectLink(new LinkEventArgs(link_selection_));
					BeginTriggerSelection(this, EventArgs.Empty);
				}
				return;
			}
		}

		private void OnMouseMiddleDown(MouseEventArgs e)
		{
			Zoom = 1.0f;
		}

		private void OnMouseRightDown(MouseEventArgs e)
		{
			// begin dragging the camera
			if (DragState.None == drag_state_)
				BeginCameraMove(this, EventArgs.Empty);
		}

		protected override void OnMouseMove(MouseEventArgs e)
		{
			if (!rendering_enabled_ || triggers_ == null)
				return;
			if (DesignMode || null == triggers_)
				return;
			if (disk_control_.Visible)
			{
				float ex = e.X;
				float ey = e.Y;
				ScreenToWorld(ref ex, ref ey);
				float x, y;
				triggers_.GetInterpolatedPosition((int)selection_[0], out x, out y);
				disk_control_.OnMouseMove(ex - x, ey - y);
			}
			HighlightTriggers(new PointF(e.X, e.Y));
			HighlightLinks(new PointF(e.X, e.Y));
			switch (drag_state_)
			{
				case DragState.MoveCamera:
					camera_.SetPosition(
						old_camera_position_.X + (drag_start_.X - Cursor.Position.X) * camera_.Zoom,
						old_camera_position_.Y + (drag_start_.Y - Cursor.Position.Y) * camera_.Zoom);
					break;
				case DragState.MoveMarker:
					MoveTrigger();
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
				case DragState.Selection:
				{
					switch (selection_state_)
					{
						case SelectionState.Zoom:
							float ex = e.X;
							float ey = e.Y;
							ScreenToWorld(ref ex, ref ey);
							selection_rect_ = RectangleF.FromLTRB(
								(float)Math.Min(drag_start_.X, ex),
								(float)Math.Min(drag_start_.Y, ey),
								(float)Math.Max(drag_start_.X, ex),
								(float)Math.Max(drag_start_.Y, ey));
							break;
						case SelectionState.Select:
							MoveTriggerSelection();
							break;
					}
				} break;
			}
			Render();
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
				case DragState.Selection:
				{
					switch (selection_state_)
					{
						case SelectionState.Zoom:
							ZoomRect         = selection_rect_;
							selection_rect_  = RectangleF.Empty;
							drag_state_      = DragState.None;
							selection_state_ = SelectionState.None;
							break;
						case SelectionState.Select:
							EndTriggerSelection(this, EventArgs.Empty);
							break;
					}
				} break;
			}
			Capture = false;
			base.OnMouseUp(e);
		}

		protected override void OnMouseWheel(MouseEventArgs e)
		{
			if (!rendering_enabled_)
				return;
			camera_.IncrementZoom(e.Delta / 120);
			while (camera_.Zoom < 1.0f)
				camera_.IncrementZoom();
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
			Render();
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

		private float            cell_spacing_ = 96;
		private DiscControl      disk_control_;
		private MarkerLayout     marker_layout_;
		private TriggerContainer triggers_;
		private TriggerRenderer  trigger_renderer_;
		private bool             rendering_enabled_;
		// dragging
		private Point     drag_start_;
		private DragState drag_state_;
		private PointF    drag_offset_;
		// selection
		private int            active_      = -1;
		private int            active_link_ = -1;
		private ArrayList      selection_;      // of int
		private ArrayList      link_selection_; // of int
		private SelectionMode  selection_mode_;
		private SelectionState selection_state_;
		private RectangleF     selection_rect_;
		// camera
		private Camera camera_;
		private PointF old_camera_position_;
		// DirectX
		private Device device_;
		// FPS
		private float                           fps_;
		private Microsoft.DirectX.Direct3D.Font fps_font_;
		private Sprite                          fps_sprite_;
		private int                             frames_passed_;
		private int                             last_frame_time_;
		private bool                            show_frame_rate_;
		// animation
		private bool           animation_active_;
		private float          animation_remainder_;
		private float          animation_remainder_step_;
		private float          animation_speed_ = 96.0f / 1000.0f; // pixels per millisecond
		private AnimationState animation_state_;
		private int            last_step_time_;
		private float          temperature_;

		#endregion

		//----------
		// generated
		//----------

		#region code

		/// <summary>
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{

		}

		#endregion
		
		#region data

		private System.ComponentModel.IContainer components = null;

		#endregion
	}
}
