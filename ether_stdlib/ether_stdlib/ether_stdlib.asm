	;; syscall-convention: rdi, rsi, rdx, r10, r8, r9
	;; call-convention:    rdi, rsi, rdx, rcx, r8, r9 <stack-reverse>

	section .text
	global malloc
malloc: ; rdi = u64
	cmp rdi, byte 0
	je .malloc_error
	push rdi

	mov rax, 12 ; syscall for brk
	mov rdi, 0
	syscall
	mov [malloc_current_break], rax
	mov [malloc_init_break], rax

	mov rax, 12
	mov rdi, [malloc_current_break]
	pop rcx ; u64 size
	add rdi, rcx
	syscall
	mov [malloc_new_break], rax
	mov [malloc_current_break], rax
	ret	
.malloc_error:
	mov rax, 0
	ret

	section .bss	
malloc_init_break:
	resq 1
malloc_current_break:
	resq 1
malloc_new_break:
	resq 1
