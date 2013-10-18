.text
.align 16
.global __lwt_trampoline
__lwt_trampoline:
        push %ebp
        movl %esp, %ebp
        push $999
        call __lwt_start
        leave
        ret
        /* should never get here */
        movl $0, %eax      /* die */
        movl (%eax), %ebx /* here */
