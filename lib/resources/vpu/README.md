# VPU

## VPU modes (5bits)

0. idle
1. add (`add`): vector-scalar addition
2. accumulate (`add_acc`): vector-scalar addition, then vector-vector addition with the accumulator register
3. subtract (`sub`): vector-scalar subtraction
4. subtract and accumulate (`sub_acc`): vector-scalar subtraction, then vector-vector addition with the accumulator register
5. multiply (`mul`): vector-scalar multiplication
6. multiply and accumulate (`mac`): vector-scalar multiplication, then vector-vector addition with the accumulator register (updates the accumulator register)
7. minimum (`min`): element-wise vector-scalar minimum
8. maximum (`max`): element-wise vector-scalar maximum
9. shift left (`lshift`): vector-scalar shift
10. shift right (`rshift`): vector-scalar shift
11. set if equal (`seq`): element-wise vector-scalar comparison
12. set if not equal (`sne`): element-wise vector-scalar comparison
13. set if less than (`slt`): element-wise vector-scalar comparison
14. set if less than or equal (`sle`): element-wise vector-scalar comparison
15. set if greater than (`sgt`): element-wise vector-scalar comparison
16. set if greater than or equal (`sge`): element-wise vector-scalar comparison
17. bitwise and (`and`): element-wise vector-scalar bitwise and
18. bitwise or (`or`): element-wise vector-scalar bitwise or
19. bitwise xor (`xor`): element-wise vector-scalar bitwise xor
20. add (`vadd`): vector-vector addition
21. accumulate (`vadd_acc`): vector-vector addition, then vector-vector addition with the accumulator register
22. subtract (`vsub`): vector-vector subtraction
23. subtract and accumulate (`vsub_acc`): vector-vector subtraction, then vector-vector addition with the accumulator register
24. multiply (`vmul`): vector-vector multiplication
25. multiply and accumulate (`vmac`): vector-vector multiplication, then vector-vector addition with the accumulator register (updates the accumulator register)
26. minimum (`vmin`): element-wise vector-vector minimum
27. maximum (`vmax`): element-wise vector-vector maximum
28. shift left (`vlshift`): vector-vector shift
29. shift right (`vrshift`): vector-vector shift
30. set if equal (`vseq`): element-wise vector-vector comparison
31. set if not equal (`vsne`): element-wise vector-vector comparison
32. set if less than (`vslt`): element-wise vector-vector comparison
33. set if less than or equal (`vsle`): element-wise vector-vector comparison
34. set if greater than (`vsgt`): element-wise vector-vector comparison
35. set if greater than or equal (`vsge`): element-wise vector-vector comparison
36. bitwise and (`vand`): element-wise vector-vector bitwise and
37. bitwise or (`vor`): element-wise vector-vector bitwise or
38. bitwise xor (`vxor`): element-wise vector-vector bitwise xor
39. bitwise not (`vnot`): element-wise vector-vector bitwise not (ignores the second vector operand)
40. add immediate (`add_imm`): vector-scalar addition, where the scalar is an immediate value
41. multiply immediate (`mul_imm`): vector-scalar multiplication, where the scalar is an immediate value
42. minimum immediate (`min_imm`): element-wise vector-scalar minimum, where the scalar is an immediate value
43. maximum immediate (`max_imm`): element-wise vector-scalar maximum, where the scalar is an immediate value
44. shift left immediate (`lshift_imm`): vector-scalar shift, where the scalar is an immediate value
45. shift right immediate (`rshift_imm`): vector-scalar shift, where the scalar is an immediate value
46. set if equal immediate (`seq_imm`): element-wise vector-scalar comparison, where the scalar is an immediate value
47. set if not equal immediate (`sne_imm`): element-wise vector-scalar comparison, where the scalar is an immediate value
48. set if less than immediate (`slt_imm`): element-wise vector-scalar comparison, where the scalar is an immediate value
49. set if less than or equal immediate (`sle_imm`): element-wise vector-scalar comparison, where the scalar is an immediate value
50. set if greater than immediate (`sgt_imm`): element-wise vector-scalar comparison, where the scalar is an immediate value
51. set if greater than or equal immediate (`sge_imm`): element-wise vector-scalar comparison, where the scalar is an immediate value
52. bitwise and immediate (`and_imm`): element-wise vector-scalar bitwise and, where the scalar is an immediate value
53. bitwise or immediate (`or_imm`): element-wise vector-scalar bitwise or, where the scalar is an immediate value
54. bitwise xor immediate (`xor_imm`): element-wise vector-scalar bitwise xor, where the scalar is an immediate value
