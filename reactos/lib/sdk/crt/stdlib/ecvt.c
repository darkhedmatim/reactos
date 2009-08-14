/*
 * PROJECT:         ReactOS CRT
 * LICENSE:         See COPYING in the top level directory
 * PURPOSE:         CRT's ecvt
 * FILE:            lib/sdk/crt/stdlib/ecvt.c
 * PROGRAMERS:      Gregor Schneider (parts based on ecvtbuf.c by DJ Delorie)
 */

#include <precomp.h>

/*
 * @implemented
 */
char *
_ecvt (double value, int ndigits, int *decpt, int *sign)
{
  char *ecvtbuf, *cvtbuf;
  char *s, *d;

  s = cvtbuf = (char*)malloc(ndigits + 18); /* sign, dot, null, 15 for alignment */
  d = ecvtbuf = (char*)malloc(DBL_MAX_10_EXP + 10);

  *sign = 0;
  *decpt = 0;

  if (cvtbuf == NULL || ecvtbuf == NULL)
  {
    return NULL;
  }

  sprintf(cvtbuf, "%-+.*E", ndigits, value);
  /* Treat special values */
  if (strncmp(s, "NaN", 3) == 0)
  {
    memcpy(ecvtbuf, s, 4);
  }
  else if (strncmp(s + 1, "Inf", 3) == 0)
  {
    memcpy(ecvtbuf, s, 5);
  }
  else 
  {
    /* Set the sign */
    if (*s && *s == '-')
    {
      *sign = 1;
    }
    s++;
    /* Copy the first digit */
    if (*s && *s != '.')
    {
      if (d - ecvtbuf < ndigits)
      {
        *d++ = *s++;
      }
      else
      {
        s++;
      }
    }
    /* Skip the decimal point */
    if (*s && *s == '.')
    {
      s++;
    }
    /* Copy fractional digits */
    while (*s && *s != 'E')
    {
      if (d - ecvtbuf < ndigits)
      {
        *d++ = *s++;
      }
      else
      {
        s++;
      }
    }
    /* Skip the exponent */
    if (*s && *s == 'E')
    {
      s++;
    }
    /* Set the decimal point to the exponent value plus the one digit we copied */
    *decpt = atoi(s) + 1;
    /* Handle special decimal point cases */
    if (cvtbuf[1] == '0')
    {
      *decpt = 0;
    }
    if (ndigits < 1)
    {
      /* Need enhanced precision*/
      char* tbuf = (char*)malloc(ndigits + 18);
      sprintf(tbuf, "%-+.*E", ndigits + 2, value);
      if (tbuf[1] >= '5')
      {
        (*decpt)++;
      }
      free(tbuf);
    }
    /* Pad with zeroes */
    while (d - ecvtbuf < ndigits)
    {
      *d++ = '0';
    }
    /* Terminate */
    *d = '\0';
  }
  free(cvtbuf);
  return ecvtbuf;
}
