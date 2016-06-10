/* WINE ipifcons.h
 * Copyright (C) 2003 Juan Lang
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
#ifndef WINE_IPIFCONS_H__
#define WINE_IPIFCONS_H__

#define IF_TYPE_OTHER                            1
#define IF_TYPE_REGULAR_1822                     2
#define IF_TYPE_HDH_1822                         3
#define IF_TYPE_DDN_X25                          4
#define IF_TYPE_RFC877_X25                       5
#define IF_TYPE_ETHERNET_CSMACD                  6
#define IF_TYPE_IS088023_CSMACD                  7
#define IF_TYPE_ISO88024_TOKENBUS                8
#define IF_TYPE_ISO88025_TOKENRING               9
#define IF_TYPE_ISO88026_MAN                     10
#define IF_TYPE_STARLAN                          11
#define IF_TYPE_PROTEON_10MBIT                   12
#define IF_TYPE_PROTEON_80MBIT                   13
#define IF_TYPE_HYPERCHANNEL                     14
#define IF_TYPE_FDDI                             15
#define IF_TYPE_LAP_B                            16
#define IF_TYPE_SDLC                             17
#define IF_TYPE_DS1                              18
#define IF_TYPE_E1                               19
#define IF_TYPE_BASIC_ISDN                       20
#define IF_TYPE_PRIMARY_ISDN                     21
#define IF_TYPE_PROP_POINT2POINT_SERIAL          22
#define IF_TYPE_PPP                              23
#define IF_TYPE_SOFTWARE_LOOPBACK                24
#define IF_TYPE_EON                              25
#define IF_TYPE_ETHERNET_3MBIT                   26
#define IF_TYPE_NSIP                             27
#define IF_TYPE_SLIP                             28
#define IF_TYPE_ULTRA                            29
#define IF_TYPE_DS3                              30
#define IF_TYPE_SIP                              31
#define IF_TYPE_FRAMERELAY                       32
#define IF_TYPE_RS232                            33
#define IF_TYPE_PARA                             34
#define IF_TYPE_ARCNET                           35
#define IF_TYPE_ARCNET_PLUS                      36
#define IF_TYPE_ATM                              37
#define IF_TYPE_MIO_X25                          38
#define IF_TYPE_SONET                            39
#define IF_TYPE_X25_PLE                          40
#define IF_TYPE_ISO88022_LLC                     41
#define IF_TYPE_LOCALTALK                        42
#define IF_TYPE_SMDS_DXI                         43
#define IF_TYPE_FRAMERELAY_SERVICE               44
#define IF_TYPE_V35                              45
#define IF_TYPE_HSSI                             46
#define IF_TYPE_HIPPI                            47
#define IF_TYPE_MODEM                            48
#define IF_TYPE_AAL5                             49
#define IF_TYPE_SONET_PATH                       50
#define IF_TYPE_SONET_VT                         51
#define IF_TYPE_SMDS_ICIP                        52
#define IF_TYPE_PROP_VIRTUAL                     53
#define IF_TYPE_PROP_MULTIPLEXOR                 54
#define IF_TYPE_IEEE80212                        55
#define IF_TYPE_FIBRECHANNEL                     56
#define IF_TYPE_HIPPIINTERFACE                   57
#define IF_TYPE_FRAMERELAY_INTERCONNECT          58
#define IF_TYPE_AFLANE_8023                      59
#define IF_TYPE_AFLANE_8025                      60
#define IF_TYPE_CCTEMUL                          61
#define IF_TYPE_FASTETHER                        62
#define IF_TYPE_ISDN                             63
#define IF_TYPE_V11                              64
#define IF_TYPE_V36                              65
#define IF_TYPE_G703_64K                         66
#define IF_TYPE_G703_2MB                         67
#define IF_TYPE_QLLC                             68
#define IF_TYPE_FASTETHER_FX                     69
#define IF_TYPE_CHANNEL                          70
#define IF_TYPE_IEEE80211                        71
#define IF_TYPE_IBM370PARCHAN                    72
#define IF_TYPE_ESCON                            73
#define IF_TYPE_DLSW                             74
#define IF_TYPE_ISDN_S                           75
#define IF_TYPE_ISDN_U                           76
#define IF_TYPE_LAP_D                            77
#define IF_TYPE_IPSWITCH                         78
#define IF_TYPE_RSRB                             79
#define IF_TYPE_ATM_LOGICAL                      80
#define IF_TYPE_DS0                              81
#define IF_TYPE_DS0_BUNDLE                       82
#define IF_TYPE_BSC                              83
#define IF_TYPE_ASYNC                            84
#define IF_TYPE_CNR                              85
#define IF_TYPE_ISO88025R_DTR                    86
#define IF_TYPE_EPLRS                            87
#define IF_TYPE_ARAP                             88
#define IF_TYPE_PROP_CNLS                        89
#define IF_TYPE_HOSTPAD                          90
#define IF_TYPE_TERMPAD                          91
#define IF_TYPE_FRAMERELAY_MPI                   92
#define IF_TYPE_X213                             93
#define IF_TYPE_ADSL                             94
#define IF_TYPE_RADSL                            95
#define IF_TYPE_SDSL                             96
#define IF_TYPE_VDSL                             97
#define IF_TYPE_ISO88025_CRFPRINT                98
#define IF_TYPE_MYRINET                          99
#define IF_TYPE_VOICE_EM                         100
#define IF_TYPE_VOICE_FXO                        101
#define IF_TYPE_VOICE_FXS                        102
#define IF_TYPE_VOICE_ENCAP                      103
#define IF_TYPE_VOICE_OVERIP                     104
#define IF_TYPE_ATM_DXI                          105
#define IF_TYPE_ATM_FUNI                         106
#define IF_TYPE_ATM_IMA                          107
#define IF_TYPE_PPPMULTILINKBUNDLE               108
#define IF_TYPE_IPOVER_CDLC                      109
#define IF_TYPE_IPOVER_CLAW                      110
#define IF_TYPE_STACKTOSTACK                     111
#define IF_TYPE_VIRTUALIPADDRESS                 112
#define IF_TYPE_MPC                              113
#define IF_TYPE_IPOVER_ATM                       114
#define IF_TYPE_ISO88025_FIBER                   115
#define IF_TYPE_TDLC                             116
#define IF_TYPE_GIGABITETHERNET                  117
#define IF_TYPE_HDLC                             118
#define IF_TYPE_LAP_F                            119
#define IF_TYPE_V37                              120
#define IF_TYPE_X25_MLP                          121
#define IF_TYPE_X25_HUNTGROUP                    122
#define IF_TYPE_TRANSPHDLC                       123
#define IF_TYPE_INTERLEAVE                       124
#define IF_TYPE_FAST                             125
#define IF_TYPE_IP                               126
#define IF_TYPE_DOCSCABLE_MACLAYER               127
#define IF_TYPE_DOCSCABLE_DOWNSTREAM             128
#define IF_TYPE_DOCSCABLE_UPSTREAM               129
#define IF_TYPE_A12MPPSWITCH                     130
#define IF_TYPE_TUNNEL                           131
#define IF_TYPE_COFFEE                           132
#define IF_TYPE_CES                              133
#define IF_TYPE_ATM_SUBINTERFACE                 134
#define IF_TYPE_L2_VLAN                          135
#define IF_TYPE_L3_IPVLAN                        136
#define IF_TYPE_L3_IPXVLAN                       137
#define IF_TYPE_DIGITALPOWERLINE                 138
#define IF_TYPE_MEDIAMAILOVERIP                  139
#define IF_TYPE_DTM                              140
#define IF_TYPE_DCN                              141
#define IF_TYPE_IPFORWARD                        142
#define IF_TYPE_MSDSL                            143
#define IF_TYPE_IEEE1394                         144
#define IF_TYPE_IF_GSN                           145
#define IF_TYPE_DVBRCC_MACLAYER                  146
#define IF_TYPE_DVBRCC_DOWNSTREAM                147
#define IF_TYPE_DVBRCC_UPSTREAM                  148
#define IF_TYPE_ATM_VIRTUAL                      149
#define IF_TYPE_MPLS_TUNNEL                      150
#define IF_TYPE_SRP                              151
#define IF_TYPE_VOICEOVERATM                     152
#define IF_TYPE_VOICEOVERFRAMERELAY              153
#define IF_TYPE_IDSL                             154
#define IF_TYPE_COMPOSITELINK                    155
#define IF_TYPE_SS7_SIGLINK                      156
#define IF_TYPE_PROP_WIRELESS_P2P                157
#define IF_TYPE_FR_FORWARD                       158
#define IF_TYPE_RFC1483                          159
#define IF_TYPE_USB                              160
#define IF_TYPE_IEEE8023AD_LAG                   161
#define IF_TYPE_BGP_POLICY_ACCOUNTING            162
#define IF_TYPE_FRF16_MFR_BUNDLE                 163
#define IF_TYPE_H323_GATEKEEPER                  164
#define IF_TYPE_H323_PROXY                       165
#define IF_TYPE_MPLS                             166
#define IF_TYPE_MF_SIGLINK                       167
#define IF_TYPE_HDSL2                            168
#define IF_TYPE_SHDSL                            169
#define IF_TYPE_DS1_FDL                          170
#define IF_TYPE_POS                              171
#define IF_TYPE_DVB_ASI_IN                       172
#define IF_TYPE_DVB_ASI_OUT                      173
#define IF_TYPE_PLC                              175
#define IF_TYPE_NFAS                             175
#define IF_TYPE_TR008                            176
#define IF_TYPE_GR303_RDT                        177
#define IF_TYPE_GR303_IDT                        178
#define IF_TYPE_ISUP                             179
#define IF_TYPE_PROP_DOCS_WIRELESS_MACLAYER      180
#define IF_TYPE_PROP_DOCS_WIRELESS_DOWNSTREAM    181
#define IF_TYPE_PROP_DOCS_WIRELESS_UPSTREAM      182
#define IF_TYPE_HIPERLAN2                        183
#define IF_TYPE_PROP_BWA_P2MP                    184
#define IF_TYPE_SONET_OVERHEAD_CHANNEL           185
#define IF_TYPE_DIGITAL_WRAPPER_OVERHEAD_CHANNEL 186
#define IF_TYPE_AAL2                             187
#define IF_TYPE_RADIO_MAC                        188
#define IF_TYPE_ATM_RADIO                        189
#define IF_TYPE_IMT                              190
#define IF_TYPE_MVL                              191
#define IF_TYPE_REACH_DSL                        192
#define IF_TYPE_FR_DLCI_ENDPT                    193
#define IF_TYPE_ATM_VCI_ENDPT                    194
#define IF_TYPE_OPTICAL_CHANNEL                  195
#define IF_TYPE_OPTICAL_TRANSPORT                196
#define IF_TYPE_IEEE80216_WANN                   237
#define IF_TYPE_WWANPP                           243
#define IF_TYPE_WWANPP2                          244
#define MAX_IF_TYPE                              244

#define MIB_IF_TYPE_OTHER               1
#define MIB_IF_TYPE_ETHERNET            6
#define MIB_IF_TYPE_TOKENRING           9
#define MIB_IF_TYPE_FDDI                15
#define MIB_IF_TYPE_PPP                 23
#define MIB_IF_TYPE_LOOPBACK            24
#define MIB_IF_TYPE_SLIP                28

typedef ULONG IFTYPE;

#define MIB_IF_ADMIN_STATUS_UP          1
#define MIB_IF_ADMIN_STATUS_DOWN        2
#define MIB_IF_ADMIN_STATUS_TESTING     3

typedef enum _INTERNAL_IF_OPER_STATUS
{
    IF_OPER_STATUS_NON_OPERATIONAL = 0,
    IF_OPER_STATUS_UNREACHABLE = 1,
    IF_OPER_STATUS_DISCONNECTED = 2,
    IF_OPER_STATUS_CONNECTING = 3,
    IF_OPER_STATUS_CONNECTED = 4,
    IF_OPER_STATUS_OPERATIONAL = 5,
} INTERNAL_IF_OPER_STATUS;

#define MIB_IF_OPER_STATUS_NON_OPERATIONAL IF_OPER_STATUS_NON_OPERATIONAL
#define MIB_IF_OPER_STATUS_UNREACHABLE     IF_OPER_STATUS_UNREACHABLE
#define MIB_IF_OPER_STATUS_DISCONNECTED    IF_OPER_STATUS_DISCONNECTED
#define MIB_IF_OPER_STATUS_CONNECTING      IF_OPER_STATUS_CONNECTING
#define MIB_IF_OPER_STATUS_CONNECTED       IF_OPER_STATUS_CONNECTED
#define MIB_IF_OPER_STATUS_OPERATIONAL     IF_OPER_STATUS_OPERATIONAL

#endif /* WINE_IPIFCONS_H__ */
