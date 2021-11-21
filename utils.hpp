#ifndef XSG_UTILS_HPP
#define XSG_UTILS_HPP
# pragma once

#include <cstdint>

#include <algorithm>
#include <compare>

#include <utility>

namespace xsg
{

namespace detail
{

constexpr auto assign(auto& ...a) noexcept
{ // assign idiom
  return [&](auto const ...v) noexcept { ((a = v), ...); };
}

constexpr auto conv(auto const ...n) noexcept
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
  for (decltype(n) l; (l = left_node(n, p)); assign(p, n)(n, l));

  return std::pair(n, p);
}

inline auto last_node(auto n, decltype(n) p) noexcept
{
  for (decltype(n) r; (r = right_node(n, p)); assign(p, n)(n, r));

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
inline std::size_t height(auto const n, decltype(n) p) noexcept
{
  return n ?
    (left_node(n, p) || right_node(n, p)) +
    std::max(height(left_node(n, p), n), height(right_node(n, p), n)) :
    std::size_t{};
}

inline std::size_t size(auto const n, decltype(n) p) noexcept
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

  return std::pair(std::pair(n, p), std::pair(gn, gp));
}

inline auto find(auto n, decltype(n) p, auto&& k) noexcept
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

inline auto erase(auto& r0, auto const n, decltype(n) p, decltype(n) pp, std::uintptr_t* q)
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

      if (q)
      {
        *q = conv(fnn, pp);
      }
      else
      {
        r0 = fnn;
      }
    }
    else // erase from the left side
    {
      auto const [lnn, lnp](last_node(l, n));

      if (r == nnn)
      {
        nnp = lnn;
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

      if (q)
      {
        *q = conv(lnn, pp);
      }
      else
      {
        r0 = lnn;
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
{
  using pointer = std::remove_cvref_t<decltype(r0)>;
  using node = std::remove_pointer_t<pointer>;

  std::uintptr_t* q{};

  for (pointer n(r0), p{}, pp{}; n;)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      assign(pp, q, n, p)(p, &n->l_, left_node(n, p), n);
    }
    else if (c > 0)
    {
      assign(pp, q, n, p)(p, &n->r_, right_node(n, p), n);
    }
    else
    {
      return erase(r0, n, p, pp, q);
    }
  }

  return std::pair(pointer{}, pointer{});
}

inline auto erase(auto& r0, auto const n, decltype(n) p)
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

  return erase(r0, n, p, pp, q);
}

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
