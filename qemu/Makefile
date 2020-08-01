all:

clean:
	rm -rf *.s2e.*

# dummy target to enable auto-completion for clean-% target
$(patsubst %.s2e,clean-%,$(wildcard *.s2e)):

clean-%:
	rm -rf $*.s2e.*
