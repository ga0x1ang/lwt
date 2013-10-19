.text
.align 16
.global __lwt_trampoline
__lwt_trampoline:
        pushl %ebp
        movl %esp, %ebp
        pushl %ecx
        pushl %eax
        jmp __lwt_start_broken
        ;leave
        ;ret
        /* should never get here */
        movl $0, %eax      /* die */
        movl (%eax), %ebx /* here */

.align 16
.global __lwt_start_broken
__lwt_start_broken:
        pushl %ebp
        movl %esp, %ebp
        pushl 8(%ebp)
        call *4(%ebp)
        pushl %eax
        call lwt_die
        jmp __lwt_schedule
