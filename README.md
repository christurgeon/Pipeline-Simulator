# MIPS Pipeline Simulator

This program simulates the five stages (IF, ID, EX, MEM, WB) of MIPS pipelining. It supports add, sub, and, or, lw, and sw instructions. Note that there is no implementation of forwarding in this simulation.

There is one command line argument which is the input file. Each instruction must end with a newline character. The output of the program shows each cycle of execution.

## Program Assumptions

* There is a single space character between instruction and parameters
* Each parameter is delimited by a comma or parentheses
* Each instruction has correct syntax
* All input files are valid and the length of the file is no more than 128 characters

Example instructions:
```
add $t0,$s2,$s3
add $t1,$t3,73
or $s0,$s0,$t3
lw $a0,12($sp)
sw $t6,32($a1)
```
