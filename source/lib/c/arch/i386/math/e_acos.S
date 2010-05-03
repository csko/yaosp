/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

.global acos

/* acos = atan (sqrt(1 - x^2) / x) */

.type acos, @function
acos:
	fldl	4(%esp)			/* x */
	fld	%st			/* x : x */
	fmul	%st(0)			/* x^2 : x */
	fld1				/* 1 : x^2 : x */
	fsubp				/* 1 - x^2 : x */
	fsqrt				/* sqrt (1 - x^2) : x */
	fxch	%st(1)			/* x : sqrt (1 - x^2) */
	fpatan				/* atan (sqrt(1 - x^2) / x) */
	ret
.size acos,.-acos
