using System;
using System.Drawing;
using Microsoft.DirectX;

namespace TriggerEdit
{
	/// <summary>
	/// Summary description for Camera.
	/// </summary>
	public class Camera
	{
		#region interface

		public Camera()
		{
			position_      = new PointF();
			projection_    = new Matrix();
			view_          = new Matrix();
			viewport_size_ = new Size(1, 1);
			zoom_          = 1.0f;
			zoom_degree_   = 0;
		}

		public void DecrementZoom()
		{
			--zoom_degree_;
			CalculateZoom();
		}

		public void DecrementZoom(int n)
		{
			zoom_degree_ -= n;
			CalculateZoom();
		}

		public void IncrementZoom()
		{
			++zoom_degree_;
			CalculateZoom();
		}

		public void IncrementZoom(int n)
		{
			zoom_degree_ += n;
			CalculateZoom();
		}

		public PointF Position
		{
			get
			{
				return position_;
			}
			set
			{
				position_ = value;
				position_.X += 0.5f;
				position_.Y += 0.5f;
				CalculateView();
			}
		}

		public Matrix Projection
		{
			get { return projection_; }
		}

		public Matrix View
		{
			get { return view_; }
		}

		public Size ViewportSize
		{
			set
			{
				viewport_size_ = value;
				CalculateProjection();
			}
		}

		public void SetPosition(float x, float y)
		{
			position_.X = x;
			position_.Y = y;
			CalculateView();
		}

		public float Zoom
		{
			get
			{
				return zoom_;
			}
		}

		#endregion

		#region implementation

		private void CalculateView()
		{
			view_ = Microsoft.DirectX.Matrix.LookAtLH(
				new Vector3(position_.X, position_.Y, 1.0f),
				new Vector3(position_.X, position_.Y, 0.0f),
				new Vector3(0.0f,        1.0f,        0.0f));
		}

		private void CalculateProjection()
		{
			projection_ = Microsoft.DirectX.Matrix.OrthoLH(
				-viewport_size_.Width  * zoom_,
				-viewport_size_.Height * zoom_,
				0.1f,
				2.0f);
		}

		private void CalculateZoom()
		{
			zoom_ = (float)Math.Pow(1.05, zoom_degree_);
			CalculateProjection();
		}

		#endregion

		#region data

		private PointF  position_;
		private Matrix projection_;
		private Matrix view_;
		private Size   viewport_size_;
		private int    zoom_degree_;
		private float  zoom_;

		#endregion
	}
}
