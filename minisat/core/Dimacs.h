/****************************************************************************************[Dimacs.h]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

/*
 * This code has been modified as to implement Symmetry Propagation For Sat (SPFS).
 * These modifications are copyrighted to Jo Devriendt (2011-2012), student of the University of Leuven.
 *
 * The same license as above applies concerning the code containing symmetry modifications.
 */

#ifndef Minisat_Dimacs_h
#define Minisat_Dimacs_h

#include <stdio.h>

#include "minisat/utils/ParseUtils.h"
#include "minisat/core/SolverTypes.h"

namespace Minisat {

//=================================================================================================
// DIMACS Parser:

template<class B, class Solver>
static void readClause(B& in, Solver& S, vec<Lit>& lits) {
	int     parsed_lit, var;
	lits.clear();
	for (;;){
		parsed_lit = parseInt(in);
		if (parsed_lit == 0) break;
		var = abs(parsed_lit)-1;
		while (var >= S.nVars()) S.newVar();
		lits.push( (parsed_lit > 0) ? mkLit(var) : ~mkLit(var) );
	}
}

template<class B, class Solver>
static void parse_DIMACS_main(B& in, Solver& S) {
	vec<Lit> lits;
	int vars    = 0;
	int clauses = 0;
	int cnt     = 0;
	for (;;){
		skipWhitespace(in);
		if (*in == EOF) break;
		else if (*in == 'p'){
			if (eagerMatch(in, "p cnf")){
				vars    = parseInt(in);
				clauses = parseInt(in);
				// SATRACE'06 hack
				// if (clauses > 4000000)
				//     S.eliminate(true);
			}else{
				printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
			}
		} else if (*in == 'c' || *in == 'p')
			skipLine(in);
		else{
			cnt++;
			readClause(in, S, lits);
			S.addClause_(lits); }
	}
	if (vars != S.nVars())
		fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of variables.\n");
	if (cnt  != clauses)
		fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
}

// Symmetry is a commented line with cycles.
// The line starts with "c", followed by a blank, followed by the permutation cycles comprised of literals, followed by a blank followed by a zero.
// Every literal in a cycle is either a positive or negative literal.
// The positive literal is itself, the negative literal is the positive literal + the total number of variables.
// e.g.: c (1 2 3)(4 5 6) 0 denotes the symmetry 1->2, 2->3, 3->1, -1->-2, -2->-3, -3->-1.
template<class B, class Solver>
static void parse_SYMMETRY_main(B& in, Solver& S) {
	int nrVars=S.nVars();
	assert(nrVars>0);
	assert(*in=='[');
	++in; // skipping the "["
	++in; // jumping to next line
	int start,first, second;
	while(*in!=']'){
		vec<Lit> symFrom, symTo;
		while(*in=='('){
			++in;
			start = parseInt(in);
			if(start>nrVars){ //adjusting shatter's format
				start=nrVars-start;
			}
			second = start;
			assert(*in==',');
			while(*in==','){
				++in;
				first = second;
				second = parseInt(in);
				if(second>nrVars){ //adjusting shatter's format
					second=nrVars-second;
				}
				if(second>= -nrVars && first>= -nrVars){ //check for phase shift symmetries
					symFrom.push(mkLit(abs(first)-1,first<0));
					symTo.push(mkLit(abs(second)-1,second<0));
				}else{
					assert(second< -nrVars && first< -nrVars);
				}
			}
			assert(*in==')'); ++in;
			first = second;
			second = start;
			if(second>=-nrVars && first>=-nrVars){ //check for phase shift symmetries
				symFrom.push(mkLit(abs(first)-1,first<0));
				symTo.push(mkLit(abs(second)-1,second<0));
			}else{
				assert(second< -nrVars && first< -nrVars);
			}
		}
		S.addSymmetry(symFrom,symTo);
		if(*in==','){
			++in; ++in; assert(*in=='(');
		}else{
			++in; assert(*in==']');
		}
	}
}

// Inserts problem into solver.
//
template<class Solver>
static void parse_DIMACS(gzFile input_stream, Solver& S) {
	StreamBuffer in(input_stream);
	parse_DIMACS_main(in, S); }

// Inserts symmetry into solver.
//
template<class Solver>
static void parse_SYMMETRY(gzFile input_stream, Solver& S) {
	StreamBuffer in(input_stream);
	parse_SYMMETRY_main(in, S); }

//=================================================================================================
}

#endif
