#!/usr/bin/make

default:
	@echo Use the appropriate platform specific Makefile: Makefile.STM32FX.

clean:
	$(MAKE) -f Makefile.STM32FX clean

.FORCE:

