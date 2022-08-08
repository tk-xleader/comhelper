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
#include<memory>
#include<cassert>
#include<objbase.h>

namespace comhelper{
	struct addref_t{};
	constexpr addref_t addref;
	
	template<typename T>
	class com_ptr{
		struct com_releaser{
			inline void operator()(T* ptr)const noexcept{
				ptr->Release();
			}
		};
		
		std::unique_ptr<T, com_releaser> ptr;
		class modifier;
		static struct enabler_t{} *enabler;
	public:
		using element_type = T;
		using pointer = T*;
		
		com_ptr() = default;
		com_ptr(std::nullptr_t):noexcept{}
		
		explicit com_ptr(T *ptr_)noexcept:ptr(ptr_){}
		explicit com_ptr(T *ptr_, addref_t)noexcept:ptr(ptr_){
			if(ptr_) ptr_->AddRef();
		}
		
		template<typename U, typename std::enable_if<std::is_convertible<U*, T*>::value, enabler_t>::type*& = enabler>
		com_ptr(const com_ptr<U>& src)noexcept: com_ptr(static_cast<T*>(src.get()), addref_t()){}
		
		template<typename U, typename std::enable_if<!std::is_convertible<U*, T*>::value, enabler_t>::type*& = enabler>
		explicit com_ptr(const com_ptr<U>& src)noexcept{
			if(src) src.QueryInterface(&ptr);
		}
		
		com_ptr(com_ptr&&src) = default;
		com_ptr(const com_ptr& src)noexcept:com_ptr(src.ptr.get(), addref_t()){}
		
		~com_ptr() = default;

		void swap(com_ptr<T>& other)noexcept{
			std::swap(ptr,other.ptr);
		}
		T* get()const noexcept{
			return ptr.get();
		}
		
		T* release()noexcept{
			return ptr.release();
		}
		T* waive()noexcept{ // deprecated.
			return this->release(); //same as release member function.
		}
		void reset(T* ptr_ = nullptr)noexcept{
			ptr.reset(ptr_);
		}

		com_ptr& operator=(com_ptr&& src) = default;
		com_ptr& operator=(com_ptr const& src)noexcept{
			return this->operator=(com_ptr(src)); // copy and move.
		}
		
		explicit operator bool()const noexcept{return static_cast<bool>(ptr);}
		operator T*()const noexcept{return ptr.get();}
		
		T* operator->()const noexcept{return ptr.get();}
		T& operator*()const noexcept{return *ptr;}
		
		friend inline bool operator==(com_ptr const& left, com_ptr const& right)noexcept{return left.ptr==right.ptr;}
		friend inline bool operator!=(com_ptr const& left, com_ptr const& right)noexcept{return !(left==right);}

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
