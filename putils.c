/* Copyright (c) 1994 Burra Gopal, Udi Manber.  All Rights Reserved. */

#include "agrep.h"

int
is_complex_boolean(buffer, len)
	CHAR	*buffer;
	int	len;
{
	int	i = 0;
	CHAR	cur = '\0';

	while (i < len) {
		if (buffer[i] == '\\') i+=2;
		else if (buffer[i] == ',') {
			if ((cur == ';') || (cur == '~')) return 1;
			else cur = ',';
			i++;
		}
		else if (buffer[i] == ';') {
			if ((cur == ',') || (cur == '~')) return 1;
			else cur = ';';
			i++;
		}
		/* else if ((buffer[i] == '~') || (buffer[i] == '{') || (buffer[i] == '}')) { */
		else if (buffer[i] == '~') {
			/* even if pattern has just ~s... user must use -v option for single NOT */
			return 1;
		}
		else i++;
	}
	return 0;
}

/* The possible tokens are: ; , a e ~ { } */
int
get_token_bool(buffer, len, ptr, tokenbuf, tokenlen)
	CHAR	*buffer, *tokenbuf;
	int	len, *ptr, *tokenlen;
{
	if ((*ptr>=len) || (buffer[*ptr] == '\n') || (buffer[*ptr] == '\0')) return 'e';
	while ((*ptr<len) && (buffer[*ptr] != '\n') && (buffer[*ptr] != '\0') && ((buffer[*ptr] == ' ') || (buffer[*ptr] == '\t'))) (*ptr)++;
	if ((*ptr>=len) || (buffer[*ptr] == '\n') || (buffer[*ptr] == '\0'))  return 'e';
	if ((buffer[*ptr] == ',') || (buffer[*ptr] == ';') || (buffer[*ptr] == '~') || (buffer[*ptr] == '{') || (buffer[*ptr] == '}')) {
		tokenbuf[0] = buffer[*ptr];
		*tokenlen = 1;
		return buffer[(*ptr)++];
	}
	*tokenlen = 0;
	if (buffer[*ptr] == '\\') {
		tokenbuf[(*tokenlen)++] = buffer[(*ptr)++];
		tokenbuf[(*tokenlen)++] = buffer[(*ptr)++];
	}
	else tokenbuf[(*tokenlen)++] = buffer[(*ptr)++];
	while (	(*ptr<len) && (buffer[*ptr] != '\n') && (buffer[*ptr] != '\0') &&
		(buffer[*ptr] != ',') && (buffer[*ptr] != ';') && (buffer[*ptr] != '~') && (buffer[*ptr] != '{') && (buffer[*ptr] != '}')) {
		if (buffer[*ptr] == '\\') {
			tokenbuf[(*tokenlen)++] = buffer[(*ptr)++];
			tokenbuf[(*tokenlen)++] = buffer[(*ptr)++];
		}
		else tokenbuf[(*tokenlen)++] = buffer[(*ptr)++];
	}
	tokenbuf[*tokenlen] = '\0';
	return 'a';
	/* It will return next 'e' next time if we broke out of the loop because (*ptr >= len) */
}

void
print_tree(t, level)
	ParseTree *t;
	int level;
{
	int	i;

	if (t == NULL) printf("NULL");
	else if (t->type == LEAF) {
		for (i=0; i<level; i++) printf("  ");
		printf("LEAF %x %x %s\n", t->op, t->terminalindex, t->data.leaf.value);
	}
	else if (t->type == INTERNAL) {
		if (t->data.internal.left != NULL) print_tree(t->data.internal.left, level + 1);
		for (i=0; i<level; i++) printf("  ");
		printf("INTERNAL %x\n", t->op);
		if (t->data.internal.right != NULL) print_tree(t->data.internal.right, level + 1);
	}
}

void
destroy_tree(t)
	ParseTree *t;
{
	if (t == NULL) return;
	if (t->type == LEAF) {
		free(t->data.leaf.value);	/* t itself should not be freed: static allocation */
	}
	else if (t->type == INTERNAL) {
		if (t->data.internal.left != NULL) destroy_tree(t->data.internal.left);
		if (t->data.internal.right != NULL) destroy_tree(t->data.internal.right);
		free(t);
	}
}
