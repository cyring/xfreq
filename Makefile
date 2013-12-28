CC = gcc
CFLAGS = -Wall -g -pthread -x c
APP_NAME=xfreq
SRC_PATH=.
OBJ_PATH=./obj
BIN_PATH=./bin
INC4x86_PATH=
INC4RPi_PATH=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux/
LIB4x86_FLAGS=-lX11 -lpthread
LIB4RPi_FLAGS=-lm -lGLESv2 -lEGL -lbcm_host -lvcos -L/opt/vc/lib/
#
all :		$(APP_NAME)
		@echo 'Ready     : '$(BIN_PATH)/$(APP_NAME)
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
		@echo -n 'Linking   : '
		@if [ ! -d $(BIN_PATH) ]; then mkdir $(BIN_PATH); fi
		@if [ `uname -m` == 'x86_64' ]; \
			then $(CC) $(OBJ_PATH)/$(APP_NAME).o $(LIB4x86_FLAGS) -o $(BIN_PATH)/$(APP_NAME) ; \
			else $(CC) $(OBJ_PATH)/$(APP_NAME).o $(LIB4RPi_FLAGS) -o $(BIN_PATH)/$(APP_NAME) ; \
		fi
		@echo 'Done.'
#
$(APP_NAME).o	: $(APP_NAME).c
		@echo -n 'Compiling : '
		@if [ ! -d $(OBJ_PATH) ]; then mkdir $(OBJ_PATH); fi
		@if [ `uname -m` == 'x86_64' ]; \
			then $(CC) $(CFLAGS) -c $(APP_NAME).c $(INC4x86_PATH) -o $(OBJ_PATH)/$(APP_NAME).o ; \
			else $(CC) $(CFLAGS) -c $(APP_NAME).c $(INC4RPi_PATH) -o $(OBJ_PATH)/$(APP_NAME).o ; \
		fi
		@echo 'Done.'
