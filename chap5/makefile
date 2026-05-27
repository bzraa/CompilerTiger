CC = gcc-15
CFLAGS = -g
YACC = yacc
LEX = lex

OBJS = parsetest.o y.tab.o lex.yy.o errormsg.o util.o absyn.o symbol.o \
       parse.o table.o prabsyn.o semant.o env.o types.o

a.out: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

y.tab.c y.tab.h: tiger.grm
	$(YACC) -dv tiger.grm

lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h

lex.yy.c: tiger.lex
	$(LEX) tiger.lex

parsetest.o: parsetest.c errormsg.h util.h parse.h absyn.h semant.h
y.tab.o: y.tab.c absyn.h errormsg.h util.h
errormsg.o: errormsg.c errormsg.h util.h
util.o: util.c util.h
absyn.o: absyn.c absyn.h symbol.h util.h
symbol.o: symbol.c symbol.h table.h util.h
parse.o: parse.c parse.h absyn.h errormsg.h symbol.h util.h
table.o: table.c table.h util.h
prabsyn.o: prabsyn.c prabsyn.h absyn.h symbol.h util.h
semant.o: semant.c semant.h absyn.h env.h errormsg.h symbol.h types.h util.h
env.o: env.c env.h symbol.h types.h util.h
types.o: types.c types.h symbol.h util.h

clean:
	rm -f a.out $(OBJS) y.tab.c y.tab.h y.output lex.yy.c
