APP_NAME=xfreq-intel xfreq-gui xfreq-cli
#
all :		$(APP_NAME)
#
help :
		@echo 'make [all]     : Compile, link and store executable file in' $(BIN_PATH)/$(APP_NAME)
		@echo 'make clean-all : Remove object and binary sub-directories.'
#
xfreq-intel :
		$(MAKE) -C svr
xfreq-gui :
		$(MAKE) -C gui
xfreq-cli :
		$(MAKE) -C cli
#
clean-all:
		@echo 'Cleaning all ...'
		$(MAKE) clean -C svr
		$(MAKE) clean -C gui
		$(MAKE) clean -C cli
