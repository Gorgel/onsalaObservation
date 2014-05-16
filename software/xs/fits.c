/**************************** XS ********************************************
Copyright (C) 2000-2012  P. Bergman

This program is free software; you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
software; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA  02111-1307 USA                             
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FITSKNOWN
#include "fits.h"

#define TRUE  1
#define FALSE 0

#define IS_TTYPE(i)  ((i)==KW_TTYPE || (i)==KW_TFORM || (i)==KW_TUNIT)

void send_line(char *);

static char  record[PHYSRECLEN+1]="";
static char  keyword[KEYWORDLEN+1];
static char  line[LOGRECLEN+1];
static char  variable[MAXVARLEN+1];
static char  buf[200];
static char *value;
static char *current = NULL;
static FILE *fits = NULL;
static int BinaryTable = 0, FieldCnt=0;
static field *fieldPtr = NULL;

double swap2bytes(short int v)
{
    char tmp;
    union {
        char c[2];
        short int s;
    } b;
    
    b.s = v;
    tmp = b.c[0];
    b.c[0] = b.c[1];
    b.c[1] = tmp;
    
    return ((double)b.s);
}

double swap4bytes(long int v)
{
    char tmp;
    union {
        char c[4];
        long int s;
    } b;
    
    b.s = v;
    tmp = b.c[0]; b.c[0] = b.c[3]; b.c[3] = tmp;
    tmp = b.c[1]; b.c[1] = b.c[2]; b.c[2] = tmp;
    
    return ((double)b.s);
}

double swapf(float f)
{
    char tmp;
    union {
        char  c[4];
	    float f;
	} b;
	
    b.f = f;
    tmp = b.c[0]; b.c[0] = b.c[3]; b.c[3] = tmp;
    tmp = b.c[1]; b.c[1] = b.c[2]; b.c[2] = tmp;
	
	return ((double)b.f);
}

double swapd(double d)
{
    char tmp;
    union {
        char  c[8];
	    double d;
	} b;
	
    b.d = d;
    tmp = b.c[0]; b.c[0] = b.c[7]; b.c[7] = tmp;
    tmp = b.c[1]; b.c[1] = b.c[6]; b.c[6] = tmp;
    tmp = b.c[2]; b.c[2] = b.c[5]; b.c[5] = tmp;
    tmp = b.c[3]; b.c[3] = b.c[4]; b.c[4] = tmp;
	
	return (b.d);
}

void ClearFITSwords()
{
    int i;
    
    void destroy_fieldlist();
    
    BinaryTable = 0;
    FieldCnt = 0;
    
    if (fieldPtr) destroy_fieldlist();
    fieldPtr = NULL;

    for (i=0; i<KW_END; i++) known[i].hit = FALSE;
}

int extractFITScard()
{
    char *ptr, *quote;
    int i, nkw;
    
    field *NewType(int, char *);
    void AddForm(field *, char *);
    void AddUnit(field *, char *);

    ptr = current;
    strncpy(line, current , LOGRECLEN);    /* get next 80 characters        */
    line[LOGRECLEN] = '\0';                /* place an end of string        */
    strncpy(keyword, line, KEYWORDLEN);    /* get first 8 characters        */
    keyword[KEYWORDLEN] = '\0';            /* terminate keyword             */
#ifdef FITSDEBUG
    printf("line=   %s\n", line);
    printf("keyword=%s\n", keyword);
    printf("known[KW_END].hit = %d\n", known[KW_END].hit);
#endif

    ptr = current+KEYWORDLEN;              /* point past keyword            */
    if (strncmp(ptr, "= ", 2) == 0) {      /* is if followed by '= '?       */
        ptr += 2;                          /* point past '= '               */
        value = ptr;
        i = 0;                             /* count chars. in value field   */
        /* in the following while loop we collect all characters from the   */
        /* value field following the '= '. We allow MAXVARLEN characters at */
        /* most. The loop takes care of enclosing quotes, in case the value */
        /* is a string.                                                     */
        while (i < MAXVARLEN) {            /* extract value field           */
            switch (*ptr) {
              case '\'':                   /* quote encountered -> string   */
                ptr++;                     /* point past quote              */
                do {
                    quote = strchr(ptr, '\''); /* we expect a second quote  */
                    if (quote == NULL) return (-1);  /* else error ...      */
                    *quote++ = '\0';       /* put end of string character   */
                    strcpy(&variable[i], ptr);  /* copy string              */
                    i += strlen(ptr);      /* count characters in string    */
                    if (*quote == '\'') {
                        variable[i++] = '\'';
                        ptr = quote+1;
                    }
                } while ((*quote == '\'') && i < MAXVARLEN);
#ifdef FITSDEBUG
                if (strlen(variable) < KEYWORDLEN)
                    send_line("Non-standard string variable.");
#endif
                i = MAXVARLEN;
                break;
              case '/':                    /* start of comment              */
                variable[i] = '\0';        /* put end of string             */
                i = MAXVARLEN;             /* set condition for while loop  */
                break;
              default:
                variable[i++] = *ptr++;    /* count character               */
                if (i == MAXVARLEN) variable[i] = '\0'; /* too many ?       */
                break;
            }
        }
    } else {
        value = NULL;
    }

    /* Now, 'keyword' holds the FITS keyword string and 'variable' possibly */
    /* holds the value of the associated variable as an alphabetic or nume- */
    /* ric string. Loop through all known keywords and compare with what we */
    /* have read. Once we find a match, we know how to interpret the value  */
    /* field.                                                               */
#ifdef FITSDEBUG
    printf("keyword: '%s'  variable: '%s'\n", keyword, variable);
#endif
    nkw = sizeof(known)/sizeof(struct fitskey);
    
    for (i = 0; i < nkw; i++) {
        if (!BinaryTable || !IS_TTYPE(i)) {
            if (known[i].hit == TRUE) {
	        continue;
	    }
        }
        if (strncmp(keyword, known[i].kw, KEYWORDLEN) == 0) { /* known ?    */
            known[i].hit = TRUE;         /* indicate that we found this one */
            /*  if (i == KW_COMMENT) printf("COMMENT %s\n", line);
              if (i == KW_HISTORY) printf("HISTORY %s\n", line); */
#ifdef FITSDEBUG
            printf("keyword: '%s'[%d]", keyword, known[i].vartyp);
#endif
            /* if this keyword requires a value, but we didn't see any, we  */
            /* are in trouble                                               */
            if ((known[i].vartyp != NVARTYPE) && (value == NULL)) return (-1);

            /* take action depending on variable type                       */
            switch (known[i].vartyp) {
              case NVARTYPE:             /* no value required               */
#ifdef FITSDEBUG
                printf("\tno variable\n");
#endif
                break;
              case BOOLTYPE:             /* logical variable                */
                if (variable[19] == 'T') known[i].val.l = TRUE;
                else                     known[i].val.l = FALSE;
#ifdef FITSDEBUG
                printf("\tboolean = %ld\n", known[i].val.l);
#endif
                break;
              case CHARTYPE:             /* character string                */
                strncpy((char *)known[i].val.str, variable, 16);
#ifdef FITSDEBUG
                printf("\tstring  = %s\n", known[i].val.str);
#endif
                break;
              case LONGTYPE:             /* long integer                    */
                known[i].val.l = atoi(variable);
#ifdef FITSDEBUG
                printf("\tinteger = %ld\n", known[i].val.l);
#endif
                break;
              case REALTYPE:             /* float                           */
                known[i].val.d = STRTOD(variable);
#ifdef FITSDEBUG
                printf("\tdouble  = %e\n", known[i].val.d);
#endif
                break;
              case CPLXTYPE:             /* complex                         */
                known[i].val.c[0] = STRTOD(variable);
                known[i].val.c[1] = STRTOD(variable+20);
#ifdef FITSDEBUG
                printf("\tdouble  = %f, %f\n", 
                       known[i].val.c[0], known[i].val.c[1]);
#endif
                break;
            }
            if (i == KW_XTENSION && !BinaryTable) {
                if (strcmp(known[i].val.str, "BINTABLE")==0) {
                    BinaryTable = 1;
                }
            }
            return (i);
        }
    }
    /* here we get for unknown keywords                                     */
    if (BinaryTable) {
        if (strncmp(keyword, "TTYPE", 5)==0) {
            FieldCnt++;
            fieldPtr = NewType(FieldCnt, variable);
            return 0;
        } else if (strncmp(keyword, "TFORM", 5)==0) {
            AddForm(fieldPtr, variable);
            return 0;
        } else if (strncmp(keyword, "TUNIT", 5)==0) {
            AddUnit(fieldPtr, variable);
            return 0;
        }
    }
#ifdef FITSDEBUG
    sprintf(buf, "Ignoring FITS keyword '%s'.", keyword);
    send_line(buf);
#endif
    return (0);
}

void CloseFITSfile()
{
    if (fits) {
        fclose(fits);
        fits = NULL;
    }
}
    
struct fitskey *readFITSheader(const char *name, int bin_tab)
{   
    int len, i, nkw, bunch = PHYSRECLEN;
    
    fits = fopen(name, "r");                  /* open the file for reading  */
    if (!fits) {                       /* test for failure           */
        sprintf(buf, "Cannot open file '%s'.", name);
        send_line(buf);
        return (NULL);
    }
    nkw = sizeof(known)/sizeof(struct fitskey);
    for (i=0; i<nkw; i++) known[i].hit = FALSE;
#ifdef FITSDEBUG
    printf("reading header record, nkw=%d\n", nkw);
    printf("KW_SIMPLE = %d\n", KW_SIMPLE);
    printf("KW_END = %d  hit = %d\n", KW_END, known[KW_END].hit);
#endif

    /* If Odin binary table, advance one record before reading */
    if (bin_tab == 1) { /* Handle Odin binary tables */
        if (fseek(fits, (long)bunch, SEEK_SET)) {
            sprintf(buf, "Couldn't advance by %d bytes.", bunch);
            send_line(buf);
        }
    }
    
    /* read one record of 2880 bytes                            */
    len = fread(record, sizeof(char), bunch, fits);
    if (len != bunch) {
        sprintf(buf, "Error reading header record (%d != %d).",
                len, bunch);
        send_line(buf);
        send_line("No of read bytes doesn't match the record length.");
    }
    current = record;
    
    i = 0;
    do {
        if ((current - record) >= bunch) {
            len = fread(record, sizeof(char), bunch, fits);
            if (len != bunch) {
                sprintf(buf, "Error reading header record (%d != %d).",
                        len, bunch);
                send_line(buf);
                send_line("No of read bytes doesn't match the record length.");
            }
            current = record;
        }
        if (extractFITScard() == -1) {
            sprintf(buf, "Error parsing FITS card '%s'.", current);
            send_line(buf);
        }
        current += LOGRECLEN;
        i++;
#ifdef FITSDEBUG
        printf("i=%d %s END.hit=%d\n", i, current, known[KW_END].hit);
#endif
    } while (known[KW_END].hit == FALSE);

    
    /* we require 'SIMPLE', 'BITPIX', 'NAXIS' to be present. The value of   */
    /* 'NAXIS' must not be greater than MAXNAXIS.                           */
    if (!known[KW_SIMPLE].hit && !BinaryTable) {
        send_line("Keyword SIMPLE not present!");
        CloseFITSfile();
        return NULL;
    }
    if (known[KW_BITPIX].hit == 0) {
        send_line("Keyword BITPIX not found!");
        CloseFITSfile();
        return NULL;
    }
    if (known[KW_NAXIS ].hit == 0) {
        send_line("Keyword NAXIS not found!");
        CloseFITSfile();
        return NULL;
    }
    if (known[KW_NAXIS ].val.l > MAXNAXIS) {
        send_line("Keyword NAXIS value too big!");
        CloseFITSfile();
        return NULL;
    }
    /* we require all 'NAXISn' keywords to be present, where n is the value */
    /* of 'NAXIS',e.g. if NAXIS was 3, we'll need NAXIS1, NAXIS2 and NAXIS3 */
    for (i = 1; i <= known[KW_NAXIS].val.l; i++) {
        if (known[KW_NAXIS+i].hit == 0) {
            sprintf(buf, "Couldn't find keyword NAXIS%d.", i);
            send_line(buf);
            CloseFITSfile();
            return NULL;
        }
    }
    return (known);
}

int readFITSdata(int nobjs, int size, void *data)
{
    int bytes, len, left, bunch;
    char *ptr;

    if (!data) {
         send_line("readFITSdata: Null data pointer.");
         return 0;
    }
    if (!fits) {
         send_line("readFITSdata: Null file pointer.");
         return 0;
    }
    
    ptr = (char *)data;
    bytes = size*nobjs;
    
    left = bytes;
    while (left) {
        if (left >= PHYSRECLEN) {
            bunch = PHYSRECLEN;
        } else {
            bunch = left;
        }
#ifdef FITSDEBUG
        printf("reading data record (%d bytes of %d) %d\n", bunch, bytes, left);
#endif
        len = fread(record, sizeof(char), bunch, fits);
        if (len != bunch) {
            sprintf(buf, "Error reading data record (%d != %d, %d of %d).",
                    len, bunch, left, bytes); 
            send_line(buf);
            return (0);
        }
        left -= bunch;
        memcpy(ptr, record, bunch);
        ptr += bunch;
    }
#ifdef FITSDEBUG
    printf("Read %d bytes.\n", bytes);
#endif
    return (bytes);
}

static int card;

void addFITScard(int index, struct fitskey *fk)
{
    known[index].hit = 1;
    known[index].vartyp = fk->vartyp;
    memcpy(known[index].val.str, fk->val.str, 16);
#ifdef FITSDEBUG
    printf("added card %d '%s'\n", index, known[index].kw);
#endif
}
 
void insertFITScard(struct fitskey *fk)
{
    int i, n;

    n = 0;
    for (i = 0; i < KEYWORDLEN; i++) line[n++] = fk->kw[i];
    if (fk->vartyp == NVARTYPE) {
#ifdef FITSDEBUG
            printf("void\n");
#endif
/*        line[n++] = '\'';
        for (i = 0; i < strlen(fk->val.str); i++) {
            line[n++] = fk->val.str[i];
        }
        line[n++] = '\''; */
    } else {
        line[n++] = '=';
        line[n++] = ' ';
        switch (fk->vartyp) {
          case BOOLTYPE:
#ifdef FITSDEBUG
            printf("boolean: %ld\n", fk->val.l);
#endif
            while (n < 29) line[n++] = ' ';
            if (fk->val.l) line[n++] = 'T';
            else                line[n++] = 'F';
            break;
          case CHARTYPE:
#ifdef FITSDEBUG
            printf("character: '%s'  len=%d\n", fk->val.str, strlen(fk->val.str));
#endif
            line[n++] = '\'';
            for (i = 0; i < (int)strlen(fk->val.str); i++) {
                line[n++] = fk->val.str[i];
            }
            line[n++] = '\'';
            break;
          case LONGTYPE:
#ifdef FITSDEBUG
            printf("integer: %ld\n", fk->val.l);
#endif
            sprintf(variable, "%20ld", fk->val.l);
            for (i = 0; i < 20; i++) {
                line[n++] = variable[i];
            }
            break;
          case REALTYPE:
#ifdef FITSDEBUG
            printf("floating: %f\n", fk->val.d);
#endif
            sprintf(variable, "%20.13e", fk->val.d);
            for (i = 0; i < 20; i++) {
                line[n++] = variable[i];
            }
            break;
          case CPLXTYPE:
#ifdef FITSDEBUG
            printf("complex: %f, %f\n", fk->val.c[0], fk->val.c[1]);
#endif
            sprintf(variable, "%20f%20f", fk->val.c[0], fk->val.c[1]);
            for (i = 0; i < MAXVARLEN; i++) {
                line[n++] = variable[i];
            }
            break;
        }
    }
    
    while (n < UPTOCOMMENT) line[n++] = ' ';


/* PB 20120505 Removed slash for NVARTYPE keywords */
    if (fk->vartyp == NVARTYPE)
        line[n++] = ' ';
    else
        line[n++] = '/';

    line[n++] = ' ';
    
    for (i=0; i<COMMENTLEN; i++) {
        if (fk->c[i] == '\0' || n >= LOGRECLEN) break;
        line[n++] = fk->c[i];
    }
    
    while (n < LOGRECLEN) {
        line[n++] = ' ';
    }
/* PB 20120505 Removed slash for NVARTYPE keywords */
/*    line[LOGRECLEN-1] = '\n'; */
    line[LOGRECLEN] = '\0';
    
    strncpy(&record[card*LOGRECLEN], line, LOGRECLEN);
    card++;
#ifdef FITSDEBUG
    printf("%s", line);
#endif    
}

int writeFITSheader(const char *name)
{   
    int len, i;

    if (name) fits = fopen(name, "w");
    if (!fits) {
        sprintf(buf, "Cannot open file '%s' for writing.", name);
        send_line(buf);
        return (-1);
    }
    
    card = 0;
    known[KW_SIMPLE].val.l = 1L;
    insertFITScard(&known[KW_SIMPLE]);
    insertFITScard(&known[KW_BITPIX]);
    insertFITScard(&known[KW_NAXIS]);
    if (known[KW_NAXIS].val.l > MAXNAXIS) {
        send_line("Maximum number of axes exceeded.");
        CloseFITSfile();
        return (-1);
    }
    for (i = 0; i < known[KW_NAXIS].val.l; i++) {
        insertFITScard(&known[KW_NAXIS1+i]);
    }
    for (i = 0; i < known[KW_NAXIS].val.l; i++) {
        insertFITScard(&known[KW_CTYPE1+i*5]);
        insertFITScard(&known[KW_CRPIX1+i*5]);
        insertFITScard(&known[KW_CRVAL1+i*5]);
        insertFITScard(&known[KW_CDELT1+i*5]);
        if (known[KW_CROTA1+i*5].hit) {
            insertFITScard(&known[KW_CROTA1+i*5]);
        }
    }
    for (i = KW_CROTA5+1; i <= KW_END; i++) {
        if (known[i].hit) {
            insertFITScard(&known[i]); 
        }
        if (card == (PHYSRECLEN/LOGRECLEN)) {
#ifdef FITSDEBUG
            printf("writing header record\n");
#endif
            len = fwrite(record, sizeof(char), PHYSRECLEN, fits);
            if (len != PHYSRECLEN) {
                send_line("Error writing header record.");
                CloseFITSfile();
                return (-1);
            }
            card = 0;
        }
    }
    memset(line, ' ', LOGRECLEN-1);
    line[LOGRECLEN-1] = ' ';
    line[LOGRECLEN] = '\0';
    if (card) {
        while (card < (PHYSRECLEN/LOGRECLEN)) {
            strncpy(&record[card*LOGRECLEN], line, LOGRECLEN);
            card++;
#ifdef FITSDEBUG
            printf("%s", line);
#endif    
        }
#ifdef FITSDEBUG
        printf("writing header record\n");
#endif
        len = fwrite(record, sizeof(char), PHYSRECLEN, fits);
        if (len != PHYSRECLEN) {
            send_line("Error writing header record.");
            CloseFITSfile();
            return (-1);
        }
        card = 0;
    }
    return (0);
}

int writeBINTABLEheader(fkey *fk, int nf)
{   
    int len, i;
    fkey *t;

    if (fits == NULL) {
        send_line("Can't open file.");
        return (-1);
    }
    
    card = 0;
    insertFITScard(&known[KW_XTENSION]);
    insertFITScard(&known[KW_BITPIX]);
    insertFITScard(&known[KW_NAXIS]);
    if (known[KW_NAXIS].val.l > MAXNAXIS) {
        send_line("Maximum number of axes exceeded.");
        CloseFITSfile();
        return (-1);
    }
    for (i = 0; i < known[KW_NAXIS].val.l; i++) {
        insertFITScard(&known[KW_NAXIS1+i]);
    }
    insertFITScard(&known[KW_PCOUNT]);
    insertFITScard(&known[KW_GCOUNT]);
    insertFITScard(&known[KW_TFIELDS]);
    
    t = fk;
    for (i = 0; i<3*nf; i++) {
        if (t->hit) insertFITScard(t);
        if (card == (PHYSRECLEN/LOGRECLEN)) {
#ifdef FITSDEBUG
            printf("writing header record\n");
#endif
            len = fwrite(record, sizeof(char), PHYSRECLEN, fits);
            if (len != PHYSRECLEN) {
                send_line("Error writing header record.");
                CloseFITSfile();
                return (-1);
            }
            card = 0;
        }
        t++;
    }
    
    for (i = KW_EXTNAME; i <= KW_END; i++) {
        if (known[i].hit) {
            insertFITScard(&known[i]); 
        }
        if (card == (PHYSRECLEN/LOGRECLEN)) {
#ifdef FITSDEBUG
            printf("writing header record\n");
#endif
            len = fwrite(record, sizeof(char), PHYSRECLEN, fits);
            if (len != PHYSRECLEN) {
                send_line("Error writing header record.");
                CloseFITSfile();
                return (-1);
            }
            card = 0;
        }
    }
    memset(line, ' ', LOGRECLEN-1);
    line[LOGRECLEN-1] = ' ';
    line[LOGRECLEN] = '\0';
    if (card) {
        while (card < (PHYSRECLEN/LOGRECLEN)) {
            strncpy(&record[card*LOGRECLEN], line, LOGRECLEN);
            card++;
#ifdef FITSDEBUG
            printf("%s", line);
#endif    
        }
#ifdef FITSDEBUG
        printf("writing header record\n");
#endif
        len = fwrite(record, sizeof(char), PHYSRECLEN, fits);
        if (len != PHYSRECLEN) {
            send_line("Error writing header record.");
            CloseFITSfile();
            return (-1);
        }
        card = 0;
    }
    return (0);
}


int writeFITSdata(int nobjs, int size, void *data)
{
    int bytes, i, len, left;
    char *ptr;

    if (!fits) {
        send_line("File stream not open.");
        return 0;
    }
    ptr = (char *)data;
    bytes = size*nobjs;
    i = 0;
    while (i < bytes) {
        memset(record, '\0', PHYSRECLEN);
        left = bytes - i;
        if (left > PHYSRECLEN) left = PHYSRECLEN;
        memcpy(record, ptr, left);
        ptr += left;
        i += left;
#ifdef FITSDEBUG
        printf("writing data record (%d of %d)\n", i, bytes);
#endif
        len = fwrite(record, sizeof(char), PHYSRECLEN, fits);
        if (len != PHYSRECLEN) {
            send_line("Error writing data record.");
            return (0);
        }
    }
    
    /* CloseFITSfile(); */
    
    return (bytes);
}
