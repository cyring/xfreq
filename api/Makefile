CC = cc
CFLAGS = -Wall -x c
KERNEL = `uname -s`
SRC_PATH=.
OBJ_PATH=./obj
INC4x86_PATH=-I .
#
all :		xfreq-api.o xfreq-smbios.o
#
help :
		@echo 'make [all]     : Compile and link files in' $(OBJ_PATH)/
		@echo 'make clean     : Remove object files.'
		@echo 'make clean-all : Remove object sub-directories.'
#
clean :
		@echo 'Cleaning ...'
		@if [ -e $(OBJ_PATH)/xfreq-api.o ]; then rm -v $(OBJ_PATH)/xfreq-api.o; else echo 'Nothing to do in '$(OBJ_PATH); fi
		@if [ -e $(OBJ_PATH)/xfreq-smbios.o ]; then rm -v $(OBJ_PATH)/xfreq-smbios.o; else echo 'Nothing to do in '$(OBJ_PATH); fi
#
clean-all:
		@echo 'Cleaning all ...'
		@if [ -d $(OBJ_PATH) ]; then rm -vr $(OBJ_PATH); else echo 'Nothing to do in '$(OBJ_PATH); fi
#
xfreq-api.o	: xfreq-api.c
		@if [ ! -d $(OBJ_PATH) ]; then mkdir $(OBJ_PATH); fi
		$(CC) -D$(KERNEL) $(CFLAGS) -c xfreq-api.c $(INC4x86_PATH) -o $(OBJ_PATH)/xfreq-api.o
xfreq-smbios.o	: xfreq-smbios.c
		@if [ ! -d $(OBJ_PATH) ]; then mkdir $(OBJ_PATH); fi
		$(CC) -D$(KERNEL) $(CFLAGS) -c xfreq-smbios.c $(INC4x86_PATH) -o $(OBJ_PATH)/xfreq-smbios.o
