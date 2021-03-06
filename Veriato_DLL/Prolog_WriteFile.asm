Prolog_Fun PROTO

EXTERNDEF hFile_Param1: QWORD
EXTERNDEF lpBuffer_Param2: QWORD
EXTERNDEF nNumOfBytesToWrite_Param3: QWORD
EXTERNDEF Fixup_Prolog_ASM_Return: QWORD

.CODE

Prolog_WriteFile proc
	
	; Break Point For Debugging
	;INT 3
	
	; Setup Stack Frame
	PUSH RBX
	SUB RSP, 30h
	XOR EBX, EBX
	
	; Grab Parameters
	MOV hFile_Param1, RCX
	MOV lpBuffer_Param2, RDX
	MOV nNumOfBytesToWrite_Param3, R8

	; Save Flags
	PUSHFQ

	; Save Registers
	PUSH RAX
	PUSH RCX
	PUSH RDX
	PUSH RBX
	PUSH RBP
	PUSH RSI
	PUSH RDI
	
	PUSH R8
	PUSH R9
	PUSH R10
	PUSH R11
	PUSH R12
	PUSH R13
	PUSH R14
	PUSH R15
	
	CALL Prolog_Fun

	; Restore Registers
	POP R15
	POP R14
	POP R13
	POP R12
	POP R11
	POP R10
	POP R9
	POP R8
	
	POP RDI
	POP RSI
	POP RBP	
	POP RBX
	POP RDX
	POP RCX
	POP RAX
	
	; Restore Flags
	POPFQ
	
	; Reset Stack Frame
	ADD RSP, 30h
	POP RBX

	; Jmp Home	
	JMP [Fixup_Prolog_ASM_Return]	

Prolog_WriteFile endp

end