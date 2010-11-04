#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

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

int
_strcmpx(const char *string1, const char *string2)
{
    while (*string2)
    {
        if (*string1 != *string2) return (*string1 - *string2);
        string1++;
        string2++;
    }
    return 0;
}

char *
NextLine(char *pc)
{
    /* Skip until the next line or EOF */
    while (*pc != '\n' && *pc != '\r' && *pc != 0) pc++;
    if (*pc++ == '\r') pc++;
    return pc;
}

char *
NextToken(char *pc)
{
    /* Skip string */
    while (isprint(*pc) && !isspace(*pc)) pc++;

    /* Skip white spaces */
    while (*pc == ' ' || *pc == '\t') pc++;

    return pc;
}

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

int
ParseLine(char* pcLine, int nLine, EXPORT *pexp)
{
    char *pcCurrent;

    //fprintf(stderr, "info: line %d, pcLine:'%.30s'\n", nLine, pcLine);

    pcCurrent = pcLine;

    /* skip white spaces */
    while (*pcCurrent == ' ') pcCurrent++;

    /* Check for line break */
    if (*pcCurrent == '\n' || *pcCurrent == '\r' || *pcCurrent == 0) return 0;

    /* Check for comment */
    if (*pcCurrent == '#' || *pcCurrent == ';') return 0;

    /* Now we should get either an ordinal or @ */
    if (*pcCurrent == '@') pexp->nOrdinal = -1;
    else pexp->nOrdinal = atol(pcCurrent);

    /* Go to next token */
    pcCurrent = NextToken(pcCurrent);

    //fprintf(stderr, "info: Token:'%.10s'\n", pcCurrent);

    /* Now we should get the calling convention */
    if (_strcmpx(pcCurrent, "stdcall ") == 0)
    {
        pexp->nCallingConvention = CC_STDCALL;
    }
    else if (_strcmpx(pcCurrent, "cdecl ") == 0)
    {
        pexp->nCallingConvention = CC_CDECL;
    }
    else if (_strcmpx(pcCurrent, "fastcall ") == 0 ||
             _strcmpx(pcCurrent, "FASTCALL ") == 0)
    {
        pexp->nCallingConvention = CC_FASTCALL;
    }
    else if (_strcmpx(pcCurrent, "extern ") == 0)
    {
        pexp->nCallingConvention = CC_EXTERN;
    }
    else
    {
        fprintf(stderr, "error: line %d, expected cc, got (%p) %d\n", nLine, pcCurrent, *pcCurrent);
        return -1;
    }

    //fprintf(stderr, "info: nCallingConvention: %d\n", pexp->nCallingConvention);

    /* Go to next token */
    pcCurrent = NextToken(pcCurrent);

    /* Handle option */
    while (*pcCurrent == '-')
    {
        fprintf(stderr, "info: got option: '%.10s'\n", pcCurrent);
        // FIXME: handle options

        /* Go to next token */
        pcCurrent = NextToken(pcCurrent);
    }

    //fprintf(stderr, "info: Name:'%.10s'\n", pcCurrent);

    /* Get name */
    pexp->pcName = pcCurrent;
    while (isprint(*pcCurrent) && *pcCurrent != '(') pcCurrent++;
    pexp->nNameLength = pcCurrent - pexp->pcName;

    /* Handle parameters */
    pexp->nStackBytes = 0;
    if (pexp->nCallingConvention != CC_EXTERN)
    {
        //fprintf(stderr, "info: options:'%.10s'\n", pcCurrent);

        /* Syntax check */
        if (*pcCurrent++ != '(')
        {
            fprintf(stderr, "error: line %d, expected '('\n", nLine);
            return -1;
        }
        
        pexp->nStackBytes = 0;
        while (isalpha(*pcCurrent))
        {
            if (_strcmpx(pcCurrent, "long") == 0)
                pexp->nStackBytes += 4;
            else if (_strcmpx(pcCurrent, "double") == 0)
                pexp->nStackBytes += 8;
            else if (_strcmpx(pcCurrent, "ptr") == 0 ||
                     _strcmpx(pcCurrent, "str") == 0 ||
                     _strcmpx(pcCurrent, "wstr") == 0)
                pexp->nStackBytes += sizeof(void*);
            else
                fprintf(stderr, "error: line %d, expected type, got: %.10s\n", nLine, pcCurrent);

            /* Go to next parameter */
            while (isalpha(*pcCurrent)) pcCurrent++;
            while (isspace(*pcCurrent)) pcCurrent++;
        }

        /* Check syntax */
        if (*pcCurrent++ != ')')
        {
            fprintf(stderr, "error: line %d, expected ')'\n", nLine);
            return -1;
        }

        /* Skip white spaces */
        while (*pcCurrent == ' ' || *pcCurrent == '\t') pcCurrent++;
    }
    
    /* Get optional redirection */
    if (isprint(*pcCurrent))
    {
        pexp->pcRedirection = pcCurrent;
        while (isprint(*pcCurrent) && !isspace(*pcCurrent)) pcCurrent++;
        pexp->nRedirectionLength = pcCurrent - pexp->pcRedirection;
    }
    else
    {
        pexp->pcRedirection = 0;
        pexp->nRedirectionLength = 0;
    }
    
    return 1;
}

int
OutputLine_def(FILE *fileDest, EXPORT *exp)
{
    fprintf(fileDest, " ");
    if (exp->nCallingConvention == CC_FASTCALL) fprintf(fileDest, "@");
    fprintf(fileDest, "%.*s", exp->nNameLength, exp->pcName);

    if (exp->nCallingConvention == CC_STDCALL || 
        exp->nCallingConvention == CC_FASTCALL)
    {
        fprintf(fileDest, "@%d", exp->nStackBytes);
    }

    if (exp->pcRedirection)
    {
        if (exp->nCallingConvention == CC_FASTCALL) fprintf(fileDest, "@");
        fprintf(fileDest, "=%.*s", exp->nRedirectionLength, exp->pcRedirection);
        if (exp->nCallingConvention == CC_STDCALL || 
            exp->nCallingConvention == CC_FASTCALL)
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
OutputLine(FILE *fileDest, EXPORT *exp)
{
    fprintf(fileDest, "_NAME(%.*s,%s,%d)",
            exp->nNameLength, exp->pcName,
            astrCallingConventions[exp->nCallingConvention],
            exp->nStackBytes);

    if (exp->pcRedirection)
    {
        fprintf(fileDest, "= _NAME(%.*s,%s,%d)",
                exp->nRedirectionLength, exp->pcRedirection,
                astrCallingConventions[exp->nCallingConvention],
                exp->nStackBytes);
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

void
OutputHeader(FILE *file)
{
    fprintf(file, 
            "; File generated automatically, do not edit!\n\n"
            "LIBRARY ntoskrnl.exe\n\n"
            "EXPORTS\n"
            "#define FOOL(x) x\n"
            "#ifdef _MSC_VER\n"
            "#define _NAME_STDCALL(name, stackbytes) name\n"
            "#define _NAME_FASTCALL(name, stackbytes) name\n"
            "#define _NAME_CDECL(name, stackbytes) name\n"
            "#else\n"
            "#define _NAME_STDCALL(name, stackbytes) FOOL(name)@stackbytes\n"
            "#define _NAME_FASTCALL(name, stackbytes) FOOL(@)FOOL(name)@stackbytes\n"
            "#define _NAME_CDECL(name, stackbytes) FOOL(name)\n"
            "#endif\n"
            "#define _NAME_EXTERN(name, stackbytes) name\n"
            "#define _NAME(name, cc, stackbytes) _NAME_##cc(name, stackbytes)\n");
}


int main(int argc, char *argv[])
{
    size_t nFileSize;
    char *pszSource, *pcLine;
    int nLine, result;
    FILE *fileSource, *fileDest;
    EXPORT exp;

    /* Open input file argv[1] */
    fileSource = fopen(argv[1], "r");
    if (!fileSource)
    {
        fprintf(stderr, "error: could not open file %s ", argv[1]);
        return -1;
    }

    /* Get file size */
    fseek(fileSource, 0, SEEK_END);
    nFileSize = ftell(fileSource);
    rewind(fileSource);

    /* Allocate memory buffer */
    pszSource = malloc(nFileSize + 1);
    if (!pszSource) return -1;

    /* Load input file into memory */
    nFileSize = fread(pszSource, 1, nFileSize, fileSource);

    /* Zero terminate the source */
    pszSource[nFileSize] = '\0';

    // open output file argv[2]
    fileDest = fopen(argv[2], "w");
    if (!fileDest)
    {
        fprintf(stderr, "error: could not open output file %s ", argv[2]);
        return -1;
    }

    OutputHeader(fileDest);

    pcLine = pszSource;
    for (nLine = 1; *pcLine != 0; pcLine = NextLine(pcLine), nLine++)
    {
        /* Parse the spec file line */
        result = ParseLine(pcLine, nLine, &exp);

        if (result == 1)
        {
            OutputLine(fileDest, &exp);
        }
        else if (result == -1) break;

        
    }

    fclose(fileDest);
    fclose(fileSource);
    return 0;
}
