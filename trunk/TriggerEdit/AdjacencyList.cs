using System;
using System.Collections;

namespace TriggerEdit
{
	//------
	// using
	//------

	#region

	using Link = TriggerContainer.Link;

	#endregion
 
	public class AdjacencyList
	{
		//-------------
		// nested types
		//-------------

		#region

		public class Enumerator
		{
			//----------
			// interface
			//----------

			public Enumerator(AdjacencyList list)
			{
				list_ = list;
			}

			#region IEnumerator Members

			public void Reset()
			{
				index_ = 0;
			}

			public Link Current
			{
				get { return list_[index_]; }
			}

			public bool MoveNext()
			{
				++index_;
				return index_ < list_.Count;
			}

			#endregion

			//-----
			// data
			//-----

			AdjacencyList list_;
			int           index_;

		}


		#endregion

		//----------
		// interface
		//----------

		#region

		public AdjacencyList()
		{
			links_ = new Link[0];
			count_ = 0;
		}

		public void Add(Link link)
		{
			if (links_.Length == count_)
			{
				Link[] new_links = new Link[GrowPolicy(count_)];
				links_.CopyTo(new_links, 0);
				links_ = new_links;
			}
			links_[count_] = link;
			++count_;
		}

		public Link Remove(int tail, int head)
		{
			int index = IndexOf(tail, head);
			if (index < 0)
				throw new Exception("Link was not found.");
			return RemoveAt(index);
		}

		public Link RemoveAt(int index)
		{
			if (index >= count_)
				throw new IndexOutOfRangeException();
			Link link = links_[index];
			for (int i = index; i != count_ - 1; ++i)
				links_[i] = links_[i + 1];
			--count_;
			return link;
		}

		public void RemoveVertex(int index)
		{
			// count the number of links to be deleted
			int n = 0;
			for (int i = 0; i != count_; ++i)
				if (links_[i].head_ == index || links_[i].tail_ == index)
					++n;
			// decide on the strategy
			if (0 == n)
				return;
			// create a new array
			Link[] new_links = new Link[count_ - n];
			int iter = 0;
			for (int i = 0; i != count_; ++i)
				if (links_[i].head_ != index && links_[i].tail_ != index)
				{
					new_links[iter] = links_[i];
					if (new_links[iter].head_ > index)
						--new_links[iter].head_;
					if (new_links[iter].tail_ > index)
						--new_links[iter].tail_;
					++iter;
				}
			// replace the old array
			links_  = new_links;
			count_ -= n;
		}

		public int Count
		{
			get { return count_; }
		}

		public Link this[int index]
		{
			get
			{
				if (index >= count_)
					throw new IndexOutOfRangeException();
				return links_[index];
			}
			set
			{
				if (index >= count_)
					throw new IndexOutOfRangeException();
				links_[index] = value;
			}
		}

		public int IndexOf(int tail, int head)
		{
			for (int i = 0; i != count_; ++i)
				if (links_[i].head_ == head && links_[i].tail_ == tail)
					return i;
			return -1;
		}

		public void AddStatus(int index, Link.Status status)
		{
			if (index >= count_)
				throw new IndexOutOfRangeException();
			links_[index].status_ |= status;
		}

		public void RemoveStatus(int index, Link.Status status)
		{
			if (index >= count_)
				throw new IndexOutOfRangeException();
			links_[index].status_ &= ~status;
		}

		public void SetStatus(int index, Link.Status status)
		{
			if (index >= count_)
				throw new IndexOutOfRangeException();
			links_[index].status_ = status;
		}

		public void SetGroup(int index, int group)
		{
			if (index >= count_)
				throw new IndexOutOfRangeException();
			links_[index].group_ = group;
		}

		public void SetPersistence(int index, bool persist)
		{
			if (index >= count_)
				throw new IndexOutOfRangeException();
			links_[index].persistent_ = persist;
		}

		#endregion

		#region IEnumerable Members

		public Enumerator GetEnumerator()
		{
			return new Enumerator(this);
		}

		#endregion

		//---------------
		// implementation
		//---------------

		public int GrowPolicy(int count)
		{
			return count + 1;
		}

		//-----
		// data
		//-----

		#region

		private Link[] links_;
		private  int   count_;

		#endregion
	}
}
