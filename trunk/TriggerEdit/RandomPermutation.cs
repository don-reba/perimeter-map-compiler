using System;
using System.Collections;

namespace TriggerEdit
{
	public class RandomPermutation : IEnumerable
	{
		#region interface

		public RandomPermutation(int length)
		{
			if (length <= 0)
				throw new ArgumentOutOfRangeException();
			indices_ = new int[length];
			for (int i = 0; i != length; ++i)
				indices_[i] = i;
			random_ = new Random(0);
			Next();
		}

		public void Next()
		{
			int temp, i, j;
			for (i = 0; i != indices_.Length - 1; ++i)
			{
				j = random_.Next(indices_.Length - i) + i;
				temp        = indices_[i];
				indices_[i] = indices_[j];
				indices_[j] = temp;
			}
		}

		public int this [int index]
		{
			get { return indices_[index]; }
		}

		public int Count
		{
			get { return indices_.Length; }
		}

		#endregion

		#region IEnumerable Members

		public IEnumerator GetEnumerator()
		{
			return indices_.GetEnumerator();
		}

		#endregion

		#region data

		private int[]  indices_;
		private Random random_;

		#endregion
	}
}
