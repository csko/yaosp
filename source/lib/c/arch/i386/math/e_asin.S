/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

.global asin

/* asin = atan (x / sqrt(1 - x^2)) */

.type asin, @function
asin:
	fldl	4(%esp)			/* x */
	fld	%st
	fmul	%st(0)			/* x^2 */
	fld1
	fsubp				/* 1 - x^2 */
	fsqrt				/* sqrt (1 - x^2) */
	fpatan
	ret
.size asin,.-asin
