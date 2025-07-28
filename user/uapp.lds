OUTPUT_ARCH("riscv")
ENTRY(_start)

PHDRS
{
    text_segment PT_LOAD FLAGS(5) ;
    data_segment PT_LOAD FLAGS(6) ;
}

SECTIONS
{
    . = 0;

    .text : ALIGN(0x1000) {
        *(.text.init)
        *(.text .text.*)
    } :text_segment

    .rodata : ALIGN(0x1000) {
        *(.rodata .rodata*)
    } :text_segment

    .data : ALIGN(0x1000) {
        *(.sdata .sdata.*)
        *(.data .data.*)
    } :data_segment

    .bss : ALIGN(0x1000) {
        *(.sbss .sbss.*)
        *(.bss .bss.*)
    } :data_segment
}
