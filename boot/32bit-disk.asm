[bits 32]
; BX: sector count, CX: drive, EDX: address
; 0x1F0: Data
; 0x1F1: Error/Features
; 0x1F2: Sector Count
; 0x1F3: Sector Number
; 0x1F4: Cyl LOW
; 0x1F5: Cyl HIGH
; 0x1F6: Drive select
; 0x1F7: Status/Command
; 0x3F6: Alt status
disk_op:
    ; push used params onto the stack
    push ax
    push edx
    ; Select drive
    mov dx, 0x1F6
    mov ax, cx
    call portbout
    ; Select Features
    mov dx, 0x1F1
    mov ax, 0x00
    call portbout
    ; Set sector count
    mov dx, 0x1F2
    mov ax, bx
    call portbout
    ; Select start sector
    pop edx
    mov eax, edx
    push edx
    mov dx, 0x1F3
    call portbout
    ; Set cyl low
    pop edx
    mov eax, edx
    push edx
    shr eax, 8
    mov dx, 0x1F4
    call portbout
    ; Set cyl high
    pop edx
    mov eax, edx
    push edx
    shr eax, 16
    mov dx, 0x1F4
    call portbout
    ; pop used params off the stack
    pop edx
    pop ax
    jmp select_read
    handled:
    pop ebx
    pop ecx
    ;call breakpoint
    ; return to caller
    ;jmp breakpoint
    jmp die


drq_set:
    call portbin
    call portbin
    call portbin
    call portbin
    call portbin
    and al, 0x01
    cmp al, 0x01
    ;jne breakpoint
    jne read_data
    push ebx
    mov ebx, ERR_BLOCK
    call print_string_pm
    pop ebx
    jmp handled

no_drq:
    call portbin
    call portbin
    call portbin
    call portbin
    call portbin
    call portbin
    call portbin
    call portbin
    call portbin
    and al, 0x01
    cmp al, 0x01
    je error_both
    push ebx
    mov ebx, ERR_CYCLE
    call print_string_pm
    pop ebx
    jmp handled

error_both:
    push ebx
    mov ebx, ERR_BOTH
    call print_string_pm
    pop ebx
    jmp handled

read_data:
    push ecx
    push ebx
    mov eax, 0
    push eax
    mov ecx, 0
    mov ebx, 0
    ;jmp breakpoint
    return_label:


read_loop:
    push ebx
    mov dx, 0x1F0
    call portwin
    pop ebx
    mov [0xEFFFFF + ebx], ax
    add ecx, 2
    add ebx, 2
    cmp ecx, 512
    je inc_sector_count
    jne read_loop


inc_sector_count:
    mov ecx, 0
    pop eax
    inc eax
    mov ecx, ebx
    pop ebx
    cmp bx, ax
    je handled
    push ebx
    mov ebx, ecx
    mov ecx, 0
    push eax
    jmp read_loop

select_read:
    push ax
    push dx
    mov dx, 0x1F7
    mov ax, 0x20
    call portbout
    pop dx
    pop ax
    mov ecx, 0
    mov dx, 0x3F6
    jmp check_drq

check_drq:
    call portbin ; reading ports for delay
    call portbin
    call portbin
    call portbin
    call portbin
    and al, 0x88
    cmp al, 0x08
    je drq_set
    inc ecx
    cmp ecx, 1000
    jne check_drq
    je no_drq

WRITE db "Kill the kernel lol     ", 0
READ db "Read selected      ", 0
ERR_CYCLE db "Bad Cycle!",0
ERR_BLOCK db "Bad block!",0
ERR_BOTH db "Bad Block! Bad Cycle!",0