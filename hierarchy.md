# KD vs BVH

* BVHs split objects into groups, where KD-trees split space into subspaces.
  * In BVHs, the same object **never** appears in two splits! in a KD tree, it can and often does.
  * We can use the "centroid" (center of an objects BB) to determine which group and object should be in.
