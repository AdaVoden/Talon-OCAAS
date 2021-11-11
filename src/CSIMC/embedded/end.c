/* see start.c for explanation */

	asm (".area text");
	asm ("__text_end::");
	asm (".area data");
	asm ("__data_end::");
	asm (".area bss");
	asm ("__bss_end::");
