# GDB init script for kernel debugging.
# Works with qemu-system-x86_64 (do NOT use i8086 — GDB 17 rejects it here).
#
# Usage:
#   Terminal 1:  make gdb-server
#   Terminal 2:  make gdb-connect
# Or one terminal:  make debug

set pagination off
set confirm off
set disassembly-flavor intel

printf "\n"
printf "========================================\n"
printf "  Kernel GDB — connecting to QEMU :1234\n"
printf "========================================\n"
printf "  c              continue\n"
printf "  si             step one instruction\n"
printf "  show-boot      boot / real-mode state\n"
printf "  show-kernel    kernel memory @ 10000h\n"
printf "  show-vga       VGA buffer @ B8000h\n"
printf "  show-all       all of the above\n"
printf "========================================\n\n"

source debug/addresses.gdb

target remote :1234
set architecture i386:x86-64

python
try:
    gdb.execute('add-symbol-file build/kernel.elf 0x10000')
    gdb.write('Loaded kernel symbols (C + asm) at 0x10000\n')
except gdb.error as e:
    gdb.write('Note: kernel symbols not loaded (%s)\n' % e)
end

define show-boot
  printf "\n--- Boot stage (real mode) ---\n"
  info registers
  printf "\nPC (linear) = $pc\n"
  printf "Instructions at boot sector (0x7C00):\n"
  x/8i 0x7c00
  printf "\nGDT entries near 0x7CD0:\n"
  x/3gx 0x7cd0
  printf "\n"
end

define show-kernel
  printf "\n--- Kernel @ 0x10000 ---\n"
  info registers
  printf "\nPC = $pc\n"
  x/8i $kernel_main
  x/8wx $kernel_main
  printf "\n"
end

define show-vga
  printf "\n--- VGA text @ 0xB8000 ---\n"
  x/80cb 0xb8000
  printf "\n"
end

define show-all
  show-boot
  show-kernel
  show-vga
end

define break-boot
  break *$start
  commands
    silent
    printf "\n>>> Boot start @ 0x7C00\n"
    show-boot
  end
end

define break-pm
  break *$enter_protected_mode
  commands
    silent
    printf "\n>>> PM switch — loading GDT, setting CR0.PE\n"
    show-boot
  end
end

define break-pm-entry
  break *$pm_entry
  commands
    silent
    printf "\n>>> 32-bit PM stub — jumping to kernel\n"
    printf "PC = "
    output $pc
    printf "\n"
    info registers
    x/5i $pc
  end
end

define break-kernel
  break kernel_main
  commands
    silent
    printf "\n>>> Kernel entry (asm) @ kernel_main\n"
    show-kernel
  end
end

define break-kmain
  break kmain
  commands
    silent
    printf "\n>>> C kernel kmain()\n"
    show-kernel
  end
end

define break-exception
  break exception_handler
  commands
    silent
    printf "\n>>> CPU exception — exception_handler()\n"
    printf "Vector (if frame valid): check serial log\n"
  end
end

define break-irq
  break irq_handler
  commands
    silent
    printf "\n>>> Hardware IRQ — irq_handler()\n"
  end
end

printf "Setting breakpoints...\n"
break-boot
break-pm
break-pm-entry
break-kernel
break-kmain
break-exception
break-irq

printf "\nCPU is PAUSED (-S). Nothing prints until you type:  c\n"
printf "  (VGA stays black and kmain serial lines wait until you continue.)\n"
printf "Serial boot log -> QEMU terminal (Terminal 1 if using gdb-server).\n"
printf "Type 'show-all' anytime to inspect memory.\n"
printf "Or:  GDB_AUTO_CONTINUE=1 make gdb-connect  to auto-continue.\n\n"

python
import os
if os.environ.get("GDB_AUTO_CONTINUE") == "1":
    gdb.write("GDB_AUTO_CONTINUE=1 -> continuing...\n")
    gdb.execute("continue")
end
