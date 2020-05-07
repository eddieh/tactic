all: tt

tt: tactic.c
	cc -g tactic.c -o tt

clean:
	rm tt

.PHONY: clean
