.386                            ; habilita instrucciones y registros de 32 bits
.model flat, c                  ; memoria plana (Windows 32 bits) y convención de llamada de C (la que usa C++)

.code                           ; inicio del bloque de código

                                ; Recibe dos direcciones: el texto y el arreglo de resultados.
                                ; USES guarda esi y edi al entrar y los restaura al salir (para no dañar el código de C++)
contar PROC USES esi edi, texto:PTR BYTE, resultados:PTR DWORD
    mov esi, texto              ; esi = dedo sobre el texto; son punteros, no valores
    mov edi, resultados         ; edi = dirección del arreglo de resultados

    mov DWORD PTR [edi],    0   ; letras; contadores inicializados en 0
    mov DWORD PTR [edi+4],  0   ; números (cada int ocupa 4 bytes, por eso +4)
    mov DWORD PTR [edi+8],  0   ; espacios
    mov DWORD PTR [edi+12], 0   ; especiales

siguiente:                      ; etiqueta de inicio del ciclo
    mov al, [esi]               ; carga en al el byte actual (el que señala esi)
    cmp al, 0                   ; compara con 0 (byte nulo, final del texto)
    je fin                      ; si es igual, terminar el recorrido

    cmp al, 32                  ; compara con 32 (código ASCII del espacio)
    je esEspacio                ; si es igual, es un espacio

    cmp al, '0'                 ; compara con '0' (ASCII 48)
    jb esEspecial               ; si es menor y no era espacio, es especial

    cmp al, '9'                 ; compara con '9' (ASCII 57)
    jbe esNumero                ; si es menor o igual, está entre '0' y '9': número

    cmp al, 'A'                 ; compara con 'A' (ASCII 65)
    jb esEspecial               ; entre '9' y 'A' hay : ; < = > ? @ → especial

    cmp al, 'Z'                 ; compara con 'Z' (ASCII 90)
    jbe esLetra                 ; entre 'A' y 'Z': letra mayúscula

    cmp al, 'a'                 ; compara con 'a' (ASCII 97)
    jb esEspecial               ; entre 'Z' y 'a' hay [ \ ] ^ _ ` → especial

    cmp al, 'z'                 ; compara con 'z' (ASCII 122)
    jbe esLetra                 ; entre 'a' y 'z': letra minúscula

    jmp esEspecial              ; mayor que 'z' ({ | } ~): especial.
                                ; El jmp evita caer por error en esLetra

esLetra:
    inc DWORD PTR [edi]         ; suma 1 al contador de letras (r[0])
    jmp avanzar

esNumero:
    inc DWORD PTR [edi+4]       ; suma 1 al contador de números (r[1])
    jmp avanzar

esEspacio:
    inc DWORD PTR [edi+8]       ; suma 1 al contador de espacios (r[2])
    jmp avanzar

esEspecial:
    inc DWORD PTR [edi+12]      ; suma 1 al contador de especiales (r[3])
                                ; no necesita jmp: justo abajo sigue avanzar

avanzar:
    inc esi                     ; el dedo pasa al siguiente carácter (1 byte)
    jmp siguiente               ; vuelve al inicio del ciclo

fin:
    mov eax, [edi]              ; eax = letras
    add eax, [edi+4]            ; + números
    add eax, [edi+8]            ; + espacios
    add eax, [edi+12]           ; + especiales
    mov [edi+16], eax           ; guarda el total en la quinta casilla (r[4])
    ret                         ; regresa a C++ (antes se restauran esi y edi)
contar ENDP                     ; fin de la función
END                             ; fin del archivo