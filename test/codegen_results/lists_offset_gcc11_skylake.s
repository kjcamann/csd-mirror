Disassembly of section .text:

0000000000000000 <slist_find>:
   0:	48 8b 07             	mov    (%rdi),%rax
   3:	45 31 c0             	xor    %r8d,%r8d
   6:	48 85 c0             	test   %rax,%rax
   9:	75 05                	jne    10 <slist_find+0x10>
   b:	4c 89 c0             	mov    %r8,%rax
   e:	c3                   	ret    
   f:	90                   	nop
  10:	39 70 f8             	cmp    %esi,-0x8(%rax)
  13:	74 13                	je     28 <slist_find+0x28>
  15:	48 8b 00             	mov    (%rax),%rax
  18:	48 85 c0             	test   %rax,%rax
  1b:	75 f3                	jne    10 <slist_find+0x10>
  1d:	45 31 c0             	xor    %r8d,%r8d
  20:	4c 89 c0             	mov    %r8,%rax
  23:	c3                   	ret    
  24:	0f 1f 40 00          	nopl   0x0(%rax)
  28:	4c 8d 40 f8          	lea    -0x8(%rax),%r8
  2c:	eb dd                	jmp    b <slist_find+0xb>
  2e:	66 90                	xchg   %ax,%ax

0000000000000030 <slist_find_ranges>:
  30:	48 8b 07             	mov    (%rdi),%rax
  33:	45 31 c0             	xor    %r8d,%r8d
  36:	48 85 c0             	test   %rax,%rax
  39:	74 10                	je     4b <slist_find_ranges+0x1b>
  3b:	39 70 f8             	cmp    %esi,-0x8(%rax)
  3e:	74 10                	je     50 <slist_find_ranges+0x20>
  40:	48 8b 00             	mov    (%rax),%rax
  43:	48 85 c0             	test   %rax,%rax
  46:	75 f3                	jne    3b <slist_find_ranges+0xb>
  48:	45 31 c0             	xor    %r8d,%r8d
  4b:	4c 89 c0             	mov    %r8,%rax
  4e:	c3                   	ret    
  4f:	90                   	nop
  50:	4c 8d 40 f8          	lea    -0x8(%rax),%r8
  54:	4c 89 c0             	mov    %r8,%rax
  57:	c3                   	ret    
  58:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  5f:	00 

0000000000000060 <stailq_find>:
  60:	48 8b 07             	mov    (%rdi),%rax
  63:	45 31 c0             	xor    %r8d,%r8d
  66:	48 85 c0             	test   %rax,%rax
  69:	75 05                	jne    70 <stailq_find+0x10>
  6b:	4c 89 c0             	mov    %r8,%rax
  6e:	c3                   	ret    
  6f:	90                   	nop
  70:	39 70 f8             	cmp    %esi,-0x8(%rax)
  73:	74 13                	je     88 <stailq_find+0x28>
  75:	48 8b 00             	mov    (%rax),%rax
  78:	48 85 c0             	test   %rax,%rax
  7b:	75 f3                	jne    70 <stailq_find+0x10>
  7d:	45 31 c0             	xor    %r8d,%r8d
  80:	4c 89 c0             	mov    %r8,%rax
  83:	c3                   	ret    
  84:	0f 1f 40 00          	nopl   0x0(%rax)
  88:	4c 8d 40 f8          	lea    -0x8(%rax),%r8
  8c:	eb dd                	jmp    6b <stailq_find+0xb>
  8e:	66 90                	xchg   %ax,%ax

0000000000000090 <stailq_find_ranges>:
  90:	48 8b 07             	mov    (%rdi),%rax
  93:	45 31 c0             	xor    %r8d,%r8d
  96:	48 85 c0             	test   %rax,%rax
  99:	74 10                	je     ab <stailq_find_ranges+0x1b>
  9b:	39 70 f8             	cmp    %esi,-0x8(%rax)
  9e:	74 10                	je     b0 <stailq_find_ranges+0x20>
  a0:	48 8b 00             	mov    (%rax),%rax
  a3:	48 85 c0             	test   %rax,%rax
  a6:	75 f3                	jne    9b <stailq_find_ranges+0xb>
  a8:	45 31 c0             	xor    %r8d,%r8d
  ab:	4c 89 c0             	mov    %r8,%rax
  ae:	c3                   	ret    
  af:	90                   	nop
  b0:	4c 8d 40 f8          	lea    -0x8(%rax),%r8
  b4:	4c 89 c0             	mov    %r8,%rax
  b7:	c3                   	ret    
  b8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  bf:	00 

00000000000000c0 <tailq_find>:
  c0:	48 8b 07             	mov    (%rdi),%rax
  c3:	45 31 c0             	xor    %r8d,%r8d
  c6:	48 39 c7             	cmp    %rax,%rdi
  c9:	75 05                	jne    d0 <tailq_find+0x10>
  cb:	4c 89 c0             	mov    %r8,%rax
  ce:	c3                   	ret    
  cf:	90                   	nop
  d0:	39 70 f8             	cmp    %esi,-0x8(%rax)
  d3:	74 13                	je     e8 <tailq_find+0x28>
  d5:	48 8b 00             	mov    (%rax),%rax
  d8:	48 39 f8             	cmp    %rdi,%rax
  db:	75 f3                	jne    d0 <tailq_find+0x10>
  dd:	45 31 c0             	xor    %r8d,%r8d
  e0:	4c 89 c0             	mov    %r8,%rax
  e3:	c3                   	ret    
  e4:	0f 1f 40 00          	nopl   0x0(%rax)
  e8:	4c 8d 40 f8          	lea    -0x8(%rax),%r8
  ec:	eb dd                	jmp    cb <tailq_find+0xb>
  ee:	66 90                	xchg   %ax,%ax

00000000000000f0 <tailq_find_ranges>:
  f0:	48 8b 07             	mov    (%rdi),%rax
  f3:	45 31 c0             	xor    %r8d,%r8d
  f6:	48 39 c7             	cmp    %rax,%rdi
  f9:	74 10                	je     10b <tailq_find_ranges+0x1b>
  fb:	39 70 f8             	cmp    %esi,-0x8(%rax)
  fe:	74 10                	je     110 <tailq_find_ranges+0x20>
 100:	48 8b 00             	mov    (%rax),%rax
 103:	48 39 c7             	cmp    %rax,%rdi
 106:	75 f3                	jne    fb <tailq_find_ranges+0xb>
 108:	45 31 c0             	xor    %r8d,%r8d
 10b:	4c 89 c0             	mov    %r8,%rax
 10e:	c3                   	ret    
 10f:	90                   	nop
 110:	4c 8d 40 f8          	lea    -0x8(%rax),%r8
 114:	4c 89 c0             	mov    %r8,%rax
 117:	c3                   	ret    
