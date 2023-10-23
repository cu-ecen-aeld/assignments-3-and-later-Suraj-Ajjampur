# Analysis of Kernel Oops with Faulty Write Function

## Introduction
This markdown file discusses a kernel oops caused by executing `echo "hello_world" > /dev/faulty` in a Linux system. The oops occurs in a module named `faulty`, and the error is captured in the kernel log.

## Context
The `faulty_write` function in the `faulty` module has been intentionally designed to produce a fault by dereferencing a NULL pointer:

```c
ssize_t faulty_write (struct file *filp, const char __user *buf, size_t count, loff_t *pos) {
	/* make a simple fault by dereferencing a NULL pointer */
	*(int *)0 = 0;
	return 0;
}
```

## Corresponding Assembly Code
The corresponding assembly code produced via objdump is as follows:

```c
  0:	e8 00 00 00 00       	callq  5 <faulty_write+0x5>
  5:	55                   	push   %rbp
  6:	31 c0                	xor    %eax,%eax
  8:	c7 04 25 00 00 00 00 	movl   $0x0,0x0
  f:	00 00 00 00 
 13:	48 89 e5             	mov    %rsp,%rbp
 16:	5d                   	pop    %rbp
 17:	e9 00 00 00 00       	jmpq   1c <faulty_write+0x1c>
 1c:	0f 1f 40 00          	nopl   0x0(%rax)
```

## Error Details
Executing the command echo "hello_world" > /dev/faulty generates the following kernel oops:

```c
"hello_world" > /dev/faulty
-sh: hello_world: not found
echo "hello_world" > /dev/faulty
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x96000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=000000004204b000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 96000045 [#1] SMP
Modules linked in: hello(O) faulty(O) scull(O)
CPU: 0 PID: 158 Comm: sh Tainted: G           O      5.15.18 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x14/0x20 [faulty]
lr : vfs_write+0xa8/0x2b0
sp : ffffffc008d23d80
x29: ffffffc008d23d80 x28: ffffff80020d0000 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040001000 x22: 000000000000000c x21: 000000556bb02aa0
x20: 000000556bb02aa0 x19: ffffff8002075700 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc0006f7000 x3 : ffffffc008d23df0
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x14/0x20 [faulty]
 ksys_write+0x68/0x100
 __arm64_sys_write+0x20/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x40/0xa0
 el0_svc+0x20/0x60
 el0t_64_sync_handler+0xe8/0xf0
 el0t_64_sync+0x1a0/0x1a4
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 7538efa44f7cf87b ]---
```

# Analysis

## Causes
Dereferencing NULL Pointer: The faulty_write function intentionally dereferences a NULL pointer, causing a segmentation fault.

## Consequences

1. Kernel Panic: The kernel goes into a panic state, ceasing to operate normally.
2. System Unusable: The system generally goes into an unusable state, potentially requiring a reboot.

## References
Linux Device Drivers, 3rd Edition, Chapter 4

Kernel log and dmesg output

# Conclusion
The kernel oops was intentionally triggered by dereferencing a NULL pointer in the faulty_write function. This serves as a cautionary example of how not to write a device driver and highlights the severity of system faults and oops messages in Linux Kernel Development.

