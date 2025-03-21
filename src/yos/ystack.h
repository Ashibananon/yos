/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _Y_STACK_H_
#define _Y_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * I don't know exactly about which regs should be saved as the context
 * of a task, so every reg will be saved to stack, even the [__tmp_reg__]!
 *
 * 「context」の切り替えに、どちらのレジスタを保存するのは必須かがよく分かりません。
 * 念のため、すべてのレジスタを保存するようになりました、「__tmp_reg__」さえも！
 */
#define SAVE_REGS_TO_STACK										\
	__asm__ __volatile__ (										\
		"push __tmp_reg__								\n\t"	\
		"in __tmp_reg__, __SREG__						\n\t"	\
		"push __tmp_reg__								\n\t"	\
		"push r0										\n\t"	\
		"push r1										\n\t"	\
		"push r2										\n\t"	\
		"push r3										\n\t"	\
		"push r4										\n\t"	\
		"push r5										\n\t"	\
		"push r6										\n\t"	\
		"push r7										\n\t"	\
		"push r8										\n\t"	\
		"push r9										\n\t"	\
		"push r10										\n\t"	\
		"push r11										\n\t"	\
		"push r12										\n\t"	\
		"push r13										\n\t"	\
		"push r14										\n\t"	\
		"push r15										\n\t"	\
		"push r16										\n\t"	\
		"push r17										\n\t"	\
		"push r18										\n\t"	\
		"push r19										\n\t"	\
		"push r20										\n\t"	\
		"push r21										\n\t"	\
		"push r22										\n\t"	\
		"push r23										\n\t"	\
		"push r24										\n\t"	\
		"push r25										\n\t"	\
		"push r26										\n\t"	\
		"push r27										\n\t"	\
		"push r28										\n\t"	\
		"push r29										\n\t"	\
		"push r30										\n\t"	\
		"push r31										\n\t"	\
	);


#define RESTORE_REGS_FROM_STACK									\
	__asm__ __volatile__ (										\
		"pop r31										\n\t"	\
		"pop r30										\n\t"	\
		"pop r29										\n\t"	\
		"pop r28										\n\t"	\
		"pop r27										\n\t"	\
		"pop r26										\n\t"	\
		"pop r25										\n\t"	\
		"pop r24										\n\t"	\
		"pop r23										\n\t"	\
		"pop r22										\n\t"	\
		"pop r21										\n\t"	\
		"pop r20										\n\t"	\
		"pop r19										\n\t"	\
		"pop r18										\n\t"	\
		"pop r17										\n\t"	\
		"pop r16										\n\t"	\
		"pop r15										\n\t"	\
		"pop r14										\n\t"	\
		"pop r13										\n\t"	\
		"pop r12										\n\t"	\
		"pop r11										\n\t"	\
		"pop r10										\n\t"	\
		"pop r9											\n\t"	\
		"pop r8											\n\t"	\
		"pop r7											\n\t"	\
		"pop r6											\n\t"	\
		"pop r5											\n\t"	\
		"pop r4											\n\t"	\
		"pop r3											\n\t"	\
		"pop r2											\n\t"	\
		"pop r1											\n\t"	\
		"pop r0											\n\t"	\
		"pop __tmp_reg__								\n\t"	\
		"out __SREG__, __tmp_reg__						\n\t"	\
		"pop __tmp_reg__								\n\t"	\
	);


#ifdef __cplusplus
}
#endif
#endif
