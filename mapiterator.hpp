#ifndef XSG_MAPITERATOR_HPP
# define XSG_MAPITERATOR_HPP
# pragma once

#include <iterator>
#include <type_traits>
#include <utility>

namespace xsg
{

template <typename C, typename T>
class mapiterator
{
  using inverse_const_t = std::conditional_t<
    std::is_const_v<T>,
    mapiterator<C, std::remove_const_t<T>>,
    mapiterator<C const, T const>
  >;

  friend inverse_const_t;

  C* c_;
  T* n_{};
  T* p_{};

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = std::conditional_t<
      std::is_const_v<T>,
      typename T::value_type const,
      typename T::value_type
    >;

  using pointer = value_type*;
  using reference = value_type&;

public:
  mapiterator(C* c) noexcept:
    c_(c)
  {
  }

  mapiterator(C* c,
    std::tuple<std::remove_const_t<T>*,
    std::remove_const_t<T>*> const& t) noexcept:
    c_(c)
  {
    std::tie(n_, p_) = t;
  }

  mapiterator(C* c, T* const n, T* const p) noexcept:
    c_(c),
    n_(n),
    p_(p)
  {
  }

  mapiterator(mapiterator const&) = default;
  mapiterator(mapiterator&&) = default;

  mapiterator(inverse_const_t const& o) noexcept requires(std::is_const_v<T>):
    c_(o.c_),
    n_(o.n_),
    p_(o.p_)
  {
  }

  //
  mapiterator& operator=(mapiterator const&) = default;
  mapiterator& operator=(mapiterator&&) = default;

  bool operator==(mapiterator const& o) const noexcept { return o.n_ == n_; }
  bool operator!=(mapiterator const&) const = default;

  // increment, decrement
  auto& operator++() noexcept
  {
    std::tie(n_, p_) = detail::next_node(n_, p_);

    return *this;
  }

  auto& operator--() noexcept
  {
    std::tie(n_, p_) = n_ ?
      detail::prev_node(n_, p_) :
      detail::last_node(c_->root(), {});

    return *this;
  }

  auto operator++(int) noexcept { auto const r(*this); ++*this; return r; }
  auto operator--(int) noexcept { auto const r(*this); --*this; return r; }

  // member access
  auto operator->() const noexcept { return &n_->kv_; }
  auto& operator*() const noexcept { return n_->kv_; }

  //
  auto node() const noexcept { return n_; }
};

}

#endif // XSG_MAPITERATOR_HPP
