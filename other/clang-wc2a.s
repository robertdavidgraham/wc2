	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 10, 14
	.section	__TEXT,__literal16,16byte_literals
	.p2align	4               ## -- Begin function parse_chunk
LCPI0_0:
	.quad	1                       ## 0x1
	.quad	1                       ## 0x1
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_parse_chunk
	.p2align	4, 0x90
_parse_chunk:                           ## @parse_chunk
	.cfi_startproc
## %bb.0:
	movl	(%rcx), %r9d
	movdqu	8(%rdx), %xmm0
	testq	%rsi, %rsi
	je	LBB0_8
## %bb.1:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r14
	pushq	%rbx
	.cfi_offset %rbx, -32
	.cfi_offset %r14, -24
	movl	%esi, %r8d
	andl	$1, %r8d
	cmpq	$1, %rsi
	jne	LBB0_3
## %bb.2:
	xorl	%r11d, %r11d
	testq	%r8, %r8
	jne	LBB0_6
	jmp	LBB0_7
LBB0_3:
	subq	%r8, %rsi
	xorl	%r11d, %r11d
	leaq	_my_isspace(%rip), %r10
	movdqa	LCPI0_0(%rip), %xmm1    ## xmm1 = [1,1]
	.p2align	4, 0x90
LBB0_4:                                 ## =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%r11), %eax
	xorl	%ebx, %ebx
	cmpq	$10, %rax
	sete	%bl
	testl	%r9d, %r9d
	setne	%r9b
	cmpb	$0, (%rax,%r10)
	sete	%al
	setne	%r14b
	andb	%r9b, %al
	movzbl	%al, %eax
	movd	%eax, %xmm2
	pinsrb	$8, %ebx, %xmm2
	pand	%xmm1, %xmm2
	paddq	%xmm0, %xmm2
	movzbl	1(%rdi,%r11), %eax
	xorl	%ebx, %ebx
	cmpq	$10, %rax
	movzbl	(%rax,%r10), %r9d
	sete	%bl
	testl	%r9d, %r9d
	sete	%al
	andb	%r14b, %al
	movzbl	%al, %eax
	movd	%eax, %xmm0
	pinsrb	$8, %ebx, %xmm0
	pand	%xmm1, %xmm0
	paddq	%xmm2, %xmm0
	addq	$2, %r11
	cmpq	%r11, %rsi
	jne	LBB0_4
## %bb.5:
	testq	%r8, %r8
	je	LBB0_7
LBB0_6:
	movzbl	(%rdi,%r11), %eax
	xorl	%esi, %esi
	cmpq	$10, %rax
	sete	%sil
	leaq	_my_isspace(%rip), %rdi
	testl	%r9d, %r9d
	movb	(%rax,%rdi), %al
	setne	%dil
	testb	%al, %al
	sete	%bl
	andb	%dil, %bl
	movzbl	%bl, %edi
	movd	%edi, %xmm1
	pinsrb	$8, %esi, %xmm1
	pand	LCPI0_0(%rip), %xmm1
	paddq	%xmm1, %xmm0
	movl	%eax, %r9d
LBB0_7:
	movzbl	%r9b, %r9d
	popq	%rbx
	popq	%r14
	popq	%rbp
LBB0_8:
	movl	%r9d, (%rcx)
	movdqu	%xmm0, 8(%rdx)
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_parse_file             ## -- Begin function parse_file
	.p2align	4, 0x90
_parse_file:                            ## @parse_file
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$40, %rsp
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	movq	%rcx, -56(%rbp)         ## 8-byte Spill
	movl	%edx, %eax
	movq	%rsi, %rdx
	movq	%rdi, %r12
	movl	%eax, -44(%rbp)         ## 4-byte Spill
	movl	%eax, %edi
	movq	%r12, %rsi
	movq	%rdx, -80(%rbp)         ## 8-byte Spill
	callq	_read
	testq	%rax, %rax
	jle	LBB1_1
## %bb.5:
	movl	$1, %r13d
	xorl	%ebx, %ebx
	leaq	_my_isspace(%rip), %r8
	xorl	%r15d, %r15d
	xorl	%r14d, %r14d
	.p2align	4, 0x90
LBB1_10:                                ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB1_13 Depth 2
	movl	%eax, %ecx
	andl	$1, %ecx
	cmpq	$1, %rax
	jne	LBB1_12
## %bb.11:                              ##   in Loop: Header=BB1_10 Depth=1
	xorl	%edx, %edx
	testq	%rcx, %rcx
	jne	LBB1_8
	jmp	LBB1_9
	.p2align	4, 0x90
LBB1_12:                                ##   in Loop: Header=BB1_10 Depth=1
	movq	%rbx, -72(%rbp)         ## 8-byte Spill
	movq	%rax, %r10
	movq	%rcx, -64(%rbp)         ## 8-byte Spill
	subq	%rcx, %r10
	xorl	%edx, %edx
	.p2align	4, 0x90
LBB1_13:                                ##   Parent Loop BB1_10 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	movzbl	(%r12,%rdx), %edi
	xorl	%ecx, %ecx
	cmpq	$10, %rdi
	sete	%cl
	addq	%r14, %rcx
	testl	%r13d, %r13d
	setne	%r9b
	cmpb	$0, (%rdi,%r8)
	sete	%bl
	setne	%r11b
	andb	%r9b, %bl
	movzbl	%bl, %edi
	addq	%r15, %rdi
	movzbl	1(%r12,%rdx), %esi
	xorl	%r14d, %r14d
	cmpq	$10, %rsi
	movzbl	(%rsi,%r8), %r13d
	sete	%r14b
	testl	%r13d, %r13d
	sete	%bl
	addq	%rcx, %r14
	andb	%r11b, %bl
	movzbl	%bl, %r15d
	addq	%rdi, %r15
	addq	$2, %rdx
	cmpq	%rdx, %r10
	jne	LBB1_13
## %bb.6:                               ##   in Loop: Header=BB1_10 Depth=1
	movq	-72(%rbp), %rbx         ## 8-byte Reload
	movq	-64(%rbp), %rcx         ## 8-byte Reload
	testq	%rcx, %rcx
	je	LBB1_9
LBB1_8:                                 ##   in Loop: Header=BB1_10 Depth=1
	movzbl	(%r12,%rdx), %ecx
	xorl	%edx, %edx
	cmpq	$10, %rcx
	sete	%dl
	testl	%r13d, %r13d
	leaq	_my_isspace(%rip), %rsi
	movb	(%rcx,%rsi), %dil
	setne	%sil
	testb	%dil, %dil
	sete	%cl
	andb	%sil, %cl
	movzbl	%cl, %ecx
	addq	%rcx, %r15
	addq	%rdx, %r14
	movl	%edi, %r13d
LBB1_9:                                 ##   in Loop: Header=BB1_10 Depth=1
	movzbl	%r13b, %r13d
	addq	%rax, %rbx
	movl	-44(%rbp), %edi         ## 4-byte Reload
	movq	%r12, %rsi
	movq	-80(%rbp), %rdx         ## 8-byte Reload
	callq	_read
	testq	%rax, %rax
	leaq	_my_isspace(%rip), %r8
	jg	LBB1_10
	jmp	LBB1_2
LBB1_1:
	xorl	%r14d, %r14d
	xorl	%r15d, %r15d
	xorl	%ebx, %ebx
LBB1_2:
	movq	-56(%rbp), %r8          ## 8-byte Reload
	testq	%r8, %r8
	je	LBB1_14
## %bb.3:
	cmpb	$0, (%r8)
	je	LBB1_14
## %bb.4:
	leaq	L_.str(%rip), %rdi
	movq	%r14, %rsi
	movq	%r15, %rdx
	movq	%rbx, %rcx
	xorl	%eax, %eax
	addq	$40, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	jmp	_printf                 ## TAILCALL
LBB1_14:
	leaq	L_.str.1(%rip), %rdi
	movq	%r14, %rsi
	movq	%r15, %rdx
	movq	%rbx, %rcx
	xorl	%eax, %eax
	addq	$40, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	jmp	_printf                 ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.globl	_main                   ## -- Begin function main
	.p2align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	pushq	%rax
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	movq	%rsi, %r12
	movl	%edi, %r15d
	movl	$65536, %edi            ## imm = 0x10000
	callq	_malloc
	testq	%rax, %rax
	je	LBB2_9
## %bb.1:
	movq	%rax, %r14
	cmpl	$1, %r15d
	jne	LBB2_2
## %bb.10:
	leaq	L_.str.2(%rip), %rcx
	movl	$65536, %esi            ## imm = 0x10000
	movq	%r14, %rdi
	xorl	%edx, %edx
	callq	_parse_file
	jmp	LBB2_8
LBB2_2:
	cmpl	$2, %r15d
	jl	LBB2_8
## %bb.3:
	movl	%r15d, %r15d
	movl	$1, %ebx
	jmp	LBB2_4
LBB2_5:                                 ##   in Loop: Header=BB2_4 Depth=1
	movq	(%r12,%rbx,8), %rdi
	callq	_perror
	jmp	LBB2_7
	.p2align	4, 0x90
LBB2_4:                                 ## =>This Inner Loop Header: Depth=1
	movq	(%r12,%rbx,8), %rdi
	xorl	%esi, %esi
	xorl	%eax, %eax
	callq	_open
	cmpl	$-1, %eax
	je	LBB2_5
## %bb.6:                               ##   in Loop: Header=BB2_4 Depth=1
	movl	%eax, %r13d
	movq	8(%r12), %rcx
	movl	$65536, %esi            ## imm = 0x10000
	movq	%r14, %rdi
	movl	%eax, %edx
	callq	_parse_file
	movl	%r13d, %edi
	callq	_close
LBB2_7:                                 ##   in Loop: Header=BB2_4 Depth=1
	incq	%rbx
	cmpq	%rbx, %r15
	jne	LBB2_4
LBB2_8:
	movq	%r14, %rdi
	callq	_free
	xorl	%eax, %eax
	addq	$8, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
LBB2_9:
	callq	_abort
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"%8llu %7llu %7llu %s\n"

L_.str.1:                               ## @.str.1
	.asciz	"%8llu %7llu %7llu\n"

L_.str.2:                               ## @.str.2
	.space	1

	.section	__TEXT,__const
	.p2align	4               ## @my_isspace
_my_isspace:
	.ascii	"\000\000\000\000\000\000\000\000\000\001\001\001\001\001\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001"
	.space	223


.subsections_via_symbols
