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

namespace comhelper{
	struct addref_t{};
	inline constexpr addref_t addref;
	
	template<typename I>
	struct interface_traits{
		static inline constexpr auto addref(I* ptr)noexcept{
			return ptr->AddRef();
		}
		static inline constexpr auto release(I* ptr)noexcept{
			return ptr->Release();
		}
	};
	
	namespace _details{
		template<typename Tr, typename Ip, typename = void>
		struct pointer_type{
			using type = Ip;
		};
		template<typename Tr, typename Ip>
		struct pointer_type<Tr, Ip, std::void_t<typename Tr::pointer>>{
			using type = typename Tr::pointer;
		};
		
		template<typename Tr, typename I>
		using pointer_type_t = typename pointer_type<Tr, std::add_pointer_t<I>>::type;
	}
	
	template<typename T, typename Traits = interface_traits<T>>
	class com_ptr{
	public:
		using element_type = T;
		using pointer = _details::pointer_type_t<Traits, T>;
		using traits_type = Traits;
		
		com_ptr() = default;
		constexpr com_ptr(std::nullptr_t):noexcept{}
		
		explicit com_ptr(pointer ptr_)noexcept:ptr(ptr_){}
		explicit com_ptr(pointer ptr_, addref_t)noexcept:ptr(ptr_){
			if(ptr_) traits_type::addref(ptr_);
		}
		
		template<typename U, std::enable_if_t<std::is_convertible<U*, T*>::value, std::nullptr_t> = nullptr>
		com_ptr(const com_ptr<U>& src)noexcept: com_ptr(static_cast<T*>(src.get()), addref_t{}){}
		
		template<typename U, std::enable_if_t<!std::is_convertible<U*, T*>::value && std::is_constructible<T*, U*>::value, std::nullptr_t> = nullptr>
		explicit com_ptr(const com_ptr<U>& src)noexcept: com_ptr(static_cast<T*>(src.get()), addref_t{}){}
		
		com_ptr(com_ptr&&src) = default;
		com_ptr(const com_ptr& src)noexcept:com_ptr(src.get(), addref_t()){}
		
		~com_ptr() = default;

		void swap(com_ptr<T>& other)noexcept{
			ptr.swap(other.ptr);
		}
		pointer get()const noexcept{
			return ptr.get();
		}
		
		pointer release()noexcept{
			return ptr.release();
		}
		void reset(pointer ptr_ = nullptr)noexcept{
			ptr.reset(ptr_);
		}

		com_ptr& operator=(com_ptr&& src) = default;
		com_ptr& operator=(com_ptr const& src)noexcept{
			return this->operator=(com_ptr(src)); // copy and move.
		}
		
		explicit operator bool()const noexcept{return static_cast<bool>(ptr);}
		
		auto operator->()const noexcept{return ptr.get();}
		auto operator*()const noexcept{return *ptr;}
		
		friend inline bool operator==(com_ptr const& left, com_ptr const& right)noexcept{return left.ptr==right.ptr;}
		friend inline bool operator!=(com_ptr const& left, com_ptr const& right)noexcept{return !(left==right);}
	private:
		struct com_releaser{
			using pointer = typename com_ptr<T, Traits>::pointer;
			inline constexpr void operator()(pointer ptr)const noexcept{
				traits_type::release(ptr);
			}
		};
		std::unique_ptr<T, com_releaser> ptr;
	};

	template<typename T>
	inline com_ptr<T> make_comptr(T* ptr) noexcept{
		return com_ptr<T>(ptr);
	}
	
	template<typename T>
	inline com_ptr<T> make_comptr(T* ptr, addref_t) noexcept{
		return com_ptr<T>(ptr, addref_t{});
	}
	
	template<typename T>
	inline void swap(com_ptr<T>& val1, com_ptr<T>& val2)noexcept{
		val1.swap(val2);
	}
	
	template<typename To, typename From>
	inline com_ptr<To> interface_cast(From* from)noexcept{
		To* dest = nullptr;
		if(!from || static_cast<long>(from->QueryInterface(__uuidof(To), (void**)&dest)) < 0L) return nullptr;
		return make_comptr(dest);
	}
	
	template<typename To, typename From>
	inline com_ptr<To> interface_cast(com_ptr<From> const& from) noexcept{
		return interface_cast<To>(from.get());
	}
}
#endif//COMPTR_H_INCLUDED
