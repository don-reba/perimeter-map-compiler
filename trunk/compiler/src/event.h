#include <list>

//----------------------------------
// a collection of delegates
// a thin wrapper based on std::list
//----------------------------------

template<typename T>
class event
{
// nested types
public:
	typedef fastdelegate::FastDelegate<T> delegate_t;
	typedef std::list<delegate_t>         list_t;
	class connection_t
	{
	public:
		connection_t()
			: list_(NULL) {}
		connection_t(list_t &list, typename list_t::iterator iter)
			: list_(&list), iter_(iter) {}
		void disconnect()
		{
			if (NULL == list_)
				return;
			list_->erase(iter_);
			list_ = NULL;
		}
	private:
		list_t                    *list_;
		typename list_t::iterator  iter_;
	};
// interface
public:
	connection_t add(delegate_t delegate)
	{
		return connection_t(delegates_, delegates_.insert(delegates_.end(), delegate));
	}
	connection_t operator+= (delegate_t delegate)
	{
		return add(delegate);
	}
// data
public:
	list_t delegates_;
};