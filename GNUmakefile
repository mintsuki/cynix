QEMUFLAGS ?= -M q35,smm=off -m 8G -cdrom cynix.iso -debugcon stdio -smp 4

.PHONY: all
all:
	rm -f cynix.iso
	$(MAKE) cynix.iso

cynix.iso: jinx
	./build-support/makeiso.sh

.PHONY: debug
debug:
	JINX_CONFIG_FILE=jinx-config-debug $(MAKE) all

jinx:
	curl -Lo jinx https://github.com/mintsuki/jinx/raw/7a101a39eb061713f9c50ceafa1d713f35f17a3b/jinx
	chmod +x jinx

.PHONY: run-kvm
run-kvm: cynix.iso
	qemu-system-x86_64 -enable-kvm -cpu host $(QEMUFLAGS)

.PHONY: run-hvf
run-hvf: cynix.iso
	qemu-system-x86_64 -accel hvf -cpu host $(QEMUFLAGS)

ovmf/ovmf-code-x86_64.fd:
	mkdir -p ovmf
	curl -Lo $@ https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-x86_64.fd

ovmf/ovmf-vars-x86_64.fd:
	mkdir -p ovmf
	curl -Lo $@ https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-vars-x86_64.fd

.PHONY: run-uefi
run-uefi: cynix.iso ovmf/ovmf-code-x86_64.fd ovmf/ovmf-vars-x86_64.fd
	qemu-system-x86_64 \
		-enable-kvm \
		-cpu host \
		-drive if=pflash,unit=0,format=raw,file=ovmf/ovmf-code-x86_64.fd,readonly=on \
		-drive if=pflash,unit=1,format=raw,file=ovmf/ovmf-vars-x86_64.fd \
		$(QEMUFLAGS)

.PHONY: run-bochs
run-bochs: cynix.iso
	bochs -f bochsrc

.PHONY: run
run: cynix.iso
	qemu-system-x86_64 $(QEMUFLAGS)

.PHONY: clean
clean:
	rm -rf iso_root sysroot cynix.iso initramfs.tar

.PHONY: distclean
distclean: clean
	make -C kernel distclean
	rm -rf .jinx-cache jinx builds host-builds host-pkgs pkgs sources ovmf
