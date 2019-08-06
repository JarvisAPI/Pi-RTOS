.globl spinlock_take
spinlock_take:
    mov	r2, r0			@ shuffle the address over
	ldr	r3, =1			@ we need the "locked" value in reg to store it
_spinlock_take_loop:
	ldrex	r0, [r2]		@ and replace with the exclusive read of it
	cmp	r0, #1			@ test if its locked already, if not:
    strexne	r0, r3, [r2]		@   try to claim the lock, r0 = 0 on success, 1 on fail
	cmpne	r0, #1			@   check for a fail
	beq	_spinlock_take_loop	@ loop until we get it
	bx	lr

.globl spinlock_give
spinlock_give:
	mov	r3, #0			@ we need the "locked" value in reg to store it
    str	r3, [r0]		@   try to claim the lock, r0 = 0 on success, 1 on fail
	bx	lr
