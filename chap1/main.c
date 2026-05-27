// 23b1num1252
/* Implementation of maxargs and interpreter for the simple language
   Uses A_stm/A_exp data structures (slp.h), utility functions (util.h)
   and the example program `prog()` in prog1.c.
*/

#include "prog1.h"
#include "slp.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

typedef struct table *Table_;
typedef struct IntAndTable *IntAndTable_;

struct table {
  string id;
  int value;
  Table_ tail;
};
struct IntAndTable {
  int i;
  Table_ t;
};

struct IntAndTable interpExp(A_exp exp, Table_ t);
Table_ Table(string id, int value, struct table *tail);
Table_ interpStm(A_stm stm, Table_ tab);
Table_ update(Table_ t, string key, int value);
IntAndTable_ intAndTable(int val, Table_ t);
int llookup(Table_ t, string key);

int llookup(Table_ t, string key) {
  if (t == NULL) {
    printf("null\n");
    return -1;
  }
  if (strcmp(key, t->id) == 0)
    return t->value;
  else
    return llookup(t->tail, key);
}

Table_ Table(string id, int value, struct table *tail) {
  Table_ t = checked_malloc(sizeof(*t));
  t->id = id;
  t->value = value;
  t->tail = tail;
  return t;
}

Table_ update(Table_ t, string key, int value) { return Table(key, value, t); }

IntAndTable_ intAndTable(int val, Table_ t) {
  IntAndTable_ iat = checked_malloc(sizeof(*iat));
  iat->i = val;
  iat->t = t;
  return iat;
}

int maxargs(A_stm stm) {
  int sum = 0;
  if (stm->kind == A_compoundStm) {
    return maxargs(stm->u.compound.stm1) + maxargs(stm->u.compound.stm2);
  } else if (stm->kind == A_assignStm) {
    if (stm->u.assign.exp->kind == A_eseqExp) {
      return maxargs(stm->u.assign.exp->u.eseq.stm);
    }
  } else { // kind is A_printStm
    A_expList expList = stm->u.print.exps;
    ++sum;
    while (expList->kind == A_pairExpList) {
      A_exp exp = expList->u.pair.head;
      if (exp->kind == A_eseqExp) {
        sum += maxargs(exp->u.eseq.stm);
      }
      expList = expList->u.pair.tail;
    }
  }
  return sum;
}

Table_ interpStm(A_stm stm, Table_ tab) {
  if (stm->kind == A_compoundStm) {
    tab = interpStm(stm->u.compound.stm1, tab);
    tab = interpStm(stm->u.compound.stm2, tab);
  } else if (stm->kind == A_assignStm) {
    tab = update(tab, stm->u.assign.id, interpExp(stm->u.assign.exp, tab).i);
  } else {
    if (stm->u.print.exps->kind == A_pairExpList) {
      A_expList expList = stm->u.print.exps;
      while (expList->kind == A_pairExpList) {
        A_exp exp = expList->u.pair.head;
        printf("%d ", interpExp(exp, tab).i);
        expList = expList->u.pair.tail;
      }
      printf("%d", interpExp(expList->u.last, tab).i);
    } else {
      printf("%d", interpExp(stm->u.print.exps->u.last, tab).i);
    }
    printf("\n");
  }
  return tab;
}

struct IntAndTable interpExp(A_exp exp, Table_ t) {
  if (exp->kind == A_idExp) {
    return *intAndTable(llookup(t, exp->u.id), t);
  } else if (exp->kind == A_numExp) {
    return *intAndTable(exp->u.num, t);
  } else if (exp->kind == A_opExp) {
    int a = interpExp(exp->u.op.left, t).i, b = interpExp(exp->u.op.right, t).i,
        ans;
    if (exp->u.op.oper == A_plus) {
      ans = a + b;
    } else if (exp->u.op.oper == A_minus) {
      ans = a - b;
    } else if (exp->u.op.oper == A_times) {
      ans = a * b;
    } else { // operator is divide
      ans = a / b;
    }
    return *intAndTable(ans, t);
  } else {
    interpStm(exp->u.eseq.stm, t);
    return interpExp(exp->u.eseq.exp, t);
  }
}

void interp(A_stm stm) {
  Table_ tab = NULL;
  if (stm->kind == A_compoundStm) {
    interp(stm->u.compound.stm1);
    interp(stm->u.compound.stm2);
  } else {
    tab = interpStm(stm, tab);
  }
}

int main() {
  A_stm stm = prog();
  printf("maxargs: %d \n", maxargs(stm));
  Table_ table = NULL;
  interpStm(stm, table);
  return 0;
}
