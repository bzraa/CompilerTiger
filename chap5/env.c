#include <stdio.h>
#include "env.h"
#include "symbol.h"
#include "types.h"


static S_table tenv, venv;
E_enventry E_VarEntry(Ty_ty ty)
{
        E_enventry p = checked_malloc(sizeof(*p));
        p->kind = E_varEntry;
        p->u.var.ty = ty;
        p->u.var.readonly = FALSE;
        return p;
}

E_enventry E_ROVarEntry(Ty_ty ty)
{
        E_enventry p = E_VarEntry(ty);
        p->u.var.readonly = TRUE;
        return p;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result)
{
        E_enventry p = checked_malloc(sizeof(*p));
        p->kind = E_funEntry;
        p->u.fun.formals = formals;
        p->u.fun.result = result;
        return p;

}

S_table E_base_tenv(void)
{
    if (tenv)
        return tenv;
    tenv = S_empty();
    S_enter(tenv, S_Symbol("int"), Ty_Int());
    S_enter(tenv, S_Symbol("string"), Ty_String());
    return tenv;
}
S_table E_base_venv(void)
{
    if (venv)
        return venv;
    venv = S_empty();
    S_enter(venv, S_Symbol("print"),
            E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Void()));
    S_enter(venv, S_Symbol("flush"), E_FunEntry(NULL, Ty_Void()));
    S_enter(venv, S_Symbol("getchar"), E_FunEntry(NULL, Ty_String()));
    S_enter(venv, S_Symbol("ord"),
            E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Int()));
    S_enter(venv, S_Symbol("chr"),
            E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_String()));
    S_enter(venv, S_Symbol("size"),
            E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Int()));
    S_enter(venv, S_Symbol("substring"),
            E_FunEntry(Ty_TyList(Ty_String(),
                                 Ty_TyList(Ty_Int(),
                                           Ty_TyList(Ty_Int(), NULL))),
                       Ty_String()));
    S_enter(venv, S_Symbol("concat"),
            E_FunEntry(Ty_TyList(Ty_String(), Ty_TyList(Ty_String(), NULL)),
                       Ty_String()));
    S_enter(venv, S_Symbol("not"),
            E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_Int()));
    S_enter(venv, S_Symbol("exit"),
            E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_Void()));
    return venv;
}
