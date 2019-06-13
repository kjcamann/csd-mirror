Disassembly of section .text:

0000000000000000 <slist_find>:
   0:	48 8b 0f             	mov    (%rdi),%rcx
   3:	31 c0                	xor    %eax,%eax
   5:	48 85 c9             	test   %rcx,%rcx
   8:	74 24                	je     2e <slist_find+0x2e>
   a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  10:	48 89 ca             	mov    %rcx,%rdx
  13:	48 83 e2 fe          	and    $0xfffffffffffffffe,%rdx
  17:	39 32                	cmp    %esi,(%rdx)
  19:	74 14                	je     2f <slist_find+0x2f>
  1b:	48 8d 7a 08          	lea    0x8(%rdx),%rdi
  1f:	f6 c1 01             	test   $0x1,%cl
  22:	48 0f 44 fa          	cmove  %rdx,%rdi
  26:	48 8b 0f             	mov    (%rdi),%rcx
  29:	48 85 c9             	test   %rcx,%rcx
  2c:	75 e2                	jne    10 <slist_find+0x10>
  2e:	c3                   	ret    
  2f:	48 89 d0             	mov    %rdx,%rax
  32:	c3                   	ret    
  33:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
  3a:	84 00 00 00 00 00 

0000000000000040 <slist_find_ranges>:
  40:	48 8b 0f             	mov    (%rdi),%rcx
  43:	31 c0                	xor    %eax,%eax
  45:	48 85 c9             	test   %rcx,%rcx
  48:	74 24                	je     6e <slist_find_ranges+0x2e>
  4a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  50:	48 89 ca             	mov    %rcx,%rdx
  53:	48 83 e2 fe          	and    $0xfffffffffffffffe,%rdx
  57:	39 32                	cmp    %esi,(%rdx)
  59:	74 14                	je     6f <slist_find_ranges+0x2f>
  5b:	48 8d 7a 08          	lea    0x8(%rdx),%rdi
  5f:	f6 c1 01             	test   $0x1,%cl
  62:	48 0f 44 fa          	cmove  %rdx,%rdi
  66:	48 8b 0f             	mov    (%rdi),%rcx
  69:	48 85 c9             	test   %rcx,%rcx
  6c:	75 e2                	jne    50 <slist_find_ranges+0x10>
  6e:	c3                   	ret    
  6f:	48 89 d0             	mov    %rdx,%rax
  72:	c3                   	ret    
  73:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
  7a:	84 00 00 00 00 00 

0000000000000080 <stailq_find>:
  80:	48 8b 0f             	mov    (%rdi),%rcx
  83:	31 c0                	xor    %eax,%eax
  85:	48 85 c9             	test   %rcx,%rcx
  88:	74 24                	je     ae <stailq_find+0x2e>
  8a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  90:	48 89 ca             	mov    %rcx,%rdx
  93:	48 83 e2 fe          	and    $0xfffffffffffffffe,%rdx
  97:	39 32                	cmp    %esi,(%rdx)
  99:	74 14                	je     af <stailq_find+0x2f>
  9b:	48 8d 7a 08          	lea    0x8(%rdx),%rdi
  9f:	f6 c1 01             	test   $0x1,%cl
  a2:	48 0f 44 fa          	cmove  %rdx,%rdi
  a6:	48 8b 0f             	mov    (%rdi),%rcx
  a9:	48 85 c9             	test   %rcx,%rcx
  ac:	75 e2                	jne    90 <stailq_find+0x10>
  ae:	c3                   	ret    
  af:	48 89 d0             	mov    %rdx,%rax
  b2:	c3                   	ret    
  b3:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
  ba:	84 00 00 00 00 00 

00000000000000c0 <stailq_find_ranges>:
  c0:	48 8b 0f             	mov    (%rdi),%rcx
  c3:	31 c0                	xor    %eax,%eax
  c5:	48 85 c9             	test   %rcx,%rcx
  c8:	74 24                	je     ee <stailq_find_ranges+0x2e>
  ca:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  d0:	48 89 ca             	mov    %rcx,%rdx
  d3:	48 83 e2 fe          	and    $0xfffffffffffffffe,%rdx
  d7:	39 32                	cmp    %esi,(%rdx)
  d9:	74 14                	je     ef <stailq_find_ranges+0x2f>
  db:	48 8d 7a 08          	lea    0x8(%rdx),%rdi
  df:	f6 c1 01             	test   $0x1,%cl
  e2:	48 0f 44 fa          	cmove  %rdx,%rdi
  e6:	48 8b 0f             	mov    (%rdi),%rcx
  e9:	48 85 c9             	test   %rcx,%rcx
  ec:	75 e2                	jne    d0 <stailq_find_ranges+0x10>
  ee:	c3                   	ret    
  ef:	48 89 d0             	mov    %rdx,%rax
  f2:	c3                   	ret    
  f3:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
  fa:	84 00 00 00 00 00 

0000000000000100 <tailq_find>:
 100:	48 8b 0f             	mov    (%rdi),%rcx
 103:	31 c0                	xor    %eax,%eax
 105:	48 39 f9             	cmp    %rdi,%rcx
 108:	74 24                	je     12e <tailq_find+0x2e>
 10a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
 110:	48 89 ca             	mov    %rcx,%rdx
 113:	48 83 e2 fe          	and    $0xfffffffffffffffe,%rdx
 117:	39 32                	cmp    %esi,(%rdx)
 119:	74 14                	je     12f <tailq_find+0x2f>
 11b:	4c 8d 42 08          	lea    0x8(%rdx),%r8
 11f:	f6 c1 01             	test   $0x1,%cl
 122:	4c 0f 44 c2          	cmove  %rdx,%r8
 126:	49 8b 08             	mov    (%r8),%rcx
 129:	48 39 f9             	cmp    %rdi,%rcx
 12c:	75 e2                	jne    110 <tailq_find+0x10>
 12e:	c3                   	ret    
 12f:	48 89 d0             	mov    %rdx,%rax
 132:	c3                   	ret    
 133:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
 13a:	84 00 00 00 00 00 

0000000000000140 <tailq_find_ranges>:
 140:	4c 8b 07             	mov    (%rdi),%r8
 143:	49 39 f8             	cmp    %rdi,%r8
 146:	75 08                	jne    150 <tailq_find_ranges+0x10>
 148:	49 39 f8             	cmp    %rdi,%r8
 14b:	75 33                	jne    180 <tailq_find_ranges+0x40>
 14d:	31 c0                	xor    %eax,%eax
 14f:	c3                   	ret    
 150:	31 c0                	xor    %eax,%eax
 152:	66 66 66 66 66 2e 0f 	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
 159:	1f 84 00 00 00 00 00 
 160:	4c 89 c2             	mov    %r8,%rdx
 163:	48 83 e2 fe          	and    $0xfffffffffffffffe,%rdx
 167:	39 32                	cmp    %esi,(%rdx)
 169:	74 dd                	je     148 <tailq_find_ranges+0x8>
 16b:	48 8d 4a 08          	lea    0x8(%rdx),%rcx
 16f:	41 f6 c0 01          	test   $0x1,%r8b
 173:	48 0f 44 ca          	cmove  %rdx,%rcx
 177:	4c 8b 01             	mov    (%rcx),%r8
 17a:	49 39 f8             	cmp    %rdi,%r8
 17d:	75 e1                	jne    160 <tailq_find_ranges+0x20>
 17f:	c3                   	ret    
 180:	49 83 e0 fe          	and    $0xfffffffffffffffe,%r8
 184:	4c 89 c0             	mov    %r8,%rax
 187:	c3                   	ret    
