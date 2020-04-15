global _start
extern main
_start:
	call main

	;; calling convention: rdi rsi rdx rcx r8 r9 <stack>
	mov rdi, 0
	jmp exit

global putsln	
putsln:
	push rdi
	call strlen
	pop rdi
	mov rdx, rax
	mov rax, 4
	mov rbx, 1
	mov rcx, rdi
	int 0x80

	mov rax, 4
	mov rbx, 1
	mov rcx, newline
	mov rdx, 1
	int 0x80
	ret

global strlen
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
	
global exit	
exit:
	mov rax, 1
	mov rbx, rdi
	int 0x80

	
section .data
newline: db 10
