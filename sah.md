$C(A, B) = t_{\text{traversal}} + p_{A} \sum^{N_A}_{i=1} (t_{\text{intersect}}(a_i)) + p_{B} \sum^{N_B}_{i=1} (t_{\text{intersect}}(b_i))$

* $C(A, B)$: Cost to split into volumes A & B
* $t_{\text{traversal}}$: Time to traverse an interior node?
* $p_{A/B}$: Probability a ray passes through A or B
  * Figure out how to calculate these! ([link](https://medium.com/@bromanz/how-to-create-awesome-accelerators-the-surface-area-heuristic-e14b5dec6160))
* $N_{A/B}$: Number of leaves in A or B
* $a_{i}/b_{i}$: Object at index $i$ in A or B
* $t_{\text{intersect}}(\text{obj})$: Cost for an intersection with $\text{obj}$.
