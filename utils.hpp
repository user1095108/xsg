#ifndef XSG_UTILS_HPP
#define XSG_UTILS_HPP
# pragma once

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <compare>

namespace xsg
{

enum Direction: bool
{
  LEFT,
  RIGHT
};

namespace detail
{

constexpr auto conv(auto const n) noexcept { return std::uintptr_t(n); }

//
inline auto left_node(auto const n, decltype(n) p) noexcept
{
  return conv(p) ^ n->l_;
}

inline auto right_node(auto const n, decltype(n) p) noexcept
{
  return conv(p) ^ n->r_;
}

inline auto first_node(auto n, decltype(n) const p) noexcept
{
  for (decltype(n) p; (p = left_node(n, p)); n = p);

  return n;
}

inline auto last_node(auto n, decltype(n) const p) noexcept
{
  for (decltype(n) p; (p = right_node(n, p)); n = p);

  return n;
}

//
inline auto parent_node(auto n, enum Direction const d,
  decltype(n) const lr) noexcept
{
  return conv(lr) ^ (d == LEFT ? n->l_ : n->r);
}

inline auto next_node(auto n, enum Direction d, decltype(n) p) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (auto const r(right_node(n, p)); r)
  {
    decltype(n) const f(first_node(r, p));

    return f ? f : r;
  }
  else
  {
    auto&& key(n->key());

    do
    {
      if (auto const c(node::cmp(key, p->key())); c < 0)
      {
        break;
      }
      else
      {
        auto const tmp(p);
        p = parent_node(p, d, n);
        n = tmp;
      }
    }
    while (p);

    return p;
  }
}

}

}

#endif // SG_UTILS_HPP
