#include <iterator>
#include <list>

//--------------------------
// a collection of delegates
//--------------------------

template<typename T>
class event
{
// nested types
public:
	typedef fastdelegate::FastDelegate<T> delegate_t;
private:
	typedef std::list<delegate_t> list_t;
public:
	//---------------------------------------
	// connection
	// can be used to disconnect the delegate
	//---------------------------------------
	class connection_t
	{
	// interface
	public:
		connection_t()
			:list_(NULL)
		{}
		connection_t(list_t &list, typename list_t::iterator iter)
			:list_(&list)
			,iter_(iter)
		{}
		void disconnect()
		{
			if (NULL != list_)
				list_->erase(iter_);
			list_ = NULL;
		}
	// data
	private:
		list_t                    *list_;
		typename list_t::iterator  iter_;
	};
	//-------------------------------------------------
	// iterator
	// for events to beahve as collections of delegates
	//-------------------------------------------------
	class const_iterator : public std::iterator_traits<typename list_t::const_iterator>
	{
	public:
		const_iterator(typename list_t::const_iterator iter)
			:prev_(iter)
			,curr_(iter)
			,next_(iter)
		{
			--prev_;
			++next_;
		}
		typename list_t::const_reference operator * () const
		{
			return *curr_;
		}
		typename list_t::const_pointer operator -> () const
		{
			return curr_.operator->();
		}
		const_iterator &operator ++ ()
		{
			prev_ = curr_;
			curr_ = next_;
			++next_;
			return *this;
		}
		const_iterator operator ++ (int)
		{
			const_iterator temp(*this);
			++*this;
			return temp;
		}
		const_iterator &operator -- ()
		{
			curr_ = prev_;
			next_ = curr_;
			--prev_;
			return *this;
		}
		const_iterator operator -- (int)
		{
			const_iterator temp(*this);
			--*this;
			return temp;
		}
		bool operator == (const const_iterator & right) const
		{
			return curr_ == right.curr_;
		}
		bool operator != (const const_iterator & right) const
		{
			return curr_ != right.curr_;
		}
	protected:
		typename list_t::const_iterator prev_;
		typename list_t::const_iterator curr_;
		typename list_t::const_iterator next_;
	};
	class iterator : public std::iterator_traits<typename list_t::iterator>
	{
	public:
		iterator(typename list_t::iterator iter)
			:prev_(iter)
			,curr_(iter)
			,next_(iter)
		{
			--prev_;
			++next_;
		}
		typename list_t::reference operator * () const
		{
			return *curr_;
		}
		typename list_t::pointer operator -> () const
		{
			return curr_.operator->();
		}
		iterator &operator ++ ()
		{
			prev_ = curr_;
			curr_ = next_;
			++next_;
			return *this;
		}
		iterator operator ++ (int)
		{
			iterator temp(*this);
			++*this;
			return temp;
		}
		iterator &operator -- ()
		{
			curr_ = prev_;
			next_ = curr_;
			--prev_;
			return *this;
		}
		iterator operator -- (int)
		{
			iterator temp(*this);
			--*this;
			return temp;
		}
		bool operator == (const iterator & right) const
		{
			return curr_ == right.curr_;
		}
		bool operator != (const iterator & right) const
		{
			return curr_ != right.curr_;
		}
	protected:
		typename list_t::iterator prev_;
		typename list_t::iterator curr_;
		typename list_t::iterator next_;
	};
// interface
public:
	connection_t add(delegate_t delegate)
	{
		return connection_t(delegates_, delegates_.insert(delegates_.end(), delegate));
	}
	connection_t operator += (delegate_t delegate)
	{
		return add(delegate);
	}
	iterator begin()
	{
		return delegates_.begin();
	}
	const_iterator begin() const
	{
		return delegates_.begin();
	}
	iterator end()
	{
		return delegates_.end();
	}
	const_iterator end() const
	{
		return delegates_.end();
	}
	typename list_t::size_type size() const
	{
		return delegates_.size();
	}
// data
private:
	list_t delegates_;
};
