#ifndef XSG_MAPITERATOR_HPP
# define XSG_MAPITERATOR_HPP
# pragma once

#include <iterator>
#include <type_traits>
#include <utility>

namespace xsg
{

template <typename T>
class mapiterator
{
  using iterator_t = mapiterator<std::remove_const_t<T>>;
  friend mapiterator<T const>;

  T* n_, *p_;
  T* const* r_;

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = detail::difference_type;
  using value_type = std::conditional_t<
      std::is_const_v<T>,
      typename T::value_type const,
      typename T::value_type
    >;

  using pointer = value_type*;
  using reference = value_type&;

  template <typename, typename, class> friend class map;
  template <typename, class> friend class set;

public:
  mapiterator() = default;

  mapiterator(T* const* const r) noexcept:
    n_(),
    r_(r)
  {
  }

  mapiterator(T* const* const r, auto&& t) noexcept:
    n_(std::get<0>(t)),
    p_(std::get<1>(t)),
    r_(r)
  {
  }

  mapiterator(T* const* const r, T* const n, T* const p) noexcept:
    n_(n),
    p_(p),
    r_(r)
  {
  }

  mapiterator(mapiterator const&) = default;
  mapiterator(mapiterator&&) = default;

  mapiterator(iterator_t const& o) noexcept requires(std::is_const_v<T>):
    n_(o.n_),
    p_(o.p_),
    r_(o.r_)
  {
  }

  //
  mapiterator& operator=(mapiterator const&) = default;
  mapiterator& operator=(mapiterator&&) = default;

  mapiterator& operator=(iterator_t const& o) noexcept
    requires(std::is_const_v<T>)
  {
    n_ = o.n_; p_ = o.p_; r_ = o.r_; return *this;
  }

  bool operator==(mapiterator const& o) const noexcept { return n_ == o.n_; }

  // increment, decrement
  auto& operator++() noexcept
  {
    std::tie(n_, p_) = detail::next_node(n_, p_); return *this;
  }

  auto& operator--() noexcept
  {
    std::tie(n_, p_) = n_ ?
      detail::prev_node(n_, p_) :
      detail::last_node(*r_, {});

    return *this;
  }

  mapiterator operator++(int) noexcept
  {
    auto const n(n_), p(p_);

    std::tie(n_, p_) = detail::next_node(n_, p_);

    return {r_, n, p};
  }

  mapiterator operator--(int) noexcept
  {
    auto const n(n_), p(p_);

    std::tie(n_, p_) = n_ ?
      detail::prev_node(n_, p_) :
      detail::last_node(*r_, {});

    return {r_, n, p};
  }

  // member access
  auto operator->() const noexcept { return &n_->kv_; }
  auto& operator*() const noexcept { return n_->kv_; }

  //
  explicit operator bool() const noexcept { return n_; }
};

}

#endif // XSG_MAPITERATOR_HPP
