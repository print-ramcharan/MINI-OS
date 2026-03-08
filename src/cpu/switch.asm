global context_switch

; void context_switch(uint32_t* old_esp, uint32_t new_esp)
; old_esp -> [esp + 4]
; new_esp -> [esp + 8]

context_switch:
    ; Push callee-saved registers of old process
    push ebp
    push ebx
    push esi
    push edi

    ; Save the old ESP into *old_esp
    mov eax, [esp + 20]     ; old_esp (20 = 4 regs * 4 + 4 bytes return addr)
    mov [eax], esp

    ; Load the new ESP from new_esp
    mov esp, [esp + 24]     ; new_esp

    ; Pop callee-saved registers of new process
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret
