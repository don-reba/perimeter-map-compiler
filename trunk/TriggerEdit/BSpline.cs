using System;
using System.Diagnostics;
using System.Drawing;
using System.Collections;

namespace TriggerEdit
{
	using PositionHistory = TriggerContainer.PositionHistory;

	/// <summary>
	/// Quadratic B-Spline calculator.
	/// </summary>
	public class BSpline
	{
		#region interface

		public BSpline(PositionHistory[] source, PointF[] target, uint tesselation)
		{
			Debug.Assert(tesselation > 0);
			tesselation_ = tesselation;
			source_ = source;
			target_ = target;
			mode_   = Mode.First;
			iter_   = 0.0f;
		}

		public bool Advance()
		{
			Set(iter_);
			iter_ += 1.0f / tesselation_;
			return iter_ <= 1.0f;
		}

		public void Set(float t)
		{
			Debug.Assert(source_.Length <= target_.Length * 3);
			if (Mode.Middle == mode_)
			{
				// calculate the ratios
				float r0 = (1 - t) * (1 - t) / 2;
				float r1 = -t * t + t + 0.5f;
				float r2 = t * t / 2;
				// calculate targets
				PositionHistory src;
				for (int i = 0; i != target_.Length; ++i)
				{
					src = source_[i];
					target_[i].X = src.point1_.X * r0 + src.point2_.X * r1	+ src.point3_.X * r2;
					target_[i].Y = src.point1_.Y * r0 + src.point2_.Y * r1	+ src.point3_.Y * r2;
				}
			}
			else if (Mode.First == mode_)
			{
				float r = t / 2.0f;
				PositionHistory src;
				for (int i = 0; i != target_.Length; ++i)
				{
					src = source_[i];
					target_[i].X = src.point1_.X * (1 - r) + src.point2_.X * r;
					target_[i].Y = src.point1_.Y * (1 - r) + src.point2_.Y * r;
				}
			}
			else if (Mode.Last == mode_)
			{
				float r = 0.5f + t / 2.0f;
				PositionHistory src;
				for (int i = 0; i != target_.Length; ++i)
				{
					src = source_[i];
					target_[i].X = src.point2_.X * (1 - r) + src.point3_.X * r;
					target_[i].Y = src.point2_.Y * (1 - r) + src.point3_.Y * r;
				}
			}
		}

		public Mode SegmentMode
		{
			set
			{
				mode_ = value;
				iter_ = 0.0f;
			}
		}

		public PositionHistory[] Source
		{
			set
			{
				source_ = value;
			}
		}

		public PointF[] Target
		{
			set
			{
				target_ = value;
			}
		}

		public enum Mode
		{
			First,
			Middle,
			Last
		}

		#endregion
		
		#region data

		float                     iter_;
		private Mode              mode_;
		private PositionHistory[] source_;
		private uint              tesselation_;
		private PointF[]          target_;

		#endregion
	}
}
