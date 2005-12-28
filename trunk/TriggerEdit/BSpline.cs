using System;
using System.Diagnostics;
using System.Drawing;
using System.Collections;

namespace TriggerEdit
{
	/// <summary>
	/// Quadratic B-Spline calculator.
	/// </summary>
	public class BSpline
	{
		#region interface

		public BSpline(Point[] source, Trigger[] target, uint tesselation)
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
				uint source_iter = 0;
				uint target_iter = 0;
				while (target_iter != target_.Length)
				{
					Point p0 = source_[source_iter++];
					Point p1 = source_[source_iter++];
					Point p2 = source_[source_iter++];
					target_[target_iter].X = (int)(p0.X * r0 + p1.X * r1 + p2.X * r2);
					target_[target_iter].Y = (int)(p0.Y * r0 + p1.Y * r1 + p2.Y * r2);
					++target_iter;
				}
			}
			else if (Mode.First == mode_)
			{
				float r = t / 2.0f;
				uint source_iter = 0;
				uint target_iter = 0;
				while (target_iter != target_.Length)
				{
					Point p0 = source_[source_iter++];
					Point p1 = source_[source_iter++];
					++source_iter;
					target_[target_iter].X = (int)(p0.X * (1 - r) + p1.X * r);
					target_[target_iter].Y = (int)(p0.Y * (1 - r) + p1.Y * r);
					++target_iter;
				}
			}
			else if (Mode.Last == mode_)
			{
				float r = 0.5f + t / 2.0f;
				uint source_iter = 0;
				uint target_iter = 0;
				while (target_iter != target_.Length)
				{
					++source_iter;
					Point p1 = source_[source_iter++];
					Point p2 = source_[source_iter++];
					target_[target_iter].X = (int)(p1.X * (1 - r) + p2.X * r);
					target_[target_iter].Y = (int)(p1.Y * (1 - r) + p2.Y * r);
					++target_iter;
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

		public Point[] Source
		{
			set
			{
				source_ = value;
			}
		}

			public Trigger[] Target
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

		float             iter_;
		private Mode      mode_;
		private Point[]   source_;
		private uint      tesselation_;
		private Trigger[] target_;

		#endregion
	}
}
