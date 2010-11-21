#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef struct
{
    char *pcName;
    int nNameLength;
    char *pcRedirection;
    int nRedirectionLength;
    int nCallingConvention;
    int nOrdinal;
    int nStackBytes;
} EXPORT;

int (*OutputLine)(FILE *, EXPORT *);
void (*OutputHeader)(FILE *, char *);
int no_decoration = 0;
int no_redirections = 0;
char *pszArchString = "i386";
char *pszArchString2;

enum
{
    CC_STDCALL,
    CC_CDECL,
    CC_FASTCALL,
    CC_EXTERN,
};

char* astrCallingConventions[] =
{
    "STDCALL",
    "CDECL",
    "FASTCALL",
    "EXTERN"
};

static
int
IsSeparator(char chr)
{
    return ((chr <= ',' && chr != '$') ||
            (chr >= ':' && chr < '?') );
}

int
CompareToken(const char *token, const char *comparand)
{
    while (*comparand)
    {
        if (*token != *comparand) return 0;
        token++;
        comparand++;
    }
    if (!IsSeparator(*token)) return 0;
    return 1;
}

int
ScanToken(const char *token, char chr)
{
    while (!IsSeparator(*token))
    {
        if (*token++ == chr) return 1;
    }
    return 0;
}

char *
NextLine(char *pc)
{
    while (*pc != 0)
    {
        if (pc[0] == '\n' && pc[1] == '\r') return pc + 2;
        else if (pc[0] == '\n') return pc + 1;
        pc++;
    }
    return pc;
}

int
TokenLength(char *pc)
{
    int length = 0;

    while (!IsSeparator(*pc++)) length++;

    return length;
}

char *
NextToken(char *pc)
{
    /* Skip token */
    while (!IsSeparator(*pc)) pc++;

    /* Skip white spaces */
    while (*pc == ' ' || *pc == '\t') pc++;

    /* Check for end of line */
    if (*pc == '\n' || *pc == '\r' || *pc == 0) return 0;

    /* Check for comment */
    if (*pc == '#' || *pc == ';') return 0;

    return pc;
}

void
OutputHeader_stub(FILE *file, char *libname)
{
    fprintf(file, "; File generated automatically, do not edit! \n\n"
            ".586\n.model flat\n.code\n");
}

int
OutputLine_stub(FILE *fileDest, EXPORT *pexp)
{
    if (pexp->nCallingConvention == CC_STDCALL)
    {
        fprintf(fileDest, "PUBLIC _%.*s@%d\n_%.*s@%d: nop\n",
                pexp->nNameLength, pexp->pcName, pexp->nStackBytes,
                pexp->nNameLength, pexp->pcName, pexp->nStackBytes);
    }
    else if (pexp->nCallingConvention == CC_FASTCALL)
    {
        fprintf(fileDest, "PUBLIC @%.*s@%d\n@%.*s@%d: nop\n",
                pexp->nNameLength, pexp->pcName, pexp->nStackBytes,
                pexp->nNameLength, pexp->pcName, pexp->nStackBytes);
    }
    else if (pexp->nCallingConvention == CC_CDECL)
    {
        fprintf(fileDest, "PUBLIC _%.*s\n_%.*s: nop\n",
                pexp->nNameLength, pexp->pcName,
                pexp->nNameLength, pexp->pcName);
    }
    else if (pexp->nCallingConvention == CC_EXTERN)
    {
        fprintf(fileDest, "PUBLIC _%.*s\n_%.*s:\n",
                pexp->nNameLength, pexp->pcName,
                pexp->nNameLength, pexp->pcName);
    }

    return 1;
}

void
OutputHeader_def(FILE *file, char *libname)
{
    fprintf(file, 
            "; File generated automatically, do not edit!\n\n"
            "LIBRARY %s\n\n"
            "EXPORTS\n",
            libname);
}

int
OutputLine_def(FILE *fileDest, EXPORT *exp)
{
    fprintf(fileDest, " ");
    if (exp->nCallingConvention == CC_FASTCALL && !no_decoration)
    {
        fprintf(fileDest, "@");
    }

    fprintf(fileDest, "%.*s", exp->nNameLength, exp->pcName);

    if ((exp->nCallingConvention == CC_STDCALL || 
        exp->nCallingConvention == CC_FASTCALL) && !no_decoration)
    {
        fprintf(fileDest, "@%d", exp->nStackBytes);
    }

    if (exp->pcRedirection && !no_redirections)
    {
        int bAddDecorations = 1;

        fprintf(fileDest, "=");
        
        /* No decorations, if switch was passed or this is an external */
        if (no_decoration || ScanToken(exp->pcRedirection, '.'))
        {
            bAddDecorations = 0;
        }
        
        if (exp->nCallingConvention == CC_FASTCALL && bAddDecorations)
        {
            fprintf(fileDest, "@");
        }
        fprintf(fileDest, "%.*s", exp->nRedirectionLength, exp->pcRedirection);
        if ((exp->nCallingConvention == CC_STDCALL || 
            exp->nCallingConvention == CC_FASTCALL) && bAddDecorations)
        {
            fprintf(fileDest, "@%d", exp->nStackBytes);
        }
    }

    if (exp->nOrdinal != -1)
    {
        fprintf(fileDest, " @%d", exp->nOrdinal);
    }

    if (exp->nCallingConvention == CC_EXTERN)
    {
        fprintf(fileDest, " DATA");
    }

    fprintf(fileDest, "\n");

    return 1;
}

int
ParseFile(char* pcStart, FILE *fileDest)
{
    char *pc, *pcLine;
    int nLine;
    EXPORT exp;
    int included;

    //fprintf(stderr, "info: line %d, pcStart:'%.30s'\n", nLine, pcStart);
    
    /* Loop all lines */
    nLine = 1;
    for (pcLine = pcStart; *pcLine; pcLine = NextLine(pcLine), nLine++)
    {
        pc = pcLine;

        //fprintf(stderr, "info: line %d, token:'%d, %.20s'\n", 
        //        nLine, TokenLength(pcLine), pcLine);

        /* Skip white spaces */
        while (*pc == ' ' || *pc == '\t') pc++;

        /* Skip empty lines, stop at EOF */
        if (*pc == ';' || *pc <= '#') continue;
        if (*pc == 0) return 0;

        //fprintf(stderr, "info: line %d, token:'%.*s'\n", 
        //        nLine, TokenLength(pc), pc);

        /* Now we should get either an ordinal or @ */
        if (*pc == '@') exp.nOrdinal = -1;
        else exp.nOrdinal = atol(pc);

        /* Go to next token (type) */
        if (!(pc = NextToken(pc)))
        {
            fprintf(stderr, "error: line %d, unexpected end of line\n", nLine);
            return -10;
        }

        //fprintf(stderr, "info: Token:'%.10s'\n", pc);

        /* Now we should get the type */
        if (CompareToken(pc, "stdcall"))
        {
            exp.nCallingConvention = CC_STDCALL;
        }
        else if (CompareToken(pc, "cdecl") ||
                 CompareToken(pc, "varargs"))
        {
            exp.nCallingConvention = CC_CDECL;
        }
        else if (CompareToken(pc, "fastcall") ||
                 CompareToken(pc, "FASTCALL"))
        {
            exp.nCallingConvention = CC_FASTCALL;
        }
        else if (CompareToken(pc, "extern"))
        {
            exp.nCallingConvention = CC_EXTERN;
        }
        else if (CompareToken(pc, "stub"))
        {
            pc = NextToken(pc);
            printf("warning: stub skipped: %.*s\n", TokenLength(pc), pc);
            continue;
        }
        else
        {
            fprintf(stderr, "error: line %d, expected type, got '%.*s' %d\n", 
                    nLine, TokenLength(pc), pc, *pc);
            return -11;
        }

        //fprintf(stderr, "info: nCallingConvention: %d\n", exp.nCallingConvention);

        /* Go to next token (options or name) */
        if (!(pc = NextToken(pc)))
        {
            fprintf(stderr, "fail2\n");
            return -12;
        }

        /* Handle options */
        included = 1;
        while (*pc == '-')
        {
            if (CompareToken(pc, "-arch"))
            {
                /* Default to not included */
                included = 0;
                pc += 5;

                /* Look if we are included */
                while (*pc == '=' || *pc == ',')
                {
                    pc++;
                    if (CompareToken(pc, pszArchString) ||
                        CompareToken(pc, pszArchString2))
                    {
                        included = 1;
                    }
                    
                    /* Skip to next arch or end */
                    while (*pc > ',') pc++;
                }
            }
            else if (CompareToken(pc, "-i386"))
            {
                if (_stricmp(pszArchString, "i386") != 0) included = 0;
            }
            else if (CompareToken(pc, "-noname") ||
                     CompareToken(pc, "-norelay") ||
                     CompareToken(pc, "-ret64") ||
                     CompareToken(pc, "-private"))
            {
                /* silently ignore these */
            }
            else
            {
                fprintf(stderr, "info: ignored option: '%.*s'\n", 
                        TokenLength(pc), pc);
            }

            /* Go to next token */
            pc = NextToken(pc);
        }

        //fprintf(stderr, "info: Name:'%.10s'\n", pc);
        
        /* If arch didn't match ours, skip this entry */
        if (!included) continue;

        /* Get name */
        exp.pcName = pc;
        exp.nNameLength = TokenLength(pc);

        /* Handle parameters */
        exp.nStackBytes = 0;
        if (exp.nCallingConvention != CC_EXTERN)
        {
            //fprintf(stderr, "info: options:'%.10s'\n", pc);
            /* Go to next token */
            if (!(pc = NextToken(pc)))
            {
                fprintf(stderr, "fail4\n");
                return -13;
            }

            /* Verify syntax */
            if (*pc++ != '(')
            {
                fprintf(stderr, "error: line %d, expected '('\n", nLine);
                return -14;
            }
            
            /* Skip whitespaces */
            while (*pc == ' ' || *pc == '\t') pc++;

            exp.nStackBytes = 0;
            while (*pc >= '0')
            {
                if (CompareToken(pc, "long"))
                    exp.nStackBytes += 4;
                else if (CompareToken(pc, "double"))
                    exp.nStackBytes += 8;
                else if (CompareToken(pc, "ptr") ||
                         CompareToken(pc, "str") ||
                         CompareToken(pc, "wstr"))
                    exp.nStackBytes += sizeof(void*);
                else
                    fprintf(stderr, "error: line %d, expected type, got: %.10s\n", nLine, pc);

                /* Go to next parameter */
                if (!(pc = NextToken(pc)))
                {
                    fprintf(stderr, "fail5\n");
                    return -15;
                }
            }

            /* Check syntax */
            if (*pc++ != ')')
            {
                fprintf(stderr, "error: line %d, expected ')'\n", nLine);
                return -16;
            }
        }

        /* Get optional redirection */
        if ((pc = NextToken(pc)))
        {
            exp.pcRedirection = pc;
            exp.nRedirectionLength = TokenLength(pc);

            /* Check syntax (end of line) */
            if (NextToken(pc))
            {
                 fprintf(stderr, "error: line %d, additional tokens after ')'\n", nLine);
                 return -17;
            }
        }
        else
        {
            exp.pcRedirection = 0;
            exp.nRedirectionLength = 0;
        }

        OutputLine(fileDest, &exp);
    }

    return 0;
}


void usage(void)
{
    printf("syntax: spec2pdef [<options> ...] <source file> <dest file>\n"
           "Possible options:\n"
           "  -d=<file> --dll=<file>   names the dll\n"
           "  -h --help   prints this screen\n"
           "  -s --stubs  generates an asm lib stub\n"
           "  -n --no-decoration  removes @xx decorations from def file\n"
           "  --arch <arch> Set architecture to <arch>. (i386, x86_64, arm)\n");
}

int main(int argc, char *argv[])
{
    size_t nFileSize;
    char *pszSource, *pszDllName = 0;
    char achDllName[40];
    FILE *file;
    int result, i;

    if (argc < 2)
    {
        usage();
        return -1;
    }

    /* Default to def file */
    OutputLine = OutputLine_def;
    OutputHeader = OutputHeader_def;

    /* Read options */
    for (i = 1; i < argc && *argv[i] == '-'; i++)
    {
        if ((_stricmp(argv[i], "--help") == 0) ||
            (_stricmp(argv[i], "-h") == 0))
        {
            usage();
            return 0;
        }
        else if ((_stricmp(argv[i], "--stublib") == 0) ||
                 (_stricmp(argv[i], "-s") == 0))
        {
            OutputLine = OutputLine_stub;
            OutputHeader = OutputHeader_stub;
        }
        else if ((_stricmp(argv[i], "--dll") == 0) ||
                 (_stricmp(argv[i], "-d") == 0))
        {
            pszDllName = argv[i + 1];
            i++;
        }
        else if ((_stricmp(argv[i], "--no-decoration") == 0) ||
                 (_stricmp(argv[i], "-n") == 0))
        {
            no_decoration = 1;
        }
        else if ((_stricmp(argv[i], "--no-redirection") == 0) ||
                 (_stricmp(argv[i], "-r") == 0))
        {
            no_redirections = 1;
        }
        else if ((_stricmp(argv[i], "--arch") == 0))
        {
            pszArchString = argv[i + 1];
            i++;
        }
        else
        {
            fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
            return -1;
        }
    }

    if ((_stricmp(pszArchString, "x86_64") == 0) ||
        (_stricmp(pszArchString, "ia64") == 0))
    {
        pszArchString2 = "win64";
    }
    else
        pszArchString2 = "win32";

    /* Set a default dll name */
    if (!pszDllName)
    {
        char *p1, *p2;
        int len;

        p1 = strrchr(argv[i], '\\');
        if (!p1) p1 = strrchr(argv[i], '/');
        p2 = p1 = p1 ? p1 + 1 : argv[i];

        /* walk up to '.' */
        while (*p2 != '.' && *p2 != 0) p2++;
        len = p2 - p1;
        if (len >= sizeof(achDllName) - 5)
        {
            fprintf(stderr, "name too long: %s\n", p1);
            return -2;
        }

        strncpy(achDllName, p1, len);
        strncpy(achDllName + len, ".dll", sizeof(achDllName) - len);
        pszDllName = achDllName;
    }

    /* Open input file argv[1] */
    file = fopen(argv[i], "r");
    if (!file)
    {
        fprintf(stderr, "error: could not open file %s ", argv[i]);
        return -3;
    }

    /* Get file size */
    fseek(file, 0, SEEK_END);
    nFileSize = ftell(file);
    rewind(file);

    /* Allocate memory buffer */
    pszSource = malloc(nFileSize + 1);
    if (!pszSource) return -4;

    /* Load input file into memory */
    nFileSize = fread(pszSource, 1, nFileSize, file);
    fclose(file);

    /* Zero terminate the source */
    pszSource[nFileSize] = '\0';

    /* Open output file */
    file = fopen(argv[i + 1], "w");
    if (!file)
    {
        fprintf(stderr, "error: could not open output file %s ", argv[i + 1]);
        return -5;
    }

    OutputHeader(file, pszDllName);

    result = ParseFile(pszSource, file);

    if (OutputHeader == OutputHeader_stub) fprintf(file, "\nEND\n");

    fclose(file);

    return result;
}
