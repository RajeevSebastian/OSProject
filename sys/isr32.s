



.global timer_handler
.global isr32

isr32 :
   cli
   pushq %rax
   pushq %rbx
   pushq %rcx
   pushq %rdx
   pushq %rbp
   pushq %rsi
   pushq %rdi
   callq timer_handler
   popq %rdi
   popq %rsi
   popq %rbp
   popq %rdx
   popq %rcx
   popq %rbx
   popq %rax
   sti
   iretq

