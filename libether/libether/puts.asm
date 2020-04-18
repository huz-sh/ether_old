	section .text
	extern strlen
	global puts
puts:
	push rdi
	call strlen
	pop rdi
	mov rdx, rax
	mov rax, 1
	mov rsi, rdi
	mov rdi, 1
	syscall
	ret
