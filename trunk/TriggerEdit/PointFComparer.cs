using System;
using System.Collections;
using System.Drawing;

namespace TriggerEdit
{
	/// <summary>
	/// Summary description for PointFComparer.
	/// </summary>
	public class PointFComparer : IComparer
	{
		public int Compare(object x, object y)
		{
			PointF p1 = (PointF)x;
			PointF p2 = (PointF)y;
			if (p1.X != p2.X)
				return (int)(p1.X - p2.X);
			return (int)(p1.Y - p2.Y);
		}
	}
}
