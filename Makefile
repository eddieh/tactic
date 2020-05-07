all: tt

tt: tactic.c
	cc -g -ledit -ltermcap tactic.c -o tt

clean:
	rm tt

.PHONY: clean
