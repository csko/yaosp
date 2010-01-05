/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Fixed errno handling by Ulrich Drepper <drepper@redhat.com>.
 * Public domain.
 */

.global tan

.type tan, @function
tan:
	fldl	4(%esp)
	fxam
	fstsw	%ax
	movb	$0x45, %dh
	andb	%ah, %dh
	cmpb	$0x05, %dh
	je	3f
4:	fptan
	fnstsw	%ax
	testl	$0x400,%eax
	jnz	1f
	fstp	%st(0)
	ret
1:	fldpi
	fadd	%st(0)
	fxch	%st(1)
2:	fprem1
	fstsw	%ax
	testl	$0x400,%eax
	jnz	2b
	fstp	%st(1)
	fptan
	fstp	%st(0)
	ret
3:
    /* todo: set EDOM to errno */
	jmp	4b
.size tan,.-tan
