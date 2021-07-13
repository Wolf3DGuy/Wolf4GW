.386
.MODEL flat

SCREENBWIDE = 80
TEXTUREMID  = 0200000h

.DATA
loopcount       dd      0
pixelcount      dd      0
EXTRN    _centery:DWORD
EXTRN    _ylookup:DWORD
EXTRN   _dc_yl:DWORD
EXTRN   _dc_yh:DWORD
EXTRN   _dc_iscale:DWORD

.CODE
SEGMENT text USE32
        ALIGN   16

;================
;
; DrawPost
;
;================

PROC    DrawPost_
PUBLIC  DrawPost_

        push  ebp

        or    ecx,ecx
        jz    donedraw

        mov   edx,0
        mov   eax,TEXTUREMID
        idiv  ecx
        mov   edx,eax

        mov   [DWORD PTR patch1+2],edx
        shr   edx,1

        mov   ebx,[_centery]
        add   edi,[_ylookup+ebx*4]

        mov   ebp,SCREENBWIDE
        sub   edi,SCREENBWIDE

        cmp   ecx,ebx
        jle   drawloop
        mov   ecx,ebx

        ALIGN 16
drawloop:
        mov   eax,edx
        shr   eax,16
patch1:
        add   edx,12345678h       
;
; write top half
;
        mov   ebx,31
        sub   ebx,eax
        mov   bl,[BYTE PTR esi+ebx]
        mov   [edi],bl

;
; write bottom half
;
        mov   ebx,32
        add   ebx,eax
        mov   bl,[BYTE PTR esi+ebx]
        mov   [edi+ebp],bl

        sub   edi,SCREENBWIDE
        add   ebp,SCREENBWIDE*2

        dec   ecx
        jnz   drawloop

donedraw:
        pop   ebp
        ret

ENDP


;================
;
; DrawMaskedPost
;
;================

PROC   DrawMaskedPost_
PUBLIC   DrawMaskedPost_

        push    ebp

        mov     ebp,[_dc_yl]
        mov     eax,ebp
        add     edi,[_ylookup+eax*4]
        mov     eax,[_dc_yh]
        sub     eax,ebp                         ; pixel count
        mov     [pixelcount],eax                ; save for final pixel
        js      done                            ; nothing to scale
        shr     eax,1                           ; double pixel count
        mov     [loopcount],eax

        mov     ecx,[_dc_iscale]

        mov     eax,[_centery]
        sub     eax,ebp
        imul    ecx
        mov     ebp,TEXTUREMID
        shl     ebx,16
        sub     ebp,ebx
        sub     ebp,eax

        mov     ebx,[_dc_iscale]
        mov     eax,OFFSET patch2+2             ; convice tasm to modify code...
        mov     [eax],ebx
        mov     eax,OFFSET patch3+2             ; convice tasm to modify code...
        mov     [eax],ebx

; ecx,edx       scratch
; esi           virtual source
; edi           moving destination pointer
; ebp           frac

        mov     ecx,ebp                         ; begin calculating first pixel
        add     ebp,ebx                         ; advance frac pointer
        shr     ecx,16                          ; finish calculation for first pixel
        mov     edx,ebp                         ; begin calculating second pixel
        add     ebp,ebx                         ; advance frac pointer
        shr     edx,16                          ; finish calculation for second pixel

        mov     al,[esi+ecx]                    ; get first pixel
        mov     bl,[esi+edx]                    ; get second pixel

        test    [pixelcount],0fffffffeh
        jnz     doubleloop                      ; at least two pixels to map
        jmp     checklast

        ALIGN   16
doubleloop:
        mov     ecx,ebp                         ; begin calculating third pixel
patch2:
        add     ebp,12345678h                   ; advance frac pointer
        mov     [edi],al                        ; write first pixel
        shr     ecx,16                          ; finish calculation for third pixel
        mov     edx,ebp                         ; begin calculating fourth pixel
patch3:
        add     ebp,12345678h                   ; advance frac pointer
        mov     [edi+SCREENBWIDE],bl            ; write second pixel
        shr     edx,16                          ; finish calculation for fourth pixel
        mov     al,[esi+ecx]                    ; get third pixel
        add     edi,SCREENBWIDE*2               ; advance to third pixel destination
        mov     bl,[esi+edx]                    ; get fourth pixel

        dec     [loopcount]                     ; done with loop?
        jnz     doubleloop

; check for final pixel
checklast:
        test    [pixelcount],1
        jz      done
        mov     [edi],al                        ; write final pixel
done:
        pop     ebp
        ret

ENDP

ENDS

END
