%macro gensys 2
    global sys_%2:function

sys_%2:
    push r10
    mov r10, rcx
    mov rax, %1
    syscall
    pop r10
    ret
%endmacro

extern errno

section .text
    gensys   1, write
    gensys  13, rt_sigaction
    gensys  14, rt_sigprocmask
    gensys  34, pause
    gensys  35, nanosleep
    gensys  37, alarm
    gensys  60, exit
    gensys 127, rt_sigpending

global __myrt: function
__myrt:
    mov rax, 15
    syscall
    ret

global sleep:function
sleep:
    sub     rsp, 32         ; allocate timespec * 2
    mov     [rsp], rdi              ; req.tv_sec
    mov     QWORD [rsp+8], 0        ; req.tv_nsec
    mov     rdi, rsp        ; rdi = req @ rsp
    lea     rsi, [rsp+16]   ; rsi = rem @ rsp+16
    call    sys_nanosleep
    cmp     rax, 0
    jge     sleep_quit      ; no error :)
sleep_error:
    neg     rax
    cmp     rax, 4          ; rax == EINTR?
    jne     sleep_failed
sleep_interrupted:
    lea     rsi, [rsp+16]
    mov     rax, [rsi]      ; return rem.tv_sec
    jmp     sleep_quit
sleep_failed:
    mov     rax, 0          ; return 0 on error
sleep_quit:
    add     rsp, 32
    ret

extern sigsetjmp
global setjmp: function
setjmp:
    push rdi

    %ifdef NASM
        call [rel sigsetjmp wrt ..gotpc]
    %else
        call [rel sigsetjmp wrt ..gotpcrel]
    %endif

    pop rdi
    xor rax, rax
    mov [rdi + 8 * 0], rbx
    mov [rdi + 8 * 1], rsp
    mov [rdi + 8 * 2], rbp
    mov [rdi + 8 * 3], r12
    mov [rdi + 8 * 4], r13
    mov [rdi + 8 * 5], r14
    mov [rdi + 8 * 6], r15
    pop QWORD[rdi + 8 * 7]
    push QWORD[rdi + 8 * 7]
    ret

extern siglongjmp
global longjmp: function
longjmp:
    pop rax
    push rdi
    push rsi

    %ifdef NASM
        call [rel siglongjmp wrt ..gotpc]
    %else
        call [rel siglongjmp wrt ..gotpcrel]
    %endif

    pop rsi
    pop rdi
    test rsi, rsi
    je rsi_is_0
    jmp restore_val

rsi_is_0:
    mov rsi, 1

restore_val:
    mov rax, rsi
    mov rbx, QWORD[rdi + 8 * 0]
    mov rsp, QWORD[rdi + 8 * 1]
    mov rbp, QWORD[rdi + 8 * 2]
    mov r12, QWORD[rdi + 8 * 3]
    mov r13, QWORD[rdi + 8 * 4]
    mov r14, QWORD[rdi + 8 * 5]
    mov r15, QWORD[rdi + 8 * 6]
    push QWORD[rdi + 8 * 7]
    ret