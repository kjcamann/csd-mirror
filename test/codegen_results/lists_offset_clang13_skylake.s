Disassembly of section .text:

0000000000000000 <slist_find>:
   0:	48 8b 0f             	mov    (%rdi),%rcx
   3:	31 c0                	xor    %eax,%eax
   5:	48 85 c9             	test   %rcx,%rcx
   8:	74 13                	je     1d <slist_find+0x1d>
   a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  10:	39 71 f8             	cmp    %esi,-0x8(%rcx)
  13:	74 09                	je     1e <slist_find+0x1e>
  15:	48 8b 09             	mov    (%rcx),%rcx
  18:	48 85 c9             	test   %rcx,%rcx
  1b:	75 f3                	jne    10 <slist_find+0x10>
  1d:	c3                   	ret    
  1e:	48 83 c1 f8          	add    $0xfffffffffffffff8,%rcx
  22:	48 89 c8             	mov    %rcx,%rax
  25:	c3                   	ret    
  26:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2d:	00 00 00 

0000000000000030 <slist_find_ranges>:
  30:	48 8b 0f             	mov    (%rdi),%rcx
  33:	31 c0                	xor    %eax,%eax
  35:	48 85 c9             	test   %rcx,%rcx
  38:	74 13                	je     4d <slist_find_ranges+0x1d>
  3a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  40:	39 71 f8             	cmp    %esi,-0x8(%rcx)
  43:	74 09                	je     4e <slist_find_ranges+0x1e>
  45:	48 8b 09             	mov    (%rcx),%rcx
  48:	48 85 c9             	test   %rcx,%rcx
  4b:	75 f3                	jne    40 <slist_find_ranges+0x10>
  4d:	c3                   	ret    
  4e:	48 83 c1 f8          	add    $0xfffffffffffffff8,%rcx
  52:	48 89 c8             	mov    %rcx,%rax
  55:	c3                   	ret    
  56:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  5d:	00 00 00 

0000000000000060 <stailq_find>:
  60:	48 8b 0f             	mov    (%rdi),%rcx
  63:	31 c0                	xor    %eax,%eax
  65:	48 85 c9             	test   %rcx,%rcx
  68:	74 13                	je     7d <stailq_find+0x1d>
  6a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  70:	39 71 f8             	cmp    %esi,-0x8(%rcx)
  73:	74 09                	je     7e <stailq_find+0x1e>
  75:	48 8b 09             	mov    (%rcx),%rcx
  78:	48 85 c9             	test   %rcx,%rcx
  7b:	75 f3                	jne    70 <stailq_find+0x10>
  7d:	c3                   	ret    
  7e:	48 83 c1 f8          	add    $0xfffffffffffffff8,%rcx
  82:	48 89 c8             	mov    %rcx,%rax
  85:	c3                   	ret    
  86:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  8d:	00 00 00 

0000000000000090 <stailq_find_ranges>:
  90:	48 8b 0f             	mov    (%rdi),%rcx
  93:	31 c0                	xor    %eax,%eax
  95:	48 85 c9             	test   %rcx,%rcx
  98:	74 13                	je     ad <stailq_find_ranges+0x1d>
  9a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  a0:	39 71 f8             	cmp    %esi,-0x8(%rcx)
  a3:	74 09                	je     ae <stailq_find_ranges+0x1e>
  a5:	48 8b 09             	mov    (%rcx),%rcx
  a8:	48 85 c9             	test   %rcx,%rcx
  ab:	75 f3                	jne    a0 <stailq_find_ranges+0x10>
  ad:	c3                   	ret    
  ae:	48 83 c1 f8          	add    $0xfffffffffffffff8,%rcx
  b2:	48 89 c8             	mov    %rcx,%rax
  b5:	c3                   	ret    
  b6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  bd:	00 00 00 

00000000000000c0 <tailq_find>:
  c0:	48 8b 0f             	mov    (%rdi),%rcx
  c3:	31 c0                	xor    %eax,%eax
  c5:	48 39 f9             	cmp    %rdi,%rcx
  c8:	74 13                	je     dd <tailq_find+0x1d>
  ca:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  d0:	39 71 f8             	cmp    %esi,-0x8(%rcx)
  d3:	74 09                	je     de <tailq_find+0x1e>
  d5:	48 8b 09             	mov    (%rcx),%rcx
  d8:	48 39 f9             	cmp    %rdi,%rcx
  db:	75 f3                	jne    d0 <tailq_find+0x10>
  dd:	c3                   	ret    
  de:	48 83 c1 f8          	add    $0xfffffffffffffff8,%rcx
  e2:	48 89 c8             	mov    %rcx,%rax
  e5:	c3                   	ret    
  e6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  ed:	00 00 00 

00000000000000f0 <tailq_find_ranges>:
  f0:	48 8b 0f             	mov    (%rdi),%rcx
  f3:	48 39 f9             	cmp    %rdi,%rcx
  f6:	75 08                	jne    100 <tailq_find_ranges+0x10>
  f8:	48 39 f9             	cmp    %rdi,%rcx
  fb:	75 21                	jne    11e <tailq_find_ranges+0x2e>
  fd:	31 c0                	xor    %eax,%eax
  ff:	c3                   	ret    
 100:	31 c0                	xor    %eax,%eax
 102:	66 66 66 66 66 2e 0f 	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
 109:	1f 84 00 00 00 00 00 
 110:	39 71 f8             	cmp    %esi,-0x8(%rcx)
 113:	74 e3                	je     f8 <tailq_find_ranges+0x8>
 115:	48 8b 09             	mov    (%rcx),%rcx
 118:	48 39 f9             	cmp    %rdi,%rcx
 11b:	75 f3                	jne    110 <tailq_find_ranges+0x20>
 11d:	c3                   	ret    
 11e:	48 83 c1 f8          	add    $0xfffffffffffffff8,%rcx
 122:	48 89 c8             	mov    %rcx,%rax
 125:	c3                   	ret    
