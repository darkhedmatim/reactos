/*
 * IDL Compiler
 *
 * Copyright 2005 Eric Kohl
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"

#define END_OF_LIST(list)       \
  do {                          \
    if (list) {                 \
      while (NEXT_LINK(list))   \
        list = NEXT_LINK(list); \
    }                           \
  } while(0)

static FILE* client;
static int indent = 0;

static int print_client( const char *format, ... )
{
    va_list va;
    int i, r;

    va_start(va, format);
    for (i = 0; i < indent; i++)
        fprintf(client, "    ");
    r = vfprintf(client, format, va);
    va_end(va);
    return r;
}


static void write_procformatstring(type_t *iface)
{
    func_t *func = iface->funcs;
    var_t *var;

    print_client("static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =\n");
    print_client("{\n");
    indent++;
    print_client("0,\n");
    print_client("{\n");
    indent++;

    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        /* emit argument data */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                switch(var->type->type)
                {
                case RPC_FC_BYTE:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_BYTE */\n", RPC_FC_BYTE);
                    break;
                case RPC_FC_CHAR:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_CHAR */\n", RPC_FC_CHAR);
                    break;
                case RPC_FC_WCHAR:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_WCHAR */\n", RPC_FC_WCHAR);
                    break;
                case RPC_FC_USHORT:
                case RPC_FC_SHORT:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_USHORT */\n", RPC_FC_SHORT);
                    break;
                case RPC_FC_ULONG:
                case RPC_FC_LONG:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_LONG */\n", RPC_FC_LONG);
                    break;
                case RPC_FC_HYPER:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_HYPER */\n", RPC_FC_HYPER);
                    break;
                case RPC_FC_IGNORE:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_IGNORE */\n", RPC_FC_IGNORE);
                    break;
                case RPC_FC_SMALL:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_SMALL */\n", RPC_FC_SMALL);
                    break;
                case RPC_FC_FLOAT:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_FLOAT */\n", RPC_FC_FLOAT);
                    break;
                case RPC_FC_DOUBLE:
                    print_client("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                    print_client("0x%02x,    /* FC_DOUBLE */\n", RPC_FC_DOUBLE);
                    break;
                default:
                    error("Unknown/unsupported type\n");
                    return;
                }

                var = PREV_LINK(var);
            }
        }

        /* emit return value data */
        var = func->def;
        if (is_void(var->type, NULL))
        {
            print_client("0x5b,    /* FC_END */\n");
            print_client("0x5c,    /* FC_PAD */\n");
        }
        else
        {
            switch(var->type->type)
            {
            case RPC_FC_BYTE:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_BYTE */\n", var->type->type);
                break;
            case RPC_FC_CHAR:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_CHAR */\n", var->type->type);
                break;
            case RPC_FC_WCHAR:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_WCHAR */\n", var->type->type);
                break;
            case RPC_FC_USHORT:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_USHORT */\n", var->type->type);
                break;
            case RPC_FC_SHORT:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_SHORT */\n", var->type->type);
                break;
            case RPC_FC_ULONG:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_ULONG */\n", var->type->type);
                break;
            case RPC_FC_LONG:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_LONG */\n", var->type->type);
                break;
            case RPC_FC_HYPER:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_HYPER */\n", var->type->type);
                break;
            case RPC_FC_SMALL:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_SMALL */\n", RPC_FC_SMALL);
                break;
            case RPC_FC_FLOAT:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_FLOAT */\n", RPC_FC_FLOAT);
                break;
            case RPC_FC_DOUBLE:
                print_client("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
                print_client("0x%02x,    /* FC_DOUBLE */\n", RPC_FC_DOUBLE);
                break;
            default:
                error("Unknown/unsupported type\n");
                return;
            }
        }

        func = PREV_LINK(func);
    }

    print_client("0x0\n");
    indent--;
    print_client("}\n");
    indent--;
    print_client("};\n");
    print_client("\n");
}


static void write_typeformatstring(void)
{
    print_client("static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =\n");
    print_client("{\n");
    indent++;
    print_client("0,\n");
    print_client("{\n");
    indent++;
    print_client("NdrFcShort(0x0),\n");
    print_client("0x0\n");
    indent--;
    print_client("}\n");
    indent--;
    print_client("};\n");
    print_client("\n");
}


static void print_message_buffer_size(func_t *func)
{
    unsigned int alignment;
    int size;
    int last_size = -1;
    var_t *var;

    if (!func->args)
    {
        fprintf(client, " 0U");
        return;
    }

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    while (var)
    {
        alignment = 0;
        switch (var->type->type)
        {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_SMALL:
            size = 1;
            alignment = 0;
            break;

        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
            size = 2;
            if (last_size != -1 && last_size < 2)
                alignment += (2 - last_size);
            break;

        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_FLOAT:
            size = 4;
            if (last_size != -1 && last_size < 4)
                alignment += (4 - last_size);
            break;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            size = 8;
            if (last_size != -1 && last_size < 4)
                alignment += (4 - last_size);
            break;

        case RPC_FC_IGNORE:
            size = 0;
            break;

        default:
            error("Unknown/unsupported type!");
        }

        if (last_size != -1)
            fprintf(client, " +");
        fprintf(client, " %dU", (size == 0) ? 0 : size + alignment);

        last_size = size;

        var = PREV_LINK(var);
    }
}


static void marshall_arguments(func_t *func)
{
    unsigned int alignment;
    unsigned int size;
    unsigned int last_size = 0;
    var_t *var;

    if (!func->args)
        return;

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    while (var)
    {
        alignment = 0;
        switch (var->type->type)
        {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_SMALL:
            size = 1;
            alignment = 0;
            break;

        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
            size = 2;
            if (last_size != 0 && last_size < 2)
                alignment = (2 - last_size);
            break;

        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_FLOAT:
            size = 4;
            if (last_size != 0 && last_size < 4)
                alignment = (4 - last_size);
            break;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            size = 8;
            if (last_size != 0 && last_size < 4)
                alignment = (4 - last_size);
            break;

        case RPC_FC_IGNORE:
            size = 0;
            break;

        default:
            error("Unknown/unsupported type!");
        }

        if (size != 0)
        {
            if (alignment != 0)
                print_client("_StubMsg.Buffer += %u;\n", alignment);

            print_client("*((");
            write_type(client, var->type, var, var->tname);
            fprintf(client, " __RPC_FAR*)_StubMsg.Buffer)++ = ");
            write_name(client, var);
            fprintf(client, ";\n");
            fprintf(client, "\n");

            last_size = size;
        }

        var = PREV_LINK(var);
    }
}


static void write_function_stubs(type_t *iface)
{
    char *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);
    int explicit_handle = is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE);
    func_t *func = iface->funcs;
    var_t* var;
    var_t* explicit_handle_var;
    int method_count = 0;
    unsigned int proc_offset = 0;

    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        var_t *def = func->def;

        /* check for a defined binding handle */
        explicit_handle_var = get_explicit_handle_var(func);
        if (explicit_handle)
        {
            if (!explicit_handle_var)
            {
                error("%s() does not define an explicit binding handle!\n", def->name);
                return;
            }
        }
        else
        {
            if (explicit_handle_var)
            {
                error("%s() must not define a binding handle!\n", def->name);
                return;
            }
        }

        write_type(client, def->type, def, def->tname);
        fprintf(client, " ");
        write_name(client, def);
        fprintf(client, "(\n");
        indent++;
        write_args(client, func->args, iface->name, 0, TRUE);
        fprintf(client, ")\n");
        indent--;

        /* write the functions body */
        fprintf(client, "{\n");
        indent++;

        /* declare return value '_RetVal' */
        if (!is_void(def->type, NULL))
        {
            print_client("");
            write_type(client, def->type, def, def->tname);
            fprintf(client, " _RetVal;\n");
        }

        if (implicit_handle || explicit_handle)
            print_client("RPC_BINDING_HANDLE _Handle = 0;\n");
        print_client("RPC_MESSAGE _RpcMessage;\n");
        print_client("MIDL_STUB_MESSAGE _StubMsg;\n");
        fprintf(client, "\n");
        print_client("RpcTryFinally\n");
        print_client("{\n");
        indent++;

        print_client("NdrClientInitializeNew(\n");
        indent++;
        print_client("(PRPC_MESSAGE)&_RpcMessage,\n");
        print_client("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
        print_client("(PMIDL_STUB_DESC)&%s_StubDesc,\n", iface->name);
        print_client("%d);\n", method_count);
        indent--;
        fprintf(client, "\n");

        if (implicit_handle)
        {
            print_client("_Handle = %s;\n", implicit_handle);
            fprintf(client, "\n");
        }
        else if (explicit_handle)
        {
            print_client("_Handle = %s;\n", explicit_handle_var->name);
            fprintf(client, "\n");
        }

        /* emit the message buffer size */
        print_client("_StubMsg.BufferLength =");
        print_message_buffer_size(func);
        fprintf(client, ";\n");


        print_client("NdrGetBuffer(\n");
        indent++;
        print_client("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
        print_client("_StubMsg.BufferLength,\n");
        if (implicit_handle || explicit_handle)
            print_client("%_Handle);\n");
        else
            print_client("%s__MIDL_AutoBindHandle);\n", iface->name);
        indent--;
        fprintf(client, "\n");

        /* marshal arguments */
        marshall_arguments(func);

        /* send/recieve message */
        print_client("NdrSendReceive(\n");
        indent++;
        print_client("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
        print_client("(unsigned char __RPC_FAR *)_StubMsg.Buffer);\n");
        indent--;

        /* unmarshal return value */
        if (!is_void(def->type, NULL))
        {
            fprintf(client, "\n");

            print_client("if ((_RpcMessage.DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)\n");
            indent++;
            print_client("NdrConvert(\n");
            indent++;
            print_client("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
            print_client("(PFORMAT_STRING)&__MIDL_ProcFormatString.Format[%u]);\n", proc_offset);
            indent -= 2;
            fprintf(client, "\n");

            print_client("_RetVal = *((");
            write_type(client, def->type, def, def->tname);
            fprintf(client, " __RPC_FAR *)_StubMsg.Buffer)++;\n");
        }

        /* update proc_offset */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                proc_offset += 2; /* FIXME */
                var = PREV_LINK(var);
            }
        }
        proc_offset += 2;  /* FIXME */

        indent--;
        print_client("}\n");
        print_client("RpcFinally\n");
        print_client("{\n");
        indent++;


        /* FIXME: emit client finally code */

        print_client("NdrFreeBuffer((PMIDL_STUB_MESSAGE)&_StubMsg);\n");

        indent--;
        print_client("}\n");
        print_client("RpcEndFinally\n");


        /* emit return code */
        if (!is_void(def->type, NULL))
        {
            fprintf(client, "\n");
            print_client("return _RetVal;\n");
        }

        indent--;
        fprintf(client, "}\n");
        fprintf(client, "\n");

        method_count++;
        func = PREV_LINK(func);
    }
}


static void write_bindinghandledecl(type_t *iface)
{
    print_client("static RPC_BINDING_HANDLE %s__MIDL_AutoBindHandle;\n", iface->name);
    fprintf(client, "\n");
}


static void write_stubdescdecl(type_t *iface)
{
    print_client("extern const MIDL_STUB_DESC %s_StubDesc;\n", iface->name);
    fprintf(client, "\n");
}


static void write_stubdescriptor(type_t *iface)
{
    char *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);

    print_client("static const MIDL_STUB_DESC %s_StubDesc =\n", iface->name);
    print_client("{\n");
    indent++;
    print_client("(void __RPC_FAR *)& %s___RpcClientInterface,\n", iface->name);
    print_client("MIDL_user_allocate,\n");
    print_client("MIDL_user_free,\n");
    if (implicit_handle)
        print_client("&%s,\n", implicit_handle);
    else
        print_client("&%s__MIDL_AutoBindHandle,\n", iface->name);
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    print_client("__MIDL_TypeFormatString.Format,\n");
    print_client("1, /* -error bounds_check flag */\n");
    print_client("0x10001, /* Ndr library version */\n");
    print_client("0,\n");
    print_client("0x50100a4, /* MIDL Version 5.1.164 */\n");
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,  /* notify & notify_flag routine table */\n");
    print_client("1,  /* Flags */\n");
    print_client("0,  /* Reserved3 */\n");
    print_client("0,  /* Reserved4 */\n");
    print_client("0   /* Reserved5 */\n");
    indent--;
    print_client("};\n");
    fprintf(client, "\n");
}


static void write_clientinterfacedecl(type_t *iface)
{
    unsigned long ver = get_attrv(iface->attrs, ATTR_VERSION);
    UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);

    print_client("static const RPC_CLIENT_INTERFACE %s___RpcClientInterface =\n", iface->name );
    print_client("{\n");
    indent++;
    print_client("sizeof(RPC_CLIENT_INTERFACE),\n");
    print_client("{{0x%08lx,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}},{%d,%d}},\n",
                 uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0], uuid->Data4[1],
                 uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5], uuid->Data4[6],
                 uuid->Data4[7], LOWORD(ver), HIWORD(ver));
    print_client("{{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},\n"); /* FIXME */
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    indent--;
    print_client("};\n");
    print_client("RPC_IF_HANDLE %s_v%d_%d_c_ifspec = (RPC_IF_HANDLE)& %s___RpcClientInterface;\n",
                 iface->name, LOWORD(ver), HIWORD(ver), iface->name);
    fprintf(client, "\n");
}


static void write_formatdesc( const char *str )
{
    print_client("typedef struct _MIDL_%s_FORMAT_STRING\n", str );
    print_client("{\n");
    indent++;
    print_client("short Pad;\n");
    print_client("unsigned char Format[%s_FORMAT_STRING_SIZE];\n", str);
    indent--;
    print_client("} MIDL_%s_FORMAT_STRING;\n", str);
    print_client("\n");
}


static void write_formatstringsdecl(type_t *iface)
{
    int byte_count = 1;
    func_t *func;
    var_t *var;

    print_client("#define TYPE_FORMAT_STRING_SIZE %d\n", 3); /* FIXME */

    /* determine the proc format string size */
    func = iface->funcs;
    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        /* argument list size */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                byte_count += 2; /* FIXME: determine real size */
                var = PREV_LINK(var);
            }
        }

        /* return value size */
        byte_count += 2; /* FIXME: determine real size */
        func = PREV_LINK(func);
    }

    print_client("#define PROC_FORMAT_STRING_SIZE %d\n", byte_count);

    fprintf(client, "\n");
    write_formatdesc("TYPE");
    write_formatdesc("PROC");
    fprintf(client, "\n");
    print_client("extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;\n");
    print_client("extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;\n");
    print_client("\n");
}


static void write_implicithandledecl(type_t *iface)
{
    char *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);

    if (implicit_handle)
    {
        fprintf(client, "handle_t %s;\n", implicit_handle);
        fprintf(client, "\n");
    }
}


static void init_client(void)
{
    if (client) return;
    if (!(client = fopen(client_name, "w")))
        error("Could not open %s for output\n", client_name);

    print_client("/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", WIDL_FULLVERSION, input_name);
    print_client("#include<string.h>\n");
    print_client("#ifdef _ALPHA_\n");
    print_client("#include<stdarg.h>\n");
    print_client("#endif\n");
    fprintf(client, "\n");
    print_client("#include\"%s\"\n", header_name);
    fprintf(client, "\n");
}


void write_client(ifref_t *ifaces)
{
    ifref_t *iface = ifaces;

    if (!do_client)
        return;
    if (!iface)
        return;
    END_OF_LIST(iface);

    init_client();
    if (!client)
        return;

    while (iface)
    {
        fprintf(client, "/*****************************************************************************\n");
        fprintf(client, " * %s interface\n", iface->iface->name);
        fprintf(client, " */\n");
        fprintf(client, "\n");

        write_formatstringsdecl(iface->iface);
        write_implicithandledecl(iface->iface);

        write_clientinterfacedecl(iface->iface);
        write_stubdescdecl(iface->iface);
        write_bindinghandledecl(iface->iface);

        write_function_stubs(iface->iface);
        write_stubdescriptor(iface->iface);

        print_client("#if !defined(__RPC_WIN32__)\n");
        print_client("#error  Invalid build platform for this stub.\n");
        print_client("#endif\n");
        fprintf(client, "\n");

        write_procformatstring(iface->iface);
        write_typeformatstring();

        fprintf(client, "\n");

        iface = PREV_LINK(iface);
    }

    fclose(client);
}
