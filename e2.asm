%include "io.mac"

%define BUF_SIZE 256

.DATA
    out_file_name       db  "input.txt",0
    in_file_prompt      db  "Enter the input file name:",0
    out_file_prompt     db  "Enter the output file name:",0
    in_file_err_msg     db  "Input file error:",0
    out_file_err_msg    db  "Cannot create output file",0
    endl                db  10

.UDATA
    strng   resb    4
    fd_in   resd    1
    fd_out  resd    1
    in_buf  resb BUF_SIZE
    n       resd    1
    m       resd    1
    i       resd    1

.CODE

    .STARTUP
        GetStr  strng,6
        mov EAX, -1
        mov [i], EAX

        create_file:
            mov EAX,8
            mov EBX,out_file_name
            mov ECX,0o700
            int 0x80
            mov [fd_out],EAX
            cmp EAX,0
            jge write_n
            PutStr out_file_err_msg
            jmp close_exit

        write_n:            ;first write n onto file
            mov EBP,strng
            mov EAX, 0
            mov [n], EAX    ;set n to 0

        transfer_n_to_file:
            mov AL,[EBP]
            cmp AL,0
            je print_newline

            mov EAX,[n]
                            ;PutLInt [n]
                            ;nwln
            imul EAX,10
            mov [n],EAX
                            ;PutLInt [n]
                            ;nwln

            mov EAX,4
            mov EBX,[fd_out]
            mov ECX,EBP
            mov EDX,1
            int 0x80

            mov AL,[EBP]
            sub AL,48
            add [n],AL

            inc EBP
            jmp transfer_n_to_file

        repeat_read_and_write:
            mov EAX,[i]
            cmp EAX,[n]
            jge done

            GetStr strng,6
            jmp staart_transfer

            mov EAX,[i]
            inc EAX
            mov [i],EAX
            jmp repeat_read_and_write

        staart_transfer:
            mov EBP,strng
            jmp transfer_data;

        transfer_data:
            mov AL,[EBP]
            cmp AL,0
            je print_newline

            mov EAX,4
            mov EBX,[fd_out]
            mov ECX,EBP
            mov EDX,1
            int 0x80
            inc EBP
            jmp transfer_data

        print_newline:
                            ;PutLInt [n]
                            ;nwln
            mov EAX,4
            mov EBX,[fd_out]
            mov ECX,endl
            mov EDX,1
            int 0x80
            inc EBP
            mov EAX,[i]
            inc EAX
            mov [i],EAX
                            ;PutLInt [i]
                            ;nwln
            jmp repeat_read_and_write

        close_exit:
            mov EAX,6
            mov EBX,[fd_in]

        done:
.EXIT
