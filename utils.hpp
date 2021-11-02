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

constexpr auto conv(auto const ...n) noexcept
{
  return (std::uintptr_t(n) ^ ...);
}

//
inline auto left_node(auto const n, decltype(n) p) noexcept
{
  assert(n);
  return std::remove_const_t<decltype(n)>(conv(p) ^ n->l_);
}

inline auto right_node(auto const n, decltype(n) p) noexcept
{
  assert(n);
  return std::remove_const_t<decltype(n)>(conv(p) ^ n->r_);
}

inline auto first_node(auto n, decltype(n) p) noexcept
{
  for (decltype(n) l; (l = left_node(n, p)); p = n, n = l);

  return std::tuple(n, p);
}

inline auto last_node(auto n, decltype(n) p) noexcept
{
  for (decltype(n) r; (r = right_node(n, p)); p = n, n = r);

  return std::tuple(n, p);
}

//
inline auto lparent_node(auto const n, decltype(n) l) noexcept
{
  assert(n);
  return std::remove_const_t<decltype(n)>(conv(l) ^ n->l_);
}

inline auto rparent_node(auto const n, decltype(n) r) noexcept
{
  assert(n);
  return std::remove_const_t<decltype(n)>(conv(r) ^ n->r_);
}

inline auto next_node(auto n, decltype(n) p) noexcept
{
  using pointer = std::remove_cvref_t<decltype(n)>;
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;
  assert(n);

  if (auto const r(right_node(n, p)); r)
  {
    return first_node(r, n);
  }
  else if (p)
  {
    auto&& key(n->key());

    do
    {
      if (auto const c(node::cmp(key, p->key())); c < 0)
      {
        return std::tuple(p, lparent_node(p, n));
      }
      else
      {
        auto const tmp(p);
        p = rparent_node(p, n);
        n = tmp;
      }
    }
    while (p);
  }

  return std::tuple(pointer{}, pointer{});
}

inline auto prev_node(auto n, decltype(n) p) noexcept
{
  using pointer = std::remove_cvref_t<decltype(n)>;
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (!n)
  {
    assert(p);
    return std::tuple(p, right_node(p, {}));
  }
  else if (auto const l(left_node(n, p)); l)
  {
    return last_node(l, n);
  }
  else if (p)
  {
    auto&& key(n->key());

    do
    {
      if (auto const c(node::cmp(key, p->key())); c > 0)
      {
        return std::tuple(p, rparent_node(p, n));
      }
      else
      {
        auto const tmp(p);
        p = lparent_node(p, n);
        n = tmp;
      }
    }
    while (p);
  }

  return std::tuple(pointer{}, pointer{});
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
        auto const [gn, gp] = first_node(r, n);
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

constexpr auto invoke_all(auto f, auto&& ...a) noexcept(noexcept(
  (f(std::forward<decltype(a)>(a)), ...)))
{
  (f(std::forward<decltype(a)>(a)), ...);
}

inline void move(auto& r, auto const ...d)
{
  using pointer = std::remove_cvref_t<decltype(r)>;
  using node = std::remove_pointer_t<pointer>;

  auto const f([&](auto&& f, auto n, decltype(n) p,
    decltype(n) const d) noexcept -> std::tuple<pointer, std::size_t>
    {
      assert(n);
      std::size_t sl, sr;

      if (auto const c(node::cmp(d->key(), n->key())); c < 0)
      {
        if (auto const l(left_node(n, p)); l)
        {
          if (auto const [nn, s](f(f, l, n, d)); nn)
          {
            n->l_ = conv(nn, p);

            return {nullptr, 0};
          }
          else if (!(sl = s))
          {
            return {nullptr, 0};
          }
        }
        else
        {
          n->l_ = conv(d, p);
          d->l_ ^= conv(n); d->r_ ^= conv(n);

          sl = size(d, n);
        }

        sr = size(right_node(n, p), n);
      }
      else
      {
        if (auto const r(right_node(n, p)); r)
        {
          if (auto const [nn, s](f(f, r, n, d)); nn)
          {
            n->r_ = conv(nn, p);

            return {nullptr, 0};
          }
          else if (!(sr = s))
          {
            return {nullptr, 0};
          }
        }
        else
        {
          n->r_ = conv(d, p);
          d->l_ ^= conv(n); d->r_ ^= conv(n);

          sr = size(d, n);
        }

        sl = size(left_node(n, p), n);
      }

      //
      auto const s(1 + sl + sr), S(2 * s);

      return (3 * sl > S) || (3 * sr > S) ?
        std::tuple(node::rebuild(n, p), 0) :
        std::tuple(nullptr, s);
    }
  );

  invoke_all(
    [&](auto const p)
    {
      if (r)
      {
        if (auto const [nn, s](f(f, r, nullptr, p)); nn)
        {
          r = nn;
        }
      }
      else
      {
        r = p;
      }
    },
    d...
  );
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
        pp = p;

        auto const tmp(n);
        q = &n->l_;
        n = left_node(n, p);
        p = tmp;
      }
      else if (c > 0)
      {
        pp = p;

        auto const tmp(n);
        q = &n->r_;
        n = right_node(n, p);
        p = tmp;
      }
      else
      {
        auto const nn(next_node(n, p));

        // pp - p - n - lr
        auto const l(left_node(n, p)), r(right_node(n, p));
        delete n;

        if (l)
        {
          l->l_ ^= conv(n); l->r_ ^= conv(n);
        }

        if (r)
        {
          r->l_ ^= conv(n); r->r_ ^= conv(n);
        }

        if (l && r)
        {
          if (q)
          {
            *q = conv(pp);
          }
          else
          {
            r0 = {};
          }

          detail::move(r0, l, r);
        }
        else
        {
          if (q)
          {
            *q = l ? conv(l, pp) : conv(r, pp);
          }
          else
          {
            r0 = l ? l : r;
          }
        }

        return nn;
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
    }
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
    }
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
