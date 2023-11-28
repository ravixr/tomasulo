# Tomasulo's Algorithm Simulator
This is a basic simulator of the Tomasulo's Algorithm. It takes in a file containing instructions and simulates the algorithm.

## Dependencies
- GCC (g++)
- GNU Make
- Git

## Building
Clone the repository and run `make` in the root directory.

## Running
Run `./tomasulo <input-file>` in the root directory.

## Input
The simulator takes in a text file containing instructions. 
The file must be in the following format:

```
<op> <dest>, <src1>, <src2>
```
or
```
<op> <dest/src>, <imm>(<src>)
```

where:
- `<op>` is one of the following: `add`, `sub`, `mul`, `div`, `lw`, `sw`
- `<dest>` is the destination register
- `<src1>` is the first source register
- `<src2>` is the second source register
- `<imm>` is an immediate value

Avaible registers in the input file are `r0` ... `r11`.

## Commands
- `help` or `h` - display help message
- `registers` or `r` - display registers
- `fus` or `f` - display functional units
- `status` or `s` - display status of instructions
- `next` or `n` - execute next clock cycle
- `clock` or `c` - display current clock cycle
- `exit` or `e` - exit simulator
