#ifndef XSG_UTILS_HPP
#define XSG_UTILS_HPP
# pragma once

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <compare>

namespace xsg
{

namespace detail
{

constexpr auto conv(auto const n) noexcept
{
  return std::uintptr_t(n);
}

constexpr auto conv(auto const a, auto const b) noexcept
{
  return std::uintptr_t(a) ^ std::uintptr_t(b);
}

//
inline auto left_node(auto const n, decltype(n) p) noexcept
{
  return std::remove_const_t<decltype(n)>(conv(p) ^ n->l_);
}

inline auto right_node(auto const n, decltype(n) p) noexcept
{
  return std::remove_const_t<decltype(n)>(conv(p) ^ n->r_);
}

inline auto first_node(auto n, decltype(n) p) noexcept
{
  for (decltype(n) tmp; (tmp = left_node(n, p)); p = n, n = tmp);

  return std::tuple(n, p);
}

inline auto last_node(auto n, decltype(n) p) noexcept
{
  for (decltype(n) tmp; (tmp = right_node(n, p)); p = n, n = tmp);

  return std::tuple(n, p);
}

//
inline auto lparent_node(auto const n, decltype(n) l) noexcept
{
  return std::remove_const_t<decltype(n)>(conv(l) ^ n->l_);
}

inline auto rparent_node(auto const n, decltype(n) r) noexcept
{
  return std::remove_const_t<decltype(n)>(conv(r) ^ n->r_);
}


inline auto next_node(auto n, decltype(n) p) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;
  assert(n);

  if (auto const r(right_node(n, p)); r)
  {
    auto const [fn, fp](first_node(r, p));

    return fn ? std::tuple(fn, fp) : std::tuple(r, n);
  }
  else
  {
    for (auto&& key(n->key()); p;)
    {
      {
        auto const tmp(p);
        p = lparent_node(p, n);
        n = tmp;
      }

      if (auto const c(node::cmp(key, n->key())); c < 0)
      {
        break;
      }
    }

    return std::tuple(p, n);
  }
}

inline auto prev_node(auto n, decltype(n) p) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;
  assert(n);

  if (auto const l(left_node(n, p)); l)
  {
    auto const [ln, lp](last_node(l, p));

    return ln ? std::tuple(ln, lp) : std::tuple(l, n);
  }
  else
  {
    for (auto&& key(n->key()); p;)
    {
      {
        auto const tmp(p);
        p = rparent_node(p, n);
        n = tmp;
      }

      if (auto const c(node::cmp(key, n->key())); c > 0)
      {
        break;
      }
    }

    return std::tuple(p, n);
  }
}

//
inline std::size_t height(auto const n, auto const p) noexcept
{
  return n ?
    (left_node(n, p) || right_node(n, p)) +
    std::max(height(left_node(n), n), height(right_node(n), n)) :
    std::size_t{};
}

inline std::size_t size(auto const n, auto const p) noexcept
{
  return n ? 1 + size(left_node(n, p), n) + size(right_node(n, p), n) : 0;
}

//
inline void destroy(auto const n, auto const p)
  noexcept(noexcept(delete n))
{
  if (n)
  {
    destroy(left_node(n, p), n);
    destroy(right_node(n, p), n);

    delete n;
  }
}

inline auto equal_range(auto n, auto p, auto&& k) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  decltype(n) gn{}, gp{};

  while (n)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      gn = n; gp = p;
      n = left_node(n, p); p = n;
    }
    else if (c > 0)
    {
      n = right_node(n); p = n;
    }
    else
    {
      if (auto const r(right_node(n, p)); r)
      {
        auto const [fn, fp] = first_node(r, n);

        std::tie(gn, gp) = fn ? std::tuple(fn, fp) : std::tuple(r, n);
      }

      break;
    }
  }

  return std::tuple(n, p, gn, gp);
}

inline auto find(auto n, decltype(n) p, auto&& k) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  while (n)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      n = left_node(n, p);
      p = n
    }
    else if (c > 0)
    {
      n = right_node(n, p);
      p = n
    }
    else
    {
      break;
    }
  }

  return std::tuple(n, p);
}

inline void move(auto& n, auto p, auto const ...d)
{
  using pointer = std::remove_cvref_t<decltype(n)>;
  using node = std::remove_pointer_t<pointer>;

  auto const f([&](auto&& f, auto& n, auto p, auto const d) noexcept -> std::size_t
    {
      std::size_t sl, sr;

      if (auto const c(node::cmp(d->key(), n->key())); c < 0)
      {
        if (auto const l(left_node(n, p)); l)
        {
          if (sl = f(f, l, n, d); !sl)
          {
            return 0;
          }
        }
        else
        {
          n->l_ = conv(p, d);
          sl = size(d);
        }

        sr = size(right_node(n, p));
      }
      else
      {
        if (auto const r(right_node(n, p)); r)
        {
          if (sr = f(f, r, n, d); !sr)
          {
            return 0;
          }
        }
        else
        {
          n->r_ = conv(p, d);
          sr = size(d);
        }

        sl = size(left_node(n, p));
      }

      //
      auto const s(1 + sl + sr), S(2 * s);

      return (3 * sl > S) || (3 * sr > S) ? (n = n->rebuild(p), 0) : s;
    }
  );

  ((n ? f(f, n, p, d) : (n = d, 0)), ...);
}

inline auto erase(auto& r0, auto&& k)
{
  using pointer = std::remove_cvref_t<decltype(r0)>;
  using node = std::remove_pointer_t<pointer>;

  if (r0)
  {
    std::uintptr_t* q{};
    pointer p{}, pp{};

    for (auto n(r0);;)
    {
      if (auto const c(node::cmp(k, n->key())); c < 0)
      {
        q = &n->l_;

        pp = p;

        auto const tmp(n);
        n = left_node(n, p);
        p = tmp;
      }
      else if (c > 0)
      {
        q = &n->r_;

        pp = p;

        auto const tmp(n);
        n = right_node(n, p);
        p = tmp;
      }
      else
      {
        auto const [nn, np](next_node(n, p));

        if (auto const l(left_node(n, p)), r(right_node(n, p)); l && r)
        {
          if (q)
          {
            *q = conv(pp);
          }
          else
          {
            delete r0;
            r0 = {};
          }

          sg::detail::move(r0, n, p, l, r);
        }
        else
        {
          if (q)
          {
            *q = l ? conv(l, pp) : conv(r, pp);
          }
          else
          {
            delete r0;
            r0 = l ? l : r;
          }
        }

        delete n;

        return std::tuple(nn, np);
      }
    }
  }

  return std::tuple(pointer{}, pointer{});
}

}

}

#endif // SG_UTILS_HPP
