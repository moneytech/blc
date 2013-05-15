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
#include <assert.h>
#include <string.h>
#include "blc.h"
#include "lambda.h"

#define BUFSIZE 256

int test_compile(char *cmd, char *spec)
{
  int retval = 0;
  char buffer[BUFSIZE];
  char *result = buffer;
  FILE *f_in = fmemopen(cmd, strlen(cmd), "r");
  FILE *f_out = fmemopen(result, BUFSIZE, "w");
  compile_lambda(f_in, f_out);
  fclose(f_out);
  fclose(f_in);
  if (strcmp(spec, result)) {
    fprintf(stderr, "Result of compiling \"%s\" is \"%s\" but should be \"%s\"\n", cmd, result, spec);
    retval = 1;
  };
  return retval;
}

int main(void)
{
  int retval = 0;
  retval = retval | test_compile("0010", "0010");
  retval = retval | test_compile("00.10", "0010");
  retval = retval | test_compile("-10", "10");
  retval = retval | test_compile("->.10", "0010");
  retval = retval | test_compile("λ.10", "0010");
  retval = retval | test_compile("-10>", "10");
  retval = retval | test_compile("->x.x", "0010");
  retval = retval | test_compile("λx.x", "0010");
  retval = retval | test_compile("->xy.xy", "0010");
  retval = retval | test_compile("-> x.x", "0010");
  retval = retval | test_compile("λ x.x", "0010");
  retval = retval | test_compile("->x .x", "0010");
  retval = retval | test_compile("->x. x", "0010");
  retval = retval | test_compile("->x.->y.x", "0000110");
  retval = retval | test_compile("->x.->y.y", "000010");
  retval = retval | test_compile("->x.->x.x", "000010");
  retval = retval | test_compile("00->x.x", "000010");
  retval = retval | test_compile("->.->y.y", "000010");
  retval = retval | test_compile("λ.λy.y", "000010");
  retval = retval | test_compile("->->y.y", "000010");
  retval = retval | test_compile("λλy.y", "000010");
  retval = retval | test_compile("->x->y.x", "0000110");
  retval = retval | test_compile("λxλy.x", "0000110");
  retval = retval | test_compile("->x y.x", "0000110");
  retval = retval | test_compile("λx y.x", "0000110");
  retval = retval | test_compile("->x.->y.x", "0000110");
  retval = retval | test_compile("->x->y.x", "0000110");
  retval = retval | test_compile("->x->.x", "0000110");
  retval = retval | test_compile("-> x -> y . x", "0000110");
  retval = retval | test_compile("01->x.x->y.y", "0100100010");
  retval = retval | test_compile("010010->y.y", "0100100010");
  retval = retval | test_compile("01->x.x 0010", "0100100010");
  retval = retval | test_compile("01 ->x.x ->y.y", "0100100010");
  retval = retval | test_compile("01 ->x.x ->x.x", "0100100010");
  retval = retval | test_compile("(->x.x ->y.y)", "0100100010");
  retval = retval | test_compile("(->x.x ->x.x)", "0100100010");
  retval = retval | test_compile("((->x->y.x 10) 110)", "0101000011010110");
  retval = retval | test_compile("((->x->y.y 10) 110)", "010100001010110");
  retval = retval | test_compile("(->x->y.x 10 110)", "0101000011010110");
  retval = retval | test_compile("(->x->y.y 10 110)", "010100001010110");
  retval = retval | test_compile("(10 ->x->y.x) 0", "011000001100");
  retval = retval | test_compile("(10)", "10");
  retval = retval | test_compile("((((((->input->output->I->true->false->Y."
                                 "((Y->f->input.(((input true)true)(f(input false))))input)"
                                 "10)110)->x.x)->x->y.x)->x->y.y)->f.(->x.(f(x x))->x.(f(x x))))",
                                 "0101010101010000000000000101100000010101101111101111100111001101"
                                 "111011111101011000100000110000010000100011100110100001110011010");
  return retval;
}

