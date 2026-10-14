#ifndef _CLIB_SETJMP_H
#define _CLIB_SETJMP_H

/**
 * @file setjmp.h
 * @brief Non-local jumps (setjmp / longjmp) for i386.
 *
 * jmp_buf layout (6 x 32-bit words):
 *   [0] ebx
 *   [1] esi
 *   [2] edi
 *   [3] ebp
 *   [4] esp  (caller's stack pointer, after the call to setjmp)
 *   [5] eip  (return address sitting on the caller's stack)
 */

typedef unsigned int jmp_buf[6];

/**
 * @brief Save the current execution context.
 *
 * Saves the callee-saved registers (ebx, esi, edi, ebp), the caller's
 * stack pointer and the return address into @p env.
 *
 * @param env  Buffer that receives the saved context.
 * @return     0 when called directly; the @p val argument passed to
 *             longjmp() (or 1 if @p val was 0) when returning via longjmp().
 */
int setjmp(jmp_buf env);

/**
 * @brief Restore a previously saved execution context.
 *
 * Restores the context saved by the matching setjmp() call, making
 * setjmp() appear to return @p val (or 1 if @p val is 0).
 *
 * @param env  Buffer previously filled by setjmp().
 * @param val  Value that setjmp() should appear to return (0 becomes 1).
 */
__attribute__((noreturn)) void longjmp(jmp_buf env, int val);

/* _setjmp / _longjmp – aliases that promise not to touch the signal mask.
   In this bare-metal libc there are no signal masks, so they are identical. */
#define _setjmp  setjmp
#define _longjmp longjmp

#endif /* _CLIB_SETJMP_H */
