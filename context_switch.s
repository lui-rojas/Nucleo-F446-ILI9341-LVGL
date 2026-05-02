.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.extern TaskSheduler
.extern currentTCB
.extern tasks

.section .text

/* --- PendSV: Der eigentliche Task-Umschalter --- */
.global PendSV_Handler
.thumb_func
PendSV_Handler:
    CPSID   I
    MRS     R0, PSP
    STMDB   R0!, {R4-R11}    /* Aktuellen Task-Kontext sichern */

    LDR     R1, =currentTCB
    LDR     R2, [R1]         /* R2 = Aktueller TCB-Zeiger */
    STR     R0, [R2]         /* SP im TCB speichern */

    PUSH    {LR}
    BL      TaskSheduler
    POP     {LR}

    LDR     R1, =currentTCB
    LDR     R1, [R1]
    LDR     R0, [R1]         /* Neuen SP laden */
    
    LDMIA   R0!, {R4-R11}    /* Software-Frame wiederherstellen */
    MSR     PSP, R0          /* PSP aktualisieren */
    CPSIE   I
    BX      LR               /* Hardware poppt den Rest */

/* --- Start_OS: Der "Booster" für die erste Task --- */
.global Start_OS
.thumb_func
Start_OS:
    LDR     R0, =currentTCB
    LDR     R0, [R0]
    LDR     R0, [R0]        /* R0 = Stack-Pointer (zeigt auf R4) */

    /* 1. PSP auf den Anfang des Hardware-Frames setzen (R4-R11 überspringen) */
    ADDS    R1, R0, #32
    MSR     PSP, R1

    /* 2. Auf Prozess-Stack umschalten */
    MOV     R1, #2
    MSR     CONTROL, R1
    ISB

    /* 3. Adresse von TaskA direkt laden und anspringen */
    LDR     R1, [R0, #56]   /* PC liegt bei R0 + 56 Bytes */
    BX      R1