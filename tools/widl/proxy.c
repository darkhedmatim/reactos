/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
 * Copyright 2004 Mike McCormack
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

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typegen.h"
#include "expr.h"

static FILE* proxy;
static int indent = 0;

static void print_proxy( const char *format, ... ) __attribute__((format (printf, 1, 2)));
static void print_proxy( const char *format, ... )
{
  va_list va;
  va_start( va, format );
  print( proxy, indent, format, va );
  va_end( va );
}

static void write_stubdescproto(void)
{
  print_proxy( "static const MIDL_STUB_DESC Object_StubDesc;\n");
  print_proxy( "\n");
}

static void write_stubdesc(int expr_eval_routines)
{
  print_proxy( "static const MIDL_STUB_DESC Object_StubDesc =\n{\n");
  indent++;
  print_proxy( "0,\n");
  print_proxy( "NdrOleAllocate,\n");
  print_proxy( "NdrOleFree,\n");
  print_proxy( "{0}, 0, 0, %s, 0,\n", expr_eval_routines ? "ExprEvalRoutines" : "0");
  print_proxy( "__MIDL_TypeFormatString.Format,\n");
  print_proxy( "1, /* -error bounds_check flag */\n");
  print_proxy( "0x%x, /* Ndr library version */\n", get_stub_mode() == MODE_Oif ? 0x50002 : 0x10001);
  print_proxy( "0,\n");
  print_proxy( "0x50100a4, /* MIDL Version 5.1.164 */\n");
  print_proxy( "0,\n");
  print_proxy("%s,\n", list_empty(&user_type_list) ? "0" : "UserMarshalRoutines");
  print_proxy( "0,  /* notify & notify_flag routine table */\n");
  print_proxy( "1,  /* Flags */\n");
  print_proxy( "0,  /* Reserved3 */\n");
  print_proxy( "0,  /* Reserved4 */\n");
  print_proxy( "0   /* Reserved5 */\n");
  indent--;
  print_proxy( "};\n");
  print_proxy( "\n");
}

static void init_proxy(const statement_list_t *stmts)
{
  if (proxy) return;
  if(!(proxy = fopen(proxy_name, "w")))
    error("Could not open %s for output\n", proxy_name);
  print_proxy( "/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", PACKAGE_VERSION, input_name);
  print_proxy( "\n");
  print_proxy( "#define __midl_proxy\n\n");

  print_proxy( "#ifdef __REACTOS__\n");
  print_proxy( "#define WIN32_NO_STATUS\n");
  print_proxy( "#define WIN32_LEAN_AND_MEAN\n");
  print_proxy( "#endif\n\n");

  print_proxy( "#include \"objbase.h\"\n");
  print_proxy( "\n");
  print_proxy( "#ifndef DECLSPEC_HIDDEN\n");
  print_proxy( "#define DECLSPEC_HIDDEN\n");
  print_proxy( "#endif\n");
  print_proxy( "\n");
}

static void clear_output_vars( const var_list_t *args )
{
  const var_t *arg;

  if (!args) return;
  LIST_FOR_EACH_ENTRY( arg, args, const var_t, entry )
  {
      if (is_attr(arg->attrs, ATTR_IN)) continue;
      if (!is_attr(arg->attrs, ATTR_OUT)) continue;
      if (is_ptr(arg->type))
      {
          if (type_get_type(type_pointer_get_ref(arg->type)) == TYPE_BASIC) continue;
          if (type_get_type(type_pointer_get_ref(arg->type)) == TYPE_ENUM) continue;
      }
      print_proxy( "if (%s) MIDL_memset( %s, 0, sizeof( *%s ));\n", arg->name, arg->name, arg->name );
  }
}

static int need_delegation(const type_t *iface)
{
    const type_t *parent = type_iface_get_inherit( iface );
    return parent && type_iface_get_inherit(parent) && (parent->ignore || is_local( parent->attrs ));
}

static int get_delegation_indirect(const type_t *iface, const type_t ** delegate_to)
{
  const type_t * cur_iface;
  for (cur_iface = iface; cur_iface != NULL; cur_iface = type_iface_get_inherit(cur_iface))
    if (need_delegation(cur_iface))
    {
      if(delegate_to)
        *delegate_to = type_iface_get_inherit(cur_iface);
      return 1;
    }
  return 0;
}

static int need_delegation_indirect(const type_t *iface)
{
  return get_delegation_indirect(iface, NULL);
}

static void free_variable( const var_t *arg, const char *local_var_prefix )
{
  unsigned int type_offset = arg->typestring_offset;
  type_t *type = arg->type;

  write_parameter_conf_or_var_exprs(proxy, indent, local_var_prefix, PHASE_FREE, arg, FALSE);

  switch (typegen_detect_type(type, arg->attrs, TDT_IGNORE_STRINGS))
  {
  case TGT_ENUM:
  case TGT_BASIC:
    break;

  case TGT_STRUCT:
    if (get_struct_fc(type) != RPC_FC_STRUCT)
      print_proxy("/* FIXME: %s code for %s struct type 0x%x missing */\n", __FUNCTION__, arg->name, get_struct_fc(type) );
    break;

  case TGT_IFACE_POINTER:
  case TGT_POINTER:
  case TGT_ARRAY:
    print_proxy( "NdrClearOutParameters( &__frame->_StubMsg, ");
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%u], ", type_offset );
    fprintf(proxy, "(void *)%s );\n", arg->name );
    break;

  default:
    print_proxy("/* FIXME: %s code for %s type %d missing */\n", __FUNCTION__, arg->name, type_get_type(type) );
  }
}

static void proxy_free_variables( var_list_t *args, const char *local_var_prefix )
{
  const var_t *arg;

  if (!args) return;
  LIST_FOR_EACH_ENTRY( arg, args, const var_t, entry )
    if (is_attr(arg->attrs, ATTR_OUT))
    {
      free_variable( arg, local_var_prefix );
      fprintf(proxy, "\n");
    }
}

static void gen_proxy(type_t *iface, const var_t *func, int idx,
                      unsigned int proc_offset)
{
  var_t *retval = type_function_get_retval(func->type);
  int has_ret = !is_void(retval->type);
  int has_full_pointer = is_full_pointer_function(func);
  const char *callconv = get_attrp(func->type->attrs, ATTR_CALLCONV);
  const var_list_t *args = type_get_function_args(func->type);
  if (!callconv) callconv = "STDMETHODCALLTYPE";

  indent = 0;
  if (is_interpreted_func( iface, func ))
  {
      if (get_stub_mode() == MODE_Oif && !is_callas( func->attrs )) return;
      write_type_decl_left(proxy, retval->type);
      print_proxy( " %s %s_%s_Proxy(\n", callconv, iface->name, get_name(func));
      write_args(proxy, args, iface->name, 1, TRUE);
      print_proxy( ")\n");
      write_client_call_routine( proxy, iface, func, "Object", proc_offset );
      return;
  }
  print_proxy( "static void __finally_%s_%s_Proxy( struct __proxy_frame *__frame )\n",
               iface->name, get_name(func) );
  print_proxy( "{\n");
  indent++;
  if (has_full_pointer) write_full_pointer_free(proxy, indent, func);
  print_proxy( "NdrProxyFreeBuffer( __frame->This, &__frame->_StubMsg );\n" );
  indent--;
  print_proxy( "}\n");
  print_proxy( "\n");

  write_type_decl_left(proxy, retval->type);
  print_proxy( " %s %s_%s_Proxy(\n", callconv, iface->name, get_name(func));
  write_args(proxy, args, iface->name, 1, TRUE);
  print_proxy( ")\n");
  print_proxy( "{\n");
  indent ++;
  print_proxy( "struct __proxy_frame __f, * const __frame = &__f;\n" );
  /* local variables */
  if (has_ret) {
    print_proxy( "%s", "" );
    write_type_decl(proxy, retval->type, retval->name);
    fprintf( proxy, ";\n" );
  }
  print_proxy( "RPC_MESSAGE _RpcMessage;\n" );
  if (has_ret) {
    if (decl_indirect(retval->type))
        print_proxy("void *_p_%s = &%s;\n", retval->name, retval->name);
  }
  print_proxy( "\n");

  print_proxy( "RpcExceptionInit( __proxy_filter, __finally_%s_%s_Proxy );\n", iface->name, get_name(func) );
  print_proxy( "__frame->This = This;\n" );

  if (has_full_pointer)
    write_full_pointer_init(proxy, indent, func, FALSE);

  /* FIXME: trace */
  clear_output_vars( type_get_function_args(func->type) );

  print_proxy( "RpcTryExcept\n" );
  print_proxy( "{\n" );
  indent++;
  print_proxy( "NdrProxyInitialize(This, &_RpcMessage, &__frame->_StubMsg, &Object_StubDesc, %d);\n", idx);
  write_pointer_checks( proxy, indent, func );

  print_proxy( "RpcTryFinally\n" );
  print_proxy( "{\n" );
  indent++;

  write_remoting_arguments(proxy, indent, func, "", PASS_IN, PHASE_BUFFERSIZE);

  print_proxy( "NdrProxyGetBuffer(This, &__frame->_StubMsg);\n" );

  write_remoting_arguments(proxy, indent, func, "", PASS_IN, PHASE_MARSHAL);

  print_proxy( "NdrProxySendReceive(This, &__frame->_StubMsg);\n" );
  fprintf(proxy, "\n");
  print_proxy( "__frame->_StubMsg.BufferStart = _RpcMessage.Buffer;\n" );
  print_proxy( "__frame->_StubMsg.BufferEnd   = __frame->_StubMsg.BufferStart + _RpcMessage.BufferLength;\n\n" );

  print_proxy("if ((_RpcMessage.DataRepresentation & 0xffff) != NDR_LOCAL_DATA_REPRESENTATION)\n");
  indent++;
  print_proxy("NdrConvert( &__frame->_StubMsg, &__MIDL_ProcFormatString.Format[%u]);\n", proc_offset );
  indent--;
  fprintf(proxy, "\n");

  write_remoting_arguments(proxy, indent, func, "", PASS_OUT, PHASE_UNMARSHAL);

  if (has_ret)
  {
      if (decl_indirect(retval->type))
          print_proxy("MIDL_memset(&%s, 0, sizeof(%s));\n", retval->name, retval->name);
      else if (is_ptr(retval->type) || is_array(retval->type))
          print_proxy("%s = 0;\n", retval->name);
      write_remoting_arguments(proxy, indent, func, "", PASS_RETURN, PHASE_UNMARSHAL);
  }

  indent--;
  print_proxy( "}\n");
  print_proxy( "RpcFinally\n" );
  print_proxy( "{\n" );
  indent++;
  print_proxy( "__finally_%s_%s_Proxy( __frame );\n", iface->name, get_name(func) );
  indent--;
  print_proxy( "}\n");
  print_proxy( "RpcEndFinally\n" );
  indent--;
  print_proxy( "}\n" );
  print_proxy( "RpcExcept(__frame->_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)\n" );
  print_proxy( "{\n" );
  if (has_ret) {
    indent++;
    proxy_free_variables( type_get_function_args(func->type), "" );
    print_proxy( "_RetVal = NdrProxyErrorHandler(RpcExceptionCode());\n" );
    indent--;
  }
  print_proxy( "}\n" );
  print_proxy( "RpcEndExcept\n" );

  if (has_ret) {
    print_proxy( "return _RetVal;\n" );
  }
  indent--;
  print_proxy( "}\n");
  print_proxy( "\n");
}

static void gen_stub(type_t *iface, const var_t *func, const char *cas,
                     unsigned int proc_offset)
{
  const var_t *arg;
  int has_ret = !is_void(type_function_get_rettype(func->type));
  int has_full_pointer = is_full_pointer_function(func);

  if (is_interpreted_func( iface, func )) return;

  indent = 0;
  print_proxy( "struct __frame_%s_%s_Stub\n{\n", iface->name, get_name(func));
  indent++;
  print_proxy( "__DECL_EXCEPTION_FRAME\n" );
  print_proxy( "MIDL_STUB_MESSAGE _StubMsg;\n");
  print_proxy( "%s * _This;\n", iface->name );
  declare_stub_args( proxy, indent, func );
  indent--;
  print_proxy( "};\n\n" );

  print_proxy( "static void __finally_%s_%s_Stub(", iface->name, get_name(func) );
  print_proxy( " struct __frame_%s_%s_Stub *__frame )\n{\n", iface->name, get_name(func) );
  indent++;
  write_remoting_arguments(proxy, indent, func, "__frame->", PASS_OUT, PHASE_FREE);
  if (has_full_pointer)
    write_full_pointer_free(proxy, indent, func);
  indent--;
  print_proxy( "}\n\n" );

  print_proxy( "void __RPC_STUB %s_%s_Stub(\n", iface->name, get_name(func));
  indent++;
  print_proxy( "IRpcStubBuffer* This,\n");
  print_proxy( "IRpcChannelBuffer *_pRpcChannelBuffer,\n");
  print_proxy( "PRPC_MESSAGE _pRpcMessage,\n");
  print_proxy( "DWORD* _pdwStubPhase)\n");
  indent--;
  print_proxy( "{\n");
  indent++;
  print_proxy( "struct __frame_%s_%s_Stub __f, * const __frame = &__f;\n\n",
               iface->name, get_name(func) );

  print_proxy("__frame->_This = (%s*)((CStdStubBuffer*)This)->pvServerObject;\n\n", iface->name);

  /* FIXME: trace */

  print_proxy("NdrStubInitialize(_pRpcMessage, &__frame->_StubMsg, &Object_StubDesc, _pRpcChannelBuffer);\n");
  fprintf(proxy, "\n");
  print_proxy( "RpcExceptionInit( 0, __finally_%s_%s_Stub );\n", iface->name, get_name(func) );

  write_parameters_init(proxy, indent, func, "__frame->");

  print_proxy("RpcTryFinally\n");
  print_proxy("{\n");
  indent++;
  if (has_full_pointer)
    write_full_pointer_init(proxy, indent, func, TRUE);
  print_proxy("if ((_pRpcMessage->DataRepresentation & 0xffff) != NDR_LOCAL_DATA_REPRESENTATION)\n");
  indent++;
  print_proxy("NdrConvert( &__frame->_StubMsg, &__MIDL_ProcFormatString.Format[%u]);\n", proc_offset );
  indent--;
  fprintf(proxy, "\n");

  write_remoting_arguments(proxy, indent, func, "__frame->", PASS_IN, PHASE_UNMARSHAL);
  fprintf(proxy, "\n");

  assign_stub_out_args( proxy, indent, func, "__frame->" );

  print_proxy("*_pdwStubPhase = STUB_CALL_SERVER;\n");
  fprintf(proxy, "\n");
  print_proxy( "%s", has_ret ? "__frame->_RetVal = " : "" );
  if (cas) fprintf(proxy, "%s_%s_Stub", iface->name, cas);
  else fprintf(proxy, "__frame->_This->lpVtbl->%s", get_name(func));
  fprintf(proxy, "(__frame->_This");

  if (type_get_function_args(func->type))
  {
      LIST_FOR_EACH_ENTRY( arg, type_get_function_args(func->type), const var_t, entry )
          fprintf(proxy, ", %s__frame->%s", is_array(arg->type) && !type_array_is_decl_as_ptr(arg->type) ? "*" :"" , arg->name);
  }
  fprintf(proxy, ");\n");
  fprintf(proxy, "\n");
  print_proxy("*_pdwStubPhase = STUB_MARSHAL;\n");
  fprintf(proxy, "\n");

  write_remoting_arguments(proxy, indent, func, "__frame->", PASS_OUT, PHASE_BUFFERSIZE);

  if (!is_void(type_function_get_rettype(func->type)))
    write_remoting_arguments(proxy, indent, func, "__frame->", PASS_RETURN, PHASE_BUFFERSIZE);

  print_proxy("NdrStubGetBuffer(This, _pRpcChannelBuffer, &__frame->_StubMsg);\n");

  write_remoting_arguments(proxy, indent, func, "__frame->", PASS_OUT, PHASE_MARSHAL);
  fprintf(proxy, "\n");

  /* marshall the return value */
  if (!is_void(type_function_get_rettype(func->type)))
    write_remoting_arguments(proxy, indent, func, "__frame->", PASS_RETURN, PHASE_MARSHAL);

  indent--;
  print_proxy("}\n");
  print_proxy("RpcFinally\n");
  print_proxy("{\n");
  indent++;
  print_proxy( "__finally_%s_%s_Stub( __frame );\n", iface->name, get_name(func) );
  indent--;
  print_proxy("}\n");
  print_proxy("RpcEndFinally\n");

  print_proxy("_pRpcMessage->BufferLength = __frame->_StubMsg.Buffer - (unsigned char *)_pRpcMessage->Buffer;\n");
  indent--;

  print_proxy("}\n");
  print_proxy("\n");
}

static void gen_stub_thunk( type_t *iface, const var_t *func, unsigned int proc_offset )
{
    int has_ret = !is_void( type_function_get_rettype( func->type ));
    const var_t *arg, *callas = is_callas( func->attrs );
    const var_list_t *args = type_get_function_args( func->type );

    indent = 0;
    print_proxy( "void __RPC_API %s_%s_Thunk( PMIDL_STUB_MESSAGE pStubMsg )\n",
                 iface->name, get_name(func) );
    print_proxy( "{\n");
    indent++;
    write_func_param_struct( proxy, iface, func->type,
                             "*pParamStruct = (struct _PARAM_STRUCT *)pStubMsg->StackTop", has_ret );
    print_proxy( "%s%s_%s_Stub( pParamStruct->This",
                 has_ret ? "pParamStruct->_RetVal = " : "", iface->name, callas->name );
    indent++;
    if (args) LIST_FOR_EACH_ENTRY( arg, args, const var_t, entry )
    {
        fprintf( proxy, ",\n%*spParamStruct->%s", 4 * indent, "", arg->name );
    }
    fprintf( proxy, " );\n" );
    indent--;
    indent--;
    print_proxy( "}\n\n");
}

int count_methods(const type_t *iface)
{
    const statement_t *stmt;
    int count = 0;

    if (type_iface_get_inherit(iface))
        count = count_methods(type_iface_get_inherit(iface));

    STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface)) {
        const var_t *func = stmt->u.var;
        if (!is_callas(func->attrs)) count++;
    }
    return count;
}

static const statement_t * get_callas_source(const type_t * iface, const var_t * def)
{
  const statement_t * source;
  STATEMENTS_FOR_EACH_FUNC( source, type_iface_get_stmts(iface)) {
    const var_t * cas = is_callas(source->u.var->attrs );
    if (cas && !strcmp(def->name, cas->name))
      return source;
  }
  return NULL;
}

static int write_proxy_procformatstring_offsets( const type_t *iface, int skip )
{
    const statement_t *stmt;
    int i = 0;

    if (type_iface_get_inherit(iface))
        i = write_proxy_procformatstring_offsets( type_iface_get_inherit(iface), need_delegation(iface));
    else
        return 0;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        const var_t *func = stmt->u.var;
        int missing = 0;

        if (is_callas(func->attrs)) continue;
        if (is_local(func->attrs))
        {
            const statement_t * callas_source = get_callas_source(iface, func);
            if (!callas_source)
                missing = 1;
            else
                func = callas_source->u.var;
        }
        if (skip || missing)
            print_proxy( "(unsigned short)-1,  /* %s::%s */\n", iface->name, get_name(func));
        else
            print_proxy( "%u,  /* %s::%s */\n", func->procstring_offset, iface->name, get_name(func));
        i++;
    }
    return i;
}

static int write_proxy_methods(type_t *iface, int skip)
{
  const statement_t *stmt;
  int i = 0;

  if (type_iface_get_inherit(iface))
    i = write_proxy_methods(type_iface_get_inherit(iface),
                            need_delegation(iface));
  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface)) {
    const var_t *func = stmt->u.var;
    if (!is_callas(func->attrs)) {
      if (skip || (is_local(func->attrs) && !get_callas_source(iface, func)))
          print_proxy( "0,  /* %s::%s */\n", iface->name, get_name(func));
      else if (is_interpreted_func( iface, func ) &&
               !is_local( func->attrs ) &&
               type_iface_get_inherit(iface))
          print_proxy( "(void *)-1,  /* %s::%s */\n", iface->name, get_name(func));
      else
          print_proxy( "%s_%s_Proxy,\n", iface->name, get_name(func));
      i++;
    }
  }
  return i;
}

static int write_stub_methods(type_t *iface, int skip)
{
  const statement_t *stmt;
  int i = 0;

  if (type_iface_get_inherit(iface))
    i = write_stub_methods(type_iface_get_inherit(iface), need_delegation(iface));
  else
    return i; /* skip IUnknown */

  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface)) {
    const var_t *func = stmt->u.var;
    if (!is_callas(func->attrs)) {
      int missing = 0;
      const char * fname = get_name(func);
      if(is_local(func->attrs)) {
        const statement_t * callas_source = get_callas_source(iface, func);
        if(!callas_source)
          missing = 1;
        else
          fname = get_name(callas_source->u.var);
      }
      if (i) fprintf(proxy,",\n");
      if (skip || missing) print_proxy("STUB_FORWARDING_FUNCTION");
      else if (is_interpreted_func( iface, func ))
          print_proxy( "(PRPC_STUB_FUNCTION)%s", get_stub_mode() == MODE_Oif ? "NdrStubCall2" : "NdrStubCall" );
      else print_proxy( "%s_%s_Stub", iface->name, fname);
      i++;
    }
  }
  return i;
}

static void write_thunk_methods( type_t *iface, int skip )
{
    const statement_t *stmt;

    if (type_iface_get_inherit( iface ))
        write_thunk_methods( type_iface_get_inherit(iface), need_delegation(iface) );
    else
        return; /* skip IUnknown */

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *func = stmt->u.var;
        const statement_t * callas_source = NULL;

        if (is_callas(func->attrs)) continue;
        if (is_local(func->attrs)) callas_source = get_callas_source(iface, func);

        if (!skip && callas_source && is_interpreted_func( iface, func ))
            print_proxy( "%s_%s_Thunk,\n", iface->name, get_name(callas_source->u.var) );
        else
            print_proxy( "0, /* %s::%s */\n", iface->name, get_name(func) );
    }
}

static void write_proxy(type_t *iface, unsigned int *proc_offset)
{
  int count;
  const statement_t *stmt;
  int first_func = 1;
  int needs_stub_thunks = 0;
  int needs_inline_stubs = need_inline_stubs( iface ) || need_delegation( iface );

  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface)) {
    var_t *func = stmt->u.var;
    if (first_func) {
      fprintf(proxy, "/*****************************************************************************\n");
      fprintf(proxy, " * %s interface\n", iface->name);
      fprintf(proxy, " */\n");
      first_func = 0;
    }
    if (!is_local(func->attrs)) {
      const var_t *cas = is_callas(func->attrs);
      const char *cname = cas ? cas->name : NULL;
      int idx = func->type->details.function->idx;
      if (cname) {
          const statement_t *stmt2;
          STATEMENTS_FOR_EACH_FUNC(stmt2, type_iface_get_stmts(iface)) {
              const var_t *m = stmt2->u.var;
              if (!strcmp(m->name, cname))
              {
                  idx = m->type->details.function->idx;
                  break;
              }
          }
      }
      func->procstring_offset = *proc_offset;
      gen_proxy(iface, func, idx, *proc_offset);
      gen_stub(iface, func, cname, *proc_offset);
      if (cas && is_interpreted_func( iface, func ))
      {
          needs_stub_thunks = 1;
          gen_stub_thunk(iface, func, *proc_offset);
      }
      *proc_offset += get_size_procformatstring_func( iface, func );
    }
  }

  count = count_methods(iface);

  print_proxy( "static const unsigned short %s_FormatStringOffsetTable[] =\n", iface->name );
  print_proxy( "{\n" );
  indent++;
  if (write_proxy_procformatstring_offsets( iface, 0 ) == 0)
  {
      print_proxy( "0\n" );
  }
  indent--;
  print_proxy( "};\n\n" );

  /* proxy info */
  if (get_stub_mode() == MODE_Oif)
  {
      print_proxy( "static const MIDL_STUBLESS_PROXY_INFO %s_ProxyInfo =\n", iface->name );
      print_proxy( "{\n" );
      indent++;
      print_proxy( "&Object_StubDesc,\n" );
      print_proxy( "__MIDL_ProcFormatString.Format,\n" );
      print_proxy( "&%s_FormatStringOffsetTable[-3],\n", iface->name );
      print_proxy( "0,\n" );
      print_proxy( "0,\n" );
      print_proxy( "0\n" );
      indent--;
      print_proxy( "};\n\n" );
  }

  /* proxy vtable */
  print_proxy( "static %sCINTERFACE_PROXY_VTABLE(%d) _%sProxyVtbl =\n",
               need_delegation_indirect(iface) ? "" : "const ", count, iface->name);
  print_proxy( "{\n");
  indent++;
  print_proxy( "{\n");
  indent++;
  if (get_stub_mode() == MODE_Oif) print_proxy( "&%s_ProxyInfo,\n", iface->name );
  print_proxy( "&IID_%s,\n", iface->name);
  indent--;
  print_proxy( "},\n");
  print_proxy( "{\n");
  indent++;
  write_proxy_methods(iface, FALSE);
  indent--;
  print_proxy( "}\n");
  indent--;
  print_proxy( "};\n\n");

  /* stub thunk table */
  if (needs_stub_thunks)
  {
      print_proxy( "static const STUB_THUNK %s_StubThunkTable[] =\n", iface->name);
      print_proxy( "{\n");
      indent++;
      write_thunk_methods( iface, 0 );
      indent--;
      print_proxy( "};\n\n");
  }

  /* server info */
  print_proxy( "static const MIDL_SERVER_INFO %s_ServerInfo =\n", iface->name );
  print_proxy( "{\n" );
  indent++;
  print_proxy( "&Object_StubDesc,\n" );
  print_proxy( "0,\n" );
  print_proxy( "__MIDL_ProcFormatString.Format,\n" );
  print_proxy( "&%s_FormatStringOffsetTable[-3],\n", iface->name );
  if (needs_stub_thunks)
      print_proxy( "&%s_StubThunkTable[-3],\n", iface->name );
  else
      print_proxy( "0,\n" );
  print_proxy( "0,\n" );
  print_proxy( "0,\n" );
  print_proxy( "0\n" );
  indent--;
  print_proxy( "};\n\n" );

  /* stub vtable */
  if (needs_inline_stubs)
  {
      print_proxy( "static const PRPC_STUB_FUNCTION %s_table[] =\n", iface->name);
      print_proxy( "{\n");
      indent++;
      if (write_stub_methods(iface, FALSE) == 0)
      {
          fprintf(proxy, "0");
      }
      fprintf(proxy, "\n");
      indent--;
      fprintf(proxy, "};\n\n");
  }
  print_proxy( "static %sCInterfaceStubVtbl _%sStubVtbl =\n",
               need_delegation_indirect(iface) ? "" : "const ", iface->name);
  print_proxy( "{\n");
  indent++;
  print_proxy( "{\n");
  indent++;
  print_proxy( "&IID_%s,\n", iface->name);
  print_proxy( "&%s_ServerInfo,\n", iface->name );
  print_proxy( "%d,\n", count);
  if (needs_inline_stubs) print_proxy( "&%s_table[-3]\n", iface->name );
  else print_proxy( "0\n" );
  indent--;
  print_proxy( "},\n");
  print_proxy( "{\n");
  indent++;
  print_proxy( "CStdStubBuffer_%s\n", need_delegation_indirect(iface) ? "DELEGATING_METHODS" : "METHODS");
  indent--;
  print_proxy( "}\n");
  indent--;
  print_proxy( "};\n");
  print_proxy( "\n");
}

static int does_any_iface(const statement_list_t *stmts, type_pred_t pred)
{
  const statement_t *stmt;

  if (stmts)
    LIST_FOR_EACH_ENTRY(stmt, stmts, const statement_t, entry)
    {
      if (stmt->type == STMT_TYPE && type_get_type(stmt->u.type) == TYPE_INTERFACE)
      {
        if (pred(stmt->u.type))
          return TRUE;
      }
    }

  return FALSE;
}

int need_proxy(const type_t *iface)
{
    if (!is_object( iface )) return 0;
    if (is_local( iface->attrs )) return 0;
    if (is_attr( iface->attrs, ATTR_DISPINTERFACE )) return 0;
    return 1;
}

int need_stub(const type_t *iface)
{
  return !is_object(iface) && !is_local(iface->attrs);
}

int need_proxy_file(const statement_list_t *stmts)
{
    return does_any_iface(stmts, need_proxy);
}

int need_proxy_delegation(const statement_list_t *stmts)
{
    return does_any_iface(stmts, need_delegation);
}

int need_inline_stubs(const type_t *iface)
{
    const statement_t *stmt;

    if (get_stub_mode() == MODE_Os) return 1;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        const var_t *func = stmt->u.var;
        if (is_local( func->attrs )) continue;
        if (!is_interpreted_func( iface, func )) return 1;
    }
    return 0;
}

static int need_proxy_and_inline_stubs(const type_t *iface)
{
    const statement_t *stmt;

    if (!need_proxy( iface )) return 0;
    if (get_stub_mode() == MODE_Os) return 1;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        const var_t *func = stmt->u.var;
        if (is_local( func->attrs )) continue;
        if (!is_interpreted_func( iface, func )) return 1;
    }
    return 0;
}

int need_stub_files(const statement_list_t *stmts)
{
  return does_any_iface(stmts, need_stub);
}

int need_inline_stubs_file(const statement_list_t *stmts)
{
  return does_any_iface(stmts, need_inline_stubs);
}

static void write_proxy_stmts(const statement_list_t *stmts, unsigned int *proc_offset)
{
  const statement_t *stmt;
  if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
  {
    if (stmt->type == STMT_TYPE && type_get_type(stmt->u.type) == TYPE_INTERFACE)
    {
      if (need_proxy(stmt->u.type))
        write_proxy(stmt->u.type, proc_offset);
    }
  }
}

static int cmp_iid( const void *ptr1, const void *ptr2 )
{
    const type_t * const *iface1 = ptr1;
    const type_t * const *iface2 = ptr2;
    const UUID *uuid1 = get_attrp( (*iface1)->attrs, ATTR_UUID );
    const UUID *uuid2 = get_attrp( (*iface2)->attrs, ATTR_UUID );
    return memcmp( uuid1, uuid2, sizeof(UUID) );
}

static void build_iface_list( const statement_list_t *stmts, type_t **ifaces[], int *count )
{
    const statement_t *stmt;

    if (!stmts) return;
    LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE && type_get_type(stmt->u.type) == TYPE_INTERFACE)
        {
            type_t *iface = stmt->u.type;
            if (type_iface_get_inherit(iface) && need_proxy(iface))
            {
                *ifaces = xrealloc( *ifaces, (*count + 1) * sizeof(**ifaces) );
                (*ifaces)[(*count)++] = iface;
            }
        }
    }
}

static type_t **sort_interfaces( const statement_list_t *stmts, int *count )
{
    type_t **ifaces = NULL;

    *count = 0;
    build_iface_list( stmts, &ifaces, count );
    qsort( ifaces, *count, sizeof(*ifaces), cmp_iid );
    return ifaces;
}

static void write_proxy_routines(const statement_list_t *stmts)
{
  int expr_eval_routines;
  unsigned int proc_offset = 0;
  char *file_id = proxy_token;
  int i, count, have_baseiid = 0;
  type_t **interfaces;
  const type_t * delegate_to;

  print_proxy( "#ifndef __REDQ_RPCPROXY_H_VERSION__\n");
  print_proxy( "#define __REQUIRED_RPCPROXY_H_VERSION__ %u\n", get_stub_mode() == MODE_Oif ? 475 : 440);
  print_proxy( "#endif\n");
  print_proxy( "\n");
  if (get_stub_mode() == MODE_Oif) print_proxy( "#define USE_STUBLESS_PROXY\n");
  print_proxy( "#include \"rpcproxy.h\"\n");
  print_proxy( "#ifndef __RPCPROXY_H_VERSION__\n");
  print_proxy( "#error This code needs a newer version of rpcproxy.h\n");
  print_proxy( "#endif /* __RPCPROXY_H_VERSION__ */\n");
  print_proxy( "\n");
  print_proxy( "#include \"%s\"\n", header_name);
  print_proxy( "\n");

  if (does_any_iface(stmts, need_proxy_and_inline_stubs))
  {
      write_exceptions( proxy );
      print_proxy( "\n");
      print_proxy( "struct __proxy_frame\n");
      print_proxy( "{\n");
      print_proxy( "    __DECL_EXCEPTION_FRAME\n");
      print_proxy( "    MIDL_STUB_MESSAGE _StubMsg;\n");
      print_proxy( "    void             *This;\n");
      print_proxy( "};\n");
      print_proxy( "\n");
      print_proxy("static int __proxy_filter( struct __proxy_frame *__frame )\n");
      print_proxy( "{\n");
      print_proxy( "    return (__frame->_StubMsg.dwStubPhase != PROXY_SENDRECEIVE);\n");
      print_proxy( "}\n");
      print_proxy( "\n");
  }

  write_formatstringsdecl(proxy, indent, stmts, need_proxy);
  write_stubdescproto();
  write_proxy_stmts(stmts, &proc_offset);

  expr_eval_routines = write_expr_eval_routines(proxy, proxy_token);
  if (expr_eval_routines)
      write_expr_eval_routine_list(proxy, proxy_token);
  write_user_quad_list(proxy);
  write_stubdesc(expr_eval_routines);

  print_proxy( "#if !defined(__RPC_WIN%u__)\n", pointer_size == 8 ? 64 : 32);
  print_proxy( "#error Currently only Wine and WIN32 are supported.\n");
  print_proxy( "#endif\n");
  print_proxy( "\n");
  write_procformatstring(proxy, stmts, need_proxy);
  write_typeformatstring(proxy, stmts, need_proxy);

  interfaces = sort_interfaces(stmts, &count);
  fprintf(proxy, "static const CInterfaceProxyVtbl* const _%s_ProxyVtblList[] =\n", file_id);
  fprintf(proxy, "{\n");
  for (i = 0; i < count; i++)
      fprintf(proxy, "    (const CInterfaceProxyVtbl*)&_%sProxyVtbl,\n", interfaces[i]->name);
  fprintf(proxy, "    0\n");
  fprintf(proxy, "};\n");
  fprintf(proxy, "\n");

  fprintf(proxy, "static const CInterfaceStubVtbl* const _%s_StubVtblList[] =\n", file_id);
  fprintf(proxy, "{\n");
  for (i = 0; i < count; i++)
      fprintf(proxy, "    &_%sStubVtbl,\n", interfaces[i]->name);
  fprintf(proxy, "    0\n");
  fprintf(proxy, "};\n");
  fprintf(proxy, "\n");

  fprintf(proxy, "static PCInterfaceName const _%s_InterfaceNamesList[] =\n", file_id);
  fprintf(proxy, "{\n");
  for (i = 0; i < count; i++)
      fprintf(proxy, "    \"%s\",\n", interfaces[i]->name);
  fprintf(proxy, "    0\n");
  fprintf(proxy, "};\n");
  fprintf(proxy, "\n");

  for (i = 0; i < count; i++)
      if ((have_baseiid = get_delegation_indirect( interfaces[i], NULL ))) break;

  if (have_baseiid)
  {
      fprintf(proxy, "static const IID * _%s_BaseIIDList[] =\n", file_id);
      fprintf(proxy, "{\n");
      for (i = 0; i < count; i++)
      {
          if (get_delegation_indirect(interfaces[i], &delegate_to))
              fprintf( proxy, "    &IID_%s,  /* %s */\n", delegate_to->name, interfaces[i]->name );
          else
              fprintf( proxy, "    0,\n" );
      }
      fprintf(proxy, "    0\n");
      fprintf(proxy, "};\n");
      fprintf(proxy, "\n");
  }

  fprintf(proxy, "static int __stdcall _%s_IID_Lookup(const IID* pIID, int* pIndex)\n", file_id);
  fprintf(proxy, "{\n");
  fprintf(proxy, "    int low = 0, high = %d;\n", count - 1);
  fprintf(proxy, "\n");
  fprintf(proxy, "    while (low <= high)\n");
  fprintf(proxy, "    {\n");
  fprintf(proxy, "        int pos = (low + high) / 2;\n");
  fprintf(proxy, "        int res = IID_GENERIC_CHECK_IID(_%s, pIID, pos);\n", file_id);
  fprintf(proxy, "        if (!res) { *pIndex = pos; return 1; }\n");
  fprintf(proxy, "        if (res > 0) low = pos + 1;\n");
  fprintf(proxy, "        else high = pos - 1;\n");
  fprintf(proxy, "    }\n");
  fprintf(proxy, "    return 0;\n");
  fprintf(proxy, "}\n");
  fprintf(proxy, "\n");

  fprintf(proxy, "const ExtendedProxyFileInfo %s_ProxyFileInfo DECLSPEC_HIDDEN =\n", file_id);
  fprintf(proxy, "{\n");
  fprintf(proxy, "    (const PCInterfaceProxyVtblList*)_%s_ProxyVtblList,\n", file_id);
  fprintf(proxy, "    (const PCInterfaceStubVtblList*)_%s_StubVtblList,\n", file_id);
  fprintf(proxy, "    _%s_InterfaceNamesList,\n", file_id);
  if (have_baseiid) fprintf(proxy, "    _%s_BaseIIDList,\n", file_id);
  else fprintf(proxy, "    0,\n");
  fprintf(proxy, "    _%s_IID_Lookup,\n", file_id);
  fprintf(proxy, "    %d,\n", count);
  fprintf(proxy, "    %d,\n", get_stub_mode() == MODE_Oif ? 2 : 1);
  fprintf(proxy, "    0,\n");
  fprintf(proxy, "    0,\n");
  fprintf(proxy, "    0,\n");
  fprintf(proxy, "    0\n");
  fprintf(proxy, "};\n");
}

void write_proxies(const statement_list_t *stmts)
{
  if (!do_proxies) return;
  if (do_everything && !need_proxy_file(stmts)) return;

  init_proxy(stmts);
  if(!proxy) return;

  if (do_win32 && do_win64)
  {
      fprintf(proxy, "\n#ifndef _WIN64\n\n");
      pointer_size = 4;
      write_proxy_routines( stmts );
      fprintf(proxy, "\n#else /* _WIN64 */\n\n");
      pointer_size = 8;
      write_proxy_routines( stmts );
      fprintf(proxy, "\n#endif /* _WIN64 */\n");
  }
  else if (do_win32)
  {
      pointer_size = 4;
      write_proxy_routines( stmts );
  }
  else if (do_win64)
  {
      pointer_size = 8;
      write_proxy_routines( stmts );
  }

  fclose(proxy);
}
