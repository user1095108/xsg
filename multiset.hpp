#ifndef XSG_MULTIMAP_HPP
# define XSG_MULTIMAP_HPP
# pragma once

#include "utils.hpp"

#include "multimapiterator.hpp"

namespace xsg
{

template <typename Key, class Compare = std::compare_three_way>
class multiset
{
public:
  struct node;

  using key_type = Key;
  using value_type = Key;

  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;
  using reference = value_type&;
  using const_reference = value_type const&;

  using const_iterator = multimapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = multimapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  struct node
  {
    using value_type = multiset::value_type;

    static constinit inline Compare const cmp;

    std::uintptr_t l_, r_;
    xl::list<value_type> v_;

    explicit node(auto&& k)
      noexcept(noexcept(v_.emplace_back(std::forward<decltype(k)>(k))))
    {
      v_.emplace_back(std::forward<decltype(k)>(k));
    }

    //
    auto&& key() const noexcept { return v_.front(); }

    //
    static auto emplace(auto& r, auto&& ...a)
    {
      enum Direction: bool { LEFT, RIGHT };

      key_type k(std::forward<decltype(a)>(a)...);

      node* q, *qp;

      auto const create_node([&](decltype(q) const p)
        {
          auto const q(new node(std::move(k)));
          q->l_ = q->r_ = detail::conv(p);

          return q;
        }
      );

      auto const f([&](auto&& f, auto const n, decltype(n) p,
        enum Direction const d) -> size_type
        {
          size_type sl, sr;

          if (auto const c(cmp(k, n->key())); c < 0)
          {
            if (auto const l(detail::left_node(n, p)); l)
            {
              if (auto const sz(f(f, l, n, LEFT)); sz)
              {
                sl = sz;
              }
              else
              {
                return {};
              }
            }
            else
            {
              sl = bool(q = create_node(qp = n));
              n->l_ = detail::conv(q, p);
            }

            sr = detail::size(detail::right_node(n, p), n);
          }
          else if (c > 0)
          {
            if (auto const r(detail::right_node(n, p)); r)
            {
              if (auto const sz(f(f, r, n, RIGHT)); sz)
              {
                sr = sz;
              }
              else
              {
                return {};
              }
            }
            else
            {
              sr = bool(q = create_node(qp = n));
              n->r_ = detail::conv(q, p);
            }

            sl = detail::size(detail::left_node(n, p), n);
          }
          else
          {
            (qp = p, q = n)->v_.emplace_back(std::move(k));

            return {};
          }

          //
          if (auto const s(1 + sl + sr), S(2 * s);
            (3 * sl > S) || (3 * sr > S))
          {
            if (auto const nn(rebuild(n, p, q, qp, s)); p)
            {
              d ?
                p->r_ = detail::conv(nn, detail::right_node(p, n)) :
                p->l_ = detail::conv(nn, detail::left_node(p, n));
            }
            else
            {
              r = nn;
            }

            return {};
          }
          else
          {
            return s;
          }
        }
      );

      if (r)
      {
        f(f, r, {}, {});
      }
      else
      {
        r = q = create_node(qp = {});
      }

      return std::pair(q, qp);
    }

    inline auto equal_range(auto n, decltype(n) p, auto&& k) noexcept
    {
      using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

      decltype(n) gn{}, gp{};

      for (; n;)
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

    static iterator erase(auto& r, const_iterator const i)
    {
      if (auto const n(i.n()), p(i.p()); 1 == n->v_.size())
      {
        auto const [nn, np, s](node::erase(r, std::get<0>(*i)));

        return {&r, nn, np};
      }
      else if (auto const it(i.i()); std::next(it) == n->v_.end())
      {
        auto const ni(std::next(i));

        n->v_.erase(it);

        return {&r, ni.n(), ni.p()};
      }
      else
      {
        return {&r, n, p, n->v_.erase(it)};
      }
    }

    static inline auto erase(auto& r0, auto const pp, decltype(pp) p,
      decltype(pp) n, std::uintptr_t* const q)
    {
      auto const s(n->v_.size());
      auto [nnn, nnp](detail::next_node(n, p));

      // pp - p - n - lr
      if (auto const l(detail::left_node(n, p)),
        r(detail::right_node(n, p)); l && r)
      {
        if (detail::size(l, n) < detail::size(r, n)) // erase from right side?
        {
          auto const [fnn, fnp](detail::first_node(r, n));

          if (fnn == nnn)
          {
            nnp = p;
          }

          if (q)
          {
            *q = detail::conv(fnn, pp);
          }
          else
          {
            r0 = fnn;
          }

          // convert and attach l to fnn
          fnn->l_ = detail::conv(l, p); 

          {
            auto const nfnn(detail::conv(n, fnn));
            l->l_ ^= nfnn; l->r_ ^= nfnn;
          }

          if (r == fnn)
          {
            r->r_ ^= detail::conv(n, p);
          }
          else
          {
            // attach right node of fnn to parent left
            {
              auto const fnpp(detail::left_node(fnp, fnn));
              auto const rn(detail::right_node(fnn, fnp));

              fnp->l_ = detail::conv(rn, fnpp);

              if (rn)
              {
                auto const fnnfnp(detail::conv(fnn, fnp));
                rn->l_ ^= fnnfnp; rn->r_ ^= fnnfnp;
              }
            }

            // convert and attach r to fnn
            fnn->r_ = detail::conv(r, p);

            {
              auto const nfnn(detail::conv(n, fnn));
              r->l_ ^= nfnn; r->r_ ^= nfnn;
            }
          }
        }
        else // erase from the left side
        {
          auto const [lnn, lnp](detail::last_node(l, n));

          if (r == nnn)
          {
            nnp = lnn;
          }

          if (q)
          {
            *q = detail::conv(lnn, pp);
          }
          else
          {
            r0 = lnn;
          }

          // convert and attach r to lnn
          lnn->r_ = detail::conv(r, p); 

          {
            auto const nlnn(detail::conv(n, lnn));
            r->l_ ^= nlnn; r->r_ ^= nlnn;
          }

          if (l == lnn)
          {
            l->l_ ^= detail::conv(n, p);
          }
          else
          {
            {
              auto const lnpp(detail::right_node(lnp, lnn));
              auto const ln(detail::left_node(lnn, lnp));

              lnp->r_ = detail::conv(ln, lnpp);

              if (ln)
              {
                auto const lnnlnp(detail::conv(lnn, lnp));
                ln->l_ ^= lnnlnp; ln->r_ ^= lnnlnp;
              }
            }

            // convert and attach l to lnn
            lnn->l_ = detail::conv(l, p);

            {
              auto const nlnn(detail::conv(n, lnn));
              l->l_ ^= nlnn; l->r_ ^= nlnn;
            }
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

          auto const np(detail::conv(n, p));
          lr->l_ ^= np; lr->r_ ^= np;
        }

        if (q)
        {
          *q = detail::conv(lr, pp);
        }
        else
        {
          r0 = lr;
        }
      }

      delete n;

      return std::tuple(nnn, nnp, s);
    }

    static auto erase(auto& r0, auto&& k)
    {
      using pointer = std::remove_cvref_t<decltype(r0)>;
      using node = std::remove_pointer_t<pointer>;

      std::uintptr_t* q{};

      for (pointer pp{}, p{}, n(r0); n;)
      {
        if (auto const c(node::cmp(k, n->key())); c < 0)
        {
          detail::assign(pp, p, n, q)(p, n, detail::left_node(n, p), &n->l_);
        }
        else if (c > 0)
        {
          detail::assign(pp, p, n, q)(p, n, detail::right_node(n, p), &n->r_);
        }
        else
        {
          return erase(r0, pp, p, n, q);
        }
      }

      return std::tuple(pointer{}, pointer{}, std::size_t{});
    }

    static auto erase(auto& r0, auto const n, decltype(n) const p)
    {
      using pointer = std::remove_cvref_t<decltype(r0)>;
      using node = std::remove_pointer_t<pointer>;

      pointer pp{};
      std::uintptr_t* q{};

      if (p)
      {
        if (node::cmp(n->key(), p->key()) < 0)
        {
          detail::assign(pp, q)(detail::left_node(p, n), &p->l_);
        }
        else
        {
          detail::assign(pp, q)(detail::right_node(p, n), &p->r_);
        }
      }

      return erase(r0, pp, p, n, q);
    }

    static auto rebuild(auto const n, decltype(n) p,
      decltype(n) q, auto& qp, size_type const sz)
    {
      auto const l(static_cast<node**>(ALLOCA(sizeof(node*) * sz)));

      {
        auto f([l(l)](auto&& f, auto const n,
          decltype(n) const p) mutable noexcept -> void
          {
            if (n)
            {
              f(f, detail::left_node(n, p), n);

              *l++ = n;

              f(f, detail::right_node(n, p), n);
            }
          }
        );

        f(f, n, p);
      }

      auto const f([l, q, &qp](auto&& f, auto const p,
        size_type const a, decltype(a) b) noexcept -> node*
        {
          auto const i((a + b) / 2);
          auto const n(l[i]);

          if (n == q)
          {
            qp = p;
          }

          switch (b - a)
          {
            case 0:
              n->l_ = n->r_ = detail::conv(p);

              break;

            case 1:
              {
                // n - nb
                auto const nb(l[b]);

                nb->l_ = nb->r_ = detail::conv(n);
                n->l_ = detail::conv(p); n->r_ = detail::conv(nb, p);

                if (nb == q)
                {
                  qp = n;
                }
              }

              break;

            default:
              detail::assign(n->l_, n->r_)(
                detail::conv(f(f, n, a, i - 1), p),
                detail::conv(f(f, n, i + 1, b), p)
              );
          }

          return n;
        }
      );

      //
      return f(f, p, {}, sz - 1);
    }
  };

private:
  using this_class = multiset;
  node* root_{};

public:
  multiset() = default;

  multiset(std::initializer_list<value_type> l)
    noexcept(noexcept(*this = l))
  {
    *this = l;
  }

  multiset(multiset const& o)
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = o;
  }

  multiset(multiset&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  multiset(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
    requires(std::is_constructible_v<value_type, decltype(*i)>)
  {
    insert(i, j);
  }

  ~multiset() noexcept(noexcept(delete root_)) { detail::destroy(root_, {}); }

# include "common.hpp"

  //
  auto size() const noexcept
  {
    static constinit auto const f(
      [](auto&& f, auto const n, decltype(n) p) noexcept -> size_type
      {
        return n ?
          n->v_.size() +
          f(f, detail::left_node(n, p), n) +
          f(f, detail::right_node(n, p), n) :
          std::size_t{};
      }
    );

    return f(f, root_, {});
  }

  //
  size_type count(auto const& k) const noexcept
  {
    for (decltype(root_) p{}, n(root_); n;)
    {
      if (auto const c(node::cmp(k, n->key())); c < 0)
      {
        detail::assign(n, p)(detail::left_node(n, p), n);
      }
      else if (c > 0)
      {
        detail::assign(n, p)(detail::right_node(n, p), n);
      }
      else
      {
        return n->v_.size();
      }
    }

    return {};
  }

  //
  iterator emplace(auto&& k)
  {
    return {
      &root_,
      node::emplace(root_, std::forward<decltype(k)>(k))
    };
  }

  //
  auto equal_range(Key const& k) noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));

    return std::pair(
      iterator(&root_, e ? e : g),
      iterator(&root_, g)
    );
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));

    return std::pair(
      const_iterator(&root_, e ? e : g),
      const_iterator(&root_, g)
    );
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));

    return std::pair(
      iterator(&root_, e ? e : g),
      iterator(&root_, g)
    );
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));

    return std::pair(
      const_iterator(&root_, e ? e : g),
      const_iterator(&root_, g)
    );
  }

  //
  iterator erase(const_iterator const i)
  {
    return node::erase(root_, i);
  }

  size_type erase(Key const& k)
  {
    return std::get<1>(node::erase(root_, k));
  }

  //
  iterator insert(value_type const& v)
  {
    return {
      &root_,
      node::emplace(root_, v.first)
    };
  }

  iterator insert(value_type&& v)
  {
    return {
      &root_,
      node::emplace(root_, std::move(v))
    };
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
  {
    std::for_each(
      i,
      j,
      [&](auto&& v) { emplace(v); }
    );
  }
};

}

#endif // XSG_MULTISET_HPP