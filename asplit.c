/* Copyright (c) 1994 Burra Gopal, Udi Manber.  All Rights Reserved. */

#include "agrep.h"

#include "putils.c"

extern int checksg();
extern int D;
extern FILE *debug;

/* All borrowed from agrep.c and are needed for searching the index */
extern	ParseTree aterminals[MAXNUM_PAT];
extern int	AComplexBoolean;

/* returns where it found the distinguishing token: until that from prev value of begin is the current pattern (not just the "words" in it) */
CHAR *
aparse_flat(begin, end, prev, next)
	CHAR	*begin;
	CHAR	*end;
	int	prev;
	int	*next;
{
	if (begin > end) {
		*next = prev;
		return end;
	}

	if (prev & ENDSUB_EXP) prev &= ~ATTR_EXP;
	if ((prev & ATTR_EXP) && !(prev & VAL_EXP)) prev |= VAL_EXP;

	while (begin <= end) {
		if (*begin == ',') {
			prev |= OR_EXP;
			prev |= VAL_EXP;
			prev |= ENDSUB_EXP;
			if (prev & AND_EXP) {
				fprintf(stderr, "asplit.c: parse error at character '%c'\n", *begin);
				return NULL;
			}
			*next = prev;
			return begin;
		}
		else if (*begin == ';') {
			prev |= AND_EXP;
			prev |= VAL_EXP;
			prev |= ENDSUB_EXP;
			if (prev & OR_EXP) {
				fprintf(stderr, "asplit.c: parse error at character '%c'\n", *begin);
				return NULL;
			}
			*next = prev;
			return begin;
		}
		else if (*begin == '\\') begin ++;	/* skip two things */
		begin++;
	}

	*next = prev;
	return begin;
}

int
asplit_pattern_flat(APattern, AM, terminals, pnum_terminals, pAParse)
	CHAR	*APattern;
	int	AM;
	ParseTree terminals[MAXNUM_PAT];
	int	*pnum_terminals;
	int	*pAParse;
{
	CHAR  *buffer;
	CHAR  *buffer_pat;
	CHAR  *buffer_end;

	buffer = APattern;
	buffer_end = buffer + AM;
	*pAParse = 0;

	/*
	 * buffer is the runnning pointer, buffer_pat is the place where
	 * the distinguishing delimiter was found, buffer_end is the end.
	 */
	 while (buffer_pat = aparse_flat(buffer, buffer_end, *pAParse, pAParse)) {
		/* there is no pattern until after the distinguishing delimiter position: some agrep garbage */
		if (buffer_pat <= buffer) {
			buffer = buffer_pat+1;
			if (buffer_pat >= buffer_end) break;
			continue;
		}
		if (*pnum_terminals >= MAXNUM_PAT) {
			fprintf(stderr, "boolean expression has too many terms\n");
			return -1;
		}
		terminals[*pnum_terminals].op = 0;
		terminals[*pnum_terminals].type = LEAF;
		terminals[*pnum_terminals].terminalindex = *pnum_terminals;
		terminals[*pnum_terminals].data.leaf.attribute = 0;	/* default is no structure */
		terminals[*pnum_terminals].data.leaf.value = (CHAR *)malloc(buffer_pat - buffer + 2);
		memcpy(terminals[*pnum_terminals].data.leaf.value, buffer, buffer_pat - buffer);	/* without distinguishing delimiter */
		terminals[*pnum_terminals].data.leaf.value[buffer_pat - buffer] = '\0';
		(*pnum_terminals)++;
		if (buffer_pat >= buffer_end) break;
		buffer = buffer_pat+1;
	}
	if (buffer_pat == NULL) return -1;	/* got out of while loop because of NULL rather than break */
	return(*pnum_terminals);
}

/*
 * Recursive descent; C-style => AND + OR have equal priority => must bracketize expressions appropriately or will go left->right.
 * Grammar:
 * 	E = {E} | ~a | ~{E} | E ; E | E , E | a
 * Parser:
 *	One look ahead at each literal will tell you what to do.
 *	~ has highest priority, ; and , have equal priority (left to right associativity), ~~ is not allowed.
 */
ParseTree *
aparse_tree(buffer, len, bufptr, terminals, pnum_terminals)
	CHAR	*buffer;
	int	len;
	int	*bufptr;
	ParseTree terminals[];
	int	*pnum_terminals;
{
	int	token, tokenlen;
	CHAR	tokenbuf[MAXNAME];
	int	oldtokenlen;
	CHAR	oldtokenbuf[MAXNAME];
	ParseTree *t, *n, *leftn;

	token = get_token_bool(buffer, len, bufptr, tokenbuf, &tokenlen);
	switch(token)
	{
	case	'{':	/* (exp) */
		if ((t = aparse_tree(buffer, len, bufptr, terminals, pnum_terminals)) == NULL) return NULL;
		if ((token = get_token_bool(buffer, len, bufptr, tokenbuf, &tokenlen)) != '}') {
			fprintf(stderr, "asplit.c: parse error at offset %d\n", *bufptr);
			destroy_tree(t);
			return (NULL);
		}
		if ((token = get_token_bool(buffer, len, bufptr, tokenbuf, &tokenlen)) == 'e') return t;
		switch(token)
		{
		/* must find boolean infix operator */
		case ',':
		case ';':
			leftn = t;
			if ((t = aparse_tree(buffer, len, bufptr, terminals, pnum_terminals)) == NULL) return NULL;
			n = (ParseTree *)malloc(sizeof(ParseTree));
			n->op = (token == ';') ? ANDPAT : ORPAT ;
			n->type = INTERNAL;
			n->data.internal.left = leftn;
			n->data.internal.right = t;
			return n;

		/* or end of parent sub expression */
		case '}':
			unget_token_bool(bufptr, tokenlen);	/* part of someone else who called me */
			return t;

		default:
			destroy_tree(t);
			fprintf(stderr, "asplit.c: parse error at offset %d\n", *bufptr);
			return NULL;
		}

	/* Go one level deeper */
	case	'~':	/* not exp */
		if ((token = get_token_bool(buffer, len, bufptr, tokenbuf, &tokenlen)) == 'e') return NULL;
		switch(token)
		{
		case 'a':
			if (*pnum_terminals >= MAXNUM_PAT) {
				fprintf(stderr, "Pattern expression too large (> %d)\n", MAXNUM_PAT);
				return NULL;
			}
			n = &terminals[*pnum_terminals];
			n->op = 0;
			n->type = LEAF;
			n->terminalindex = (*pnum_terminals);
			n->data.leaf.attribute = 0;
			n->data.leaf.value = (unsigned char*)malloc(tokenlen + 2);
			memcpy(n->data.leaf.value, tokenbuf, tokenlen);
			n->data.leaf.value[tokenlen] = '\0';
			(*pnum_terminals)++;
			n->op |= NOTPAT;
			t = n;
			break;

		case '{':
			if ((t = aparse_tree(buffer, len, bufptr, terminals, pnum_terminals)) == NULL) return NULL;
			if (t->op & NOTPAT) t->op &= ~NOTPAT;
			else t->op |= NOTPAT;
			if ((token = get_token_bool(buffer, len, bufptr, tokenbuf, &tokenlen)) != '}') {
				fprintf(stderr, "asplit.c: parse error at offset %d\n", *bufptr);
				destroy_tree(t);
				return NULL;
			}
			break;

		default:
			fprintf(stderr, "asplit.c: parse error at offset %d\n", *bufptr);
			return NULL;
		}
		/* The resulting tree is in t. Now do another lookahead at this level */

		if ((token = get_token_bool(buffer, len, bufptr, tokenbuf, &tokenlen)) == 'e') return t;
		switch(token)
		{
		/* must find boolean infix operator */
		case ',':
		case ';':
			leftn = t;
			if ((t = aparse_tree(buffer, len, bufptr, terminals, pnum_terminals)) == NULL) return NULL;
			n = (ParseTree *)malloc(sizeof(ParseTree));
			n->op = (token == ';') ? ANDPAT : ORPAT ;
			n->type = INTERNAL;
			n->data.internal.left = leftn;
			n->data.internal.right = t;
			return n;

		case '}':
			unget_token_bool(bufptr, tokenlen);
			return t;

		default:
			destroy_tree(t);
			fprintf(stderr, "asplit.c: parse error at offset %d\n", *bufptr);
			return NULL;
		}

	case	'a':	/* individual term (attr=val) */
		if (tokenlen == 0) return NULL;
		memcpy(oldtokenbuf, tokenbuf, tokenlen);
		oldtokenlen = tokenlen;
		oldtokenbuf[oldtokenlen] = '\0';
		token = get_token_bool(buffer, len, bufptr, tokenbuf, &tokenlen);
		switch(token)
		{
		case '}':	/* part of case '{' above: else syntax error not detected but semantics ok */
			unget_token_bool(bufptr, tokenlen);
		case 'e':	/* endof input */
		case ',':
		case ';':
			if (*pnum_terminals >= MAXNUM_PAT) {
				fprintf(stderr, "Pattern expression too large (> %d)\n", MAXNUM_PAT);
				return NULL;
			}
			n = &terminals[*pnum_terminals];
			n->op = 0;
			n->type = LEAF;
			n->terminalindex = (*pnum_terminals);
			n->data.leaf.attribute = 0;
			n->data.leaf.value = (unsigned char*)malloc(oldtokenlen + 2);
			strcpy(n->data.leaf.value, oldtokenbuf);
			(*pnum_terminals)++;
			if ((token == 'e') || (token == '}')) return n;	/* nothing after terminal in expression */

			leftn = n;
			if ((t = aparse_tree(buffer, len, bufptr, terminals, pnum_terminals)) == NULL) return NULL;
			n = (ParseTree *)malloc(sizeof(ParseTree));
			n->op = (token == ';') ? ANDPAT : ORPAT ;
			n->type = INTERNAL;
			n->data.internal.left = leftn;
			n->data.internal.right = t;
			return n;

		default:
			fprintf(stderr, "asplit.c: parse error at offset %d\n", *bufptr);
			return NULL;
		}

	case	'e':	/* can't happen as I always do a lookahead above and return current tree if e */
	default:
		fprintf(stderr, "asplit.c: parse error at offset %d\n", *bufptr);
		return NULL;
	}
}

int
asplit_pattern(APattern, AM, terminals, pnum_terminals, pAParse)
	CHAR	*APattern;
	int	AM;
	ParseTree terminals[];
	int	*pnum_terminals;
	ParseTree **pAParse;
{
	int	bufptr = 0, ret, i, j;

	if (is_complex_boolean(APattern, AM)) {
		AComplexBoolean = 1;
		*pnum_terminals = 0;
		if ((*pAParse = aparse_tree(APattern, AM, &bufptr, terminals, pnum_terminals)) == NULL)
			return -1;
		/* print_tree(*pAParse, 0); */
		return *pnum_terminals;
	}
	else {
		for (i=0; i<AM; i++) {
			if (APattern[i] == '\\') i++;
			else if ((APattern[i] == '{') || (APattern[i] == '}')) {
				/* eliminate it from pattern by shifting (including '\0') since agrep must essentially do a flat search */
				for (j=i; j<AM; j++)
					APattern[j] = APattern[j+1];
				AM --;
				i--;	/* to counter the ++ on top */
			}
		}

		AComplexBoolean = 0;
		*pnum_terminals = 0;
		if ((ret = asplit_pattern_flat(APattern, AM, terminals, pnum_terminals, (int *)pAParse)) == -1)
			return -1;
		return ret;
	}
}

/*
int
dd(b, e)
	char	*b, *e;
{
	int	i=0;
	extern int anum_terminals;
	extern char amatched_terminals[];
	for(;i<anum_terminals; i++) printf("%d ", amatched_terminals[i]);
	putchar(':');
	while (b != e) putchar(*b++);
	putchar('\n');
	return 1;
}
*/

/* fast interpreter for the tree using which terminals matched: array bound checks are not done: its recursive */
int
eval_tree(tree, matched_terminals)
	ParseTree	*tree;
	char		matched_terminals[];
{
	int	res;
	if (tree == NULL) {
		fprintf(stderr, "Eval on empty tree: returning true\n");
		return 1;	/* safety sake, but cannot happen! */
	}
	if (tree->type == LEAF) return ((tree->op & NOTPAT) ? (!matched_terminals[tree->terminalindex]) : (matched_terminals[tree->terminalindex]));
	else if (tree->type == INTERNAL) {
		if ((tree->op & OPMASK) == ANDPAT) {	/* sequential evaluation */
			if ((res = eval_tree(tree->data.internal.left, matched_terminals)) != 0) res = eval_tree(tree->data.internal.right, matched_terminals);
			return (tree->op & NOTPAT) ? !res : res;
		}
		else {	/* sequential evaluation */
			if ((res = eval_tree(tree->data.internal.left, matched_terminals)) == 0) res = eval_tree(tree->data.internal.right, matched_terminals);
			return (tree->op & NOTPAT) ? !res : res;
		}
	}
	else {
		fprintf(stderr, "Eval on bad tree: returning false\n");
		return 0;	/* safety sake, but cannot happen! */
	}
}

/* [first, last) = C-style range for which we want the words in terminal-values' patterns: 0..num_terminals for !ComplexBoolean, term/term otherwise */
int
asplit_terminal(first, last, pat_buf, pat_ptr)
	int	first, last;
	char	*pat_buf;
	int	*pat_ptr;
{
        int	word_length;
        int	type;
	int	num_pat;

	*pat_ptr = 0;
	num_pat = 0;

	for (; first<last; first++) {
		word_length = strlen(aterminals[first].data.leaf.value);
		if (word_length <= 0) continue;
		if ((type = checksg(aterminals[first].data.leaf.value, D, 0)) == -1) return -1;
		if (!type) return -1;
		strcpy(&pat_buf[*pat_ptr], aterminals[first].data.leaf.value);
		pat_buf[*pat_ptr + word_length] = '\n';
		pat_buf[*pat_ptr + word_length + 1] = '\0';
		*pat_ptr += (word_length + 1);
		num_pat ++;
		if(num_pat >= MAXNUM_PAT) {
			fprintf(stderr, "Warning: too many words in pattern (> %d): ignoring...\n", MAXNUM_PAT);
			break;
		}
	}
	return num_pat;
}
