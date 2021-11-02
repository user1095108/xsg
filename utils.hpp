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
  }

  return std::tuple(n, p);
}

inline auto prev_node(auto n, decltype(n) p) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;
  assert(n);

  if (!n)
  {
    n = p;
    p = right_node(n, nullptr);
  }
  else if (auto const l(left_node(n, p)); l)
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
  }

  return std::tuple(n, p);
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
inline void destroy(auto const n, decltype(n) p)
  noexcept(noexcept(delete n))
{
  if (n)
  {
    destroy(left_node(n, p), n);
    destroy(right_node(n, p), n);

    delete n;
  }
}

inline auto equal_range(auto n, decltype(n) p, auto&& k) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  decltype(n) gn{}, gp{};

  while (n)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      gn = n; gp = p;
      auto const tmp(n); n = left_node(n, p); p = tmp;
    }
    else if (c > 0)
    {
      auto const tmp(n); n = right_node(n, p); p = tmp;
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

  return std::tuple(std::tuple(n, p), std::tuple(gn, gp));
}

inline auto find(auto n, decltype(n) p, auto&& k) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  while (n)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      auto const tmp(n);
      n = left_node(n, p);
      p = tmp;
    }
    else if (c > 0)
    {
      auto const tmp(n);
      n = right_node(n, p);
      p = tmp;
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

  auto const f([&](auto&& f, auto& n, auto p, auto const d) noexcept ->
    std::tuple<pointer, std::size_t>
    {
      std::size_t sl, sr;

      if (auto const c(node::cmp(d->key(), n->key())); c < 0)
      {
        if (auto const l(left_node(n, p)); l)
        {
          if (auto const [nn, s](f(f, l, n, d)); s)
          {
            sl = s;
          }
          else
          {
            if (nn)
            {
              n->l_ = conv(nn, p);
            }

            return {nullptr, 0};
          }
        }
        else
        {
          n->l_ = conv(d, p);
          sl = size(d, n);
        }

        sr = size(right_node(n, p), n);
      }
      else
      {
        if (auto const r(right_node(n, p)); r)
        {
          if (auto const [nn, s](f(f, r, n, d)); s)
          {
            sr = s;
          }
          else
          {
            if (nn)
            {
              n->r_ = conv(nn, p);
            }

            return {nullptr, 0};
          }
        }
        else
        {
          n->r_ = conv(d, p);
          sr = size(d, n);
        }

        sl = size(left_node(n, p), n);
      }

      //
      auto const s(1 + sl + sr), S(2 * s);

      return (3 * sl > S) || (3 * sr > S) ?
        std::tuple(node::rebuild(n, p), std::size_t{}) :
        std::tuple(nullptr, s);
    }
  );

  ((n ? f(f, n, p, d) : std::tuple(n = d, std::size_t())), ...);
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

          detail::move(r0, n, p, l, r);
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

        if (q)
        {
          delete n;
        }

        return std::tuple(nn, np);
      }
    }
  }

  return std::tuple(pointer{}, pointer{});
}

}

constexpr bool operator==(auto const& lhs, decltype(lhs) rhs) noexcept
  requires(
    requires{
      lhs.begin(); lhs.end();
      &std::remove_cvref_t<decltype(lhs)>::node::cmp;
    } &&
    !std::is_const_v<std::remove_reference_t<decltype(lhs)>>
  )
{
  return std::equal(
    lhs.begin(), lhs.end(),
    rhs.begin(), rhs.end(),
    [](auto&& a, auto && b) noexcept
    {
      return std::remove_cvref_t<decltype(lhs)>::node::cmp(a, b) == 0;
    }
  );
}

constexpr auto operator<=>(auto const& lhs, decltype(lhs) rhs) noexcept
  requires(
    requires{
      lhs.begin(); lhs.end();
      &std::remove_cvref_t<decltype(lhs)>::node::cmp;
    } &&
    !std::is_const_v<std::remove_reference_t<decltype(lhs)>>
  )
{
  return std::lexicographical_compare_three_way(
    lhs.begin(), lhs.end(),
    rhs.begin(), rhs.end(),
    std::remove_cvref_t<decltype(lhs)>::node::cmp
  );
}

constexpr auto erase(auto& c, auto const& k)
  requires(
    requires{
      c.begin(); c.end();
      &std::remove_cvref_t<decltype(c)>::node::cmp;
    } &&
    !std::is_const_v<std::remove_reference_t<decltype(c)>>
  )
{
  return c.erase(k);
}

constexpr auto erase_if(auto& c, auto pred)
  requires(
    requires{
      c.begin(); c.end();
      &std::remove_cvref_t<decltype(c)>::node::cmp;
    } &&
    !std::is_const_v<std::remove_reference_t<decltype(c)>>
  )
{
  std::size_t r{};

  for (auto i(c.begin()); i.node();)
  {
    i = pred(*i) ? (++r, c.erase(i)) : std::next(i);
  }

  return r;
}

constexpr void swap(auto& lhs, decltype(lhs) rhs) noexcept
  requires(
    requires{
      lhs.begin(); lhs.end();
      &std::remove_cvref_t<decltype(lhs)>::node::cmp;
    } &&
    !std::is_const_v<std::remove_reference_t<decltype(lhs)>>
  )
{
  lhs.swap(rhs);
}

}

#endif // XSG_UTILS_HPP
