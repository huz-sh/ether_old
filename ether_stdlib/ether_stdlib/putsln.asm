	section .text
	extern puts
	global putsln
putsln:
	call puts
	mov rax, 1
	mov rdi, 1
	mov rsi, newline
	mov rdx, 1
	syscall
	ret
	
	section .data
newline:
	db 10
