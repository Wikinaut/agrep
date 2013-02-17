/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
/* this file contains various utility functions for accessing
   and manipulating regular expression syntax trees.	*/

#include <stdio.h>
#include <ctype.h>
#include "re.h"

/************************************************************************/
/*                                                                      */
/*  the following routines implement an abstract data type "stack".	*/
/*                                                                      */
/************************************************************************/

Stack Push(s, v)
Stack *s;
Re_node v;
{
	Stack node;

	new_node(Stack, node, node);
	if (s == NULL || node == NULL) return NULL;	    /* can't allocate */
	node->next = *s;
	node->val = v;
	if (*s == NULL) node->size = 1;
	else node->size = (*s)->size + 1;
	*s = node;
	return *s;
}

Re_node Pop(s)
Stack *s;
{
	Re_node node;
	Stack temp;

	if (s == NULL || *s == NULL) return NULL;
	else {
		temp = *s;
		node = (*s)->val;
		*s = (*s)->next;
		free(temp);
		return node;
	}
}

Re_node Top(s)
Stack s;
{
	if (s == NULL) return NULL;
	else return s->val;
}

int Size(s)
Stack s;
{
	if (s == NULL) return 0;
	else return s->size;
}

/************************************************************************/
/*                                                                      */
/*	the following routines manipulate sets of positions.		*/
/*                                                                      */
/************************************************************************/

int occurs_in(n, p)
int n;
Pset p;
{
	while (p != NULL)
		if (n == p->posnum) return 1;
		else p = p->nextpos;
	return 0;
}

/* pset_union() takes two position-sets and returns their union.    */

Pset pset_union(s1, s2, dontreplicate)
Pset s1, s2;
int dontreplicate;
{
	Pset hd, curr, new = NULL;
	Pset replicas2 = NULL, temps2 = s2;	/* code added: 26/Aug/96 */

	/* Code added on 26/Aug/96 */
	if (dontreplicate) replicas2 = s2;
	else while (temps2 != NULL) {
		new_node(Pset, new, new);
		if (new == NULL) return NULL;
		new->posnum = temps2->posnum;
		if (replicas2 == NULL) replicas2 = new;
		else curr->nextpos = new;
		curr = new;
		temps2 = temps2->nextpos;
	}

	hd = NULL; 
	curr = NULL;
	while (s1 != NULL) {
		if (!occurs_in(s1->posnum, s2)) {
			new_node(Pset, new, new);
			if (new == NULL) return NULL;
			new->posnum = s1->posnum;
			if (hd == NULL) hd = new;
			else curr->nextpos = new;
		}
		curr = new;
		s1 = s1->nextpos;
	}
	if (hd == NULL) hd = replicas2;	/* changed from s2: 26/Aug/96 */
	else curr->nextpos = replicas2;	/* changed from s2: 26/Aug/96 */
	return hd;
}

/* create_pos() creates a position node with the position value given,
   then returns a pointer to this node.					*/

Pset create_pos(n)
int n;
{
	Pset x;

	new_node(Pset, x, x);
	if (x == NULL) return NULL;
	x->posnum = n;
	x->nextpos = NULL;
	return x;
}

/* eq_pset() takes two position sets and checks to see if they are
   equal.  It returns 1 if the sets are equal, 0 if they are not.	*/
int
subset_pset(s1, s2)
Pset s1, s2;
{
	int subs = 1;

	while (s1 != NULL && subs != 0) {
		subs = 0;
		while (s2 != NULL && subs != 1)
			if (s1->posnum == s2->posnum) subs = 1;
			else s2 = s2->nextpos;
		s1 = s1->nextpos;
	}
	return subs;
}	

int eq_pset(s1, s2)
Pset s1, s2;
{
	return subset_pset(s1, s2) && subset_pset(s2, s1);
}

int
word_exists(word, wordlen, line, linelen)
unsigned char *word, *line;
int wordlen, linelen;
{
	unsigned char oldchar, *lineend = line+linelen;
	int i;

	i = 0;
	while(line<lineend) {
		if (!isalnum(line[i])) {
			oldchar = line[i];
			line[i] = '\0';
			if (!strncmp(line, word, wordlen)) {
				line[i] = oldchar;
				return 1;
			}
			line[i] = oldchar;
			line += i + 1;
			i = 0;
		} else i++;
	}
	return 0;
}
