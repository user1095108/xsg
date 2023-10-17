#ifndef XSG_UTILS_HPP
#define XSG_UTILS_HPP
# pragma once

#if defined(_WIN32)
# include <malloc.h>
# define XSG_ALLOCA(x) _alloca(x)
#else
# include <stdlib.h>
# define XSG_ALLOCA(x) alloca(x)
#endif // XSG_ALLOCA

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <compare>

#include <tuple>
#include <utility>

namespace xsg::detail
{

using difference_type = std::intmax_t;
using size_type = std::uintmax_t;

template <class C, class U, class V>
concept Comparable =
  !std::is_void_v<
    std::common_comparison_category_t<
      std::remove_cvref_t<
        decltype(std::declval<C>()(std::declval<U>(), std::declval<V>()))
      >
    >
  >;

inline auto assign(auto& ...a) noexcept
{ // assign idiom
  return [&](auto const ...v) noexcept { ((a = v), ...); };
}

inline auto conv(auto const ...n) noexcept
{
  return (std::uintptr_t(n) ^ ...);
}

//
inline auto left_node(auto const n, decltype(n) p) noexcept
{
  return std::remove_const_t<decltype(n)>(n->l_ ^ conv(p));
}

inline auto right_node(auto const n, decltype(n) p) noexcept
{
  return std::remove_const_t<decltype(n)>(n->r_ ^ conv(p));
}

inline auto first_node(auto n, decltype(n) p) noexcept
{
  for (decltype(n) l; (l = left_node(n, p)); assign(n, p)(l, n));

  return std::pair(n, p);
}

inline auto last_node(auto n, decltype(n) p) noexcept
{
  for (decltype(n) r; (r = right_node(n, p)); assign(n, p)(r, n));

  return std::pair(n, p);
}

//
inline auto next_node(auto n, decltype(n) p) noexcept
{
  using pointer = std::remove_cvref_t<decltype(n)>;
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (auto const r(right_node(n, p)); r)
  {
    return first_node(r, n);
  }
  else
  {
    for (auto&& key(n->key()); p;)
    {
      if (node::cmp(key, p->key()) < 0)
      {
        return std::pair(p, left_node(p, n));
      }
      else
      {
        assign(n, p)(p, right_node(p, n));
      }
    }
  }

  return std::pair(pointer{}, pointer{});
}

inline auto next_node(auto const r0, auto n, decltype(n) p) noexcept
{
  using pointer = std::remove_cvref_t<decltype(n)>;
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (auto const r(right_node(n, p)); r)
  {
    return first_node(r, n);
  }
  else
  {
    for (auto&& key(n->key()); p && (r0 != n);)
    {
      if (node::cmp(key, p->key()) < 0)
      {
        return std::pair(p, left_node(p, n));
      }
      else
      {
        assign(n, p)(p, right_node(p, n));
      }
    }
  }

  return std::pair(pointer{}, pointer{});
}

inline auto prev_node(auto n, decltype(n) p) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;
  using pointer = std::remove_cvref_t<decltype(n)>;

  if (auto const l(left_node(n, p)); l)
  {
    return last_node(l, n);
  }
  else
  {
    for (auto&& key(n->key()); p;)
    {
      if (node::cmp(key, p->key()) > 0)
      {
        return std::pair(p, right_node(p, n));
      }
      else
      {
        assign(n, p)(p, left_node(p, n));
      }
    }
  }

  return std::pair(pointer{}, pointer{});
}

//
inline size_type height(auto const n, decltype(n) p) noexcept
{
  return n ?
    (left_node(n, p) || right_node(n, p)) +
    std::max(height(left_node(n, p), n), height(right_node(n, p), n)) :
    size_type{};
}

inline size_type size(auto const n, decltype(n) p) noexcept
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
  requires(Comparable<decltype(n->cmp), decltype(k), decltype(n->key())>)
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  decltype(n) gn{}, gp{};

  while (n)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      assign(gn, gp, n, p)(n, p, left_node(n, p), n);
    }
    else if (c > 0)
    {
      assign(n, p)(right_node(n, p), n);
    }
    else
    {
      if (auto const r(right_node(n, p)); r)
      {
        std::tie(gn, gp) = first_node(r, n);
      }

      break;
    }
  }

  return std::pair(
    n ? std::pair(n, p) : std::pair(gn, gp),
    std::pair(gn, gp)
  );
}

inline auto find(auto n, decltype(n) p, auto& k) noexcept
  requires(Comparable<decltype(n->cmp), decltype(k), decltype(n->key())>)
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  while (n)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      assign(n, p)(left_node(n, p), n);
    }
    else if (c > 0)
    {
      assign(n, p)(right_node(n, p), n);
    }
    else
    {
      break;
    }
  }

  return std::pair(n, p);
}

inline auto erase(auto& r0, auto const pp, decltype(pp) p, decltype(pp) n,
  std::uintptr_t* const q)
  noexcept(noexcept(delete r0))
{
  auto [nnn, nnp](next_node(n, p));

  // pp - p - n - lr
  if (auto const l(left_node(n, p)), r(right_node(n, p)); l && r)
  {
    if (size(l, n) < size(r, n)) // erase from right side?
    {
      auto const [fnn, fnp](first_node(r, n));

      if (fnn == nnn)
      {
        nnp = p;
      }

      if (q)
      {
        *q = conv(fnn, pp);
      }
      else
      {
        r0 = fnn;
      }

      // convert and attach l to fnn
      fnn->l_ = conv(l, p); 

      {
        auto const nfnn(conv(n, fnn));
        l->l_ ^= nfnn; l->r_ ^= nfnn;
      }

      if (r == fnn)
      {
        r->r_ ^= conv(n, p);
      }
      else
      {
        // attach right node of fnn to parent left
        {
          auto const fnpp(left_node(fnp, fnn));
          auto const rn(right_node(fnn, fnp));

          fnp->l_ = conv(rn, fnpp);

          if (rn)
          {
            auto const fnnfnp(conv(fnn, fnp));
            rn->l_ ^= fnnfnp; rn->r_ ^= fnnfnp;
          }
        }

        // convert and attach r to fnn
        fnn->r_ = conv(r, p);

        auto const nfnn(conv(n, fnn));
        r->l_ ^= nfnn; r->r_ ^= nfnn;
      }
    }
    else // erase from the left side
    {
      auto const [lnn, lnp](last_node(l, n));

      if (r == nnn)
      {
        nnp = lnn;
      }

      if (q)
      {
        *q = conv(lnn, pp);
      }
      else
      {
        r0 = lnn;
      }

      // convert and attach r to lnn
      lnn->r_ = conv(r, p); 

      {
        auto const nlnn(conv(n, lnn));
        r->l_ ^= nlnn; r->r_ ^= nlnn;
      }

      if (l == lnn)
      {
        l->l_ ^= conv(n, p);
      }
      else
      {
        {
          auto const lnpp(right_node(lnp, lnn));
          auto const ln(left_node(lnn, lnp));

          lnp->r_ = conv(ln, lnpp);

          if (ln)
          {
            auto const lnnlnp(conv(lnn, lnp));
            ln->l_ ^= lnnlnp; ln->r_ ^= lnnlnp;
          }
        }

        // convert and attach l to lnn
        lnn->l_ = conv(l, p);

        auto const nlnn(conv(n, lnn));
        l->l_ ^= nlnn; l->r_ ^= nlnn;
      }
    }
  }
  else
  {
    auto const lr(l ? l : r);

    if (lr)
    {
      if (lr == nnn)
      {
        nnp = p;
      }

      auto const np(conv(n, p));
      lr->l_ ^= np; lr->r_ ^= np;
    }

    if (q)
    {
      *q = conv(lr, pp);
    }
    else
    {
      r0 = lr;
    }
  }

  delete n;

  return std::pair(nnn, nnp);
}

inline auto erase(auto& r0, auto&& k)
  noexcept(noexcept(delete r0))
  requires(Comparable<decltype(r0->cmp), decltype(k), decltype(r0->key())>)
{
  using pointer = std::remove_cvref_t<decltype(r0)>;
  using node = std::remove_pointer_t<pointer>;

  std::uintptr_t* q{};

  for (pointer n(r0), p{}, pp{}; n;)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      assign(pp, p, n, q)(p, n, left_node(n, p), &n->l_);
    }
    else if (c > 0)
    {
      assign(pp, p, n, q)(p, n, right_node(n, p), &n->r_);
    }
    else
    {
      return erase(r0, pp, p, n, q);
    }
  }

  return std::pair(pointer{}, pointer{});
}

inline auto erase(auto& r0, auto const n, decltype(n) p)
  noexcept(noexcept(delete r0))
{
  using pointer = std::remove_cvref_t<decltype(r0)>;
  using node = std::remove_pointer_t<pointer>;

  pointer pp{};
  std::uintptr_t* q{};

  if (p)
  {
    if (node::cmp(n->key(), p->key()) < 0)
    {
      assign(pp, q)(left_node(p, n), &p->l_);
    }
    else
    {
      assign(pp, q)(right_node(p, n), &p->r_);
    }
  }

  return erase(r0, pp, p, n, q);
}

}

#endif // XSG_UTILS_HPP
