/*************
 * com_ptr template.
 * the class managing the reference count of COM Object.
 * Author: tk-xleader.
 * License: Unlicense. (for more infomation see LICENSE)
 * */
#ifndef COMPTR_H_INCLUDED
#define COMPTR_H_INCLUDED
#include<utility>
#include<type_traits>
#include<cassert>
#include<objbase.h>

namespace comhelper{
	template<typename T>
	class com_ptr{
		T* ptr = nullptr;
		class modifier;
		static struct enabler_t{} *enabler;
	public:
		using element_type = T;
		using pointer = T*;
		
		com_ptr(std::nullptr_t = nullptr){}
		com_ptr(const com_ptr& src)noexcept:ptr(src.ptr){
			if(src) src.AddRef();
		}
		com_ptr(com_ptr&&src)noexcept:ptr(src.waive()){}
		explicit com_ptr(T *ptr_)noexcept:ptr(ptr_){}
		
		template<typename U, typename std::enable_if<std::is_convertible<U*, T*>::value, enabler_t>::type*& = enabler>
		com_ptr(const com_ptr<U>& src)noexcept: com_ptr(src.ptr){
			if(src) src.AddRef();
		}
		template<typename U, typename std::enable_if<!std::is_convertible<U*, T*>::value, enabler_t>::type*& = enabler>
		explicit com_ptr(const com_ptr<U>& src)noexcept{
			if(src) src.QueryInterface(&ptr);
		}
		~com_ptr(){
			if(ptr) ptr->Release();
		}

		void swap(com_ptr<T>& other)noexcept{
			std::swap(ptr,other.ptr);
		}
		T* get()const noexcept{
			return ptr;
		}

		T* waive()noexcept{
			return std::exchange(ptr,nullptr);
		}
		void reset(T* ptr_ = nullptr)noexcept{
			com_ptr(ptr_).swap(*this);
		}

		com_ptr& operator=(com_ptr&& src)noexcept{
			reset(src.waive());
			return *this;
		}
		com_ptr& operator=(com_ptr const& src)noexcept{
			if(this != &src) com_ptr(src).swap(*this);
			return *this;
		}
		explicit operator bool()const noexcept{return static_cast<bool>(ptr);}
		operator T*()const noexcept{return ptr;}
		T* operator->()const noexcept{return ptr;}
		T& operator*()const noexcept{return *ptr;}
		bool operator==(com_ptr const& right)const noexcept{return ptr==right.ptr;}
		bool operator!=(com_ptr const& right)const noexcept{return ptr!=right.ptr;}

		template<typename U>
		HRESULT QueryInterface(com_ptr<U>& ptr_)const noexcept{
			return QueryInterface(getpp(ptr_));
		}
		template<typename U>
		HRESULT QueryInterface(U** pptr)const noexcept{
			return QueryInterface(IID_PPV_ARGS(pptr));
		}
		HRESULT QueryInterface(REFIID riid,void** ppvObject)const noexcept{
			assert(ptr != nullptr);
			return ptr->QueryInterface(riid,ppvObject);
		}
		ULONG AddRef()const noexcept{
			assert(ptr != nullptr);
			return ptr->AddRef();
		}
		ULONG Release()noexcept{
			return ptr ? std::exchange(ptr,nullptr)->Release() : 0;
		}

		friend inline T** getpp(modifier&& target)noexcept{
			return target.getpp();
		}
		operator modifier()noexcept{
			return modifier(this);
		}
	private:
		class modifier{
			T* ptr = nullptr;
			com_ptr<T>* target = nullptr;
		public:
			explicit modifier(com_ptr<T>* target_)noexcept:ptr(target->get()),target(target_){}
			modifier(modifier&& other)noexcept:ptr(other.ptr),target(other.target){
				other.target = nullptr;
			}
			~modifier(){
				if(target&&(target->get() != ptr)){ *target = com_ptr(ptr); }
			}

			T** getpp()noexcept{return &ptr;}
		private:
			modifier() = delete;
			modifier(modifier const&) = delete;
			void operator=(modifier const&) = delete;
			void operator=(modifier&&) = delete;
		};
		
	};

	template<typename T>
	inline com_ptr<T> make_comptr(T* ptr) noexcept{
		return com_ptr<T>(ptr);
	}
	
	template<typename T>
	inline void swap(com_ptr<T>& val1, com_ptr<T>& val2)noexcept{
		val1.swap(val2);
	}
}
#endif//COMPTR_H_INCLUDED
