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

#include <numeric> // std::midpoint()
#include <tuple>
#include <utility>

namespace xsg::detail
{

using difference_type = std::ptrdiff_t;
using size_type = std::size_t;

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
  return [&](auto const ...b) noexcept { assign((a = b)...); };
}

inline auto conv(auto const ...n) noexcept
{
  return (std::uintptr_t(n) ^ ...);
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
    for (auto const& key(n->key()); p;)
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
    for (auto const& key(n->key()); p;)
    {
      if (node::cmp(key, p->key()) < 0)
      {
        assign(n, p)(p, left_node(p, n));
      }
      else
      {
        return std::pair(p, right_node(p, n));
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

inline auto equal_range(auto n, decltype(n) p, auto const& k) noexcept
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
    else [[unlikely]]
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

inline auto find(auto n, decltype(n) p, auto const& k) noexcept
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
    else [[unlikely]]
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

      q ? *q = conv(fnn, pp) : bool(r0 = fnn);

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

      q ? *q = conv(lnn, pp) : bool(r0 = lnn);

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

    q ? *q = conv(lr, pp) : bool(r0 = lr);
  }

  delete n;

  return std::pair(nnn, nnp);
}

inline auto erase(auto& r0, auto const& k)
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
    else [[unlikely]]
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

inline auto rebalance(auto const n, decltype(n) p,
  decltype(n) q, auto& qp, size_type const sz) noexcept
{
  using node_t = std::remove_pointer_t<std::remove_const_t<decltype(n)>>;

  auto const a(static_cast<node_t**>(XSG_ALLOCA(sizeof(node_t*) * sz)));

  struct S
  {
    std::remove_const_t<decltype(a)> b_;

    void operator()(decltype(n) n, decltype(n) p) noexcept
    {
      if (n)
      {
        operator()(detail::left_node(n, p), n);

        *b_++ = n;

        operator()(detail::right_node(n, p), n);
      }
    }
  };

  struct T
  {
    decltype(q) q_;
    decltype(qp) qp_;

    node_t* f(decltype(p) p, decltype(a) a, decltype(a) b) const noexcept
    {
      node_t* n;

      if (b == a)
      {
        if ((n = *a) == q_) qp_ = p;

        detail::assign(n->l_, n->r_)(detail::conv(p), detail::conv(p));
      }
      else if (b == a + 1)
      { // n - nb
        auto const nb(*b);

        if ((n = *a) == q_) qp_ = p; else if (nb == q_) qp_ = n;

        detail::assign(nb->l_, nb->r_, n->l_, n->r_)(detail::conv(n),
          detail::conv(n), detail::conv(p), detail::conv(p, nb));
      }
      else
      {
        auto const m(std::midpoint(a, b));

        if ((n = *m) == q_) qp_ = p;

        detail::assign(n->l_, n->r_)(
          detail::conv(f(n, a, m - 1), p),
          detail::conv(f(n, m + 1, b), p)
        );
      }

      return n;
    }
  };

  S s{a}; s(n, p);

  return T{q, qp}.f(p, a, s.b_ - 1);
}

inline auto emplace(auto& r, auto const& k, auto const& create_node)
  noexcept(noexcept(create_node({})))
{
  using node_t = std::remove_pointer_t<std::remove_reference_t<decltype(r)>>;

  struct S
  {
    enum Direction: bool { LEFT, RIGHT };

    decltype(r) r_;
    decltype(k) k_;
    decltype(create_node) create_node_;

    node_t* q_, *qp_;
    bool s_;

    explicit S(decltype(r) r, decltype(k) k,
      decltype(create_node) cn) noexcept:
      r_(r), k_(k), create_node_(cn)
    {
    }

    size_type operator()(node_t* n, decltype(n) p, enum Direction const d)
      noexcept(noexcept(create_node_({})))
    {
      size_type sl, sr;

      if (auto const c(node_t::cmp(k_, n->key())); c < 0)
      {
        if (auto const l = left_node(n, p))
        {
          if (!(sl = (*this)(l, n, LEFT))) return {};
        }
        else
        {
          assign(sl, q_, qp_, s_)(1, create_node_(n), n, true);
          n->l_ = conv(q_, p);
        }

        sr = size(right_node(n, p), n);
      }
      else if (c > 0)
      {
        if (auto const r = right_node(n, p))
        {
          if (!(sr = ((*this)(r, n, RIGHT)))) return {};
        }
        else
        {
          assign(sr, q_, qp_, s_)(1, create_node_(n), n, true);
          n->r_ = conv(q_, p);
        }

        sl = size(left_node(n, p), n);
      }
      else [[unlikely]]
      {
        assign(q_, qp_, s_)(n, p, false);

        return {};
      }

      //
      if (auto const s(1 + sl + sr), S(2 * s);
        (3 * sl > S) || (3 * sr > S))
      {
        if (auto const nn(rebalance(n, p, q_, qp_, s)); p)
        {
          d ? p->r_ = conv(nn, right_node(p, n)) :
            p->l_ = conv(nn, left_node(p, n));
        }
        else
        {
          r_ = nn;
        }

        return {};
      }
      else
      {
        return s;
      }
    }
  };

  //
  S s(r, k, create_node); s(r, {}, {});

  return std::tuple(s.q_, s.qp_, s.s_);
}

}

#endif // XSG_UTILS_HPP
