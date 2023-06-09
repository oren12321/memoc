#ifndef MEMOC_POINTERS_H
#define MEMOC_POINTERS_H

#include <memory>
#include <cstdint>
#include <compare>
#include <utility>

#include <memoc/allocators.h>
//#include <erroc/errors.h>
#include <memoc/blocks.h>

namespace memoc {
	namespace details {
		template <typename T, typename ...Args>
		inline constexpr T* construct_at(T* dst_address, Args&&... args)
		{
			return new (dst_address) T(std::forward<Args>(args)...);
		}

		template <typename T>
		constexpr void destruct_at(T* dst_address)
		{
			dst_address->~T();
		}

		// These class are not thread safe
		// The behaviour for array, pointer or reference is undefined
		template <typename T, Allocator Internal_allocator = Malloc_allocator>
		class Unique_ptr final {
		public:
			// Not recommended - ptr should be allocated using Internal_allocator
			constexpr Unique_ptr(T* ptr = nullptr)
				: ptr_(ptr) {}

			template <typename T_o>
			Unique_ptr(const Unique_ptr<T_o, Internal_allocator>& other) noexcept = delete;
			Unique_ptr(const Unique_ptr& other) noexcept = delete;

			template <typename T_o>
			Unique_ptr& operator=(const Unique_ptr<T_o, Internal_allocator>& other) noexcept = delete;
			Unique_ptr& operator=(const Unique_ptr& other) noexcept = delete;

			template <typename T_o>
			constexpr Unique_ptr(Unique_ptr<T_o, Internal_allocator>&& other) noexcept
				: allocator_(other.allocator_), ptr_(other.ptr_)
			{
				other.ptr_ = nullptr;
			}
			constexpr Unique_ptr(Unique_ptr&& other) noexcept
				: allocator_(other.allocator_), ptr_(other.ptr_)
			{
				other.ptr_ = nullptr;
			}

			template <typename T_o>
			constexpr Unique_ptr& operator=(Unique_ptr<T_o, Internal_allocator>&& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				ptr_ = other.ptr_;

				other.ptr_ = nullptr;
				return *this;
			}
			constexpr Unique_ptr& operator=(Unique_ptr&& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				ptr_ = other.ptr_;

				other.ptr_ = nullptr;
				return *this;
			}

			constexpr ~Unique_ptr() noexcept
			{
				remove_reference();
			}

			[[nodiscard]] constexpr T* get() const noexcept
			{
				return ptr_;
			}

			[[nodiscard]] constexpr T* operator->() const noexcept
			{
				return ptr_;
			}

			[[nodiscard]] constexpr T& operator*() const noexcept
			{
				return *(ptr_);
			}

			[[nodiscard]] constexpr operator bool() const noexcept
			{
				return ptr_;
			}

			constexpr void reset() noexcept
			{
				remove_reference();
				ptr_ = nullptr;
			}

			[[nodiscard]] constexpr T* release() noexcept
			{
				T* tmp_ptr = ptr_;
				ptr_ = nullptr;
				return tmp_ptr;
			}

			template <typename T_o>
			constexpr void reset(T_o* ptr) noexcept
			{
				if (!ptr) {
					reset();
				}
				remove_reference();
				ptr_ = ptr;
			}

			template <typename T_o, Allocator Internal_allocator_o>
			friend class Unique_ptr;

			template <typename T_o, Allocator Internal_allocator_o>
			friend constexpr bool operator==(const Unique_ptr<T_o, Internal_allocator_o>& lhs, const Unique_ptr<T_o, Internal_allocator_o>& rhs);

			template <typename T_o, Allocator Internal_allocator_o>
			friend constexpr std::strong_ordering operator<=>(const Unique_ptr<T_o, Internal_allocator_o>& lhs, const Unique_ptr<T_o, Internal_allocator_o>& rhs);

			template <typename T_o, Allocator Internal_allocator_o>
			friend constexpr bool operator==(const Unique_ptr<T_o, Internal_allocator_o>& lhs, std::nullptr_t);

			template <typename T_o, Allocator Internal_allocator_o>
			friend constexpr std::strong_ordering operator<=>(const Unique_ptr<T_o, Internal_allocator_o>& lhs, std::nullptr_t);

		private:
			constexpr void remove_reference()
			{
				// Check if there's an object in use
				if (ptr_) {
					memoc::details::destruct_at<T>(ptr_);
					Block<void> ptr_b = { MEMOC_SSIZEOF(T), const_cast<std::remove_const_t<T>*>(ptr_) };
					allocator_.deallocate(ptr_b);
					ptr_ = nullptr;
				}
			}

			Internal_allocator allocator_{};
			T* ptr_{ nullptr };
		};

		template <typename T, Allocator Internal_allocator>
		[[nodiscard]] inline constexpr bool operator==(const Unique_ptr<T, Internal_allocator>& lhs, const Unique_ptr<T, Internal_allocator>& rhs)
		{
			return lhs.ptr_ == rhs.ptr_;
		}

		template <typename T, Allocator Internal_allocator>
		[[nodiscard]] inline constexpr std::strong_ordering operator<=>(const Unique_ptr<T, Internal_allocator>& lhs, const Unique_ptr<T, Internal_allocator>& rhs)
		{
			return std::compare_three_way{}(lhs.ptr_, rhs.ptr_);
		}

		template <typename T, Allocator Internal_allocator>
		[[nodiscard]] inline constexpr bool operator==(const Unique_ptr<T, Internal_allocator>& lhs, std::nullptr_t)
		{
			return !lhs;
		}

		template <typename T, Allocator Internal_allocator>
		[[nodiscard]] inline constexpr std::strong_ordering operator<=>(const Unique_ptr<T, Internal_allocator>& lhs, std::nullptr_t)
		{
			return std::compare_three_way{}(lhs.ptr_, nullptr);
		}


		template <typename T, Allocator Internal_allocator = Malloc_allocator, typename ...Args>
		[[nodiscard]] inline constexpr Unique_ptr<T, Internal_allocator> make_unique(Args&&... args)
		{
			Internal_allocator allocator_{};
			Block<void> b = allocator_.allocate(MEMOC_SSIZEOF(T)).value();
			T* ptr = memoc::details::construct_at<T>(reinterpret_cast<T*>(b.data()), std::forward<Args>(args)...);
			return Unique_ptr<T, Internal_allocator>(ptr);
		}

		struct Control_block {
			std::int64_t use_count{ 0 };
			std::int64_t weak_count{ 0 };
		};

		template <typename T, Allocator Internal_allocator>
		class Weak_ptr;

		template <typename T, Allocator Internal_allocator = Malloc_allocator>
		class Shared_ptr final {
		public:
			template <typename T_o, Allocator Internal_allocator_o>
			friend class Weak_ptr;

			// Not recommended - ptr should be allocated using Internal_allocator
			constexpr explicit Shared_ptr(T* ptr = nullptr)
				: cb_(ptr ? reinterpret_cast<Control_block*>(const_cast<void*>(allocator_.allocate(MEMOC_SSIZEOF(Control_block)).value().data())) : nullptr), ptr_(ptr)
			{
				// Using value from allocate API that throws an exception if not available.
				//ERROC_EXPECT((ptr && cb_) || (!ptr && !cb_), std::runtime_error, "internal memory allocation failed");
				if (cb_) {
					memoc::details::construct_at<Control_block>(cb_);
					cb_->use_count = ptr_ ? 1 : 0;
					cb_->weak_count = 0;
				}
			}

			template <typename T_o>
			constexpr Shared_ptr(const Shared_ptr<T_o, Internal_allocator>& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				if (other.ptr_ && other.cb_) {
					++cb_->use_count;
				}
			}
			constexpr Shared_ptr(const Shared_ptr& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				if (other.ptr_ && other.cb_) {
					++cb_->use_count;
				}
			}

			// Should not be used directly
			// If used directly, user should release 'ptr'
			template <typename T_o>
			constexpr Shared_ptr(const Shared_ptr<T_o, Internal_allocator>& other, T* ptr) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(ptr)
			{
				if (other.ptr_ && other.cb_) {
					++cb_->use_count;
				}
			}

			template <typename T_o>
			constexpr Shared_ptr& operator=(const Shared_ptr<T_o, Internal_allocator>& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				if (other.ptr_ && other.cb_) {
					++cb_->use_count;
				}
				return *this;
			}
			constexpr Shared_ptr& operator=(const Shared_ptr& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				if (other.ptr_ && other.cb_) {
					++cb_->use_count;
				}
				return *this;
			}

			template <typename T_o>
			constexpr Shared_ptr(Shared_ptr<T_o, Internal_allocator>&& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				other.cb_ = nullptr;
				other.ptr_ = nullptr;
			}
			constexpr Shared_ptr(Shared_ptr&& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				other.cb_ = nullptr;
				other.ptr_ = nullptr;
			}

			template <typename T_o>
			constexpr Shared_ptr(Shared_ptr<T_o, Internal_allocator>&& other, T* ptr) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(ptr)
			{
				other.cb_ = nullptr;
				other.ptr_ = nullptr;
			}

			template <typename T_o>
			constexpr Shared_ptr& operator=(Shared_ptr<T_o, Internal_allocator>&& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				other.cb_ = nullptr;
				other.ptr_ = nullptr;
				return *this;
			}
			constexpr Shared_ptr& operator=(Shared_ptr&& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				other.cb_ = nullptr;
				other.ptr_ = nullptr;
				return *this;
			}

			constexpr ~Shared_ptr() noexcept
			{
				remove_reference();
			}

			[[nodiscard]] constexpr std::int64_t use_count() const noexcept
			{
				return cb_ ? cb_->use_count : 0;
			}

			[[nodiscard]] constexpr T* get() const noexcept
			{
				return ptr_;
			}

			[[nodiscard]] constexpr T* operator->() const noexcept
			{
				return ptr_;
			}

			[[nodiscard]] constexpr T& operator*() const noexcept
			{
				return *(ptr_);
			}

			[[nodiscard]] constexpr explicit operator bool() const noexcept
			{
				return ptr_;
			}

			constexpr void reset() noexcept
			{
				remove_reference();
				cb_ = nullptr;
				ptr_ = nullptr;
			}

			template <typename T_o>
			constexpr void reset(T_o* ptr)
			{
				remove_reference();
				if (ptr) {
					cb_ = reinterpret_cast<Control_block*>(const_cast<void*>(allocator_.allocate(MEMOC_SSIZEOF(Control_block)).value().data()));
					// Using value from allocate API that throws an exception if not available.
					//ERROC_EXPECT(cb_, std::runtime_error, "internal memory allocation failed");
					memoc::details::construct_at<Control_block>(cb_);
					cb_->use_count = 1;
					cb_->weak_count = 0;
				}
				else {
					cb_ = nullptr;
				}
				ptr_ = ptr;
			}

			template <typename T_o>
			constexpr Shared_ptr(Unique_ptr<T_o, Internal_allocator>&& other) noexcept
				: Shared_ptr<T_o, Internal_allocator>(other.release()) {}

			template <typename T_o>
			constexpr Shared_ptr& operator=(Unique_ptr<T_o, Internal_allocator>&& other) noexcept
			{
				reset(other.release());
				return *this;
			}

			template <typename T_o, Allocator Internal_allocator_o>
			friend class Shared_ptr;

			template <typename T_o, Allocator Internal_allocator_o>
			friend constexpr bool operator==(const Shared_ptr<T_o, Internal_allocator_o>& lhs, const Shared_ptr<T_o, Internal_allocator_o>& rhs) noexcept;

			template <typename T_o, Allocator Internal_allocator_o>
			friend constexpr std::strong_ordering operator<=>(const Shared_ptr<T_o, Internal_allocator_o>& lhs, const Shared_ptr<T_o, Internal_allocator_o>& rhs) noexcept;

			template <typename T_o, Allocator Internal_allocator_o>
			friend constexpr bool operator==(const Shared_ptr<T_o, Internal_allocator_o>& lhs, std::nullptr_t) noexcept;

			template <typename T_o, Allocator Internal_allocator_o>
			friend constexpr std::strong_ordering operator<=>(const Shared_ptr<T_o, Internal_allocator_o>& lhs, std::nullptr_t) noexcept;

		private:
			constexpr void remove_reference() noexcept
			{
				if (!ptr_ && !cb_) {
					return;
				}
				if (cb_->use_count > 0) {
					--cb_->use_count;
				}
				if (cb_->use_count == 0 && ptr_) {
					memoc::details::destruct_at<T>(ptr_);
					Block<void> ptr_b = { MEMOC_SSIZEOF(T), const_cast<std::remove_const_t<T>*>(ptr_) };
					allocator_.deallocate(ptr_b);
					ptr_ = nullptr;
				}
				if (cb_->use_count == 0 && cb_->weak_count == 0) {
					memoc::details::destruct_at<Control_block>(cb_);
					Block<void> cb_b = { MEMOC_SSIZEOF(Control_block), cb_ };
					allocator_.deallocate(cb_b);
					cb_ = nullptr;
				}
			}

			Internal_allocator allocator_{};
			Control_block* cb_{ nullptr };
			T* ptr_{ nullptr };
		};

		template <typename T, Allocator Internal_allocator>
		[[nodiscard]] inline constexpr bool operator==(const Shared_ptr<T, Internal_allocator>& lhs, const Shared_ptr<T, Internal_allocator>& rhs) noexcept
		{
			return lhs.ptr_ == rhs.ptr_;
		}

		template <typename T, Allocator Internal_allocator>
		[[nodiscard]] inline constexpr std::strong_ordering operator<=>(const Shared_ptr<T, Internal_allocator>& lhs, const Shared_ptr<T, Internal_allocator>& rhs) noexcept
		{
			return std::compare_three_way{}(lhs.ptr_, rhs.ptr_);
		}

		template <typename T, Allocator Internal_allocator>
		[[nodiscard]] inline constexpr bool operator==(const Shared_ptr<T, Internal_allocator>& lhs, std::nullptr_t) noexcept
		{
			return !lhs;
		}

		template <typename T, Allocator Internal_allocator>
		[[nodiscard]] inline constexpr std::strong_ordering operator<=>(const Shared_ptr<T, Internal_allocator>& lhs, std::nullptr_t) noexcept
		{
			return std::compare_three_way{}(lhs.ptr_, nullptr);
		}


		template <typename T, Allocator Internal_allocator = Malloc_allocator, typename ...Args>
		[[nodiscard]] inline constexpr Shared_ptr<T, Internal_allocator> make_shared(Args&&... args)
		{
			Internal_allocator allocator_{};
			Block<void> b = allocator_.allocate(MEMOC_SSIZEOF(T)).value();
			T* ptr = memoc::details::construct_at<T>(reinterpret_cast<T*>(b.data()), std::forward<Args>(args)...);
			return Shared_ptr<T, Internal_allocator>(ptr);
		}

		template <typename T, typename U, Allocator Internal_allocator = Malloc_allocator>
		[[nodiscard]] inline constexpr Shared_ptr<T, Internal_allocator> static_pointer_cast(const Shared_ptr<U, Internal_allocator>& other) noexcept
		{
			T* p = static_cast<T*>(other.get());
			return Shared_ptr<T, Internal_allocator>(other, p);
		}

		template <typename T, typename U, Allocator Internal_allocator = Malloc_allocator>
		[[nodiscard]] inline constexpr Shared_ptr<T, Internal_allocator> static_pointer_cast(Shared_ptr<U, Internal_allocator>&& other) noexcept
		{
			T* p = static_cast<T*>(other.get());
			return Shared_ptr<T, Internal_allocator>(std::move(other), p);
		}

		template <typename T, typename U, Allocator Internal_allocator = Malloc_allocator>
		[[nodiscard]] inline constexpr Shared_ptr<T, Internal_allocator> dynamic_pointer_cast(const Shared_ptr<U, Internal_allocator>& other) noexcept
		{
			if (T* p = dynamic_cast<T*>(other.get())) {
				return Shared_ptr<T, Internal_allocator>(other, p);
			}
			return Shared_ptr<T, Internal_allocator>{};
		}

		template <typename T, typename U, Allocator Internal_allocator = Malloc_allocator>
		[[nodiscard]] inline constexpr Shared_ptr<T, Internal_allocator> dynamic_pointer_cast(Shared_ptr<U, Internal_allocator>&& other) noexcept
		{
			if (T* p = dynamic_cast<T*>(other.get())) {
				return Shared_ptr<T, Internal_allocator>(std::move(other), p);
			}
			return Shared_ptr<T, Internal_allocator>{};
		}

		template <typename T, typename U, Allocator Internal_allocator = Malloc_allocator>
		[[nodiscard]] inline constexpr Shared_ptr<T, Internal_allocator> const_pointer_cast(const Shared_ptr<U, Internal_allocator>& other) noexcept
		{
			T* p = const_cast<T*>(other.get());
			return Shared_ptr<T, Internal_allocator>(other, p);
		}

		template <typename T, typename U, Allocator Internal_allocator = Malloc_allocator>
		[[nodiscard]] inline constexpr Shared_ptr<T, Internal_allocator> const_pointer_cast(Shared_ptr<U, Internal_allocator>&& other) noexcept
		{
			T* p = const_cast<T*>(other.get());
			return Shared_ptr<T, Internal_allocator>(std::move(other), p);
		}

		template <typename T, typename U, Allocator Internal_allocator = Malloc_allocator>
		[[nodiscard]] inline constexpr Shared_ptr<T, Internal_allocator> reinterpret_pointer_cast(const Shared_ptr<U, Internal_allocator>& other) noexcept
		{
			T* p = reinterpret_cast<T*>(other.get());
			return Shared_ptr<T, Internal_allocator>(other, p);
		}

		template <typename T, typename U, Allocator Internal_allocator = Malloc_allocator>
		[[nodiscard]] inline constexpr Shared_ptr<T, Internal_allocator> reinterpret_pointer_cast(Shared_ptr<U, Internal_allocator>&& other) noexcept
		{
			T* p = reinterpret_cast<T*>(other.get());
			return Shared_ptr<T, Internal_allocator>(std::move(other), p);
		}


		template <typename T, Allocator Internal_allocator = Malloc_allocator>
		class Weak_ptr final {
		public:
			constexpr Weak_ptr() = default;

			template <typename T_o>
			constexpr Weak_ptr(const Shared_ptr<T_o, Internal_allocator>& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				if (cb_) {
					++cb_->weak_count;
				}
			}
			constexpr Weak_ptr(const Shared_ptr<T, Internal_allocator>& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				if (cb_) {
					++cb_->weak_count;
				}
			}
			template <typename T_o>
			constexpr Weak_ptr& operator=(const Shared_ptr<T_o, Internal_allocator>& other) noexcept
			{
				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				if (cb_) {
					++cb_->weak_count;
				}
				return *this;
			}
			constexpr Weak_ptr& operator=(const Shared_ptr<T, Internal_allocator>& other) noexcept
			{
				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				if (cb_) {
					++cb_->weak_count;
				}
				return *this;
			}

			template <typename T_o>
			constexpr Weak_ptr(const Weak_ptr<T_o, Internal_allocator>& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				if (cb_) {
					++cb_->weak_count;
				}
			}
			constexpr Weak_ptr(const Weak_ptr& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				if (cb_) {
					++cb_->weak_count;
				}
			}

			template <typename T_o>
			constexpr Weak_ptr& operator=(const Weak_ptr<T_o, Internal_allocator>& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				if (cb_) {
					++cb_->weak_count;
				}
				return *this;
			}
			constexpr Weak_ptr& operator=(const Weak_ptr& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				if (cb_) {
					++cb_->weak_count;
				}
				return *this;
			}

			template <typename T_o>
			constexpr Weak_ptr(Weak_ptr<T_o, Internal_allocator>&& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				other.cb_ = nullptr;
				other.ptr_ = nullptr;
			}
			constexpr Weak_ptr(Weak_ptr&& other) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(other.ptr_)
			{
				other.cb_ = nullptr;
				other.ptr_ = nullptr;
			}

			template <typename T_o>
			constexpr Weak_ptr(Weak_ptr<T_o, Internal_allocator>&& other, T* ptr) noexcept
				: allocator_(other.allocator_), cb_(other.cb_), ptr_(ptr)
			{
				other.cb_ = nullptr;
				other.ptr_ = nullptr;
			}

			template <typename T_o>
			constexpr Weak_ptr& operator=(Weak_ptr<T_o, Internal_allocator>&& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				other.cb_ = nullptr;
				other.ptr_ = nullptr;
				return *this;
			}
			constexpr Weak_ptr& operator=(Weak_ptr&& other) noexcept
			{
				if (this == &other) {
					return *this;
				}

				remove_reference();

				allocator_ = other.allocator_;
				cb_ = other.cb_;
				ptr_ = other.ptr_;

				other.cb_ = nullptr;
				other.ptr_ = nullptr;
				return *this;
			}

			constexpr ~Weak_ptr() noexcept
			{
				remove_reference();
			}

			[[nodiscard]] constexpr std::int64_t use_count() const noexcept
			{
				return cb_ ? cb_->use_count : 0;
			}

			[[nodiscard]] constexpr bool expired() const noexcept
			{
				return use_count() == 0;
			}

			constexpr void reset() noexcept
			{
				remove_reference();
				cb_ = nullptr;
				ptr_ = nullptr;
			}

			template <typename T_o, Allocator Internal_allocator_o>
			friend class Shared_ptr;

			[[nodiscard]] constexpr Shared_ptr<T, Internal_allocator> lock() noexcept
			{
				Shared_ptr<T, Internal_allocator> sp{ nullptr };
				if (!cb_) {
					return sp;
				}
				sp.ptr_ = ptr_;
				sp.cb_ = cb_;
				if (ptr_) {
					++sp.cb_->use_count;
				}
				return sp;
			}

		private:
			constexpr void remove_reference() noexcept
			{
				if (cb_ && !cb_->use_count) {
					ptr_ = nullptr;
				}
				if (!ptr_ && !cb_) {
					return;
				}
				if (cb_->weak_count > 0) {
					--cb_->weak_count;
				}
				if (cb_->use_count == 0 && cb_->weak_count == 0) {
					memoc::details::destruct_at<Control_block>(cb_);
					Block<void> cb_b = { MEMOC_SSIZEOF(Control_block), cb_ };
					allocator_.deallocate(cb_b);
					cb_ = nullptr;
				}
			}

			Internal_allocator allocator_{};
			Control_block* cb_{ nullptr };
			T* ptr_{ nullptr };
		};
	}

	using details::Shared_ptr;
	using details::Unique_ptr;
	using details::Weak_ptr;
	using details::const_pointer_cast;
	using details::dynamic_pointer_cast;
	using details::make_shared;
	using details::make_unique;
	using details::reinterpret_pointer_cast;
	using details::static_pointer_cast;
}

#endif // MEMOC_POINTERS_H

