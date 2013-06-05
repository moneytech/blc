/* BLC - Binary Lambda Calculus VM
 * Copyright (C) 2013  Jan Wedekind
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */
#ifndef __BLC_H
#define __BLC_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>

#define NIL -1

int gc_push(int expression);

void gc_pop(int n);

int is_definition(int cell);

int term(int cell);

int body(int cell);

int read_expression(int input);

int eval_expression(int expression, int local_env);

void print_expression(int expression, FILE *file);

void print_variable(int variable, FILE *file);

int make_variable(int variable);

int make_lambda(int lambda);

int make_call(int function, int argument);

int make_definition(int term, int body);

int make_input(FILE *file);

int make_output(FILE *file);

int make_false(void);

int make_true(void);

int make_pair(int first, int second);

int first(int list);

int second(int list);

#endif

