CC = /usr/bin/gcc
CFLAGS = -Wall -Wextra -Wpedantic -fomit-frame-pointer -Os
NO_OPT_CFLAGS = -Wall -Wextra -Wpedantic -fomit-frame-pointer

# Build directory for all output files
BUILD_DIR = build

# If ALL_PARM_SETS is defined, override tune.h to allow all parameter sets
ifeq ($(ALL_PARM_SETS),1)
DFLAGS += -DTUNE_H_ -DTS_SUPPORT_L5=1 -DTS_SUPPORT_SHAKE=1 \
	  -DTS_SUPPORT_SHA2=1 -DTS_SUPPORT_S=1 \
	  -DTS_SHA2_OPTIMIZATION=1
endif

.PHONY: clean all

# Ensure build directory exists
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

%.o : %.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(DFLAGS) $< -o $(BUILD_DIR)/$@

OBJECTS = tiny_sphincs.o key_gen.o size.o \
	  verify.o \
	  fips202.o shake256_hash.o shake256_simple.o \
	  sha256.o sha256_hash.o sha2_simple.o sha256_L1_hash.o \
	  sha512.o sha512_hash.o \
	  sha256_L1_hash_simple.o \
	  sha512_L35_hash_simple.o \
	  endian.o \
	  shake256_128f_simple.o shake256_128s_simple.o \
	  shake256_192f_simple.o shake256_192s_simple.o \
	  shake256_256f_simple.o shake256_256s_simple.o \
	  sha2_128f_simple.o sha2_128s_simple.o \
	  sha2_192f_simple.o sha2_192s_simple.o \
	  sha2_256f_simple.o sha2_256s_simple.o

TEST_SOURCES = test_sphincs.c test_testvector.c test_sha512.c test_shake.c \
	       test_verify.c

#
# Makes the regression test executable
test_sphincs: $(TEST_SOURCES) $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(BUILD_DIR)/$@ $(TEST_SOURCES) $(addprefix $(BUILD_DIR)/,$(OBJECTS))

#
# Makes the double-pass streaming test executable
test_double_pass: test_double_pass.c $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(BUILD_DIR)/$@ test_double_pass.c $(addprefix $(BUILD_DIR)/,$(OBJECTS))

#
# Makes the compare test executable
test_compare: test_compare.c $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(BUILD_DIR)/$@ test_compare.c $(addprefix $(BUILD_DIR)/,$(OBJECTS))

#
# Makes the performance test executable for incremental-double-pass branch
test_performance_double_pass: test_performance_double_pass.c $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(BUILD_DIR)/$@ test_performance_double_pass.c $(addprefix $(BUILD_DIR)/,$(OBJECTS))

#
# Makes the original performance test executable for main branch
test_performance_orig: test_performance_orig.c $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(BUILD_DIR)/$@ test_performance_orig.c $(addprefix $(BUILD_DIR)/,$(OBJECTS))

#
# Makes the simple verify test executable
test_simple_verify: test_simple_verify.c $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(BUILD_DIR)/$@ test_simple_verify.c $(addprefix $(BUILD_DIR)/,$(OBJECTS))

#
# Makes the double-pass verify test executable
test_double_pass_verify: test_double_pass_verify.c $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(BUILD_DIR)/$@ test_double_pass_verify.c $(addprefix $(BUILD_DIR)/,$(OBJECTS))

#
# Makes the SHAKE verify test executable
test_shake_verify: test_shake_verify.c $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(BUILD_DIR)/$@ test_shake_verify.c $(addprefix $(BUILD_DIR)/,$(OBJECTS))

clean:
	-$(RM) -r $(BUILD_DIR)

#
# Internal rule used by the ./ramspace.py script to find the amount of RAM
# used by the parameter set specified by $(PARM_SET).
# This shouldn't be called manually; instead, run the ./ramspace.py script
ramspace: ramspace.c get_space.c get_space2.c stack.c $(OBJECTS) | $(BUILD_DIR)
	$(CC) -o $(BUILD_DIR)/$@ $(NO_OPT_CFLAGS) $(DFLAGS) -DPARM_SET=$(PARM_SET) ramspace.c get_space.c get_space2.c stack.c $(addprefix $(BUILD_DIR)/,$(OBJECTS))
	$(BUILD_DIR)/ramspace $(OUTPUT_FILE)
