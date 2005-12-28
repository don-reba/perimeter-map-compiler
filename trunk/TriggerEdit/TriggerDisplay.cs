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
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace TriggerEdit
{
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
			LastStep,
			Done
		}

		#endregion

		#region interface

		public TriggerDisplay()
		{
			// This call is required by the Windows.Forms Form Designer.
			InitializeComponent();
			// initialize variables
			cell_size_ = new Size(Font.Height * 6, Font.Height * 2 + 4);
			animation_state_ = AnimationState.Off;
			// hook event handlers
			Application.Idle += new EventHandler(OnApplicationIdle);
		}

		public void SetTriggers(ref Trigger[] triggers)
		{
			triggers_ = triggers;
			// initialize the adjacency matrix
			trigger_adj_ = new ArrayList(triggers_.Length);
			for (int i = 0; i != triggers_.Length; ++i)
				trigger_adj_.Add(new BitArray(triggers_.Length));
			for (int i = 0; i != triggers_.Length; ++i)
			{
				foreach (Trigger.Link link in triggers_[i].links)
				{
					((BitArray)trigger_adj_[i])[link.target] = true;
					((BitArray)trigger_adj_[link.target])[i] = true;
				}
			}
			Random rand = new Random(0);
			bounds_ = new Rectangle(
				0,
				0,
				(cell_size_.Width  + cell_spacing_) * ((int)Math.Sqrt(triggers_.Length) + 1) * 2,
				(cell_size_.Height + cell_spacing_) * ((int)Math.Sqrt(triggers_.Length) + 1) * 2);
			bounds_.Inflate(cell_size_);
			bounds_ = Rectangle.Union(bounds_, ClientRectangle);
			for (int i = 0; i != triggers_.Length; ++i)
			{
				triggers_[i].X = cell_size_.Width  / 2 + rand.Next(bounds_.Width  - cell_size_.Width);
				triggers_[i].Y = cell_size_.Height / 2 + rand.Next(bounds_.Height - cell_size_.Height);
			}
			history_ = new Point[triggers_.Length * 3];
			InitGraphics();
			bspline_   = new BSpline(history_, triggers_, 32);
			animation_state_ = AnimationState.FirstStep;
		}

		public int GetTriggerAtPoint(Point point)
		{
			int radius_h = cell_size_.Width  / 2;
			int radius_v = cell_size_.Height / 2;
			int index;
			for (index = 0; index != triggers_.Length; ++index)
			{
				int x = triggers_[index].X + AutoScrollPosition.X;
				int y = triggers_[index].Y + AutoScrollPosition.Y;
				if (
					x - radius_h <= point.X &&
					x + radius_h >= point.X &&
					y - radius_v <= point.Y &&
					y + radius_v >= point.Y)
					break;
			}
			return (triggers_.Length == index) ? -1 : index;
		}

		#endregion

		#region internal implementation

		private void AdjustBounds()
		{
			// find the the bounding rectangle
			Point min = new Point(int.MaxValue, int.MaxValue);
			Point max = new Point(int.MinValue, int.MinValue);
			foreach (Trigger trigger in triggers_)
			{
				if (min.X > trigger.X) min.X = trigger.X;
				if (min.Y > trigger.Y) min.Y = trigger.Y;
				if (max.X < trigger.X) max.X = trigger.X;
				if (max.Y < trigger.Y) max.Y = trigger.Y;
			}
			Rectangle old_bounds = bounds_;
			bounds_ = Rectangle.FromLTRB(min.X, min.Y, max.X, max.Y);
			bounds_.Inflate(cell_size_.Width / 2, cell_size_.Height / 2);
			// normalize the vertices
			for (int i = 0; i != triggers_.Length; ++i)
			{
				triggers_[i].X -= bounds_.Left;
				triggers_[i].Y -= bounds_.Top;
			}
			// reposition the viewport, if necessary
			AutoScrollMinSize = bounds_.Size;
		}

		private void AdvanceAnimation()
		{
			switch (animation_state_)
			{
				case AnimationState.FirstStep:
				{
					Point[] positions = new Point[triggers_.Length];
					// copy trigger positions
					for (uint i = 0; i != triggers_.Length; ++i)
					{
						positions[i].X = triggers_[i].X;
						positions[i].Y = triggers_[i].Y;
					}
					// shift
					temperature_ = 1.0f;
					ShiftVertices(ref positions);
					// copy positions into the history array
					uint history_iter = 0;
					for (uint i = 0; i != triggers_.Length; ++i)
					{
						history_[history_iter] = positions[i];
						history_iter += 3;
					}
					// shift
					ShiftVertices(ref positions);
					// copy positions into the history array
					history_iter = 1;
					for (uint i = 0; i != triggers_.Length; ++i)
					{
						history_[history_iter] = positions[i];
						history_iter += 3;
					}
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
						Point[] positions = new Point[triggers_.Length];
						uint history_iter = 1;
						for (uint i = 0; i != triggers_.Length; ++i)
						{
							positions[i] = history_[history_iter];
							history_iter += 3;
						}
						// shift
						ShiftVertices(ref positions);
						// copy positions into the history array
						history_iter = 2;
						for (uint i = 0; i != triggers_.Length; ++i)
						{
							history_[history_iter] = positions[i];
							history_iter += 3;
						}
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
						Point [] positions = new Point[triggers_.Length];
						// shift history and recall the most recent positions
						uint history_iter = 0;
						for (uint i = 0; i != triggers_.Length; ++i)
						{
							history_[history_iter + 0] = history_[history_iter + 1];
							history_[history_iter + 1] = history_[history_iter + 2];
							positions[i]               = history_[history_iter + 2];
							history_iter += 3;
						}
						// calculate new positions
						float temperature = ShiftVertices(ref positions);
						if (temperature <= 3.0f)
						{
							while (ShiftVertices(ref positions) >= 1.0f);
							// advance state
							bspline_.SegmentMode = BSpline.Mode.Last;
							animation_state_ = AnimationState.LastStep;
						}
						else if (temperature <= 24.0f)
							while (ShiftVertices(ref positions) > 3.0f);
						// copy positions into the history array
						history_iter = 2;
						for (uint i = 0; i != triggers_.Length; ++i)
						{
							history_[history_iter] = positions[i];
							history_iter += 3;
						}
						// move vertices
						bspline_.Set(0.0f);
						// advance state
						last_step_time_ = Environment.TickCount;
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
				Message msg;
				return !PeekMessage(out msg, IntPtr.Zero, 0, 0, 0);
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
				device_ = new Device(
					0,
					DeviceType.Hardware,
					this, 
					CreateFlags.HardwareVertexProcessing,
					presentParams);
				trigger_renderer_ = new TriggerRenderer(device_, Font);
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
			Vector2 position = new Vector2(
				ClientRectangle.Width / 2.0f  - AutoScrollPosition.X,
				ClientRectangle.Height / 2.0f - AutoScrollPosition.Y);
			device_.Transform.View = Microsoft.DirectX.Matrix.LookAtLH(
				new Vector3(position.X, position.Y, 1.0f),
				new Vector3(position.X, position.Y, 0.0f),
				new Vector3(0.0f,       1.0f,       0.0f));
			device_.Transform.Projection = Microsoft.DirectX.Matrix.OrthoLH(
				-ClientRectangle.Width,
				-ClientRectangle.Height,
				0.1f,
				2.0f);
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

		private float ShiftVertices(ref Point[] positions)
		{
			float k = cell_spacing_;
			float range = (Math.Max(cell_size_.Width, cell_size_.Height) + k) * 3;
			// calculate forces
			for (int i = 0; i != triggers_.Length; ++i)
			{
				float v_x = 0;
				float v_y = 0;
				BitArray adj = (BitArray)trigger_adj_[i];
				for (int j = 0; j != triggers_.Length; ++j)
					if (i != j)
						if (adj[j])
						{
							// attraction
							float delta_x   = positions[j].X - positions[i].X;
							float delta_y   = positions[j].Y - positions[i].Y;
							float dx = Math.Max(0, Math.Abs(delta_x) - cell_size_.Width);
							float dy = Math.Max(0, Math.Abs(delta_y) - cell_size_.Height);
							float delta_abs = (float)Math.Sqrt(dx * dx + dy * dy);
							if (delta_abs > k)
							{
								float ratio = delta_abs / k;
								delta_x *= ratio;
								delta_y *= ratio;
								v_x += delta_x;
								v_y += delta_y;
							}
							else if (i < j)
								v_x += cell_size_.Width + k;
						}
						else
						{
							// repulsion
							float delta_x       = positions[i].X - positions[j].X;
							float delta_y       = positions[i].Y - positions[j].Y;
							float dx = Math.Max(0, Math.Abs(delta_x) - cell_size_.Width);
							float dy = Math.Max(0, Math.Abs(delta_y) - cell_size_.Height);
							float delta_abs_sqr = dx * dx + dy * dy;
							if (delta_abs_sqr > 0 && delta_abs_sqr < range * range)
							{
								float ratio = k * k / delta_abs_sqr;
								delta_x *= ratio;
								delta_y *= ratio;
								v_x += delta_x;
								v_y += delta_y;
							}
						}
				triggers_[i].v_x_ = v_x;
				triggers_[i].v_y_ = v_y;
			}
			// displace vertices
			float temperature = k * 4 / temperature_;
			for (int i = 0; i != triggers_.Length; ++i)
			{
				float v_x = triggers_[i].v_x_;
				float v_y = triggers_[i].v_y_;
				float v_abs = (float)Math.Sqrt(v_x * v_x + v_y * v_y);
				if (v_abs != 0)
				{
					// calculate new coordinates
					float x = positions[i].X + (v_x / v_abs) * Math.Min(v_abs, temperature);
					float y = positions[i].Y + (v_y / v_abs) * Math.Min(v_abs, temperature);
					// set the new coordinates
					positions[i].X = (int)Math.Round(x);
					positions[i].Y = (int)Math.Round(y);
				}
			}
			// cool down, with simmering
			if (temperature <= 2.0f)
				temperature_ *= 1.01f;
			else if (temperature <= 3)
				temperature_ *= 1.05f;
			else
				temperature_ *= 1.1f;
			return temperature;
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
				AdjustBounds();
				Render();
			}
		}

		protected override void OnMouseDown(MouseEventArgs e)
		{
			int new_selection = GetTriggerAtPoint(new Point(e.X, e.Y));
			if (selection_ == new_selection)
				return;
			if (selection_ >= 0)
				triggers_[selection_].outline_ = Color.Orange;
			if (new_selection >= 0)
				triggers_[new_selection].outline_ = Color.Yellow;
			selection_ = new_selection;
			OnSelectTrigger(new TriggerEventArgs(selection_));
			base.OnMouseDown(e);
		}


		protected override void OnMouseMove(MouseEventArgs e)
		{
			base.OnMouseMove(e);
			if (DesignMode || null == triggers_)
				return;
			// find the cell under cursor
			int index = GetTriggerAtPoint(new Point(e.X, e.Y));
			if (index == active_)
				return;
			// unhighlight the previous cell and its children
			if (active_ >= 0)
			{
				triggers_[active_].color_ = Color.Orange;
				foreach (Trigger.Link link in triggers_[active_].links)
					triggers_[link.target].color_ = Color.Orange;
			}
			active_ = index;
			if (index < 0)
				return;
			// highlight the selected trigger and its dependants
			triggers_[index].color_ = Color.Yellow;
			foreach (Trigger.Link link in triggers_[index].links)
				triggers_[link.target].color_ = Color.Gold;
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
			int radius_h = cell_size_.Width  / 2;
			int radius_v = cell_size_.Height / 2;
			return Rectangle.FromLTRB(x - radius_h, y - radius_v, x + radius_h, y + radius_v);
		}

		#endregion

		#region import

		[StructLayout(LayoutKind.Sequential)]
		public struct Message
		{
			public IntPtr hWnd;
			public uint   msg;
			public IntPtr wParam;
			public IntPtr lParam;
			public uint   time;
			public System.Drawing.Point p;
		}

		[System.Security.SuppressUnmanagedCodeSecurity] // We won't use this maliciously
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		public static extern bool PeekMessage(out Message msg, IntPtr hWnd, uint messageFilterMin, uint messageFilterMax, uint flags);

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

		#region data

		private Rectangle bounds_;
		private BSpline   bspline_;
		private Size      cell_size_;
		private int       cell_spacing_ = 64;
		private Point[]   history_;
		private int       active_ = -1;
		private int       selection_ = -1;
		private float     temperature_;
		private ArrayList trigger_adj_;
		private Trigger[] triggers_;
		private TriggerRenderer trigger_renderer_;
		// DirectX
		Device device_;
		// animation
		private const int      step_duration_ = 1000; // milliseconds
		private int            last_step_time_;
		private AnimationState animation_state_;

		#endregion
	}
}
