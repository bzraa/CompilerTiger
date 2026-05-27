#include "parse.h"
#include "absyn.h"
#include "errormsg.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include "semant.h"
#include "prabsyn.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: a.out filename\n");
    exit(1);
  }
  A_exp ast = parse(argv[1]);
  if (ast) {
          pr_exp(stdout, ast, 0);
          fprintf(stdout, "\n");
  }
  SEM_transProg(ast);
  return 0;
}
