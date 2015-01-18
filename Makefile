export CC
APP_NAME=xfreq-api xfreq-intel xfreq-gui xfreq-cli
#
all :		$(APP_NAME)
#
help :
		@echo 'make [all]       : Compile and link files {'$(APP_NAME)'}'
		@echo 'make clean[-all] : Remove object and binary sub-directories.'
#
xfreq-api:
		$(MAKE) -C api
xfreq-intel :
		$(MAKE) -C svr
xfreq-gui :
		$(MAKE) -C gui
xfreq-cli :
		$(MAKE) -C cli
#
clean:
		@echo 'Cleaning ...'
		$(MAKE) clean -C api
		$(MAKE) clean -C svr
		$(MAKE) clean -C gui
		$(MAKE) clean -C cli
#
clean-all:
		@echo 'Cleaning all ...'
		$(MAKE) clean-all -C api
		$(MAKE) clean-all -C svr
		$(MAKE) clean-all -C gui
		$(MAKE) clean-all -C cli
