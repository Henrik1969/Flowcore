	.text
	.file	"sel.ll"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	subq	$120, %rsp
	.cfi_def_cfa_offset 128
# %bb.1:                                # %flow_block_0
	movq	$0, 112(%rsp)
	movl	$0, 108(%rsp)
	movl	$0, 104(%rsp)
	movl	$0, 100(%rsp)
	movl	$0, 96(%rsp)
	movl	$0, 92(%rsp)
	movl	$0, 88(%rsp)
	movl	$1, 84(%rsp)
	movl	$2, 80(%rsp)
	movl	$256, 76(%rsp)                  # imm = 0x100
	movl	$258, 72(%rsp)                  # imm = 0x102
	movl	$259, 68(%rsp)                  # imm = 0x103
	movl	$76, 64(%rsp)
	leaq	.Lflow_string_41(%rip), %rax
	movq	%rax, 56(%rsp)
	leaq	.Lflow_string_42(%rip), %rax
	movq	%rax, 48(%rsp)
	leaq	.Lflow_string_43(%rip), %rax
	movq	%rax, 40(%rsp)
	leaq	.Lflow_string_44(%rip), %rax
	movq	%rax, 32(%rsp)
	leaq	.Lflow_string_45(%rip), %rax
	movq	%rax, 24(%rsp)
	leaq	.Lflow_string_46(%rip), %rax
	movq	%rax, 16(%rsp)
	leaq	.Lflow_string_47(%rip), %rax
	movq	%rax, 8(%rsp)
	leaq	.Lflow_string_48(%rip), %rax
	movq	%rax, (%rsp)
	callq	flow_terminal_open@PLT
	movq	%rax, 112(%rsp)
	callq	flow_terminal_enter@PLT
	movl	%eax, 108(%rsp)
	movq	56(%rsp), %rdi
	movl	64(%rsp), %esi
	callq	flow_terminal_write@PLT
	movl	%eax, 108(%rsp)
	callq	flow_terminal_present@PLT
	movl	%eax, 108(%rsp)
.LBB0_2:                                # %flow_loop_condition_0
                                        # =>This Inner Loop Header: Depth=1
	movl	96(%rsp), %eax
	cmpl	84(%rsp), %eax
	jge	.LBB0_25
# %bb.3:                                # %flow_block_1
                                        #   in Loop: Header=BB0_2 Depth=1
	callq	flow_terminal_read_event@PLT
	movl	%eax, 104(%rsp)
	cmpl	$113, 104(%rsp)
	jne	.LBB0_5
# %bb.4:                                # %flow_block_2
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	84(%rsp), %eax
	movl	%eax, 92(%rsp)
	movl	84(%rsp), %eax
	movl	%eax, 96(%rsp)
	jmp	.LBB0_24
.LBB0_5:                                # %flow_block_3
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	104(%rsp), %eax
	cmpl	76(%rsp), %eax
	jne	.LBB0_7
# %bb.6:                                # %flow_block_4
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	84(%rsp), %eax
	movl	%eax, 96(%rsp)
	jmp	.LBB0_23
.LBB0_7:                                # %flow_block_5
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	104(%rsp), %eax
	cmpl	72(%rsp), %eax
	jne	.LBB0_11
# %bb.8:                                # %flow_block_6
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	100(%rsp), %eax
	cmpl	88(%rsp), %eax
	jle	.LBB0_10
# %bb.9:                                # %flow_block_7
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	100(%rsp), %eax
	subl	84(%rsp), %eax
	movl	%eax, 100(%rsp)
.LBB0_10:                               # %flow_join_5
                                        #   in Loop: Header=BB0_2 Depth=1
	jmp	.LBB0_16
.LBB0_11:                               # %flow_block_8
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	104(%rsp), %eax
	cmpl	68(%rsp), %eax
	jne	.LBB0_15
# %bb.12:                               # %flow_block_9
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	100(%rsp), %eax
	cmpl	80(%rsp), %eax
	jge	.LBB0_14
# %bb.13:                               # %flow_block_10
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	100(%rsp), %eax
	addl	84(%rsp), %eax
	movl	%eax, 100(%rsp)
.LBB0_14:                               # %flow_join_7
                                        #   in Loop: Header=BB0_2 Depth=1
	jmp	.LBB0_15
.LBB0_15:                               # %flow_join_6
                                        #   in Loop: Header=BB0_2 Depth=1
	jmp	.LBB0_16
.LBB0_16:                               # %flow_join_4
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	100(%rsp), %eax
	cmpl	88(%rsp), %eax
	jne	.LBB0_18
# %bb.17:                               # %flow_block_11
                                        #   in Loop: Header=BB0_2 Depth=1
	movq	48(%rsp), %rdi
	movl	$14, %esi
	callq	flow_terminal_write@PLT
	movl	%eax, 108(%rsp)
	jmp	.LBB0_22
.LBB0_18:                               # %flow_block_12
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	100(%rsp), %eax
	cmpl	84(%rsp), %eax
	jne	.LBB0_20
# %bb.19:                               # %flow_block_13
                                        #   in Loop: Header=BB0_2 Depth=1
	movq	40(%rsp), %rdi
	movl	$13, %esi
	callq	flow_terminal_write@PLT
	movl	%eax, 108(%rsp)
	jmp	.LBB0_21
.LBB0_20:                               # %flow_block_14
                                        #   in Loop: Header=BB0_2 Depth=1
	movq	32(%rsp), %rdi
	movl	$14, %esi
	callq	flow_terminal_write@PLT
	movl	%eax, 108(%rsp)
.LBB0_21:                               # %flow_join_9
                                        #   in Loop: Header=BB0_2 Depth=1
	jmp	.LBB0_22
.LBB0_22:                               # %flow_join_8
                                        #   in Loop: Header=BB0_2 Depth=1
	callq	flow_terminal_present@PLT
	movl	%eax, 108(%rsp)
.LBB0_23:                               # %flow_join_3
                                        #   in Loop: Header=BB0_2 Depth=1
	jmp	.LBB0_24
.LBB0_24:                               # %flow_join_2
                                        #   in Loop: Header=BB0_2 Depth=1
	jmp	.LBB0_2
.LBB0_25:                               # %flow_loop_exit_1
	callq	flow_terminal_close@PLT
	movl	%eax, 108(%rsp)
	movl	92(%rsp), %eax
	cmpl	84(%rsp), %eax
	jne	.LBB0_27
# %bb.26:                               # %flow_block_15
	movq	(%rsp), %rdi
	callq	puts@PLT
	movl	%eax, 108(%rsp)
	movl	$1, %eax
	addq	$120, %rsp
	.cfi_def_cfa_offset 8
	retq
.LBB0_27:                               # %flow_block_16
	.cfi_def_cfa_offset 128
	movl	100(%rsp), %eax
	cmpl	88(%rsp), %eax
	jne	.LBB0_29
# %bb.28:                               # %flow_block_17
	movq	24(%rsp), %rdi
	callq	puts@PLT
	movl	%eax, 108(%rsp)
	jmp	.LBB0_33
.LBB0_29:                               # %flow_block_18
	movl	100(%rsp), %eax
	cmpl	84(%rsp), %eax
	jne	.LBB0_31
# %bb.30:                               # %flow_block_19
	movq	16(%rsp), %rdi
	callq	puts@PLT
	movl	%eax, 108(%rsp)
	jmp	.LBB0_32
.LBB0_31:                               # %flow_block_20
	movq	8(%rsp), %rdi
	callq	puts@PLT
	movl	%eax, 108(%rsp)
.LBB0_32:                               # %flow_join_12
	jmp	.LBB0_33
.LBB0_33:                               # %flow_join_11
	xorl	%eax, %eax
	addq	$120, %rsp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	.Lflow_string_41,@object        # @flow_string_41
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
.Lflow_string_41:
	.asciz	"Flowcore sel\n\n> alpha\n  beta\n  gamma\n\nUp/Down: move  Enter: select  q: quit\n"
	.size	.Lflow_string_41, 77

	.type	.Lflow_string_42,@object        # @flow_string_42
	.section	.rodata.str1.1,"aMS",@progbits,1
.Lflow_string_42:
	.asciz	"cursor: alpha\n"
	.size	.Lflow_string_42, 15

	.type	.Lflow_string_43,@object        # @flow_string_43
.Lflow_string_43:
	.asciz	"cursor: beta\n"
	.size	.Lflow_string_43, 14

	.type	.Lflow_string_44,@object        # @flow_string_44
.Lflow_string_44:
	.asciz	"cursor: gamma\n"
	.size	.Lflow_string_44, 15

	.type	.Lflow_string_45,@object        # @flow_string_45
.Lflow_string_45:
	.asciz	"selected: alpha"
	.size	.Lflow_string_45, 16

	.type	.Lflow_string_46,@object        # @flow_string_46
.Lflow_string_46:
	.asciz	"selected: beta"
	.size	.Lflow_string_46, 15

	.type	.Lflow_string_47,@object        # @flow_string_47
.Lflow_string_47:
	.asciz	"selected: gamma"
	.size	.Lflow_string_47, 16

	.type	.Lflow_string_48,@object        # @flow_string_48
.Lflow_string_48:
	.asciz	"selection: none"
	.size	.Lflow_string_48, 16

	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym puts
	.addrsig_sym flow_terminal_close
	.addrsig_sym flow_terminal_enter
	.addrsig_sym flow_terminal_open
	.addrsig_sym flow_terminal_present
	.addrsig_sym flow_terminal_read_event
	.addrsig_sym flow_terminal_write
