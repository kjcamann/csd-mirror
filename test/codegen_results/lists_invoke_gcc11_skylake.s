Disassembly of section .text:

0000000000000000 <slist_find>:
   0:	48 89 f8             	mov    %rdi,%rax
   3:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
   7:	48 8d 50 08          	lea    0x8(%rax),%rdx
   b:	83 e7 01             	and    $0x1,%edi
   e:	48 0f 45 c2          	cmovne %rdx,%rax
  12:	48 8b 10             	mov    (%rax),%rdx
  15:	31 c0                	xor    %eax,%eax
  17:	48 85 d2             	test   %rdx,%rdx
  1a:	74 20                	je     3c <slist_find+0x3c>
  1c:	48 89 d0             	mov    %rdx,%rax
  1f:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
  23:	39 30                	cmp    %esi,(%rax)
  25:	74 19                	je     40 <slist_find+0x40>
  27:	83 e2 01             	and    $0x1,%edx
  2a:	48 8d 48 08          	lea    0x8(%rax),%rcx
  2e:	48 0f 45 c1          	cmovne %rcx,%rax
  32:	48 8b 10             	mov    (%rax),%rdx
  35:	48 85 d2             	test   %rdx,%rdx
  38:	75 e2                	jne    1c <slist_find+0x1c>
  3a:	31 c0                	xor    %eax,%eax
  3c:	c3                   	ret    
  3d:	0f 1f 00             	nopl   (%rax)
  40:	c3                   	ret    
  41:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  48:	00 00 00 00 
  4c:	0f 1f 40 00          	nopl   0x0(%rax)

0000000000000050 <slist_find_ranges>:
  50:	48 89 f8             	mov    %rdi,%rax
  53:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
  57:	48 8d 50 08          	lea    0x8(%rax),%rdx
  5b:	83 e7 01             	and    $0x1,%edi
  5e:	48 0f 45 c2          	cmovne %rdx,%rax
  62:	48 8b 10             	mov    (%rax),%rdx
  65:	31 c0                	xor    %eax,%eax
  67:	48 85 d2             	test   %rdx,%rdx
  6a:	74 20                	je     8c <slist_find_ranges+0x3c>
  6c:	48 89 d0             	mov    %rdx,%rax
  6f:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
  73:	39 30                	cmp    %esi,(%rax)
  75:	74 15                	je     8c <slist_find_ranges+0x3c>
  77:	83 e2 01             	and    $0x1,%edx
  7a:	48 8d 48 08          	lea    0x8(%rax),%rcx
  7e:	48 0f 45 c1          	cmovne %rcx,%rax
  82:	48 8b 10             	mov    (%rax),%rdx
  85:	48 85 d2             	test   %rdx,%rdx
  88:	75 e2                	jne    6c <slist_find_ranges+0x1c>
  8a:	31 c0                	xor    %eax,%eax
  8c:	c3                   	ret    
  8d:	0f 1f 00             	nopl   (%rax)

0000000000000090 <stailq_find>:
  90:	48 89 f8             	mov    %rdi,%rax
  93:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
  97:	48 8d 50 08          	lea    0x8(%rax),%rdx
  9b:	83 e7 01             	and    $0x1,%edi
  9e:	48 0f 45 c2          	cmovne %rdx,%rax
  a2:	48 8b 10             	mov    (%rax),%rdx
  a5:	31 c0                	xor    %eax,%eax
  a7:	48 85 d2             	test   %rdx,%rdx
  aa:	74 20                	je     cc <stailq_find+0x3c>
  ac:	48 89 d0             	mov    %rdx,%rax
  af:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
  b3:	39 30                	cmp    %esi,(%rax)
  b5:	74 19                	je     d0 <stailq_find+0x40>
  b7:	83 e2 01             	and    $0x1,%edx
  ba:	48 8d 48 08          	lea    0x8(%rax),%rcx
  be:	48 0f 45 c1          	cmovne %rcx,%rax
  c2:	48 8b 10             	mov    (%rax),%rdx
  c5:	48 85 d2             	test   %rdx,%rdx
  c8:	75 e2                	jne    ac <stailq_find+0x1c>
  ca:	31 c0                	xor    %eax,%eax
  cc:	c3                   	ret    
  cd:	0f 1f 00             	nopl   (%rax)
  d0:	c3                   	ret    
  d1:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  d8:	00 00 00 00 
  dc:	0f 1f 40 00          	nopl   0x0(%rax)

00000000000000e0 <stailq_find_ranges>:
  e0:	48 89 f8             	mov    %rdi,%rax
  e3:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
  e7:	48 8d 50 08          	lea    0x8(%rax),%rdx
  eb:	83 e7 01             	and    $0x1,%edi
  ee:	48 0f 45 c2          	cmovne %rdx,%rax
  f2:	48 8b 10             	mov    (%rax),%rdx
  f5:	31 c0                	xor    %eax,%eax
  f7:	48 85 d2             	test   %rdx,%rdx
  fa:	74 20                	je     11c <stailq_find_ranges+0x3c>
  fc:	48 89 d0             	mov    %rdx,%rax
  ff:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
 103:	39 30                	cmp    %esi,(%rax)
 105:	74 15                	je     11c <stailq_find_ranges+0x3c>
 107:	83 e2 01             	and    $0x1,%edx
 10a:	48 8d 48 08          	lea    0x8(%rax),%rcx
 10e:	48 0f 45 c1          	cmovne %rcx,%rax
 112:	48 8b 10             	mov    (%rax),%rdx
 115:	48 85 d2             	test   %rdx,%rdx
 118:	75 e2                	jne    fc <stailq_find_ranges+0x1c>
 11a:	31 c0                	xor    %eax,%eax
 11c:	c3                   	ret    
 11d:	0f 1f 00             	nopl   (%rax)

0000000000000120 <tailq_find>:
 120:	48 8b 17             	mov    (%rdi),%rdx
 123:	31 c0                	xor    %eax,%eax
 125:	48 39 d7             	cmp    %rdx,%rdi
 128:	74 20                	je     14a <tailq_find+0x2a>
 12a:	48 89 d0             	mov    %rdx,%rax
 12d:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
 131:	39 30                	cmp    %esi,(%rax)
 133:	74 1b                	je     150 <tailq_find+0x30>
 135:	83 e2 01             	and    $0x1,%edx
 138:	48 8d 48 08          	lea    0x8(%rax),%rcx
 13c:	48 0f 45 c1          	cmovne %rcx,%rax
 140:	48 8b 10             	mov    (%rax),%rdx
 143:	48 39 fa             	cmp    %rdi,%rdx
 146:	75 e2                	jne    12a <tailq_find+0xa>
 148:	31 c0                	xor    %eax,%eax
 14a:	c3                   	ret    
 14b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
 150:	c3                   	ret    
 151:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
 158:	00 00 00 00 
 15c:	0f 1f 40 00          	nopl   0x0(%rax)

0000000000000160 <tailq_find_ranges>:
 160:	48 8b 17             	mov    (%rdi),%rdx
 163:	31 c0                	xor    %eax,%eax
 165:	48 39 d7             	cmp    %rdx,%rdi
 168:	74 20                	je     18a <tailq_find_ranges+0x2a>
 16a:	48 89 d0             	mov    %rdx,%rax
 16d:	48 83 e0 fe          	and    $0xfffffffffffffffe,%rax
 171:	39 30                	cmp    %esi,(%rax)
 173:	74 15                	je     18a <tailq_find_ranges+0x2a>
 175:	83 e2 01             	and    $0x1,%edx
 178:	48 8d 48 08          	lea    0x8(%rax),%rcx
 17c:	48 0f 45 c1          	cmovne %rcx,%rax
 180:	48 8b 10             	mov    (%rax),%rdx
 183:	48 39 d7             	cmp    %rdx,%rdi
 186:	75 e2                	jne    16a <tailq_find_ranges+0xa>
 188:	31 c0                	xor    %eax,%eax
 18a:	c3                   	ret    
