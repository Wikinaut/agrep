/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/* the functions in this file parse a string that represents
   a regular expression, and return a pointer to a syntax
   tree for that regular expression.				*/

#ifdef _WIN32
#include <memory.h>
#include <stdlib.h>
#endif

#include <stdio.h>
#include "re.h"

#define FALSE	0
#define TRUE	1

#define NextChar(s)	 	*(*s)++
#define Unexpected(s, c) 	(**s == NUL || **s == c)
#define Invalid_range(x, y)	(x == NUL || x == '-' || x == ']' || x < y)

extern Stack Push();
extern Re_node Pop();
extern Re_node Top();
extern int Size();
extern Pset pset_union();
extern Pset create_pos();
extern void free_re();

int final_pos, pos_cnt = 0;

/* retract_token() moves the string pointer back, effectively "unseeing"
   the last character seen.  It is used only to retract a right paren --
   the idea is that the incarnation of parse_re() that saw the corresponding
   left paren is supposed to take care of matching the right paren.  This
   is necessary to prevent recursive calls from mistakenly eating up someone
   else's right parens.						    */

#define retract_token(s)    --(*s)

/* mk_leaf() creates a leaf node that is (usually) a literal node.	*/

Re_node mk_leaf(opval, type, ch, cset)
short opval, type;
char ch;
Ch_Set cset;
{
	Re_node node; 
	Re_Lit l;

	new_node(Re_Lit, l, l);
	new_node(Re_node, node, node);
	if (l == NULL || node == NULL) {
		if (l != NULL) free(l);
		if (node != NULL) free(node);
		return NULL;
	}
	lit_type(l) = type;
	lit_pos(l)  = pos_cnt++;
	if (type == C_SET) lit_cset(l) = cset;
	else lit_char(l) = ch;			/* type == C_LIT */
	Op(node) = opval;
	Lit(node) = l;
	Nullable(node) = FALSE;
	Firstpos(node) = create_pos(lit_pos(l));
	Lastpos(node) = Firstpos(node);
	return node;
}

/* parse_cset() takes a pointer to a pointer to a string and parses
   a prefix of it denoting a character set literal.  It returns a pointer
   to a Re_node node, NULL if there is an error.			*/

Re_node parse_cset(s)
char **s;
{
	Ch_Set cs_ptr, curr_ptr, prev_ptr;
	char ch;
	Ch_Range range = NULL;

	if (Unexpected(s, ']')) return NULL;
	new_node(Ch_Set, curr_ptr, curr_ptr); 
	cs_ptr = curr_ptr;
	while (!Unexpected(s, ']')) {
		new_node(Ch_Range, range, range);
		curr_ptr->elt = range;
		ch = NextChar(s);
		if (ch == '-') {
			free(range);
			free(curr_ptr);
			return NULL;	/* invalid range */
		}
		range->low_bd = ch;
		if (**s == NUL) {
			free(range);
			free(curr_ptr);
			return NULL;
		}
		else if (**s == '-') {		/* character range */
			(*s)++;
			if (Invalid_range(**s, ch)) {
				free(range);
				free(curr_ptr);
				return NULL;
			}
			else range->hi_bd = NextChar(s);
		}
		else range->hi_bd = ch;
		prev_ptr = curr_ptr;
		new_node(Ch_Set, curr_ptr, curr_ptr);
		prev_ptr->rest = curr_ptr;
	};
	if (**s == ']') {
		prev_ptr->rest = NULL;
		return mk_leaf(LITERAL, C_SET, NUL, cs_ptr);
	}
	else {
		if (range != NULL) free(range);
		free(curr_ptr);
		return NULL;
	}
} /* parse_cset */


/* parse_wildcard() "parses" a wildcard -- a wildcard is treated as a
   character range whose values span all ASCII values.  parse_wildcard()
   creates a node representing such a range.				*/

Re_node parse_wildcard()
{
	Ch_Set s; 
	Ch_Range r;

	new_node(Ch_Range, r, r);
	r->low_bd = ASCII_MIN;	/* smallest ASCII value */
	r->hi_bd  = ASCII_MAX;	/* greatest ASCII value */
	new_node(Ch_Set, s, s);
	s->elt = r;
	s->rest = NULL;
	return mk_leaf(LITERAL, C_SET, NUL, s);
}

/* parse_chlit() parses a character literal.  It is assumed that the
   character in question does not have any special meaning.  It returns
   a pointer to a node for that literal.				*/

Re_node parse_chlit(ch)
char ch;
{
	if (ch == NUL) return NULL;
	else return mk_leaf(LITERAL, C_LIT, ch, NULL);
}

/* routine to free the malloced token */
void free_tok(next_token)
Tok_node next_token;
{
	if (next_token == NULL) return;
	switch (tok_type(next_token)) {
	case LITERAL:
		free_re(tok_val(next_token));
	case EOS:
	case RPAREN:
	case LPAREN:
	case OPSTAR:
	case OPALT:
	case OPOPT:
	default:
		free(next_token);
		break;
	}
}

/* get_token() returns the next token -- this may be a character
   literal, a character set, an escaped character, a punctuation (i.e.
   parenthesis), or an operator.  It traverses the character string
   representing the RE, given by a pointer s; leaves s positioned
   immediately after the unit it parsed, and returns a pointer to
   a token node for that unit.  */

Tok_node get_token(s)
char **s;
{
	Tok_node rn = NULL;

	if (s == NULL || *s == NULL) return NULL;	/* error */
	new_node(Tok_node, rn, rn);
	if (**s == NUL) tok_type(rn) = EOS; /* end of string */
	else {
		switch (**s) {
		case '.':			/* wildcard */
			tok_type(rn) = LITERAL;
			tok_val(rn) =  parse_wildcard();
			if (tok_val(rn) == NULL) {
				free_tok(rn);
				return NULL;
			}
			break;
		case '[':			/* character set literal */
			(*s)++;
			tok_type(rn) = LITERAL;
			tok_val(rn) = parse_cset(s);
			if (tok_val(rn) == NULL) {
				free_tok(rn);
				return NULL;
			}
			break;
		case '(':
			tok_type(rn) = LPAREN;
			break;
		case ')' : 
			tok_type(rn) = RPAREN;
			break;
		case '*' :
			tok_type(rn) = OPSTAR;
			break;
		case '|' :
			tok_type(rn) = OPALT;
			break;
		case '?' : 
			tok_type(rn) = OPOPT;
			break;
		case '\\':			/* escaped character */
			(*s)++;
		default :			/* must be ordinary character */
			tok_type(rn) = LITERAL;
			tok_val(rn) = parse_chlit(**s);
			if (tok_val(rn) == NULL) {
				free_tok(rn);
				return NULL;
			}
			break;
		} /* switch (**s) */
		(*s)++;
	} /* else */
	return rn;
}

/* cat2() takes a stack of RE-nodes and, if the stack contains
   more than one node, returns the stack obtained by condensing
   the top two nodes of the stack into a single CAT-node.  If there
   is only one node on the stack,  nothing is done.		    */

Stack cat2(stk)
Stack *stk;
{
	Re_node r;

	if (stk == NULL) return NULL;
	if (*stk == NULL || (*stk)->next == NULL) return *stk;
	new_node(Re_node, r, r);
	if (r == NULL) return NULL;	    /* can't allocate memory */
	Op(r) = OPCAT;
	Rchild(r) = Pop(stk);
	Lchild(r) = Pop(stk);
	if (Push(stk, r) == NULL) {
		free_re(Rchild(r));
		free_re(Lchild(r));
		free(r);
		return NULL;
	}
	Nullable(r) = Nullable(Lchild(r)) && Nullable(Rchild(r));
	if (Nullable(Lchild(r)))
		Firstpos(r) = pset_union(Firstpos(Lchild(r)), Firstpos(Rchild(r)));
	else Firstpos(r) = Firstpos(Lchild(r));
	if (Nullable(Rchild(r)))
		Lastpos(r) = pset_union(Lastpos(Lchild(r)), Lastpos(Rchild(r)));
	else Lastpos(r) = Lastpos(Rchild(r));
	return *stk;
}

/* wrap() takes a stack and an operator, takes the top element of the
   stack and "wraps" that operator around it, then puts this back on the
   stack and returns the resulting stack.				*/

Stack wrap(s, opv)
Stack *s;
short opv;
{
	Re_node r;

	if (s == NULL || *s == NULL) return NULL;
	new_node(Re_node, r, r);
	if (r == NULL) return NULL;
	Op(r) = opv;
	Child(r) = Pop(s);
	if (Push(s, r) == NULL) {
		free_re(Child(r));
		free(r);
		return NULL;
	}
	Nullable(r) = TRUE;
	Firstpos(r) = Firstpos(Child(r));
	Lastpos(r)  = Lastpos(Child(r));
	return *s;
}

/* mk_alt() takes a stack and a regular expression, creates an ALT-node
   from the top of the stack and the given RE, and replaces the top-of-stack
   by the resulting ALT-node.						*/   

Stack mk_alt(s, r)
Stack *s;
Re_node r;
{
	Re_node node;

	if (s == NULL || *s == NULL || r == NULL) return NULL;
	new_node(Re_node, node, node);
	if (node == NULL) return NULL;
	Op(node) = OPALT;
	Lchild(node) = Pop(s);
	Rchild(node) = r;
	if (Push(s, node) == NULL) return NULL;
	Nullable(node) = Nullable(Lchild(node)) || Nullable(Rchild(node));
	Firstpos(node) = pset_union(Firstpos(Lchild(node)), Firstpos(Rchild(node)));
	Lastpos(node) = pset_union(Lastpos(Lchild(node)), Lastpos(Rchild(node)));
	return *s;
}

/* parse_re() takes a pointer to a string and traverses that string,
   returning a pointer to a syntax tree for the regular expression
   represented by that string, NULL if there is an error.		*/

Re_node parse_re(s, end)
char **s;
short end;
{
	Stack stk = NULL, ret = NULL, top, temp;
	Tok_node next_token, t1;
	Re_node re = NULL, val;

	if (s == NULL || *s == NULL) return NULL;
	while (TRUE) {
		ret = NULL;
		if ((next_token = get_token(s)) == NULL) return NULL;
		switch (tok_type(next_token)) {
		case RPAREN:
			retract_token(s);
		case EOS:
			if (end == tok_type(next_token)) {
				free_tok(next_token);
				top = cat2(&stk);
				val = Top(top);
				free(top);
				return val;
			}
			else {
				free_tok(next_token);
				return NULL;
			}
		case LPAREN:
			free_tok(next_token);
			re = parse_re(s, RPAREN);
			if ((ret = Push(&stk, re)) == NULL) return NULL;
			if ((t1 = get_token(s)) == NULL) {
				free(ret);
				return NULL;
			}
			if ((tok_type(t1) != RPAREN) || (re == NULL)) {
				free(ret);
				free_tok(t1);
				return NULL;
			}
			free_tok(t1);
			if (Size(stk) > 2) {
				temp = stk->next;
				stk->next = cat2(&temp);	/* condense CAT nodes */
				if (stk->next == NULL) {
					free(ret);
					return NULL;
				}
				else stk->size = stk->next->size + 1;
			}
			break;
		case OPSTAR:
			if ((ret = wrap(&stk, OPSTAR)) == NULL) {
				free_tok(next_token);
				return NULL;
			}
			break;
		case OPOPT:
			if ((ret = wrap(&stk, OPOPT)) == NULL) {
				free_tok(next_token);
				return NULL;
			}
			break;
		case OPALT:
			if ((ret = cat2(&stk)) == NULL) {
				free_tok(next_token);
				return NULL;
			}
			re = parse_re(s, end);
			if (re == NULL) {
				free(ret);
				free_tok(next_token);
				return NULL;
			}
			if (mk_alt(&stk, re) == NULL) {
				free(ret);
				free_tok(next_token);
				return NULL;
			}
			break;
		case LITERAL:
			if ((ret = Push(&stk, tok_val(next_token))) == NULL) {
				free_tok(next_token);
				return NULL;
			}
			free(next_token);
			if (Size(stk) > 2) {
				temp = stk->next;
				stk->next = cat2(&temp);    /* condense CAT nodes */
				if (stk->next == NULL) {
					free(ret);
					return NULL;
				}
				else stk->size = stk->next->size + 1;
			}
			break;
		default:
			printf("parse_re: unknown token type %d\n", tok_type(next_token));
			break;
		}
		/* free_tok(next_token); */
	}
}

/* parse() essentially just calls parse_re().  Its purpose is to stick an
   end-of-string token at the end of the syntax tree returned by parse_re().
   It should really be done in parse_re, but the recursion there makes it
   more desirable to have it here.					*/

Re_node parse(s)
char *s;
{
	Re_node val, tree, temp;
	Stack top, stk = NULL;

	if ((tree = parse_re(&s, NUL)) == NULL) return NULL;
	if (Push(&stk, tree) == NULL) return NULL;
	temp = mk_leaf(EOS, C_LIT, NUL, NULL);
	if (temp == NULL || Push(&stk, temp) == NULL) return NULL;
	final_pos = --pos_cnt;
	top = cat2(&stk);
	val = Top(top);
	free(top);
	return val;
}

