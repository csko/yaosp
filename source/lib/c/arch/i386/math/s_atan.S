/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

.global atan

.type atan, @function
atan:
	fldl	4(%esp)
	fld1
	fpatan
	ret
.size atan,.-atan
