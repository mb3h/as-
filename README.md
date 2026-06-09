# as-: Limited x86_64 Assembler compatible with GCC (preprocessor macro and inline-assembler)

## Overview

This tool shares limited specification of GCC about preprocessor macro, directive and inline-assembler, but it gives limited support about C/C++ syntax.

And it aims to add unique mechanism for resolving some issues that prevents efficient debugging.

## As is (Compatible test)

See test.c and test.h.

This is a sample code and a clipping of x86_64 native codebase that I want to accomplish debugging now.

```bash
# ELF built by GCC
$ gcc -std=c99 -Wno-attributes -masm=intel -c -o orig.o test.c

# ELF built by AS-
$ as-.py -o test.o test.c

# fixing '.comment' section and comparing
$ elf-replace orig.o .comment 'AS-: 0.0.0'
$ md5sum orig.o test.o
f0528a48ad019519a38268f9df3295ac  orig.o
f0528a48ad019519a38268f9df3295ac  test.o
```

## TODO priority / progress

BEGINNING
- [ ] (performance) write pass1-scanner code by C not Python

THINKING
- [ ] help 16-bytes alignment on stack-frame (for calling requirement of "printf" family functions)
- [ ] literalize length of specified codeblock (for injection of assertion code and stuff)
- [ ] collaborate with "struct" between C/C++ and assembler
- [ ] collaborate with GDB more smoothly

ADVANCED
- [ ] (for GDB) option that outputs '.L' prefix label into '.symtab'
- [ ] (for GDB) option that outputs '.debug_info' '.rela.debug_info' '.debug_line' '.rela.debug_line' ...
- [ ] (for debug-log injection) add a mechanism (pseudo instruction and stuff) that adjusts 16 bytes alignment about stack-frame (avoid #GP by 'movdqa'/'movaps' in 'printf')
- [ ] (for portability) Makefile support
- [ ] (for portability) add an option that accepts AT&T syntax (now Intel syntax only)
- [ ] (for portability) add an option that outputs COFF binary
