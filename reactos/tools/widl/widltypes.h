/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WIDL_WIDLTYPES_H
#define __WIDL_WIDLTYPES_H

#define S_OK           0
#define S_FALSE        1
#define E_OUTOFMEMORY  ((HRESULT)0x8007000EL)
#define TYPE_E_IOERROR ((HRESULT)0x80028CA2L)

#define max(a, b) ((a) > (b) ? a : b)

#include <stdarg.h>
#include <assert.h>
#include "guiddef.h"
#include "wine/rpcfc.h"
#include "wine/list.h"

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef GUID UUID;
#endif

#ifndef TRUE
#define TRUE 1
#endif
#define FALSE 0

#define RPC_FC_MODULE   0xfc
#define RPC_FC_COCLASS  0xfd
#define RPC_FC_FUNCTION 0xfe

typedef struct _loc_info_t loc_info_t;
typedef struct _attr_t attr_t;
typedef struct _expr_t expr_t;
typedef struct _type_t type_t;
typedef struct _typeref_t typeref_t;
typedef struct _var_t var_t;
typedef struct _declarator_t declarator_t;
typedef struct _func_t func_t;
typedef struct _ifref_t ifref_t;
typedef struct _typelib_entry_t typelib_entry_t;
typedef struct _importlib_t importlib_t;
typedef struct _importinfo_t importinfo_t;
typedef struct _typelib_t typelib_t;
typedef struct _user_type_t user_type_t;
typedef struct _user_type_t context_handle_t;
typedef struct _type_list_t type_list_t;
typedef struct _statement_t statement_t;

typedef struct list attr_list_t;
typedef struct list str_list_t;
typedef struct list func_list_t;
typedef struct list expr_list_t;
typedef struct list var_list_t;
typedef struct list declarator_list_t;
typedef struct list ifref_list_t;
typedef struct list array_dims_t;
typedef struct list user_type_list_t;
typedef struct list context_handle_list_t;
typedef struct list statement_list_t;

enum attr_type
{
    ATTR_AGGREGATABLE,
    ATTR_APPOBJECT,
    ATTR_ASYNC,
    ATTR_AUTO_HANDLE,
    ATTR_BINDABLE,
    ATTR_BROADCAST,
    ATTR_CALLAS,
    ATTR_CALLCONV, /* calling convention pseudo-attribute */
    ATTR_CASE,
    ATTR_CONST, /* const pseudo-attribute */
    ATTR_CONTEXTHANDLE,
    ATTR_CONTROL,
    ATTR_DEFAULT,
    ATTR_DEFAULTCOLLELEM,
    ATTR_DEFAULTVALUE,
    ATTR_DEFAULTVTABLE,
    ATTR_DISPINTERFACE,
    ATTR_DISPLAYBIND,
    ATTR_DLLNAME,
    ATTR_DUAL,
    ATTR_ENDPOINT,
    ATTR_ENTRY,
    ATTR_EXPLICIT_HANDLE,
    ATTR_HANDLE,
    ATTR_HELPCONTEXT,
    ATTR_HELPFILE,
    ATTR_HELPSTRING,
    ATTR_HELPSTRINGCONTEXT,
    ATTR_HELPSTRINGDLL,
    ATTR_HIDDEN,
    ATTR_ID,
    ATTR_IDEMPOTENT,
    ATTR_IIDIS,
    ATTR_IMMEDIATEBIND,
    ATTR_IMPLICIT_HANDLE,
    ATTR_IN,
    ATTR_INLINE,
    ATTR_INPUTSYNC,
    ATTR_LENGTHIS,
    ATTR_LIBLCID,
    ATTR_LOCAL,
    ATTR_NONBROWSABLE,
    ATTR_NONCREATABLE,
    ATTR_NONEXTENSIBLE,
    ATTR_OBJECT,
    ATTR_ODL,
    ATTR_OLEAUTOMATION,
    ATTR_OPTIONAL,
    ATTR_OUT,
    ATTR_POINTERDEFAULT,
    ATTR_POINTERTYPE,
    ATTR_PROPGET,
    ATTR_PROPPUT,
    ATTR_PROPPUTREF,
    ATTR_PUBLIC,
    ATTR_RANGE,
    ATTR_READONLY,
    ATTR_REQUESTEDIT,
    ATTR_RESTRICTED,
    ATTR_RETVAL,
    ATTR_SIZEIS,
    ATTR_SOURCE,
    ATTR_STRICTCONTEXTHANDLE,
    ATTR_STRING,
    ATTR_SWITCHIS,
    ATTR_SWITCHTYPE,
    ATTR_TRANSMITAS,
    ATTR_UUID,
    ATTR_V1ENUM,
    ATTR_VARARG,
    ATTR_VERSION,
    ATTR_WIREMARSHAL
};

enum expr_type
{
    EXPR_VOID,
    EXPR_NUM,
    EXPR_HEXNUM,
    EXPR_DOUBLE,
    EXPR_IDENTIFIER,
    EXPR_NEG,
    EXPR_NOT,
    EXPR_PPTR,
    EXPR_CAST,
    EXPR_SIZEOF,
    EXPR_SHL,
    EXPR_SHR,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_ADD,
    EXPR_SUB,
    EXPR_AND,
    EXPR_OR,
    EXPR_COND,
    EXPR_TRUEFALSE,
    EXPR_ADDRESSOF,
    EXPR_MEMBER,
    EXPR_ARRAY,
    EXPR_MOD,
    EXPR_LOGOR,
    EXPR_LOGAND,
    EXPR_XOR,
    EXPR_EQUALITY,
    EXPR_INEQUALITY,
    EXPR_GTR,
    EXPR_LESS,
    EXPR_GTREQL,
    EXPR_LESSEQL,
    EXPR_LOGNOT,
    EXPR_POS,
    EXPR_STRLIT,
    EXPR_WSTRLIT,
};

enum type_kind
{
    TKIND_PRIMITIVE = -1,
    TKIND_ENUM,
    TKIND_RECORD,
    TKIND_MODULE,
    TKIND_INTERFACE,
    TKIND_DISPATCH,
    TKIND_COCLASS,
    TKIND_ALIAS,
    TKIND_UNION,
    TKIND_MAX
};

enum storage_class
{
    STG_NONE,
    STG_STATIC,
    STG_EXTERN,
    STG_REGISTER,
};

enum statement_type
{
    STMT_LIBRARY,
    STMT_DECLARATION,
    STMT_TYPE,
    STMT_TYPEREF,
    STMT_MODULE,
    STMT_TYPEDEF,
    STMT_IMPORT,
    STMT_IMPORTLIB,
    STMT_CPPQUOTE
};

struct _loc_info_t
{
    const char *input_name;
    int line_number;
    const char *near_text;
};

struct str_list_entry_t
{
    char *str;
    struct list entry;
};

struct _attr_t {
  enum attr_type type;
  union {
    unsigned long ival;
    void *pval;
  } u;
  /* parser-internal */
  struct list entry;
};

struct _expr_t {
  enum expr_type type;
  const expr_t *ref;
  union {
    long lval;
    double dval;
    const char *sval;
    const expr_t *ext;
    type_t *tref;
  } u;
  const expr_t *ext2;
  int is_const;
  long cval;
  /* parser-internal */
  struct list entry;
};

struct struct_details
{
  var_list_t *fields;
};

struct enumeration_details
{
  var_list_t *enums;
};

struct func_details
{
  var_list_t *args;
  int idx;
};

struct iface_details
{
  statement_list_t *stmts;
  func_list_t *disp_methods;
  var_list_t *disp_props;
};

struct module_details
{
  statement_list_t *stmts;
  func_list_t *funcs;
};

struct array_details
{
  unsigned long dim;
  expr_t *size_is, *length_is;
};

struct coclass_details
{
  ifref_list_t *ifaces;
};

enum type_type
{
    TYPE_VOID,
    TYPE_BASIC, /* ints, floats and handles */
    TYPE_ENUM,
    TYPE_STRUCT,
    TYPE_ENCAPSULATED_UNION,
    TYPE_UNION,
    TYPE_ALIAS,
    TYPE_MODULE,
    TYPE_COCLASS,
    TYPE_FUNCTION,
    TYPE_INTERFACE,
    TYPE_POINTER,
    TYPE_ARRAY,
};

struct _type_t {
  const char *name;
  unsigned char type;
  struct _type_t *ref;
  attr_list_t *attrs;
  union
  {
    struct struct_details *structure;
    struct enumeration_details *enumeration;
    struct func_details *function;
    struct iface_details *iface;
    struct module_details *module;
    struct array_details array;
    struct coclass_details coclass;
  } details;
  type_t *orig;                   /* dup'd types */
  unsigned int typestring_offset;
  unsigned int ptrdesc;           /* used for complex structs */
  int typelib_idx;
  loc_info_t loc_info;
  unsigned int declarray : 1;     /* if declared as an array */
  unsigned int ignore : 1;
  unsigned int defined : 1;
  unsigned int written : 1;
  unsigned int user_types_registered : 1;
  unsigned int tfswrite : 1;   /* if the type needs to be written to the TFS */
  unsigned int checked : 1;
  unsigned int is_alias : 1; /* is the type an alias? */
  int sign : 2;
};

struct _var_t {
  char *name;
  type_t *type;
  attr_list_t *attrs;
  expr_t *eval;
  enum storage_class stgclass;

  struct _loc_info_t loc_info;

  /* parser-internal */
  struct list entry;
};

struct _declarator_t {
  var_t *var;
  type_t *type;
  type_t *func_type;
  array_dims_t *array;

  /* parser-internal */
  struct list entry;
};

struct _func_t {
  var_t *def;

  /* parser-internal */
  struct list entry;
};

struct _ifref_t {
  type_t *iface;
  attr_list_t *attrs;

  /* parser-internal */
  struct list entry;
};

struct _typelib_entry_t {
    type_t *type;
    struct list entry;
};

struct _importinfo_t {
    int offset;
    GUID guid;
    int flags;
    int id;

    char *name;

    importlib_t *importlib;
};

struct _importlib_t {
    char *name;

    int version;
    GUID guid;

    importinfo_t *importinfos;
    int ntypeinfos;

    int allocated;

    struct list entry;
};

struct _typelib_t {
    char *name;
    char *filename;
    const attr_list_t *attrs;
    struct list importlibs;
    statement_list_t *stmts;
};

struct _user_type_t {
    struct list entry;
    const char *name;
};

struct _type_list_t {
    type_t *type;
    struct _type_list_t *next;
};

struct _statement_t {
    struct list entry;
    enum statement_type type;
    union
    {
        ifref_t iface;
        type_t *type;
        const char *str;
        var_t *var;
        typelib_t *lib;
        type_list_t *type_list;
    } u;
};

extern unsigned char pointer_default;

extern user_type_list_t user_type_list;
void check_for_additional_prototype_types(const var_list_t *list);

void init_types(void);
type_t *alloc_type(void);
void set_all_tfswrite(int val);
void clear_all_offsets(void);

int is_ptr(const type_t *t);
int is_array(const type_t *t);
int is_var_ptr(const var_t *v);
int cant_be_null(const var_t *v);
int is_struct(unsigned char tc);
int is_union(unsigned char tc);

var_t *find_const(const char *name, int f);
type_t *find_type(const char *name, int t);
type_t *make_type(unsigned char type, type_t *ref);

void init_loc_info(loc_info_t *);

static inline var_list_t *type_get_function_args(const type_t *func_type)
{
  return func_type->details.function->args;
}

static inline enum type_type type_get_type_detect_alias(const type_t *type)
{
    if (type->is_alias)
        return TYPE_ALIAS;
    switch (type->type)
    {
    case 0:
        return TYPE_VOID;
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_USMALL:
    case RPC_FC_SMALL:
    case RPC_FC_WCHAR:
    case RPC_FC_USHORT:
    case RPC_FC_SHORT:
    case RPC_FC_ULONG:
    case RPC_FC_LONG:
    case RPC_FC_HYPER:
    case RPC_FC_IGNORE:
    case RPC_FC_FLOAT:
    case RPC_FC_DOUBLE:
    case RPC_FC_ERROR_STATUS_T:
    case RPC_FC_BIND_PRIMITIVE:
        return TYPE_BASIC;
    case RPC_FC_ENUM16:
    case RPC_FC_ENUM32:
        return TYPE_ENUM;
    case RPC_FC_RP:
    case RPC_FC_UP:
    case RPC_FC_FP:
    case RPC_FC_OP:
        return TYPE_POINTER;
    case RPC_FC_STRUCT:
    case RPC_FC_PSTRUCT:
    case RPC_FC_CSTRUCT:
    case RPC_FC_CPSTRUCT:
    case RPC_FC_CVSTRUCT:
    case RPC_FC_BOGUS_STRUCT:
        return TYPE_STRUCT;
    case RPC_FC_ENCAPSULATED_UNION:
        return TYPE_ENCAPSULATED_UNION;
    case RPC_FC_NON_ENCAPSULATED_UNION:
        return TYPE_UNION;
    case RPC_FC_SMFARRAY:
    case RPC_FC_LGFARRAY:
    case RPC_FC_SMVARRAY:
    case RPC_FC_LGVARRAY:
    case RPC_FC_CARRAY:
    case RPC_FC_CVARRAY:
    case RPC_FC_BOGUS_ARRAY:
        return TYPE_ARRAY;
    case RPC_FC_FUNCTION:
        return TYPE_FUNCTION;
    case RPC_FC_COCLASS:
        return TYPE_COCLASS;
    case RPC_FC_IP:
        return TYPE_INTERFACE;
    case RPC_FC_MODULE:
        return TYPE_MODULE;
    default:
        assert(0);
        return 0;
    }
}

#define STATEMENTS_FOR_EACH_FUNC(stmt, stmts) \
  if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, statement_t, entry ) \
    if (stmt->type == STMT_DECLARATION && stmt->u.var->stgclass == STG_NONE && \
        type_get_type_detect_alias(stmt->u.var->type) == TYPE_FUNCTION)

static inline int statements_has_func(const statement_list_t *stmts)
{
  const statement_t *stmt;
  int has_func = 0;
  STATEMENTS_FOR_EACH_FUNC(stmt, stmts)
  {
    has_func = 1;
    break;
  }
  return has_func;
}

#endif
