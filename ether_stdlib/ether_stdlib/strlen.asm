	section .text
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
