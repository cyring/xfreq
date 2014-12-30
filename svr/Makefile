CC = cc
CFLAGS = -Wall -pthread -x c
KERNEL = `uname -s`
APP_NAME=xfreq-intel
SRC_PATH=.
OBJ_PATH=./obj
BIN_PATH=./bin
INC4x86_PATH=-I ../api
LIB4x86_FLAGS=-lpthread -lrt
#
all :		$(APP_NAME)
#
help :
		@echo 'make [all]     : Compile, link and store executable file in' $(BIN_PATH)/$(APP_NAME)
		@echo 'make clean     : Remove object and binary files.'
		@echo 'make clean-all : Remove object and binary sub-directories.'
#
clean :
		@echo 'Cleaning ...'
		@if [ -e $(BIN_PATH)/$(APP_NAME) ];   then rm -v $(BIN_PATH)/$(APP_NAME);   else echo 'Nothing to do in '$(BIN_PATH); fi
		@if [ -e $(OBJ_PATH)/$(APP_NAME).o ]; then rm -v $(OBJ_PATH)/$(APP_NAME).o; else echo 'Nothing to do in '$(OBJ_PATH); fi
#
clean-all:
		@echo 'Cleaning all ...'
		@if [ -d $(BIN_PATH) ]; then rm -vr $(BIN_PATH); else echo 'Nothing to do in '$(BIN_PATH); fi
		@if [ -d $(OBJ_PATH) ]; then rm -vr $(OBJ_PATH); else echo 'Nothing to do in '$(OBJ_PATH); fi
#
$(APP_NAME)	: $(APP_NAME).o
		@if [ ! -d $(BIN_PATH) ]; then mkdir $(BIN_PATH); fi
		cc $(OBJ_PATH)/$(APP_NAME).o $(LIB4x86_FLAGS) -o $(BIN_PATH)/$(APP_NAME)
#
$(APP_NAME).o	: $(APP_NAME).c
		@if [ ! -d $(OBJ_PATH) ]; then mkdir $(OBJ_PATH); fi
		cc -D$(KERNEL) $(CFLAGS) -c $(APP_NAME).c $(INC4x86_PATH) -o $(OBJ_PATH)/$(APP_NAME).o
