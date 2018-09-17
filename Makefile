####
#### Main Makefile for the Aalto NEPPI building
####
#### This one simply calls the application makefile, with any of the
#### known targets.  You can simply add more known targets if you
#### detect ones that are not listed there.
####

KNOWN_TARGETS := all clean debug term flash reset

.PHONY: $(KNOWN_TARGETS)

$(KNOWN_TARGETS):
	"$(MAKE)" -C application "$@"
