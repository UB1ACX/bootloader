
# Import compiler configuration
include make.cfg

# Get a list of source code from all subdirectories
SRCCS=$(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
SRCSS=$(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.s))

# Get the list of target files corresponding to the source code
OBJS=$(SRCCS:.c=.o) $(SRCSS:.s=.o)

# Delete the generated intermediate file
clean:
	-rm $(OBJS) $(LOCALTARGET) $(TMPTARGET) boot0.bin

