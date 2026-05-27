#include "semant.h"
#include "absyn.h"
#include "env.h"
#include "errormsg.h"
#include "symbol.h"
#include "types.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int loop_depth = 0;

Ty_ty actual_ty(Ty_ty ty) {
  int limit = 256;
  if (ty == NULL) {
    fprintf(stderr, "NULL type error actual_ty()\n");
    return NULL;
  }

  while (ty && ty->kind == Ty_name && ty->u.name.ty && limit-- > 0)
    ty = ty->u.name.ty;

  if (limit <= 0) {
    fprintf(stderr, "Cycling type error actual_ty()\n");
    return NULL;
  }

  return ty;
}

int has_same_ty(Ty_ty ty1, Ty_ty ty2) {
  ty1 = actual_ty(ty1);
  ty2 = actual_ty(ty2);

  if (!ty1 || !ty2)
    return 0;
  if (ty1 == ty2)
    return 1;
  if (ty1->kind == Ty_nil && ty2->kind == Ty_record)
    return 1;
  if (ty2->kind == Ty_nil && ty1->kind == Ty_record)
    return 1;
  return 0;
}

static int is_int(Ty_ty ty) {
  ty = actual_ty(ty);
  return ty && ty->kind == Ty_int;
}

static int is_void(Ty_ty ty) {
  ty = actual_ty(ty);
  return ty && ty->kind == Ty_void;
}

static int valid_order_ty(Ty_ty ty) {
  ty = actual_ty(ty);
  return ty && (ty->kind == Ty_int || ty->kind == Ty_string);
}

static int is_illegal_type_cycle(Ty_ty ty) {
  Ty_ty start = ty;
  int limit = 256;

  while (ty && ty->kind == Ty_name && ty->u.name.ty && limit-- > 0) {
    ty = ty->u.name.ty;
    if (ty == start)
      return 1;
    if (ty->kind != Ty_name)
      return 0;
  }

  return limit <= 0;
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList afields) {
  if (afields == NULL)
    return NULL;
  A_field afield = afields->head;
  // S_symbol name = afield.name;
  S_symbol typ = afield->typ;
  Ty_ty ty = (Ty_ty)S_look(tenv, typ);
  if (ty == NULL) {
    EM_error(afield->pos, "undefined type %s", S_name(typ));
    ty = Ty_Int();
  }
  return Ty_TyList(ty, makeFormalTyList(tenv, afields->tail));
}
string type_msg(Ty_ty ty) {
  ty = actual_ty(ty);
  if (!ty)
    return "unknown";
  switch (ty->kind) {
  case Ty_int:
    return "int";
  case Ty_string:
    return "string";
  case Ty_record:
    return "record";
  case Ty_array:
    return "array";
  case Ty_nil:
    return "nil";
  case Ty_void:
    return "void";
  case Ty_name:
    return "name";
  }
  return "unknown";
}

struct expty expTy(Tr_exp exp, Ty_ty ty) {
  struct expty e;
  e.exp = exp;
  e.ty = ty;
  return e;
}

void SEM_transProg(A_exp exp) {
  if (!exp)
    return;
  S_table tenv = E_base_tenv();
  S_table venv = E_base_venv();
  transExp(venv, tenv, exp);
}

struct expty transVar(S_table venv, S_table tenv, A_var v) {
  switch (v->kind) {
  case A_simpleVar: {
    E_enventry x = S_look(venv, v->u.simple);
    if (!x || x->kind != E_varEntry) {
      EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
      return expTy(NULL, Ty_Int());
    }
    return expTy(NULL, actual_ty(x->u.var.ty));
  }

  case A_fieldVar: {
    struct expty var = transVar(venv, tenv, v->u.field.var);
    Ty_ty t = actual_ty(var.ty);

    if (t->kind != Ty_record) {
      EM_error(v->pos, "not a record type");
      return expTy(NULL, Ty_Int());
    }

    Ty_fieldList fl = t->u.record;
    for (; fl; fl = fl->tail) {
      if (fl->head->name == v->u.field.sym)
        return expTy(NULL, fl->head->ty);
    }

    EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));
    return expTy(NULL, Ty_Int());
  }

  case A_subscriptVar: {
    struct expty var = transVar(venv, tenv, v->u.subscript.var);
    struct expty idx = transExp(venv, tenv, v->u.subscript.exp);

    if (actual_ty(var.ty)->kind != Ty_array) {
      EM_error(v->pos, "not an array");
      return expTy(NULL, Ty_Int());
    }

    if (actual_ty(idx.ty)->kind != Ty_int) {
      EM_error(v->pos, "array index must be int");
    }

    return expTy(NULL, actual_ty(var.ty)->u.array);
  }
  }
  return expTy(NULL, Ty_Int());
}

struct expty transExp(S_table venv, S_table tenv, A_exp a) {
  if (!a)
    return expTy(NULL, Ty_Void());

  switch (a->kind) {
  case A_arrayExp: {
    Ty_ty typ = S_look(tenv, a->u.array.typ);

    if (!typ) {
      EM_error(a->pos, "undefined type %s", S_name(a->u.array.typ));
      return expTy(NULL, Ty_Int());
    }

    typ = actual_ty(typ);

    if (typ->kind != Ty_array) {
      EM_error(a->pos, "not an array type");
      return expTy(NULL, Ty_Int());
    }

    struct expty size = transExp(venv, tenv, a->u.array.size);

    struct expty init = transExp(venv, tenv, a->u.array.init);

    if (actual_ty(size.ty)->kind != Ty_int)
      EM_error(a->pos, "array size must be int");

    if (!has_same_ty(typ->u.array, init.ty))
      EM_error(a->pos, "array init type mismatch");

    return expTy(NULL, typ);
  }

  case A_recordExp: {
    Ty_ty typ = S_look(tenv, a->u.record.typ);

    if (!typ) {
      EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
      return expTy(NULL, Ty_Int());
    }

    typ = actual_ty(typ);

    if (typ->kind != Ty_record) {
      EM_error(a->pos, "not a record type");
      return expTy(NULL, Ty_Int());
    }

    Ty_fieldList fl = typ->u.record;
    A_efieldList ef = a->u.record.fields;

    for (; fl && ef; fl = fl->tail, ef = ef->tail) {
      struct expty e = transExp(venv, tenv, ef->head->exp);

      if (fl->head->name != ef->head->name) {
        EM_error(a->pos, "field name mismatch");
      }

      if (!has_same_ty(fl->head->ty, e.ty)) {
        EM_error(a->pos, "record field type mismatch");
      }
    }

    if (fl || ef) {
      EM_error(a->pos, "wrong number of fields");
    }

    return expTy(NULL, typ);
  }

  case A_intExp:
    return expTy(NULL, Ty_Int());

  case A_stringExp:
    return expTy(NULL, Ty_String());

  case A_nilExp:
    return expTy(NULL, Ty_Nil());

  case A_varExp:
    return transVar(venv, tenv, a->u.var);

  case A_callExp: {
    E_enventry fun = S_look(venv, a->u.call.func);
    A_expList args = a->u.call.args;
    Ty_tyList formals;
    int arg_index = 1;
    int wrong_count = 0;

    if (!fun || fun->kind != E_funEntry) {
      EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
      for (; args; args = args->tail)
        transExp(venv, tenv, args->head);
      return expTy(NULL, Ty_Int());
    }

    formals = fun->u.fun.formals;
    while (args && formals) {
      struct expty arg = transExp(venv, tenv, args->head);
      if (!has_same_ty(formals->head, arg.ty)) {
        EM_error(args->head->pos, "argument %d type mismatch", arg_index);
      }
      args = args->tail;
      formals = formals->tail;
      arg_index++;
    }

    if (args || formals)
      wrong_count = 1;

    for (; args; args = args->tail)
      transExp(venv, tenv, args->head);

    if (wrong_count)
      EM_error(a->pos, "wrong number of arguments to %s",
               S_name(a->u.call.func));

    return expTy(NULL, fun->u.fun.result);
  }

  case A_opExp: {
    struct expty left = transExp(venv, tenv, a->u.op.left);
    struct expty right = transExp(venv, tenv, a->u.op.right);

    switch (a->u.op.oper) {
    case A_plusOp:
    case A_minusOp:
    case A_timesOp:
    case A_divideOp:
      if (!is_int(left.ty) || !is_int(right.ty)) {
        EM_error(a->pos, "integer required");
      }
      return expTy(NULL, Ty_Int());

    case A_eqOp:
    case A_neqOp:
      if (!has_same_ty(left.ty, right.ty)) {
        EM_error(a->pos, "types do not match");
      }
      return expTy(NULL, Ty_Int());

    case A_ltOp:
    case A_leOp:
    case A_gtOp:
    case A_geOp:
      if (!has_same_ty(left.ty, right.ty) || !valid_order_ty(left.ty)) {
        EM_error(a->pos, "integer or string operands required");
      }
      return expTy(NULL, Ty_Int());
    }
  }

  case A_seqExp: {
    A_expList l = a->u.seq;
    Ty_ty t = Ty_Void();

    for (; l; l = l->tail) {
      struct expty e = transExp(venv, tenv, l->head);
      t = e.ty;
    }
    return expTy(NULL, t);
  }

  case A_assignExp: {
    struct expty var = transVar(venv, tenv, a->u.assign.var);
    struct expty exp = transExp(venv, tenv, a->u.assign.exp);

    if (a->u.assign.var->kind == A_simpleVar) {
      E_enventry entry = S_look(venv, a->u.assign.var->u.simple);
      if (entry && entry->kind == E_varEntry && entry->u.var.readonly)
        EM_error(a->pos, "loop variable cannot be assigned");
    }

    if (!has_same_ty(var.ty, exp.ty)) {
      EM_error(a->pos, "assignment type mismatch");
    }

    return expTy(NULL, Ty_Void());
  }

  case A_ifExp: {
    struct expty test = transExp(venv, tenv, a->u.iff.test);
    struct expty then = transExp(venv, tenv, a->u.iff.then);

    if (!is_int(test.ty))
      EM_error(a->pos, "if test must be int");

    if (a->u.iff.elsee) {
      struct expty elsee = transExp(venv, tenv, a->u.iff.elsee);
      if (!has_same_ty(then.ty, elsee.ty)) {
        EM_error(a->pos, "then and else type mismatch");
      }
      return expTy(NULL, then.ty);
    }

    if (!is_void(then.ty))
      EM_error(a->pos, "if-then body must produce no value");

    return expTy(NULL, Ty_Void());
  }

  case A_whileExp: {
    struct expty test = transExp(venv, tenv, a->u.whilee.test);
    loop_depth++;
    struct expty body = transExp(venv, tenv, a->u.whilee.body);
    loop_depth--;

    if (!is_int(test.ty))
      EM_error(a->pos, "while test must be int");

    if (!is_void(body.ty))
      EM_error(a->pos, "while body must be void");

    return expTy(NULL, Ty_Void());
  }

  case A_forExp: {
    struct expty lo = transExp(venv, tenv, a->u.forr.lo);
    struct expty hi = transExp(venv, tenv, a->u.forr.hi);
    struct expty body;

    if (!is_int(lo.ty))
      EM_error(a->u.forr.lo->pos, "for lower bound must be int");
    if (!is_int(hi.ty))
      EM_error(a->u.forr.hi->pos, "for upper bound must be int");

    S_beginScope(venv);
    S_enter(venv, a->u.forr.var, E_ROVarEntry(Ty_Int()));
    loop_depth++;
    body = transExp(venv, tenv, a->u.forr.body);
    loop_depth--;
    S_endScope(venv);

    if (!is_void(body.ty))
      EM_error(a->pos, "for body must be void");

    return expTy(NULL, Ty_Void());
  }

  case A_breakExp:
    if (loop_depth <= 0)
      EM_error(a->pos, "break is not inside a loop");
    return expTy(NULL, Ty_Void());

  case A_letExp: {
    S_beginScope(venv);
    S_beginScope(tenv);

    A_decList d = a->u.let.decs;
    for (; d; d = d->tail)
      transDec(venv, tenv, d->head);

    struct expty body = transExp(venv, tenv, a->u.let.body);

    S_endScope(venv);
    S_endScope(tenv);

    return body;
  }

  default:
    return expTy(NULL, Ty_Int());
  }
}

void transDec(S_table venv, S_table tenv, A_dec d) {
  switch (d->kind) {

  case A_varDec: {
    struct expty init = transExp(venv, tenv, d->u.var.init);
    Ty_ty ty;

    if (d->u.var.typ) {
      ty = S_look(tenv, d->u.var.typ);
      if (!ty) {
        EM_error(d->pos, "undefined type %s", S_name(d->u.var.typ));
        ty = Ty_Int();
      }

      if (!has_same_ty(ty, init.ty))
        EM_error(d->pos, "type mismatch in var declaration");
    } else {
      ty = init.ty;
      if (actual_ty(ty) && actual_ty(ty)->kind == Ty_nil) {
        EM_error(d->pos, "nil initializer requires an explicit type");
        ty = Ty_Int();
      }
    }

    S_enter(venv, d->u.var.var, E_VarEntry(ty));
    break;
  }

  case A_functionDec: {
    A_fundecList fl;

    for (fl = d->u.function; fl; fl = fl->tail) {
      A_fundec f = fl->head;
      Ty_tyList formals = makeFormalTyList(tenv, f->params);
      Ty_ty result = Ty_Void();

      if (f->result) {
        result = S_look(tenv, f->result);
        if (!result) {
          EM_error(f->pos, "undefined result type %s", S_name(f->result));
          result = Ty_Int();
        }
      }

      S_enter(venv, f->name, E_FunEntry(formals, result));
    }

    for (fl = d->u.function; fl; fl = fl->tail) {
      A_fundec f = fl->head;
      E_enventry fun = S_look(venv, f->name);
      A_fieldList params = f->params;
      Ty_tyList formals = fun->u.fun.formals;
      struct expty body;

      S_beginScope(venv);
      for (; params && formals; params = params->tail, formals = formals->tail)
        S_enter(venv, params->head->name, E_VarEntry(formals->head));

      body = transExp(venv, tenv, f->body);
      S_endScope(venv);

      if (!has_same_ty(fun->u.fun.result, body.ty))
        EM_error(f->pos, "function %s body type does not match result type",
                 S_name(f->name));
    }
    break;
  }

  case A_typeDec: {
    A_nametyList nl;

    for (nl = d->u.type; nl; nl = nl->tail) {
      S_enter(tenv, nl->head->name, Ty_Name(nl->head->name, NULL));
    }

    for (nl = d->u.type; nl; nl = nl->tail) {
      Ty_ty ty = S_look(tenv, nl->head->name);
      ty->u.name.ty = transTy(tenv, nl->head->ty);
    }

    for (nl = d->u.type; nl; nl = nl->tail) {
      Ty_ty ty = S_look(tenv, nl->head->name);
      if (is_illegal_type_cycle(ty))
        EM_error(d->pos, "illegal type cycle involving %s",
                 S_name(nl->head->name));
    }
    break;
  }

  default:
    break;
  }
}

Ty_ty transTy(S_table tenv, A_ty a) {
  switch (a->kind) {

  case A_nameTy: {
    Ty_ty t = S_look(tenv, a->u.name);
    if (!t) {
      EM_error(a->pos, "undefined type %s", S_name(a->u.name));
      return Ty_Int();
    }
    return t;
  }

  case A_arrayTy: {
    Ty_ty t = S_look(tenv, a->u.array);
    if (!t) {
      EM_error(a->pos, "undefined type %s", S_name(a->u.array));
      return Ty_Int();
    }
    return Ty_Array(t);
  }

  case A_recordTy: {
    Ty_fieldList fields = NULL, tail = NULL;

    for (A_fieldList fl = a->u.record; fl; fl = fl->tail) {
      Ty_ty t = S_look(tenv, fl->head->typ);
      if (!t) {
        EM_error(fl->head->pos, "undefined type %s", S_name(fl->head->typ));
        t = Ty_Int();
      }

      Ty_field f = Ty_Field(fl->head->name, t);
      if (!fields)
        fields = tail = Ty_FieldList(f, NULL);
      else {
        tail->tail = Ty_FieldList(f, NULL);
        tail = tail->tail;
      }
    }

    return Ty_Record(fields);
  }
  }

  return Ty_Int();
}
