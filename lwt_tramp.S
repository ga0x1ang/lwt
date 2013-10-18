.text
.align 16
.global __lwt_trampoline
__lwt_trampoline:
        pushl %ebp
        movl %esp, %ebp
        pushl %ecx
        pushl %eax /* TODO: ecx better? */
        call __lwt_start
        leave
        ret
        /* should never get here */
        movl $0, %eax      /* die */
        movl (%eax), %ebx /* here */
