#ifndef XSG_SET_HPP
# define XSG_SET_HPP
# pragma once

#include "utils.hpp"

#include "mapiterator.hpp"

namespace xsg
{

template <typename Key, class Compare = std::compare_three_way>
class set
{
public:
  struct node;

  using key_type = Key;
  using value_type = Key;

  using difference_type = std::ptrdiff_t;
  using size_type = detail::size_type;
  using reference = value_type&;
  using const_reference = value_type const&;

  using const_iterator = mapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  struct node
  {
    using value_type = set::value_type;

    static constinit inline Compare const cmp;

    std::uintptr_t l_, r_;
    Key const kv_;

    explicit node(auto&& ...a)
      noexcept(noexcept(Key(std::forward<decltype(a)>(a)...))):
      kv_(std::forward<decltype(a)>(a)...)
    {
    }

    //
    auto&& key() const noexcept { return kv_; }

    //
    static auto emplace(auto& r, auto&& ...a)
    {
      enum Direction: bool { LEFT, RIGHT };

      bool s{}; // success
      node* q, *qp;

      key_type k(std::forward<decltype(a)>(a)...);

      auto const create_node([&](decltype(q) const p)
        noexcept(noexcept(new node(std::move(k))))
        {
          auto const q(new node(std::move(k)));
          q->l_ = q->r_ = detail::conv(p);

          return q;
        }
      );

      auto const f([&](auto&& f, auto const n, decltype(n) p,
        enum Direction const d)
        noexcept(noexcept(create_node(nullptr)))  -> size_type
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
              sl = s = (q = create_node(qp = n));
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
              sr = s = (q = create_node(qp = n));
              n->r_ = detail::conv(q, p);
            }

            sl = detail::size(detail::left_node(n, p), n);
          }
          else
          {
            qp = p;
            q = n;

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
        s = (r = q = create_node(qp = {}));
      }

      return std::tuple(q, qp, s);
    }

    static auto rebuild(auto const n, decltype(n) p,
      decltype(n) q, auto& qp, size_type const sz) noexcept
    {
      auto const l(static_cast<node**>(XSG_ALLOCA(sizeof(node*) * sz)));

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

                if (nb == q)
                {
                  qp = n;
                }

                nb->l_ = nb->r_ = detail::conv(n);
                n->l_ = detail::conv(p); n->r_ = detail::conv(nb, p);
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
  using this_class = set;
  node* root_{};

public:
  set() = default;

  set(std::initializer_list<value_type> const l)
    noexcept(noexcept(*this = l))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = l;
  }

  set(set const& o) 
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = o;
  }

  set(set&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  set(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
    requires(std::is_constructible_v<Key, decltype(*i)>)
  {
    insert(i, j);
  }

  ~set() noexcept(noexcept(delete root_))
  {
    detail::destroy(root_, {});
  }

# include "common.hpp"

  //
  auto size() const noexcept
  {
    return detail::size(root_, {});
  }

  //
  size_type count(auto const& k) const noexcept
  {
    return bool(detail::find(root_, {}, k));
  }

  //
  auto emplace(auto&& ...a)
    noexcept(noexcept(node::emplace(root_, std::forward<decltype(a)>(a)...)))
  {
    auto const [n, p, s](
      node::emplace(root_, std::forward<decltype(a)>(a)...)
    );

    return std::tuple(iterator(&root_, n, p), s);
  }

  //
  auto equal_range(auto const& k) noexcept
  {
    auto const [nl, g](detail::equal_range(root_, {}, k));

    return std::pair(iterator(&root_, nl), iterator(&root_, g));
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [nl, g](detail::equal_range(root_, {}, k));

    return std::pair(const_iterator(&root_, nl), const_iterator(&root_, g));
  }

  //
  iterator erase(const_iterator const i)
    noexcept(noexcept(
        detail::erase(
          root_,
          const_cast<node*>(i.n()),
          const_cast<node*>(i.p())
        )
      )
    )
  {
    return {
      &root_,
      detail::erase(
        root_,
        const_cast<node*>(i.n()),
        const_cast<node*>(i.p())
      )
    };
  }

  size_type erase(auto&& k)
    noexcept(noexcept(detail::erase(root_, k)))
    requires(
      std::three_way_comparable_with<
        key_type,
        std::remove_cvref_t<decltype(k)>
      >
    )
  {
    return bool(std::get<0>(detail::erase(root_, k)));
  }

  size_type erase(key_type const& k)
    noexcept(noexcept(detail::erase(root_, k)))
  {
    erase(k);
  }

  //
  auto insert(value_type const& v)
  {
    auto const [n, p, s](node::emplace(root_, v));

    return std::tuple(iterator(&root_, n, p), s);
  }

  auto insert(value_type&& v)
  {
    auto const [n, p, s](node::emplace(root_, std::move(v)));

    return std::tuple(iterator(&root_, n, p), s);
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
  {
    std::for_each(
      i,
      j,
      [&](auto&& v)
      {
        emplace(std::forward<decltype(v)>(v));
      }
    );
  }
};

//////////////////////////////////////////////////////////////////////////////
template <typename K, class C>
inline auto erase(set<K, C>& c, auto const& k)
  noexcept(noexcept(c.erase(k)))
{
  return c.erase(k);
}

template <typename K, class C>
inline auto erase_if(set<K, C>& c, auto pred)
  noexcept(noexcept(c.erase(c.begin())))
{
  typename std::remove_reference_t<decltype(c)>::size_type r{};

  for (auto i(c.begin()); i.n();)
  {
    i = pred(*i) ? ++r, c.erase(i) : std::next(i);
  }

  return r;
}

//////////////////////////////////////////////////////////////////////////////
template <typename K, class C>
inline void swap(set<K, C>& l, decltype(l) r) noexcept { l.swap(r); }

}

#endif // XSG_SET_HPP
