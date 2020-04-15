	;; syscall-convention: rdi, rsi, rdx, r10, r8, r9
	;; call-convention:    rdi, rsi, rdx, rcx, r8, r9 <stack-reverse>

	global _start
	global puts
	global putsln	
	global getc
	global gets
	global strlen
	global exit
	
	extern main

	section .text
_start:
	call main

	mov rdi, 0
	jmp exit

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
	
putsln:
	call puts
	mov rax, 1
	mov rdi, 1
	mov rsi, newline
	mov rdx, 1
	syscall
	ret
	
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
	
strlen:
	push rcx
	xor rcx, rcx

.strlen_loop:
	cmp [rdi], byte 0
	jz .strlen_done

	inc rcx
	inc rdi
	jmp .strlen_loop

.strlen_done:
	mov rax, rcx
	pop rcx
	ret
	
exit:
	mov rax, 60
	syscall
	
	section .data
newline: db 10
