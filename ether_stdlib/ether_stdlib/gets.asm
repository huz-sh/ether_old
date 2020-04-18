	section .text
	global gets
gets:
	push rdi
	mov rax, 0
	mov rdx, rsi
	mov rsi, rdi
	mov rdi, 0
	syscall

	pop rdi
	mov byte [rdi+rax-1], 0
	dec rax
	ret
