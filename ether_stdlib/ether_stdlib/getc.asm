	section .text
	global getc
getc:
	sub rsp, 8
	mov rax, 0
	mov rdi, 0
	mov rsi, rsp
	mov rdx, 1
	syscall
	mov rax, [rsp]
	push rax
	
	mov rax, 0
	mov rdi, 0
	lea rsi, [rsp+8]
	mov rdx, 1
	syscall

	pop rax
	add rsp, 8
	ret
