/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

.global sqrt

.type sqrt, @function
sqrt:
	fldl	4(%esp)
	fsqrt
	ret
.size sqrt,.-sqrt
