# xsg
c++ associative containers based on the XOR scapegoat tree. XOR BST trees are cousins of the XOR linked list. Eye-popping performance and modest resource requirements make XOR BST trees stand out of the crowd.

All iterators, but not references and pointers, are invalidated when adding to or erasing from XOR BST trees. You will be able to get away dereferencing invalidated iterators, but you won't be able to iterate with them.
