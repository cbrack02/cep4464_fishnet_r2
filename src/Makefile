CC = gcc
CFLAGS = -g -Wall -Werror -std=c2x

OS = $(shell uname -s)
PROC = $(shell uname -p)
# Some Linux installations report "unknown" for uname -p.
ifeq ($(PROC),unknown)
PROC = $(shell uname -m)
endif
EXEC_SUFFIX = $(OS)-$(PROC)
TARGET = fishnode-$(EXEC_SUFFIX)

ifeq ($(OS),SunOS)
OSLIB = -L/opt/csw/lib -lsocket -lnsl
OSINC = -I/opt/csw/include
OSDEF = -DSOLARIS
else ifeq ($(OS),Darwin)
OSLIB =
OSINC =
OSDEF = -DDARWIN
else
OSLIB =
OSINC =
OSDEF = -DLINUX
endif

# Build our L2/ARP and L3 packet handling; retain built-in routing tables.
IMPL_FLAGS ?= -DL2_IMPL -DL3_IMPL
LIBFISH ?= $(if $(wildcard libfish-$(EXEC_SUFFIX).a),libfish-$(EXEC_SUFFIX).a,libfish.a)

# fishtest_run() selects ONE registered name per invocation, not a suite.
# L2 names are verified. L3 names follow the public function names; the
# decompiled export omits their registration table, so confirm with libfish.
# Override as needed: make test TESTS='registered_name another_name'
L2_TESTS ?= send_arp_request arp_received fishnode_l2_receive fish_l2_send \
         arp_cache gratuitous_arp arp_timeout arp_resolve_queue arp_retry
L3_TESTS ?= fishnode_l3_receive fish_l3_send fish_l3_forward
TESTS ?= $(L2_TESTS) $(L3_TESTS)

# 0 tests your replacements; 1 permits built-ins for baseline comparisons.
# The library checks variable PRESENCE, so 0 must unset the environment variable.
TEST_BUILTIN ?= 0

# An empty fishhead address joins an ad hoc network without a fishhead server.
# This still creates sockets. Supply a real host:port here if needed.
TEST_HEAD ?=
TEST_ADDR ?= 1

.PHONY: all test test-l2 test-l3 clean
all: $(TARGET)

$(TARGET): fishnode.c fish.h fishnode.h $(LIBFISH) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OSINC) $(OSDEF) $(IMPL_FLAGS) \
		$(LDFLAGS) -o $@ fishnode.c $(LIBFISH) $(OSLIB) $(LDLIBS)

# FAILED is printed even when fishnode exits successfully. Require an explicit
# PASSED line, reject FAILED lines, and also check the process exit status.
test: $(TARGET)
	@set -eu; \
	case "$(TEST_BUILTIN)" in \
		1) export FISHTEST_ENABLE_BUILTIN=1 ;; \
		0) unset FISHTEST_ENABLE_BUILTIN ;; \
		*) echo 'TEST_BUILTIN must be 0 or 1' >&2; exit 1 ;; \
	esac; \
	if [ -z "$(strip $(TESTS))" ]; then \
		echo 'Specify at least one registered test in TESTS.' >&2; exit 1; \
	fi; \
	log=$$(mktemp); \
	trap 'rm -f "$$log"' 0; \
	trap 'exit 1' 1 2 3 15; \
	failed=0; \
	for name in $(TESTS); do \
		printf '\n=== %s ===\n' "$$name"; \
		status=0; \
		FISHTEST="$$name" FISHTEST_OUTPUT_FD=1 \
			./$(TARGET) -noprompt "$(TEST_HEAD)" "$(TEST_ADDR)" \
			>"$$log" 2>&1 || status=$$?; \
		cat "$$log"; \
		if [ "$$status" -ne 0 ] || grep -q '^FAILED' "$$log" || \
			! grep -q '^PASSED$$' "$$log"; then \
			printf 'Test %s failed (exit status %s).\n' "$$name" "$$status" >&2; \
			if grep -q '^FAILED Unit test .* not found' "$$log"; then \
				echo 'Use TESTS to supply the names registered in your libfish build.' >&2; \
			fi; \
			failed=1; \
		fi; \
	done; \
	exit "$$failed"

test-l2:
	$(MAKE) test TESTS="$(L2_TESTS)"

test-l3:
	$(MAKE) test TESTS="$(L3_TESTS)"

clean:
	rm -f fishnode-* *.o
