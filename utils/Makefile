
TEMP_FILE ?="path.list"
SCRIPT_FILE ?= "fuzzer"
DEST_PATH ?= "../../vm/initramfs/zero/bin"

default: random-paths gen-script
	
random-paths:
	./fsFuzzer-random-path-creator.awk nfiles 20 > $(TEMP_FILE)
.ONESHELL:
__gen-script:
	./fsFuzzer-gen-script.awk create $(TEMP_FILE) > $(SCRIPT_FILE).create.sh	
	./fsFuzzer-gen-script.awk test   $(TEMP_FILE) > $(SCRIPT_FILE).test.sh
	./fsFuzzer-gen-script.awk remove $(TEMP_FILE) > $(SCRIPT_FILE).remove.sh

.ONESHELL:
gen-script: __gen-script
	@cat <<EOF> $(SCRIPT_FILE).sh
	#!/bin/sh
	if [ -d \$$2 ] ; then
		echo $(SCRIPT_FILE).\$$1.sh \$$2
		time $(SCRIPT_FILE).\$$1.sh \$$2
		exit 0
	fi
	echo "\$$0' {create|test|remove} {dir}"
	exit 1;
	EOF
	echo "./$(SCRIPT_FILE).sh"
	
.PHONY:
install:
	chmod +x  $(SCRIPT_FILE).{remove,create,test}.sh $(SCRIPT_FILE).sh
	cp -t ../../vm/initramfs/zero/bin $(SCRIPT_FILE).{remove,create,test}.sh $(SCRIPT_FILE).sh
.PHONY:
uninstall:
	rm -v $(DEST_PATH)/{$(SCRIPT_FILE).{remove,create,test}.sh,$(SCRIPT_FILE).sh}
.PHONY:
clean:
	rm -v $(SCRIPT_FILE).{remove,create,test}.sh $(SCRIPT_FILE).sh $(TEMP_FILE)
	
