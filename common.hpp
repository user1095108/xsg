// iterators
iterator begin() noexcept
{
  return root_ ?
    iterator(&root_, detail::first_node(root_, {})) :
    iterator(&root_);
}

iterator end() noexcept { return iterator(&root_); }

// const iterators
const_iterator begin() const noexcept
{
  return root_ ?
    const_iterator(&root_, detail::first_node(root_, {})) :
    const_iterator(&root_);
}

const_iterator end() const noexcept { return const_iterator(&root_); }

auto cbegin() const noexcept { return begin(); }
auto cend() const noexcept { return end(); }

// reverse iterators
reverse_iterator rbegin() noexcept
{
  return reverse_iterator(iterator(&root_));
}

reverse_iterator rend() noexcept
{
  return root_ ?
    reverse_iterator(iterator(&root_, detail::first_node(root_, {}))) :
    reverse_iterator(iterator(&root_));
}

// const reverse iterators
const_reverse_iterator rbegin() const noexcept
{
  return const_reverse_iterator(const_iterator(&root_));
}

const_reverse_iterator rend() const noexcept
{
  return root_ ?
    const_reverse_iterator(
      const_iterator(&root_, detail::first_node(root_, {}))
    ) :
    const_reverse_iterator(const_iterator(&root_));
}

auto crbegin() const noexcept { return rbegin(); }
auto crend() const noexcept { return rend(); }

// self-assign neglected
auto& operator=(this_class const& o)
  noexcept(noexcept(clear(), insert(o.begin(), o.end())))
  requires(std::is_copy_constructible_v<value_type>)
{
  if (this != &o) clear(), insert(o.begin(), o.end());

  return *this;
}

auto& operator=(this_class&& o)
  noexcept(noexcept(detail::destroy(root_, {})))
{
  detail::destroy(root_, {});
  detail::assign(root_, o.root_)(o.root_, nullptr);

  return *this;
}

auto& operator=(std::initializer_list<value_type> const l)
  noexcept(noexcept(clear(), insert(l.begin(), l.end())))
{
  clear(); insert(l.begin(), l.end());

  return *this;
}

//
friend bool operator==(this_class const& l, this_class const& r)
  noexcept(noexcept(std::equal(l.begin(), l.end(), r.begin(), r.end())))
{
  return std::equal(l.begin(), l.end(), r.begin(), r.end());
}

friend auto operator<=>(this_class const& l, this_class const& r)
  noexcept(noexcept(
      std::lexicographical_compare_three_way(
        l.begin(), l.end(),
        r.begin(), r.end()
      )
    )
  )
{
  return std::lexicographical_compare_three_way(
      l.begin(), l.end(),
      r.begin(), r.end()
    );
}

//
auto root() const noexcept { return root_; }

//
static constexpr size_type max_size() noexcept
{
  return ~size_type{} / sizeof(node*);
}

void clear() noexcept(noexcept(detail::destroy(root_, {})))
{
  detail::destroy(root_, {}); root_ = {};
}

bool empty() const noexcept { return !root_; }

void swap(this_class& o) noexcept
{
  detail::assign(root_, o.root_)(o.root_, root_);
}

//
template <int = 0>
bool contains(auto const& k) const noexcept
  requires(detail::Comparable<Compare, key_type, decltype(k)>)
{
  return std::get<0>(detail::find(root_, {}, k));
}

auto contains(key_type const k) const noexcept { return contains<0>(k); }

//
iterator erase(const_iterator a, const_iterator const b)
  noexcept(noexcept(erase(a)))
{
  while (a != b) { a = erase(a); } return {&root_, a.n(), a.p()};
}

//
template <int = 0>
iterator find(auto const& k) noexcept
  requires(detail::Comparable<Compare, key_type, decltype(k)>)
{
  return {&root_, detail::find(root_, {}, k)};
}

auto find(key_type const k) noexcept { return find<0>(k); }

template <int = 0>
const_iterator find(auto const& k) const noexcept
  requires(detail::Comparable<Compare, key_type, decltype(k)>)
{
  return {&root_, detail::find(root_, {}, k)};
}

auto find(key_type const k) const noexcept { return find<0>(k); }

//
void insert(std::initializer_list<value_type> const l)
  noexcept(noexcept(insert(l.begin(), l.end())))
  requires(std::is_copy_constructible_v<value_type>)
{
  insert(l.begin(), l.end());
}

//
template <int = 0>
iterator lower_bound(auto const& k) noexcept
  requires(detail::Comparable<Compare, decltype(k), key_type>)
{
  return std::get<0>(equal_range(k));
}

auto lower_bound(key_type const k) noexcept { return lower_bound<0>(k); }

template <int = 0>
const_iterator lower_bound(auto const& k) const noexcept
  requires(detail::Comparable<Compare, decltype(k), key_type>)
{
  return std::get<0>(equal_range(k));
}

auto lower_bound(key_type const k) const noexcept
{
  return lower_bound<0>(k);
}

//
template <int = 0>
iterator upper_bound(auto const& k) noexcept
  requires(detail::Comparable<Compare, decltype(k), key_type>)
{
  return std::get<1>(equal_range(k));
}

auto upper_bound(key_type const k) noexcept { return upper_bound<0>(k); }

template <int = 0>
const_iterator upper_bound(auto const& k) const noexcept
  requires(detail::Comparable<Compare, decltype(k), key_type>)
{
  return std::get<1>(equal_range(k));
}

auto upper_bound(key_type const k) const noexcept
{
  return upper_bound<0>(k);
}
