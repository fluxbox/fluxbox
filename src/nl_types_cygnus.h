#ifndef _NL_TYPES_EMUL_H
#define _NL_TYPES_EMUL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Data types */

typedef int nl_catd;

/* Functions */

char *catgets(nl_catd  cat,  int set_number, int message_number, char *message);

nl_catd catopen(char *name, int flag);

void catclose (nl_catd cat);

#ifdef __cplusplus
}
#endif

#endif /* _NL_TYPES_EMUL_H */
