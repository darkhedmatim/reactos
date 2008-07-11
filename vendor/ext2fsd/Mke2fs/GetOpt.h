/*
 * PROJECT:          Mke2fs
 * FILE:             GetOpt.h
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

#ifndef _GETOPT_H_
#define _GETOPT_H_

/* INCLUDES **************************************************************/

#include "stdafx.h"

/* DEFINITIONS ***********************************************************/

int GetOption (int argc, char** argv, char* pszValidOpts, char** ppszParam); 

int getopt(int argc, char *argv[], char *optstring);
extern char *optarg;
extern int optind;

#endif // _GETOPT_H_