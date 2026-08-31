	.text
	.file	"sel.ll"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r14
	.cfi_def_cfa_offset 24
	pushq	%rbx
	.cfi_def_cfa_offset 32
	subq	$4096, %rsp                     # imm = 0x1000
	.cfi_def_cfa_offset 4128
	.cfi_offset %rbx, -32
	.cfi_offset %r14, -24
	.cfi_offset %rbp, -16
	movq	%rsi, %r14
	movl	%edi, %ebp
	callq	initscr@PLT
	movq	%rax, %rbx
	callq	noecho@PLT
	callq	cbreak@PLT
	movq	%rbx, %rdi
	movl	$1, %esi
	callq	keypad@PLT
	leaq	.Lflow_string_148(%rip), %rsi
	movq	%rbx, %rdi
	movl	$58, %edx
	callq	waddnstr@PLT
	cmpl	$2, %ebp
	jl	.LBB0_4
# %bb.1:                                # %flow_block_1
	movq	8(%r14), %rsi
	movq	%rbx, %rdi
	movl	$58, %edx
	callq	waddnstr@PLT
	jmp	.LBB0_7
.LBB0_4:                                # %flow_block_2
	movq	%rsp, %r14
	movl	$4096, %edx                     # imm = 0x1000
	movq	%r14, %rdi
	xorl	%esi, %esi
	callq	memset@PLT
	movl	$4095, %edx                     # imm = 0xFFF
	xorl	%edi, %edi
	movq	%r14, %rsi
	callq	read@PLT
	testq	%rax, %rax
	js	.LBB0_2
# %bb.5:                                # %flow_block_4
	je	.LBB0_7
# %bb.6:                                # %flow_block_5
	movq	%rsp, %rsi
	movl	$1, %edi
	movq	%rax, %rdx
	callq	write@PLT
.LBB0_7:                                # %flow_join_0
	movq	%rbx, %rdi
	callq	wrefresh@PLT
	movq	%rbx, %rdi
	callq	wgetch@PLT
	movl	%eax, %ebx
	callq	endwin@PLT
	cmpl	$113, %ebx
	jne	.LBB0_9
# %bb.8:                                # %flow_block_6
	leaq	.Lflow_string_150(%rip), %rdi
	callq	puts@PLT
	movl	$1, %eax
	jmp	.LBB0_3
.LBB0_9:                                # %flow_block_7
	leaq	.Lflow_string_149(%rip), %rdi
	callq	puts@PLT
	xorl	%eax, %eax
	jmp	.LBB0_3
.LBB0_2:                                # %flow_block_3
	callq	endwin@PLT
	movl	$2, %eax
.LBB0_3:                                # %common.ret
	addq	$4096, %rsp                     # imm = 0x1000
	.cfi_def_cfa_offset 32
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	.Lflow_string_148,@object       # @flow_string_148
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
.Lflow_string_148:
	.asciz	"Flowcore sel\n\n> alpha\n  beta\n  gamma\n\nUp/Down: move  Enter: select  q: quit"
	.size	.Lflow_string_148, 76

	.type	.Lflow_string_149,@object       # @flow_string_149
	.section	.rodata.str1.1,"aMS",@progbits,1
.Lflow_string_149:
	.asciz	"selected: alpha"
	.size	.Lflow_string_149, 16

	.type	.Lflow_string_150,@object       # @flow_string_150
.Lflow_string_150:
	.asciz	"selection: none"
	.size	.Lflow_string_150, 16

	.section	".note.GNU-stack","",@progbits
	.addrsig
