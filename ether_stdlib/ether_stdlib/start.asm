	section .text
	extern main
	extern exit
	global _start
_start:
	call main

	mov rdi, 0
	jmp exit
