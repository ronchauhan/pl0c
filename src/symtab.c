/* 
 * Copyright (c) Ronak Chauhan
 * This file is part of pl0c and is licensed under the terms of the MIT License.
 * See LICENSE for more details.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"


#define EQUAL(a, b) ( strcmp((a)->name, (b)->name) == 0 && (a)->level == (b)->level )


static symbol_t *current_tip = NULL;
static size_t current_level = 0; // globals have level 0
static size_t total_symbol_count = 0;
static bool error = false;


symbol_t* lookup(char *name)
{
	symbol_t *tmp = current_tip;
	while (tmp) {
		if (strcmp(tmp->name, name) == 0) {
			return tmp;
		}
		tmp = tmp->prev;
	}
	return NULL;
}


static symbol_t *new_symbol(char *name, sym_type_t type, int64_t value, size_t level)
{
	symbol_t *new_symbol_obj = calloc(1, sizeof(symbol_t));
	strcpy(new_symbol_obj->name, name);
	new_symbol_obj->type = type;
	new_symbol_obj->value = value;
	new_symbol_obj->level = level;
	new_symbol_obj->next = NULL;
	new_symbol_obj->prev = NULL;
	return new_symbol_obj;
}


static bool insert_sym(symbol_t **table, symbol_t *new_symbol_obj)
{
	symbol_t *current_symbol = *table;
	if (!current_symbol) {
		*table = new_symbol_obj;
		current_tip = new_symbol_obj;
		total_symbol_count++;
		return true;
	}

	while (current_symbol->next) {
		if (EQUAL(current_symbol, new_symbol_obj)) {		
			free(new_symbol_obj);
			return false;
		}
		current_symbol = current_symbol->next;
	}

	if (EQUAL(current_symbol, new_symbol_obj)) {
		free(new_symbol_obj);
		return false;
	}

	new_symbol_obj->next = NULL;
	new_symbol_obj->prev = current_symbol;
	current_symbol->next = new_symbol_obj;
	current_tip = current_tip->next;
	total_symbol_count++;
	return true;
}


static void print_sym_type(sym_type_t type)
{
	switch (type) {
		case SYM_CONST: printf("SYM_CONST\n"); break;
		case SYM_VAR: printf("SYM_VAR\n"); break;
		case SYM_PROCEDURE: printf("SYM_PROCEDURE\n"); break;
	}
}


void print_table(symbol_t **table)
{
	symbol_t *current_symbol = *table;
	if (!current_symbol) printf("table empty\n");
	while (current_symbol) {
		printf("sym_type: ");
		print_sym_type(current_symbol->type);
		printf("sym_name: %s\nsym_value: %ld\nnesting_level: %zu\n\n", current_symbol->name, 
			current_symbol->value, current_symbol->level);
		current_symbol = current_symbol->next;
	}
}


static size_t free_current_scope()
{
	size_t level = current_level;
	size_t count = 0;
	symbol_t *tmp;
	while (current_tip && current_tip->level == level) {
		tmp = current_tip;
		current_tip = current_tip->prev;
		free(tmp);
		count++;
	}
	total_symbol_count -= count;
	return count;
}


void run_semantic_checks(ast_node_t *root, symbol_t **symbol_table)
{
	if (root->label == AST_CONST_DECL) {
		ast_node_t *current = root->first_child;
		while (current) {
			symbol_t *new_symbtab_entry = new_symbol(current->ident_name, 
						SYM_CONST, current->first_child->num_value, current_level);
			if (insert_sym(symbol_table, new_symbtab_entry)) {
				printf("inserted CONST : %s\n", current->ident_name);
			}
			else {
				fprintf(stderr, "redeclaration of identifier %s\n", current->ident_name);
				error = true;
			}
			current = current->next_sibling;
		}
	}

	else if (root->label == AST_VAR_DECL) {
		ast_node_t *current = root->first_child;
		while (current) {
			symbol_t *new_symbtab_entry = new_symbol(current->ident_name, 
						SYM_VAR, 0, current_level);
			if (insert_sym(symbol_table, new_symbtab_entry)) {
				printf("inserted VAR : %s\n", current->ident_name);
			}
			else {
				fprintf(stderr, "redeclaration of identifier %s\n", current->ident_name);
				error = true;
			}
			current = current->next_sibling;
		}
	}

	else if (root->label == AST_PROC_DECL) {
		ast_node_t *current = root->first_child;
		symbol_t *new_symbtab_entry = new_symbol(root->ident_name,
						SYM_PROCEDURE, 0, current_level);
		if (insert_sym(symbol_table, new_symbtab_entry)) {
			printf("inserted PROC : %s\n", current->ident_name);
		}
		else {
			fprintf(stderr, "redeclaration of identifier %s\n", current->ident_name);
			error = true;
		}
		
		// parse procedure body separately
		current_level++;
		current = current->next_sibling;
		run_semantic_checks(current, symbol_table);
		printf("freed %zu symbols from scope %s\n", free_current_scope(), root->first_child->ident_name);
		//printf("%zu\n", free_current_scope());
		current_level--;
	}

	else if (root->label == AST_ASSIGN) {
		// printf("%s\n", root->first_child->ident_name);
		// look up left child
		symbol_t* found = lookup(root->first_child->ident_name);
		if (found && found->type != VAR) {
			fprintf(stderr, "error: cannot assign/reassign values to constants or procedures\n");
			error = true;
		}
	}

	else {
		ast_node_t *c_root = root->first_child;
		while (c_root) {
			run_semantic_checks(c_root, symbol_table);
			c_root = c_root->next_sibling;	
		}
	}

	if (root->label == AST_ROOT) {
		printf("freed %zu symbols from global scope\n", free_current_scope());
		//printf("%zu\n", free_current_scope());
		*symbol_table = NULL;
	}
}


size_t symbol_count()
{
	return total_symbol_count;
}


bool semantic_error()
{
	return error;
}