#ifndef XSG_MAP_HPP
# define XSG_MAP_HPP
# pragma once

#include "utils.hpp"

#include "mapiterator.hpp"

namespace xsg
{

template <typename Key, typename Value,
  class Compare = std::compare_three_way>
class map
{
public:
  struct node;

  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<Key const, Value>;

  using difference_type = detail::difference_type;
  using size_type = detail::size_type;
  using reference = value_type&;
  using const_reference = value_type const&;

  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_iterator = mapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  struct node
  {
    using value_type = map::value_type;

    struct empty_t{};

    static constinit inline Compare const cmp;

    std::uintptr_t l_, r_;
    value_type kv_;

    explicit node(auto&& k, empty_t&&)
      noexcept(noexcept(value_type(std::forward<decltype(k)>(k), Value()))):
      kv_(std::forward<decltype(k)>(k), Value())
    {
    }

    explicit node(auto&& a, auto&& ...b)
      noexcept(noexcept(
          value_type(
            std::piecewise_construct_t{},
            std::forward_as_tuple(std::forward<decltype(a)>(a)),
            std::forward_as_tuple(std::forward<decltype(b)>(b)...)
          )
        )
      ):
      kv_(
        std::piecewise_construct_t{},
        std::forward_as_tuple(std::forward<decltype(a)>(a)),
        std::forward_as_tuple(std::forward<decltype(b)>(b)...)
      )
    {
    }

    //
    auto& key() const noexcept { return std::get<0>(kv_); }

    //
    static auto emplace(auto& r, auto&& k, auto&& ...b)
      noexcept(noexcept(
          new node(
            std::forward<decltype(k)>(k),
            std::forward<decltype(b)>(b)...
          )
        )
      )
      requires(detail::Comparable<Compare, decltype(k), key_type>)
    {
      enum Direction: bool { LEFT, RIGHT };

      bool s{}; // success
      node* q, *qp;

      auto const create_node([&](decltype(q) const p)
        noexcept(noexcept(
            new node(
              std::forward<decltype(k)>(k),
              std::forward<decltype(b)>(b)...
            )
          )
        )
        {
          auto const q(
            new node(
              std::forward<decltype(k)>(k),
              std::forward<decltype(b)>(b)...
            )
          );

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
            if (auto const nn(rebalance(n, p, q, qp, s)); p)
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

    static auto rebalance(auto const n, decltype(n) p,
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
  using this_class = map;
  node* root_{};

public:
  map() = default;

  map(map const& o)
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = o;
  }

  map(map&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  map(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
    requires(std::is_constructible_v<value_type, decltype(*i)>)
  {
    insert(i, j);
  }

  map(std::initializer_list<value_type> l)
    noexcept(noexcept(map(l.begin(), l.end()))):
    map(l.begin(), l.end())
  {
  }

  ~map() noexcept(noexcept(detail::destroy(root_, {})))
  {
    detail::destroy(root_, {});
  }

# include "common.hpp"

  //
  auto size() const noexcept { return detail::size(root_, {}); }

  //
  template <int = 0>
  auto& operator[](auto&& k)
    noexcept(noexcept(
        node::emplace(root_, std::forward<decltype(k)>(k),
          typename node::empty_t())
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return std::get<1>(
        std::get<0>(
          node::emplace(root_, std::forward<decltype(k)>(k),
            typename node::empty_t())
        )->kv_
      );
  }

  auto& operator[](key_type k)
    noexcept(noexcept(operator[]<0>(std::move(k))))
  {
    return operator[]<0>(std::move(k));
  }

  template <int = 0>
  auto& at(auto&& k) noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return std::get<1>(detail::find(root_, k)->kv_);
  }

  auto& at(key_type k) noexcept { return at<0>(std::move(k)); }

  template <int = 0>
  auto const& at(auto&& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return std::get<1>(detail::find(root_, std::forward<decltype(k)>(k))->kv_);
  }

  auto& at(key_type k) const noexcept { return at<0>(std::move(k)); }

  //
  template <int = 0>
  size_type count(auto&& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return bool(detail::find(root_, std::forward<decltype(k)>(k)));
  }

  auto count(key_type k) const noexcept { return count<0>(std::move(k)); }

  //
  template <int = 0>
  auto emplace(auto&& k, auto&& ...a)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::forward<decltype(k)>(k),
          std::forward<decltype(a)>(a)...
        )
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [n, p, s](
      node::emplace(
        root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(a)>(a)...
      )
    );

    return std::tuple(iterator(&root_, n, p), s);
  }

  auto emplace(key_type k, auto&& ...a)
    noexcept(noexcept(
        emplace<0>(std::move(k), std::forward<decltype(a)>(a)...)
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return emplace<0>(std::move(k), std::forward<decltype(a)>(a)...);
  }

  //
  template <int = 0>
  auto equal_range(auto&& k) noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [nl, g](
      detail::equal_range(root_, {}, std::forward<decltype(k)>(k))
    );

    return std::pair(iterator(&root_, nl), iterator(&root_, g));
  }

  auto equal_range(key_type k) noexcept
  {
    return equal_range<0>(std::move(k));
  }

  template <int = 0>
  auto equal_range(auto&& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [nl, g](
      detail::equal_range(root_, {}, std::forward<decltype(k)>(k))
    );

    return std::pair(const_iterator(&root_, nl), const_iterator(&root_, g));
  }

  auto equal_range(key_type k) const noexcept
  {
    return equal_range<0>(std::move(k));
  }

  //
  template <int = 0>
  size_type erase(auto&& k)
    noexcept(noexcept(detail::erase(root_, k)))
    requires(detail::Comparable<Compare, decltype(k), key_type> &&
      !std::convertible_to<decltype(k), const_iterator>)
  {
    return bool(
      std::get<0>(detail::erase(root_, std::forward<decltype(k)>(k)))
    );
  }

  auto erase(key_type k) noexcept(noexcept(erase<0>(std::move(k))))
  {
    return erase<0>(std::move(k));
  }

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

  //
  auto insert(value_type const& v)
    noexcept(noexcept(node::emplace(root_, std::get<0>(v), std::get<1>(v))))
  {
    auto const [n, p, s](
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    );

    return std::tuple(iterator(&root_, n, p), s);
  }

  auto insert(value_type&& v)
    noexcept(noexcept(
        node::emplace(root_, std::get<0>(v), std::move(std::get<1>(v)))
      )
    )
  {
    auto const [n, p, s](
      node::emplace(root_, std::get<0>(v), std::move(std::get<1>(v)))
    );

    return std::tuple(iterator(&root_, n, p), s);
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(emplace(std::get<0>(*i), std::get<1>(*i))))
  {
    std::for_each(
      i,
      j,
      [&](auto&& v)
        noexcept(noexcept(
            emplace(
              std::get<0>(std::forward<decltype(v)>(v)),
              std::get<1>(std::forward<decltype(v)>(v))
            )
          )
        )
      {
        emplace(
          std::get<0>(std::forward<decltype(v)>(v)),
          std::get<1>(std::forward<decltype(v)>(v))
        );
      }
    );
  }

  //
  template <int = 0>
  auto insert_or_assign(auto&& k, auto&& ...b)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::forward<decltype(k)>(k),
          std::forward<decltype(b)>(b)...
        )
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [n, p, s](
      node::emplace(
        root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(b)>(b)...
      )
    );

    if (!s)
    {
      if constexpr(sizeof...(b))
      {
        std::get<1>(n->kv_) = mapped_type(std::forward<decltype(b)>(b)...);
      }
      else
      {
        std::get<1>(n->kv_) = (std::forward<decltype(b)>(b), ...);
      }
    }

    return std::tuple(iterator(&root_, n, p), s);
  }

  auto insert_or_assign(key_type k, auto&& ...b)
    noexcept(noexcept(
        insert_or_assign<0>(std::move(k), std::forward<decltype(b)>(b)...)
      )
    )
  {
    return insert_or_assign<0>(std::move(k), std::forward<decltype(b)>(b)...);
  }
};

//////////////////////////////////////////////////////////////////////////////
template <int = 0, typename K, typename V, class C>
inline auto erase(map<K, V, C>& c, auto&& k)
  noexcept(noexcept(c.erase(std::forward<decltype(k)>(k))))
  requires(detail::Comparable<C, decltype(k), K> &&
    !std::same_as<K, std::remove_cvref_t<decltype(k)>>)
{
  return c.erase(std::forward<decltype(k)>(k));
}

template <typename K, typename V, class C>
inline auto erase(map<K, V, C>& c, K const& k)
  noexcept(noexcept(erase<0>(c, k)))
{
  return erase<0>(c, k);
}

template <typename K, typename V, class C>
inline auto erase_if(map<K, V, C>& c, auto pred)
  noexcept(
    noexcept(pred(std::declval<K>())) &&
    noexcept(c.erase(c.begin()))
  )
{
  typename std::remove_reference_t<decltype(c)>::size_type r{};

  for (auto i(c.begin()); i.n(); pred(*i) ? ++r, i = c.erase(i) : ++i);

  return r;
}

//////////////////////////////////////////////////////////////////////////////
template <typename K, typename V, class C>
inline void swap(map<K, V, C>& l, decltype(l) r) noexcept { l.swap(r); }

}

#endif // XSG_MAP_HPP
