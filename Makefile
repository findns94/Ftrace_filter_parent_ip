obj-m += test.o
test-obj := test.o

all:
	make -C ${KDIR} M=`pwd` modules
clean:
	rm -rf *.o *.mod *.mod.c *.ko
