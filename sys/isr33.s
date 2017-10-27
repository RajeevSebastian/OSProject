.global keyboard_handler
.global isr33


isr33:
   cli
   pushq %rax
   pushq %rbx
   pushq %rcx
   pushq %rdx
   pushq %rbp
   pushq %rsi
   pushq %rdi
   callq keyboard_handler
   popq %rdi
   popq %rsi
   popq %rbp
   popq %rdx
   popq %rcx
   popq %rbx
   popq %rax
   sti
   iretq

