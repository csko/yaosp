.section .text

.align 4

.global fabs

.type fabs, @function
fabs:
	fldl	4(%esp)
	fabs
	ret
.size fabs,.-fabs
