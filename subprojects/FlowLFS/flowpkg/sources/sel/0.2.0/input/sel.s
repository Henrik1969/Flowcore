	.text
	.file	"sel.ll"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	subq	$56, %rsp
	.cfi_def_cfa_offset 64
# %bb.1:                                # %flow_block_0
	movq	$0, 48(%rsp)
	movl	$0, 44(%rsp)
	movl	$0, 40(%rsp)
	movl	$76, 36(%rsp)
	leaq	.Lflow_string_32(%rip), %rax
	movq	%rax, 24(%rsp)
	leaq	.Lflow_string_33(%rip), %rax
	movq	%rax, 16(%rsp)
	leaq	.Lflow_string_34(%rip), %rax
	movq	%rax, 8(%rsp)
	callq	flow_terminal_open@PLT
	movq	%rax, 48(%rsp)
	callq	flow_terminal_enter@PLT
	movl	%eax, 44(%rsp)
	movq	24(%rsp), %rdi
	movl	36(%rsp), %esi
	callq	flow_terminal_write@PLT
	movl	%eax, 44(%rsp)
	callq	flow_terminal_present@PLT
	movl	%eax, 44(%rsp)
	callq	flow_terminal_read_event@PLT
	movl	%eax, 40(%rsp)
	callq	flow_terminal_close@PLT
	movl	%eax, 44(%rsp)
	cmpl	$113, 40(%rsp)
	jne	.LBB0_3
# %bb.2:                                # %flow_block_1
	movq	8(%rsp), %rdi
	callq	puts@PLT
	movl	%eax, 44(%rsp)
	movl	$1, %eax
	addq	$56, %rsp
	.cfi_def_cfa_offset 8
	retq
.LBB0_3:                                # %flow_block_2
	.cfi_def_cfa_offset 64
	movq	16(%rsp), %rdi
	callq	puts@PLT
	movl	%eax, 44(%rsp)
	xorl	%eax, %eax
	addq	$56, %rsp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	.Lflow_string_32,@object        # @flow_string_32
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
.Lflow_string_32:
	.asciz	"Flowcore sel\n\n> alpha\n  beta\n  gamma\n\nUp/Down: move  Enter: select  q: quit\n"
	.size	.Lflow_string_32, 77

	.type	.Lflow_string_33,@object        # @flow_string_33
	.section	.rodata.str1.1,"aMS",@progbits,1
.Lflow_string_33:
	.asciz	"selected: alpha"
	.size	.Lflow_string_33, 16

	.type	.Lflow_string_34,@object        # @flow_string_34
.Lflow_string_34:
	.asciz	"selection: none"
	.size	.Lflow_string_34, 16

	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym puts
	.addrsig_sym flow_terminal_close
	.addrsig_sym flow_terminal_enter
	.addrsig_sym flow_terminal_open
	.addrsig_sym flow_terminal_present
	.addrsig_sym flow_terminal_read_event
	.addrsig_sym flow_terminal_write
