#ifndef XSG_MULTIMAPITERATOR_HPP
# define XSG_MULTIMAPITERATOR_HPP
# pragma once

#include <iterator>

#include <type_traits>

namespace xsg
{

template <typename T>
class multimapiterator
{
  using inverse_const_t = std::conditional_t<
    std::is_const_v<T>,
    multimapiterator<std::remove_const_t<T>>,
    multimapiterator<T const>
  >;

  friend inverse_const_t;

  using node_t = std::remove_const_t<T>;

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

private:
  node_t* n_;
  node_t* p_;
  std::conditional_t<
    std::is_const_v<T>,
    typename std::list<std::remove_const_t<value_type>>::const_iterator,
    typename std::list<std::remove_const_t<value_type>>::iterator
  > i_;
  node_t* const* r_;

public:
  multimapiterator() = default;

  multimapiterator(node_t* const* const r) noexcept:
    n_(),
    r_(r)
  {
  }

  multimapiterator(node_t* const* const r, node_t* const n,
    node_t* const p) noexcept:
    n_(n),
    p_(p),
    r_(r)
  {
    if (n)
    {
      if constexpr(std::is_const_v<T>)
      {
        i_ = n->v_.cbegin();
      }
      else
      {
        i_ = n->v_.begin();
      }
    }
  }

  multimapiterator(node_t* const* const r,
    std::pair<node_t*, node_t*> const& t) noexcept:
    n_(std::get<0>(t)),
    p_(std::get<1>(t)),
    r_(r)
  {
    if (n_)
    {
      if constexpr(std::is_const_v<T>)
      {
        i_ = n_->v_.cbegin();
      }
      else
      {
        i_ = n_->v_.begin();
      }
    }
  }

  multimapiterator(node_t* const* const r, node_t* const n,
    decltype(n) p, decltype(i_) const i) noexcept:
    n_(n),
    p_(p),
    i_(i),
    r_(r)
  {
  }

  multimapiterator(multimapiterator const&) = default;
  multimapiterator(multimapiterator&&) = default;

  multimapiterator(inverse_const_t const& o) noexcept
    requires(std::is_const_v<T>):
    n_(o.n_),
    p_(o.p_),
    i_(o.i_),
    r_(o.r_)
  {
  }

  //
  multimapiterator& operator=(multimapiterator const&) = default;
  multimapiterator& operator=(multimapiterator&&) = default;

  bool operator==(multimapiterator const& o) const noexcept
  {
    return (o.n_ == n_) && (o.i_ == i_);
  }

  bool operator!=(multimapiterator const&) const = default;

  // increment, decrement
  auto& operator++() noexcept
  {
    if (i_ = std::next(i_); n_->v_.end() == i_)
    {
      if (std::tie(n_, p_) = detail::next_node(n_, p_); n_)
      {
        i_ = n_->v_.begin();
      }
    }

    return *this;
  }

  auto& operator--() noexcept
  {
    if (!n_)
    {
      if (std::tie(n_, p_) = detail::last_node(*r_, {}); n_)
      {
        i_ = std::prev(n_->v_.end());
      }
    }
    else if (n_->v_.begin() == i_)
    {
      if (std::tie(n_, p_) = detail::prev_node(n_, p_); n_)
      {
        i_ = std::prev(n_->v_.end());
      }
    }
    else
    {
      i_ = std::prev(i_);
    }

    return *this;
  }

  auto operator++(int) noexcept { auto const r(*this); ++*this; return r; }
  auto operator--(int) noexcept { auto const r(*this); --*this; return r; }

  // member access
  auto& operator->() const noexcept { return i_; }
  auto& operator*() const noexcept { return *i_; }

  //
  auto& iterator() const noexcept { return i_; }
  auto node() const noexcept { return n_; }
};

}

#endif // XSG_MULTIMAPITERATOR_HPP
