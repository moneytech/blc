/* Bracket - Attempt at writing a small Racket (Scheme) interpreter
 * Copyright (C) 2012  Jan Wedekind
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
#include <stdio.h>
#include "blc.h"
#define MAX_CELLS 1024
#define MAX_REGISTERS 1024

typedef enum { VAR, LAMBDA, PAIR } type_t;

typedef struct {
  int fun;
  int arg;
} pair_t;

typedef struct {
  type_t type;
  union {
    int var;
    int lambda;
    pair_t pair;
  };
  char mark;
} cell_t;

cell_t cells[MAX_CELLS];

int n_registers = 0;
int registers[MAX_REGISTERS];

void clear_marks(void)
{
  int i;
  for (i=0; i<MAX_CELLS; i++)
    cells[i].mark = 0;
}

void mark(int expr)
{
  if (!cells[expr].mark) {
    cells[expr].mark = 1;
    switch (cells[expr].type) {
    case VAR:
      break;
    case LAMBDA:
      mark(cells[expr].lambda);
      break;
    case PAIR:
      mark(cells[expr].pair.fun);
      mark(cells[expr].pair.arg);
      break;
    }
  };
}

void mark_registers(void)
{
  int i;
  for (i=0; i<n_registers; i++)
    mark(registers[i]);
}

int gc_push(int expr)
{
  registers[n_registers++] = expr;
  return expr;
}

void gc_pop(int n)
{
  n_registers -= n;
}

int cell(void)
{
  int retval = 0;
  clear_marks();
  mark_registers();
  while (cells[retval].mark) {
    retval++;
    if (retval == MAX_CELLS) break;
  };
  if (retval == MAX_CELLS)
    retval = -1;
  else
    cells[retval].mark = 1;
  return retval;
}

int read_bit(FILE *stream)
{
  int retval;
  int c = fgetc(stream);
  switch (c) {
  case '0':
    retval = 0;
    break;
  case '1':
    retval = 1;
    break;
  case EOF:
    retval = -1;
    break;
  default:
    retval = read_bit(stream);
  };
  return retval;
}

int make_var(int var)
{
  int retval;
  if (var >= 0) {
    retval = cell();
    if (retval >= 0) {
      cells[retval].type = VAR;
      cells[retval].var = var;
    };
  } else
    retval = -1;
  return retval;
}

int read_var(FILE *stream)
{
  int retval;
  int b = read_bit(stream);
  if (b == 0)
    retval = make_var(0);
  else if (b == 1) {
    retval = read_var(stream);
    if (retval >= 0) cells[retval].var++;
  } else
    retval = -1;
  return retval;
}

void print_var(int var, FILE *stream)
{
  fputc('1', stream);
  if (var > 0)
    print_var(var - 1, stream);
  else
    fputc('0', stream);
}

int make_lambda(int lambda)
{
  int retval;
  gc_push(lambda);
  if (lambda >= 0) {
    retval = cell();
    if (retval >= 0) {
      cells[retval].type = LAMBDA;
      cells[retval].lambda = lambda;
    };
  } else
    retval = -1;
  gc_pop(1);
  return retval;
}

int read_lambda(FILE *stream)
{
  return make_lambda(read_expr(stream));
}

void print_lambda(int lambda, FILE *stream)
{
  fputs("00", stream);
  print_expr(lambda, stream);
}

int make_pair(int fun, int arg)
{
  int retval;
  gc_push(fun);
  gc_push(arg);
  if (fun >= 0 && arg >= 0) {
    retval = cell();
    if (retval >= 0) {
      cells[retval].type = PAIR;
      cells[retval].pair.fun = fun;
      cells[retval].pair.arg = arg;
    };
  } else
    retval = -1;
  gc_pop(2);
  return retval;
}

int read_pair(FILE *stream)
{
  int fun = gc_push(read_expr(stream));
  int arg = gc_push(read_expr(stream));
  gc_pop(2);
  return make_pair(fun, arg);
}

void print_pair(int fun, int arg, FILE *stream)
{
  fputs("01", stream);
  print_expr(fun, stream);
  print_expr(arg, stream);
}

int read_expr(FILE *stream)
{
  int retval;
  int b1 = read_bit(stream);
  if (b1 == 0) {
    int b2 = read_bit(stream);
    if (b2 == 0)
      retval = read_lambda(stream);
    else if (b2 == 1)
      retval = read_pair(stream);
    else
      retval = -1;
  } else if (b1 == 1)
    retval = read_var(stream);
  else
    retval = -1;
  return retval;
}

void print_expr(int expr, FILE *stream)
{
  if (expr >= 0) {
    switch (cells[expr].type) {
    case VAR:
      print_var(cells[expr].var, stream);
      break;
    case LAMBDA:
      print_lambda(cells[expr].lambda, stream);
      break;
    case PAIR:
      print_pair(cells[expr].pair.fun, cells[expr].pair.arg, stream);
      break;
    default:
      fputs("#<err>", stream);
    }
  } else
    fputs("#<err>", stream);
}

int lift_free_vars(int expr, int amount, int depth)
{
  int retval;
  int var;
  int fun;
  int arg;
  gc_push(expr);
  if (expr >= 0) {
    switch (cells[expr].type) {
    case VAR:
      var = cells[expr].var;
      if (var >= depth) {
        if (var + amount >= depth)
          retval = make_var(var + amount);
        else
          retval = -1;
      } else
        retval = expr;
      break;
    case LAMBDA:
      retval = make_lambda(lift_free_vars(cells[expr].lambda, amount, depth + 1));
      break;
    case PAIR:
      fun = gc_push(lift_free_vars(cells[expr].pair.fun, amount, depth));
      arg = gc_push(lift_free_vars(cells[expr].pair.arg, amount, depth));
      retval = make_pair(fun, arg);
      gc_pop(2);
      break;
    default:
      retval = 0;
    }
  } else
    retval = -1;
  gc_pop(1);
  return retval;
}

int subst(int expr, int replacement, int depth)
{
  int retval;
  int fun;
  int arg;
  gc_push(expr);
  gc_push(replacement);
  if (expr >= 0)
    switch (cells[expr].type) {
    case VAR:
      if (cells[expr].var == depth)
        retval = lift_free_vars(replacement, depth + 1, 0);
      else
        retval = expr;
      break;
    case LAMBDA:
      retval = make_lambda(subst(cells[expr].lambda, replacement, depth + 1));
      break;
    case PAIR:
      fun = gc_push(subst(cells[expr].pair.fun, replacement, depth));
      arg = gc_push(subst(cells[expr].pair.arg, replacement, depth));
      retval = make_pair(fun, arg);
      gc_pop(2);
      break;
    default:
      retval = -1;
    }
  else
    retval = -1;
  gc_pop(2);
  return retval;
}

int eval_expr(int expr)
{
  int retval;
  int fun;
  int arg;
  gc_push(expr);
  if (expr >= 0) {
    switch (cells[expr].type) {
    case VAR:
    case LAMBDA:
      retval = expr;
      break;
    case PAIR:
      fun = gc_push(eval_expr(cells[expr].pair.fun));
      arg = gc_push(cells[expr].pair.arg);
      if (cells[fun].type == LAMBDA)
        retval = eval_expr(lift_free_vars(subst(cells[fun].lambda, arg, 0), -1, 0));
      else
        retval = -1;
      gc_pop(2);
      break;
    default:
      retval = -1;
    }
  } else
    retval = -1;
  gc_pop(1);
  return retval;
}

