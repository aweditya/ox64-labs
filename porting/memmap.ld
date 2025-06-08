ENTRY(_start)

MEMORY
{
  OCRAM (rwx) : ORIGIN = 0x22020000, LENGTH = 64K
  WRAM (rwx) : ORIGIN = 0x22030000, LENGTH = 160K
  DRAM (rwx) : ORIGIN = 0x3ef80000, LENGTH = 16K
  VRAM (rwx) : ORIGIN = 0x3f000000, LENGTH = 16K
  XRAM (rwx) : ORIGIN = 0x40000000, LENGTH = 16K

  PSRAM (rwx) : ORIGIN = 0x50000000, LENGTH = 64M

  FLASH (rx) : ORIGIN = 0x58000000, LENGTH = 64M

  ROM (rx) : ORIGIN = 0x90000000, LENGTH = 128K
}

SECTIONS
{
    . = ORIGIN(PSRAM);
    __stack_size = 1M;

  /*
    Setting up mappings for PSRAM 
    Each page table stores 512 8-byte entries
    Level-1 can map 1GB pages so we just need
    one entry from that to map the entire PSRAM
    We still allocate a 512 entry table for ease
  */
    __pg1_size = 4K;

  /*
    The single entry in the level 1 page table
    will take us to a single level 2 page table
    that stores the mappings for 512 2MB pages
    (we only need to populate 32 of these)
  */
    __pg2_size = 4K;

  /*
    Each of the 32 possible entries in level 2
    can point to a level 3 page table so we
    collectively allocate 512*512*8 bytes
    (overestimating the 32 by 512)
  */
    __pg3_size = 2M;

    .text : {
        _kcode_start = .;
        KEEP(*(.text.boot))  
        *(.text*) 
        _kcode_end = .;
        . = ALIGN(8);
    } > PSRAM

    .rodata : {
        . = ALIGN(8);
        _krodata_start = .;
        *(.rodata*)
        *(.srodata*)
        . = ALIGN(8);
        _krodata_end = .;
    } > PSRAM

    .data : { 
        . = ALIGN(8);
        _kdata_start = .;
        _kdata_start_load = LOADADDR(.data);
        __global_pointer$ = . + 0x800;
        *(.sdata*)
        *(.data*)
        . = ALIGN(8);
        _kdata_end = .;
    } > PSRAM

    .bss : {
        . = ALIGN(8);
        _kbss_start = .;
        *(.sbss*)
        *(.bss*)
        *(COMMON)
        . = ALIGN(8);
        _kbss_end = .;
    } > PSRAM

    .pg1 : {
      . = ALIGN(0x1000);
      *(.pg1)
      . += __pg1_size;
      . = ALIGN(0x1000);
    } > PSRAM

    .pg2 : {
      . = ALIGN(0x1000);
      *(.pg2)
      . += __pg2_size;
      . = ALIGN(0x1000);
    } > PSRAM

    .pg3 : {
      . = ALIGN(0x1000);
      *(.pg3)
      . += __pg3_size;
      . = ALIGN(0x1000);
    } > PSRAM

    .stack : {
      __stack_bottom__ = .;
      . += __stack_size;
      __stack_top__ = .;
    } > PSRAM

    /DISCARD/ : {
      *(.comment)
      *(.riscv.attributes)
      *(.note)
      *(.eh_frame)
    }
}
