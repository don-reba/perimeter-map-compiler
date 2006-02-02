using System;
using System.Collections;
using System.Diagnostics;

namespace TriggerEdit
{
	public class AdjacencyMatrix
	{
		#region nested types

		public class AdjacencyList
		{
			public AdjacencyList(int row, BitArray bits, int count)
			{
				bits_     = bits;
				count_    = count;
				position_ = row * count;
			}

			public class Enumerator
			{
				public Enumerator(int position, int end, BitArray bits)
				{
					bits_     = bits;
					end_      = end;
					position_ = position - 1;
					start_    = position;
				}

				#region IEnumerator Members

				public int Current
				{
					get { return position_ - start_; }
				}

				public bool MoveNext()
				{
					do
					{
						++position_;
					} while (position_ != end_ && !bits_[position_]);
					return position_ != end_;
				}

				#endregion

				BitArray bits_;
				int      end_;
				int      position_;
				int      start_;
			}

			#region IEnumerable Members

			public Enumerator GetEnumerator()
			{
				return new Enumerator(position_, position_ + count_, bits_);
			}

			#endregion

			private BitArray bits_;
			private int      count_;
			private int      position_;
		}


		#endregion

		#region interface

		public AdjacencyMatrix()
			:this(0)
		{
		}

		public AdjacencyMatrix(int length)
		{
			if (length < 0)
				throw new ArgumentOutOfRangeException();
			count_ = length;
			bits_  = new BitArray(count_ * count_);
		}

		public int Count
		{
			get { return count_; }
		}

		public bool this [int row, int col]
		{
			get
			{
				return bits_[row * count_ + col];
			}
			set
			{
				Debug.Assert(row != col);
				bits_[row * count_ + col] = value;
			}
		}

		public AdjacencyList GetList(int index)
		{
			return new AdjacencyList(index, bits_, count_);
		}

		public void Flip(int row, int col)
		{
			int index = row * count_ + col;
			bits_[index] = !bits_[index];
		}

		public void Grow(int n)
		{
			if (n < 1)
				throw new ArgumentOutOfRangeException();
			int new_count = count_ + n;
			BitArray new_bits  = new BitArray(new_count * new_count);
			int iter_old = 0;
			int iter_new = 0;
			for (int row = 0; row != count_; ++row)
			{
				for (int col = 0; col != count_; ++col)
					new_bits[iter_new++] = bits_[iter_old++];
				iter_new += n;
			}
			count_ = new_count;
			bits_  = new_bits;
		}

		public void Insert(int index)
		{
			if (index < 0)
				throw new ArgumentOutOfRangeException();
			int      new_count = count_ + 1;
			BitArray new_bits  = new BitArray(new_count * new_count);
			int iter_old = 0;
			int iter_new = 0;
			for (int row = 0; row != count_; ++row)
			{
				for (int col = 0; col != count_; ++col)
				{
					new_bits[iter_new++] = bits_[iter_old++];
					if (col == index)
						++iter_new;
				}
				if (row == index)
					iter_new += new_count;
			}
			count_ = new_count;
			bits_  = new_bits;
		}

		public void Delete(int index)
		{
			if (index < 0)
				throw new ArgumentOutOfRangeException();
			int      new_count = count_ - 1;
			BitArray new_bits  = new BitArray(new_count * new_count);
			int      iter_old  = 0;
			int      iter_new  = 0;
			for (int row = 0; row != new_count; ++row)
			{
				if (row == index)
					iter_old += count_;
				for (int col = 0; col != index; ++col)
					new_bits[iter_new++] = bits_[iter_old++];
				++iter_old;
				for (int col = index; col != new_count; ++col)
					new_bits[iter_new++] = bits_[iter_old++];
			}
			count_ = new_count;
			bits_  = new_bits;
		}

		#endregion

		#region data

		private BitArray bits_;
		private int      count_;

		#endregion
	}
}
