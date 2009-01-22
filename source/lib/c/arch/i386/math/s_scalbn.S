/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

.section .text

.align 4

.global scalbn

.type scalbn, @function
scalbn:
	fildl	12(%esp)
	fldl	4(%esp)
	fscale
	fstp	%st(1)
	ret
.size scalbn,.-scalbn