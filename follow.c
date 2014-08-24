/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/* comments changed by TG 09.09.96 */

/* the functions in this file take a syntax tree for a regular
   expression and produce a DFA using the McNaughton-Yamada
   construction.						*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "re.h"

#define TRUE	1

extern Pset pset_union(Pset s1, Pset s2, int dontreplicate); 
extern int pos_cnt;
extern Re_node parse(char *s);

Re_lit_array lpos; 


/*  extend_re() extends the RE by adding a ".*(" at the front and a ")"
	at the back.						   	*/

char *extend_re(s)
char *s;
{
	char *s1;

	s1 = malloc((unsigned) strlen(s)+4+1);
	return strcat(strcat(strcpy(s1, ".*("), s), ")");
}

void free_pos(fpos, pos_cnt)
Pset_array fpos;
int pos_cnt;
{
	Pset tpos, pos;
	int i;

	if ((fpos == NULL) || (*fpos == NULL)) return;
	for (i=0; i<pos_cnt; i++) {
		pos = (*fpos)[i];
		while (pos != NULL) {
			tpos = pos;
			pos = pos->nextpos;
			free(tpos);
		}
	}
	free(fpos);
}

/* Function to clear out a Ch_Set */
void free_cset(cset)
Ch_Set cset;
{
	Ch_Set tset;

	while (cset != NULL) {
		tset = cset;
		cset = cset->rest;
		free(tset->elt);
		free(tset);
	}
}

/* Function to clear out the tree of re-nodes */
void free_re(e)
Re_node e;
{
	if (e == NULL) return;

/*
 * Was creating "reading freed memory", "freeing unallocated/freed memory"
 * errors. So abandoned it. Leaks are now up by 60B/call to 80B/call
 * -bg
 *
	{
	Pset tpos, pos;
	int tofree = 0;

	if ((Lastpos(e)) != (Firstpos(e))) tofree = 1;
	pos = Lastpos(e);
	while (pos != NULL) {
		tpos = pos;
		pos = pos->nextpos;
		free(tpos);
	}
	Lastpos(e) = NULL;

	if (tofree) {
		pos = Firstpos(e);
		while (pos != NULL) {
			tpos = pos;
			pos = pos->nextpos;
			free(tpos);
		}
		Firstpos(e) = NULL;
	}
	}
*/

	switch (Op(e)) {
	case EOS:
		if (lit_type(Lit(e)) == C_SET) free_cset(lit_cset(Lit(e)));
		free(Lit(e));
		break;
	case OPSTAR:
		free_re(Child(e));
		break;
	case OPCAT:
		free_re(Lchild(e));
		free_re(Rchild(e));
		break;
	case OPOPT:
		free_re(Child(e));
		break;
	case OPALT:
		free_re(Lchild(e));
		free_re(Rchild(e));
		break;
	case LITERAL:
		if (lit_type(Lit(e)) == C_SET) free_cset(lit_cset(Lit(e)));
		free(Lit(e));
		break;
	default: 
		fprintf(stderr, "free_re: unknown node type %d\n", Op(e));
	}
	free(e);
	return;
}


/* mk_followpos() takes a syntax tree for a regular expression and
   traverses it once, computing the followpos function at each node
   and returns a pointer to an array whose ith element is a pointer
   to a list of position nodes, representing the positions in
   followpos(i).							*/

void mk_followpos_1(e, fpos)
Re_node e;
Pset_array fpos;
{
	Pset pos;
	int i;

	switch (Op(e)) {
	case EOS: 
		break;
	case OPSTAR:
		pos = Lastpos(e);
		while (pos != NULL) {
			i = pos->posnum;
			(*fpos)[i] = pset_union(Firstpos(e), (*fpos)[i], 0);
			pos = pos->nextpos;
		}
		mk_followpos_1(Child(e), fpos);
		break;
	case OPCAT:
		pos = Lastpos(Lchild(e));
		while (pos != NULL) {
			i = pos->posnum;
			(*fpos)[i] = pset_union(Firstpos(Rchild(e)), (*fpos)[i], 0);
			pos = pos->nextpos;
		}
		mk_followpos_1(Lchild(e), fpos);
		mk_followpos_1(Rchild(e), fpos);
		break;
	case OPOPT:
		mk_followpos_1(Child(e), fpos);
		break;
	case OPALT:
		mk_followpos_1(Lchild(e), fpos);
		mk_followpos_1(Rchild(e), fpos);
		break;
	case LITERAL:
		break;
	default: 
		fprintf(stderr, "mk_followpos: unknown node type %d\n", Op(e));
	}
	return;
}

Pset_array mk_followpos(tree, npos)
Re_node tree;
int npos;
{
	int i;
	Pset_array fpos;

	if (tree == NULL || npos < 0) return NULL;
	fpos = (Pset_array) malloc((unsigned) (npos+1)*sizeof(Pset));
	if (fpos == NULL) return NULL;
	for (i = 0; i <= npos; i++) (*fpos)[i] = NULL;
	mk_followpos_1(tree, fpos);
	return fpos;
}

/* mk_poslist() sets a static array whose i_th element is a pointer to
   the RE-literal at position i.  It returns 1 if everything is OK,  0
   otherwise.								*/

/* init performs initialization actions; it returns -1 in case of error,
   0 if everything goes OK.						*/

int init(s, table)
char *s; 
int table[32][32];
{
	Pset_array fpos;
	Re_node e;
	Pset l;
	int i, j;
	char *s1;

	if ((s1 = extend_re(s)) == NULL) return -1;
	
	if ((e = parse(s1)) == NULL) {
		free(s1);
		return -1;
	}
	
	free(s1);
	if ((fpos = mk_followpos(e, pos_cnt)) == NULL) {
		free_re(e);
		return -1;
	}
	for (i = 0; i <= pos_cnt; i += 1) {
#ifdef Debug
		printf("followpos[%d] = ", i);
#endif
		l = (*fpos)[i];
		j = 0;
		for ( ; l != NULL; l = l->nextpos)  {
#ifdef Debug
			printf("%d ", l->posnum);
#endif
			table[i][j] = l->posnum;
			j++;
		} 
#ifdef Debug
		printf("\n");
#endif
	}
#ifdef Debug
	for (i=0; i <= pos_cnt; i += 1)  {
		j = 0;
		while (table[i][j] != 0) {
			printf(" %d ", table[i][j]);
			j++;
		}
		printf("\n");
	}
#endif
	free_pos(fpos, pos_cnt);
	free_re(e);
	return (pos_cnt);
}
