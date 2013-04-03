/* Bracket - Binary Lambda Calculus VM and DSL on top of it
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blc.h"

#define MAX_CELLS 256
#define MAX_REGISTERS 256

#define NIL -1

typedef enum { VAR, LAMBDA, CALL, PROC, WRAP, INPUT } type_t;

typedef struct { int fun; int arg; } call_t;

typedef struct { int block; int env; } proc_t;

typedef struct {
  type_t type;
  union {
    int var;
    int lambda;
    call_t call;
    proc_t proc;
    FILE *file;
  };
  char mark;
} cell_t;

cell_t cells[MAX_CELLS];

int n_registers = 0;
int registers[MAX_REGISTERS];

int is_nil(int cell) { return cell == NIL; }

int type(int cell) { return cells[cell].type; }

int is_type(int cell, int t) { return is_nil(cell) ? 0 : type(cell) == t; }

int is_var(int cell) { return is_type(cell, VAR); }
int is_lambda(int cell) { return is_type(cell, LAMBDA); }
int is_call(int cell) { return is_type(cell, CALL); }
int is_proc(int cell) { return is_type(cell, PROC); }
int is_wrap(int cell) { return is_type(cell, WRAP); }
int is_input(int cell) { return is_type(cell, INPUT); }

int var(int cell) { return is_var(cell) ? cells[cell].var : NIL; }
int lambda(int cell) { return is_lambda(cell) ? cells[cell].lambda : NIL; }
int fun(int cell) { return is_call(cell) ? cells[cell].call.fun : NIL; }
int arg(int cell) { return is_call(cell) ? cells[cell].call.arg : NIL; }
int block(int cell) { return is_proc(cell) || is_wrap(cell) ? cells[cell].proc.block : is_lambda(cell) ? lambda(cell) : NIL; }
int env(int cell) { return is_proc(cell) || is_wrap(cell) ? cells[cell].proc.env : NIL; }
FILE *file(int cell) { return is_input(cell) ? cells[cell].file : NULL; }

void clear_marks(void) {
  int i;
  for (i=0; i<MAX_CELLS; i++)
    cells[i].mark = 0;
}

int find_cell(void)
{
  int retval = 0;
  while (cells[retval].mark) {
    retval++;
    if (retval == MAX_CELLS) break;
  };
  if (retval == MAX_CELLS) retval = NIL;
  return retval;
}

void mark(int expr)
{
  if (!cells[expr].mark) {
    cells[expr].mark = 1;
    switch (type(expr)) {
    case VAR:
      break;
    case LAMBDA:
      mark(block(expr));
      break;
    case CALL:
      mark(fun(expr));
      mark(arg(expr));
      break;
    case PROC:
    case WRAP:
      mark(block(expr));
      mark(env(expr));
      break;
    case INPUT:
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

int gc_push(int expr) { registers[n_registers++] = expr; return expr; }

void gc_pop(int n) { n_registers -= n; }

int cell(void)
{
#ifdef NDEBUG
  int retval = find_cell();
#else
  int retval = NIL;
#endif
  if (is_nil(retval)) {
    clear_marks();
    mark_registers();
    retval = find_cell();
  };
  if (retval != NIL) cells[retval].mark = 1;
  return retval;
}

int make_var(int var)
{
  int retval;
  if (var >= 0) {
    retval = cell();
    if (!is_nil(retval)) {
      cells[retval].type = VAR;
      cells[retval].var = var;
    };
  } else
    retval = NIL;
  return retval;
}

int make_lambda(int lambda)
{
  int retval;
  if (!is_nil(lambda)) {
    gc_push(lambda);
    retval = cell();
    if (!is_nil(retval)) {
      cells[retval].type = LAMBDA;
      cells[retval].lambda = lambda;
    };
    gc_pop(1);
  } else
    retval = NIL;
  return retval;
}

int make_call(int fun, int arg)
{
  int retval;
  if (!is_nil(fun) && !is_nil(arg)) {
    gc_push(fun);
    gc_push(arg);
    retval = cell();
    if (!is_nil(retval)) {
      cells[retval].type = CALL;
      cells[retval].call.fun = fun;
      cells[retval].call.arg = arg;
    };
    gc_pop(2);
  } else
    retval = NIL;
  return retval;
}

int make_proc(int block, int env)
{
  int retval;
  if (!is_nil(block) && !is_nil(env)) {
    gc_push(block);
    gc_push(env);
    retval = cell();
    if (!is_nil(retval)) {
      cells[retval].type = PROC;
      cells[retval].proc.block = block;
      cells[retval].proc.env = env;
    };
    gc_pop(2);
  } else
    retval = NIL;
  return retval;
}

int make_wrap(int block, int env)
{
  int retval;
  if (!is_nil(block) && !is_nil(env)) {
    gc_push(block);
    gc_push(env);
    retval = cell();
    if (!is_nil(retval)) {
      cells[retval].type = WRAP;
      cells[retval].proc.block = block;
      cells[retval].proc.env = env;
    };
    gc_pop(2);
  } else
    retval = NIL;
  return retval;
}

int make_input(FILE *file)
{
  int retval = cell();
  if (!is_nil(retval)) {
    cells[retval].type = INPUT;
    cells[retval].file = file;
  };
  return retval;
}

int make_false(void) { return make_lambda(make_lambda(make_var(0))); }

int is_false(int expr) { return var(block(block(expr))) == 0; }

int make_true(void) { return make_lambda(make_lambda(make_var(1))); }

int is_true(int expr) { return var(block(block(expr))) == 1; }

int read_bit(int input)
{
  int retval;
  int c = fgetc(file(input));
  switch (c) {
  case '0':
    retval = make_false();
    break;
  case '1':
    retval = make_true();
    break;
  case EOF:
    retval = NIL;
    break;
  default:
    retval = read_bit(input);
  };
  return retval;
}

int cons(int car, int cdr)
{
  int retval;
  if (!is_nil(car) && !is_nil(cdr)) {
    gc_push(car);
    gc_push(cdr);
    retval = make_lambda(make_call(make_call(make_var(0), car), cdr));
    gc_pop(2);
  } else
    retval = NIL;
  return retval;
}

int car(int list)
{
  if (is_input(list))
    return read_bit(list);
  else
    return arg(fun(block(list)));
}

int cdr(int list) {
  if (is_input(list))
    return list;
  else
    return arg(block(list));
}

int read_var(int input)
{
  int retval;
  int b = gc_push(car(gc_push(input)));
  if (is_false(b)) {
    retval = cons(cdr(input), gc_push(make_var(0)));
    gc_pop(1);
  } else if (is_true(b)) {
    retval = read_var(cdr(input));
    if (!is_nil(retval)) cells[cdr(retval)].var++;
  } else
    retval = NIL;
  gc_pop(2);
  return retval;
}

int read_lambda(int input)
{
  int term = gc_push(read_expr(gc_push(input)));
  int retval = cons(car(term), gc_push(make_lambda(cdr(term))));
  gc_pop(3);
  return retval;
}

int read_call(int input)
{
  int fun = gc_push(read_expr(gc_push(input)));
  int arg = gc_push(read_expr(car(fun))); // read_expr should return input object
  int retval = cons(car(arg), gc_push(make_call(cdr(fun), cdr(arg))));
  gc_pop(4);
  return retval;
}

int read_expr(int input)
{
  int retval;
  int b1 = gc_push(car(gc_push(input)));
  input = cdr(input);
  if (is_false(b1)) {
    int b2 = gc_push(car(input));
    input = cdr(input);
    if (is_false(b2))
      retval = read_lambda(input);
    else if (is_true(b2))
      retval = read_call(input);
    else
      retval = NIL;
    gc_pop(1);
  } else if (is_true(b1))
    retval = read_var(input);
  else
    retval = NIL;
  gc_pop(2);
  return retval;
}

int length(int list)
{
  int retval;
  if (!is_call(block(list)))
    retval = 0;
  else
    retval = 1 + length(arg(block(list)));
  return retval;
}

void print_var(int var, FILE *file)
{
  fputc('1', file);
  if (var > 0)
    print_var(var - 1, file);
  else
    fputc('0', file);
}

void print_lambda(int lambda, FILE *file)
{
  fputs("00", file);
  print_expr(lambda, file);
}

void print_call(int fun, int arg, FILE *file)
{
  fputs("01", file);
  print_expr(fun, file);
  print_expr(arg, file);
}

void print_proc(int block, int env, FILE *file)
{
  fputs("#<proc:", file);
  print_expr(block, file);
  fprintf(file, ";#env=%d>", length(env));
}

void print_wrap(int block, int env, FILE *file)
{
  fputs("#<wrap:", file);
  print_expr(block, file);
  fprintf(file, ";#env=%d>", length(env));
}

void print_expr(int expr, FILE *file)
{
  if (!is_nil(expr)) {
    switch (type(expr)) {
    case VAR:
      print_var(var(expr), file);
      break;
    case LAMBDA:
      print_lambda(block(expr), file);
      break;
    case CALL:
      print_call(fun(expr), arg(expr), file);
      break;
    case PROC:
      print_proc(block(expr), env(expr), file);
      break;
    case WRAP:
      print_wrap(block(expr), env(expr), file);
      break;
    case INPUT:
      fputs("#<input>", file);
      break;
    }
  } else
    fputs("#<err>", file);
}

int lookup(int var, int env) { return var > 0 ? lookup(var - 1, cdr(env)) : car(env); }

int eval_expr(int expr, int local_env)
{
  int retval;
  int eval_fun;
  int wrap_arg;
  int call_env;
  int bit;
  gc_push(expr);
  gc_push(local_env);
  if (!is_nil(expr)) {
    switch (type(expr)) {
    case VAR:
      retval = lookup(var(expr), local_env);
      if (!is_nil(retval))
        retval = eval_expr(retval, local_env);
      else
        retval = make_var(var(expr) - length(local_env));
      break;
    case LAMBDA:
      retval = make_proc(block(expr), local_env);
      break;
    case CALL:
      eval_fun = gc_push(eval_expr(fun(expr), local_env));
      wrap_arg = gc_push(make_wrap(arg(expr), local_env));
      if (is_proc(eval_fun)) {
        call_env = gc_push(cons(wrap_arg, env(eval_fun)));
        retval = eval_expr(block(eval_fun), call_env);
        gc_pop(1);
      } else
        retval = eval_fun;
      gc_pop(2);
      break;
    case PROC:
      retval = expr;
      break;
    case WRAP:
      retval = eval_expr(block(expr), env(expr));
      break;
    case INPUT:
      bit = car(expr);
      if (!is_nil(bit))
        retval = eval_expr(cons(bit, expr), local_env);
      else
        retval = eval_expr(make_false(), local_env);
      break;
    default:
      retval = NIL;
    }
  } else
    retval = NIL;
  gc_pop(2);
  return retval;
}
