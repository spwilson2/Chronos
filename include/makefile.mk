INCLUDE_TARGET = \
	stdlib

INCLUDES := $(addprefix include/, $(INCLUDE_TARGET))
INCLUDES := $(addsuffix .o, $(INCLUDES))

includes: $(INCLUDES)

include/%.o: include/%.c
	$(CC) $(CFLAGS) $(BUILD_CFLAGS) -c -o $@ $<

include-clean:
	rm -f $(INCLUDES)
