SYNC := bin
VM_USER ?= dev
VM_HOST ?= vm

all:

sync:
	rsync -azv $(SYNC) $(VM_USER)@$(VM_HOST):/home/$(VM_USER) --exclude='*.swp'
