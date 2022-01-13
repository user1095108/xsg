# xsg
All [binary search tree](https://en.wikipedia.org/wiki/Binary_search_tree) implementations are related to either the [singly](https://en.wikipedia.org/wiki/Linked_list) or the [doubly linked list](https://en.wikipedia.org/wiki/Doubly_linked_list). Therefore, they may also be related to the [XOR linked list](https://en.wikipedia.org/wiki/XOR_linked_list), a [doubly linked list](https://en.wikipedia.org/wiki/Doubly_linked_list) variant. XOR [BST](https://en.wikipedia.org/wiki/Binary_search_tree) trees are cousins of the [XOR linked list](https://en.wikipedia.org/wiki/XOR_linked_list). Performance and modest resource requirements make XOR [scapegoat trees](https://en.wikipedia.org/wiki/Scapegoat_tree) stand out of the [BST](https://en.wikipedia.org/wiki/Binary_search_tree) crowd.

All iterators (except `end()` iterators), but not references and pointers, are invalidated, after inserting or erasing from this XOR [scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree) implementation. You can dereference invalidated iterators, except the erased iterator, but you cannot iterate with them. `end()` iterators are constant and therefore always valid.

# how XOR BST trees work
Left and right node addresses of each node are XORed with the parent node address. The address of the parent of the root node is set arbitrarily (e.g. 0). To traverse an XOR [BST](https://en.wikipedia.org/wiki/Binary_search_tree) we need a `(parent, child)` pointer pair. To obtain the address of a grandparent node, we first make a comparison of a child's key with the key of its parent, then, based on this comparison, we XOR either the left or the right node address in the parent with the child's address. To obtain the left and right node addresses of a child we XOR them with the address of the parent. This way we can traverse an XOR [BST](https://en.wikipedia.org/wiki/Binary_search_tree) in two directions, away and towards the root node.

Since tree rebalancing usually changes node parent-child relations, all iterators, except `end()` iterators, are invalidated after inserting or erasing.

# evaluation of the XOR scapegoat BST
Important advantages of scapegoat BSTs over other [BST](https://en.wikipedia.org/wiki/Binary_search_tree)s are simplicity and reduced memory consumption. XOR scapegoat BSTs introduce additional complexity with an associated performance hit, compared to plain scapegoat BSTs; insertion and erasure by key can perform worse than when using a plain scapegoat BST. This leaves tree traversal and erasure by iterator as the only advantages the XOR scapegoat BST has over the plain scapegoat BST.

# build instructions
    git submodule update --init
    g++ -std=c++20 -Ofast set.cpp -o s
    g++ -std=c++20 -Ofast map.cpp -o m
