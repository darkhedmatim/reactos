/*
 * Copyright (C) 2005 Vijay Kiran Kamuju
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

#ifndef __MSXML2DID_H__
#define __MSXML2DID_H__

#define DISPID_DOM_BASE                     0x00000001
#define DISPID_DOM_COLLECTION_BASE          0x000f4240
#define DISPID_DOM_COLLECTION_MAX           0x002dc6bf

#define DISPID_DOM_NODE                                 0x00000001
#define DISPID_DOM_NODE_NODENAME                        0x00000002
#define DISPID_DOM_NODE_NODEVALUE                       0x00000003
#define DISPID_DOM_NODE_NODETYPE                        0x00000004
#define DISPID_DOM_NODE_NODETYPEENUM                    0x00000005
#define DISPID_DOM_NODE_PARENTNODE                      0x00000006
#define DISPID_DOM_NODE_CHILDNODES                      0x00000007
#define DISPID_DOM_NODE_FIRSTCHILD                      0x00000008
#define DISPID_DOM_NODE_LASTCHILD                       0x00000009
#define DISPID_DOM_NODE_PREVIOUSSIBLING                 0x0000000a
#define DISPID_DOM_NODE_NEXTSIBLING                     0x0000000b
#define DISPID_DOM_NODE_ATTRIBUTES                      0x0000000c
#define DISPID_DOM_NODE_INSERTBEFORE                    0x0000000d
#define DISPID_DOM_NODE_REPLACECHILD                    0x0000000e
#define DISPID_DOM_NODE_REMOVECHILD                     0x0000000f
#define DISPID_DOM_NODE_APPENDCHILD                     0x00000010
#define DISPID_DOM_NODE_HASCHILDNODES                   0x00000011
#define DISPID_DOM_NODE_OWNERDOC                        0x00000012
#define DISPID_DOM_NODE_CLONENODE                       0x00000013

#define DISPID_XMLDOM_NODE                              0x00000014
#define DISPID_XMLDOM_NODE_STRINGTYPE                   0x00000015
#define DISPID_XMLDOM_NODE_SPECIFIED                    0x00000016
#define DISPID_XMLDOM_NODE_DEFINITION                   0x00000017
#define DISPID_XMLDOM_NODE_TEXT                         0x00000018
#define DISPID_XMLDOM_NODE_NODETYPEDVALUE               0x00000019
#define DISPID_XMLDOM_NODE_DATATYPE                     0x0000001a
#define DISPID_XMLDOM_NODE_XML                          0x0000001b
#define DISPID_XMLDOM_NODE_TRANSFORMNODE                0x0000001c
#define DISPID_XMLDOM_NODE_SELECTNODES                  0x0000001d
#define DISPID_XMLDOM_NODE_SELECTSINGLENODE             0x0000001e
#define DISPID_XMLDOM_NODE_PARSED                       0x0000001f
#define DISPID_XMLDOM_NODE_NAMESPACE                    0x00000020
#define DISPID_XMLDOM_NODE_PREFIX                       0x00000021
#define DISPID_XMLDOM_NODE_BASENAME                     0x00000022
#define DISPID_XMLDOM_NODE_TRANSFORMNODETOOBJECT        0x00000023
#define DISPID_XMLDOM_NODE__TOP                         0x00000024

#define DISPID_DOM_DOCUMENT                             0x00000025
#define DISPID_DOM_DOCUMENT_DOCTYPE                     0x00000026
#define DISPID_DOM_DOCUMENT_IMPLEMENTATION              0x00000027
#define DISPID_DOM_DOCUMENT_DOCUMENTELEMENT             0x00000028
#define DISPID_DOM_DOCUMENT_CREATEELEMENT               0x00000029
#define DISPID_DOM_DOCUMENT_CREATEDOCUMENTFRAGMENT      0x0000002a
#define DISPID_DOM_DOCUMENT_CREATETEXTNODE              0x0000002b
#define DISPID_DOM_DOCUMENT_CREATECOMMENT               0x0000002c
#define DISPID_DOM_DOCUMENT_CREATECDATASECTION          0x0000002d
#define DISPID_DOM_DOCUMENT_CREATEPROCESSINGINSTRUCTION 0x0000002e
#define DISPID_DOM_DOCUMENT_CREATEATTRIBUTE             0x0000002f
#define DISPID_DOM_DOCUMENT_CREATEENTITY                0x00000030
#define DISPID_DOM_DOCUMENT_CREATEENTITYREFERENCE       0x00000031
#define DISPID_DOM_DOCUMENT_GETELEMENTSBYTAGNAME        0x00000032
#define DISPID_DOM_DOCUMENT_TOP                         0x00000033

#define DISPID_XMLDOM_DOCUMENT                          0x00000034
#define DISPID_XMLDOM_DOCUMENT_DOCUMENTNODE             0x00000035
#define DISPID_XMLDOM_DOCUMENT_CREATENODE               0x00000036
#define DISPID_XMLDOM_DOCUMENT_CREATENODEEX             0x00000037
#define DISPID_XMLDOM_DOCUMENT_NODEFROMID               0x00000038
#define DISPID_XMLDOM_DOCUMENT_DOCUMENTNAMESPACES       0x00000039
#define DISPID_XMLDOM_DOCUMENT_LOAD                     0x0000003a
#define DISPID_XMLDOM_DOCUMENT_PARSEERROR               0x0000003b
#define DISPID_XMLDOM_DOCUMENT_URL                      0x0000003c
#define DISPID_XMLDOM_DOCUMENT_ASYNC                    0x0000003d
#define DISPID_XMLDOM_DOCUMENT_ABORT                    0x0000003e
#define DISPID_XMLDOM_DOCUMENT_LOADXML                  0x0000003f
#define DISPID_XMLDOM_DOCUMENT_SAVE                     0x00000040
#define DISPID_XMLDOM_DOCUMENT_VALIDATE                 0x00000041
#define DISPID_XMLDOM_DOCUMENT_RESOLVENAMESPACE         0x00000042
#define DISPID_XMLDOM_DOCUMENT_PRESERVEWHITESPACE       0x00000043
#define DISPID_XMLDOM_DOCUMENT_ONREADYSTATECHANGE       0x00000044
#define DISPID_XMLDOM_DOCUMENT_ONDATAAVAILABLE          0x00000045
#define DISPID_XMLDOM_DOCUMENT_ONTRANSFORMNODE          0x00000046
#define DISPID_XMLDOM_DOCUMENT__TOP                     0x00000047

#define DISPID_DOM_NODELIST                             0x00000048
#define DISPID_DOM_NODELIST_ITEM                        0x00000049
#define DISPID_DOM_NODELIST_LENGTH                      0x0000004a
#define DISPID_XMLDOM_NODELIST                          0x0000004b
#define DISPID_XMLDOM_NODELIST_NEXTNODE                 0x0000004c
#define DISPID_XMLDOM_NODELIST_RESET                    0x0000004d
#define DISPID_XMLDOM_NODELIST_NEWENUM                  0x0000004e
#define DISPID_XMLDOM_NODELIST__TOP                     0x0000004f

#define DISPID_DOM_NAMEDNODEMAP                         0x00000050
#define DISPID_DOM_NAMEDNODEMAP_GETNAMEDITEM            0x00000053
#define DISPID_DOM_NAMEDNODEMAP_SETNAMEDITEM            0x00000054
#define DISPID_DOM_NAMEDNODEMAP_REMOVENAMEDITEM         0x00000055
#define DISPID_XMLDOM_NAMEDNODEMAP                      0x00000056
#define DISPID_XMLDOM_NAMEDNODEMAP_GETQUALIFIEDITEM     0x00000057
#define DISPID_XMLDOM_NAMEDNODEMAP_REMOVEQUALIFIEDITEM  0x00000058
#define DISPID_XMLDOM_NAMEDNODEMAP_NEXTNODE             0x00000059
#define DISPID_XMLDOM_NAMEDNODEMAP_RESET                0x0000005a
#define DISPID_XMLDOM_NAMEDNODEMAP_NEWENUM              0x0000005b
#define DISPID_XMLDOM_NAMEDNODEMAP__TOP                 0x0000005c

#define DISPID_DOM_W3CWRAPPERS                          0x0000005d

#define DISPID_DOM_DOCUMENTFRAGMENT                     0x0000005e
#define DISPID_DOM_DOCUMENTFRAGMENT__TOP                0x0000005f

#define DISPID_DOM_ELEMENT                              0x00000060
#define DISPID_DOM_ELEMENT_GETTAGNAME                   0x00000061
#define DISPID_DOM_ELEMENT_GETATTRIBUTES                0x00000062
#define DISPID_DOM_ELEMENT_GETATTRIBUTE                 0x00000063
#define DISPID_DOM_ELEMENT_SETATTRIBUTE                 0x00000064
#define DISPID_DOM_ELEMENT_REMOVEATTRIBUTE              0x00000065
#define DISPID_DOM_ELEMENT_GETATTRIBUTENODE             0x00000066
#define DISPID_DOM_ELEMENT_SETATTRIBUTENODE             0x00000067
#define DISPID_DOM_ELEMENT_REMOVEATTRIBUTENODE          0x00000068
#define DISPID_DOM_ELEMENT_GETELEMENTSBYTAGNAME         0x00000069
#define DISPID_DOM_ELEMENT_NORMALIZE                    0x0000006a
#define DISPID_DOM_ELEMENT__TOP                         0x0000006b

#define DISPID_DOM_DATA                0x0000006c
#define DISPID_DOM_DATA_DATA           0x0000006d
#define DISPID_DOM_DATA_LENGTH         0x0000006e
#define DISPID_DOM_DATA_SUBSTRING      0x0000006f
#define DISPID_DOM_DATA_APPEND         0x00000070
#define DISPID_DOM_DATA_INSERT         0x00000071
#define DISPID_DOM_DATA_DELETE         0x00000072
#define DISPID_DOM_DATA_REPLACE        0x00000073
#define DISPID_DOM_DATA__TOP           0x00000074

#define DISPID_DOM_ATTRIBUTE           0x00000075
#define DISPID_DOM_ATTRIBUTE_GETNAME   0x00000076
#define DISPID_DOM_ATTRIBUTE_SPECIFIED 0x00000077
#define DISPID_DOM_ATTRIBUTE_VALUE     0x00000078
#define DISPID_DOM_ATTRIBUTE__TOP      0x00000079

#define DISPID_DOM_TEXT                0x0000007a
#define DISPID_DOM_TEXT_SPLITTEXT      0x0000007b
#define DISPID_DOM_TEXT_JOINTEXT       0x0000007c
#define DISPID_DOM_TEXT__TOP           0x0000007d

#define DISPID_DOM_PI                  0x0000007e
#define DISPID_DOM_PI_TARGET           0x0000007f
#define DISPID_DOM_PI_DATA             0x00000080
#define DISPID_DOM_PI__TOP             0x00000081

#define DISPID_DOM_DOCUMENTTYPE                    0x00000082
#define DISPID_DOM_DOCUMENTTYPE_NAME               0x00000083
#define DISPID_DOM_DOCUMENTTYPE_ENTITIES           0x00000084
#define DISPID_DOM_DOCUMENTTYPE_NOTATIONS          0x00000085
#define DISPID_DOM_DOCUMENTTYPE__TOP               0x00000086

#define DISPID_DOM_NOTATION            0x00000087
#define DISPID_DOM_NOTATION_PUBLICID   0x00000088
#define DISPID_DOM_NOTATION_SYSTEMID   0x00000089
#define DISPID_DOM_NOTATION__TOP       0x0000008a

#define DISPID_DOM_ENTITY              0x0000008b
#define DISPID_DOM_ENTITY_PUBLICID     0x0000008c
#define DISPID_DOM_ENTITY_SYSTEMID     0x0000008d
#define DISPID_DOM_ENTITY_NOTATIONNAME 0x0000008e
#define DISPID_DOM_ENTITY__TOP         0x0000008f

#define DISPID_DOM_W3CWRAPPERS_TOP     0x0000008f

#define DISPID_DOM_IMPLEMENTATION              0x00000090
#define DISPID_DOM_IMPLEMENTATION_HASFEATURE   0x00000091
#define DISPID_DOM_IMPLEMENTATION__TOP         0x00000092

#define DISPID_DOM__TOP                0x000000af

#define  DISPID_DOM_ERROR              0x000000b0
#define  DISPID_DOM_ERROR_ERRORCODE    0x000000b1
#define  DISPID_DOM_ERROR_URL          0x000000b2
#define  DISPID_DOM_ERROR_REASON       0x000000b3
#define  DISPID_DOM_ERROR_SRCTEXT      0x000000b4
#define  DISPID_DOM_ERROR_LINE         0x000000b5
#define  DISPID_DOM_ERROR_LINEPOS      0x000000b6
#define  DISPID_DOM_ERROR_FILEPOS      0x000000b7
#define  DISPID_DOM_ERROR__TOP         0x000000b8

#define  DISPID_XTLRUNTIME                     0x000000b9
#define  DISPID_XTLRUNTIME_UNIQUEID            0x000000ba
#define  DISPID_XTLRUNTIME_DEPTH               0x000000bb
#define  DISPID_XTLRUNTIME_CHILDNUMBER         0x000000bc
#define  DISPID_XTLRUNTIME_ANCESTORCHILDNUMBER 0x000000bd
#define  DISPID_XTLRUNTIME_ABSOLUTECHILDNUMBER 0x000000be
#define  DISPID_XTLRUNTIME_FORMATINDEX         0x000000bf
#define  DISPID_XTLRUNTIME_FORMATNUMBER        0x000000c0
#define  DISPID_XTLRUNTIME_FORMATDATE          0x000000c1
#define  DISPID_XTLRUNTIME_FORMATTIME          0x000000c2
#define  DISPID_XTLRUNTIME__TOP                0x000000c3

#define  DISPID_XMLDOMEVENT                    0x000000c4
#define  DISPID_XMLDOMEVENT_ONREADYSTATECHANGE DISPID_READYSTATECHANGE
#define  DISPID_XMLDOMEVENT_ONDATAAVAILABLE    0x000000c5
#define  DISPID_XMLDOMEVENT__TOP               0x000000c6

#define DISPID_XMLDOM_DOCUMENT2                0x000000c7
#define DISPID_XMLDOM_DOCUMENT2_NAMESPACES     0x000000c8
#define DISPID_XMLDOM_DOCUMENT2_SCHEMAS        0x000000c9
#define DISPID_XMLDOM_DOCUMENT2_VALIDATE       0x000000ca
#define DISPID_XMLDOM_DOCUMENT2_SETPROPERTY    0x000000cb
#define DISPID_XMLDOM_DOCUMENT2_GETPROPERTY    0x000000cc
#define DISPID_XMLDOM_DOCUMENT2__TOP           0x000000cd

#define DISPID_XMLDOM_SCHEMACOLLECTION                 0x00000002
#define DISPID_XMLDOM_SCHEMACOLLECTION_ADD             0x00000003
#define DISPID_XMLDOM_SCHEMACOLLECTION_GET             0x00000004
#define DISPID_XMLDOM_SCHEMACOLLECTION_REMOVE          0x00000005
#define DISPID_XMLDOM_SCHEMACOLLECTION_LENGTH          0x00000006
#define DISPID_XMLDOM_SCHEMACOLLECTION_NAMESPACEURI    0x00000007
#define DISPID_XMLDOM_SCHEMACOLLECTION_ADDCOLLECTION   0x00000008
#define DISPID_XMLDOM_SCHEMACOLLECTION__TOP            0x00000009

#define DISPID_XMLDOM_SELECTION                0x0000000a
#define DISPID_XMLDOM_SELECTION_EXPR           0x0000000b
#define DISPID_XMLDOM_SELECTION_CONTEXT        0x0000000c
#define DISPID_XMLDOM_SELECTION_PEEKNODE       0x0000000d
#define DISPID_XMLDOM_SELECTION_MATCHES        0x0000000e
#define DISPID_XMLDOM_SELECTION_REMOVENEXT     0x0000000f
#define DISPID_XMLDOM_SELECTION_REMOVEALL      0x00000010
#define DISPID_XMLDOM_SELECTION_CLONE          0x00000011
#define DISPID_XMLDOM_SELECTION_GETPROPERTY    0x00000012
#define DISPID_XMLDOM_SELECTION_SETPROPERTY    0x00000013
#define DISPID_XMLDOM_SELECTION__TOP           0x00000014

#define DISPID_XMLDOM_TEMPLATE                 0x00000001
#define DISPID_XMLDOM_TEMPLATE_STYLESHEET      0x00000002
#define DISPID_XMLDOM_TEMPLATE_CREATEPROCESSOR 0x00000003
#define DISPID_XMLDOM_TEMPLATE__TOP            0x00000004

#define DISPID_XMLDOM_PROCESSOR                0x00000001
#define DISPID_XMLDOM_PROCESSOR_INPUT          0x00000002
#define DISPID_XMLDOM_PROCESSOR_XSLTEMPLATE    0x00000003
#define DISPID_XMLDOM_PROCESSOR_SETSTARTMODE   0x00000004
#define DISPID_XMLDOM_PROCESSOR_STARTMODE      0x00000005
#define DISPID_XMLDOM_PROCESSOR_STARTMODEURI   0x00000006
#define DISPID_XMLDOM_PROCESSOR_OUTPUT         0x00000007
#define DISPID_XMLDOM_PROCESSOR_TRANSFORM      0x00000008
#define DISPID_XMLDOM_PROCESSOR_RESET          0x00000009
#define DISPID_XMLDOM_PROCESSOR_READYSTATE     0x0000000a
#define DISPID_XMLDOM_PROCESSOR_ADDPARAMETER   0x0000000b
#define DISPID_XMLDOM_PROCESSOR_ADDOBJECT      0x0000000c
#define DISPID_XMLDOM_PROCESSOR_STYLESHEET     0x0000000d
#define DISPID_XMLDOM_PROCESSOR__TOP           0x0000000e

#define  DISPID_XMLDSO                         0x10000
#define  DISPID_XMLDSO_DOCUMENT                0x10001
#define  DISPID_XMLDSO_JAVADSOCOMPATIBLE       0x10002

#endif /* __MSXML2DID_H__ */
