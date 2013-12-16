.text
.align 16
.global __lwt_trampoline
__lwt_trampoline:
        pushl %edx
        pushl %ecx
        pushl %eax
        jmp __lwt_start_broken

.align 16
.global __lwt_start_broken
__lwt_start_broken:
        pushl %ebp
        movl %esp, %ebp
        pushl 12(%ebp)
        pushl 8(%ebp)
        call *4(%ebp)
        pushl %eax
        jmp lwt_die
