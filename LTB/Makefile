
CCFLAGS = -O6 -Wall

define COMPILE_CMD
$(CC) $(CCFLAGS) $(filter %.o, $^) -o $@
endef

all: TreeLimitedRun

TreeLimitedRun: TreeLimitedRun.o
	$(COMPILE_CMD)

.PHONY: clean

clean: 
	rm TreeLimitedRun