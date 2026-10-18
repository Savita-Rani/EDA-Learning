# 4-NAND decomposition of XOR — same circuit as Phases 3 and 4's demos,
# now described as data instead of addComponent() calls in C++.
NAND n1 A B
NAND n2 A n1
NAND n3 B n1
NAND OUT n2 n3
