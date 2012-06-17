#pragma once

namespace litehtml
{
	class object
	{
	protected:
		unsigned long volatile	m_refCount;
	public:
		object()
		{ 
			m_refCount = 0;
		}
		virtual ~object()
		{

		}

		void addRef()
		{ 
			m_refCount++;
		}
		
		void release()
		{
			if(!(--m_refCount)) delete this;
		}
	};

	template<class T>
	class object_ptr
	{
		T*	m_ptr;
	public:
		object_ptr()
		{
			m_ptr = 0;
		}

		object_ptr(T* ptr)
		{
			m_ptr = ptr;
			if(m_ptr)
			{
				m_ptr->addRef();
			}
		}

		object_ptr(const object_ptr<T>& val)
		{
			m_ptr = val.m_ptr;
			if(m_ptr)
			{
				m_ptr->addRef();
			}
		}

		~object_ptr()
		{
			if(m_ptr)
			{
				m_ptr->release();
			}
			m_ptr = 0;
		}

		void operator=(const object_ptr<T>& val)
		{
			T* oldPtr = m_ptr;
			m_ptr = val.m_ptr;
			if(m_ptr)
			{
				m_ptr->addRef();
			}
			if(oldPtr)
			{
				oldPtr->release();
			}
		}

		void operator=(T* val)
		{
			T* oldPtr = m_ptr;
			m_ptr = val;
			if(m_ptr)
			{
				m_ptr->addRef();
			}
			if(oldPtr)
			{
				oldPtr->release();
			}
		}

		T* operator->()
		{
			return m_ptr;
		}

		const T* operator->() const
		{
			return m_ptr;
		}

		operator T*()
		{
			return m_ptr;
		}

		operator const T*() const
		{
			return m_ptr;
		}
	};
}
