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
#include "minisat/mtl/Matrix.h"

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

// BreakID format
template<class B, class Solver>
static void parse_SYMMETRY_main(B& in, Solver& S) {
    vec<Lit> from;
    vec<Lit> to;
    vec<Lit> cycle;
    for (;;){
        skipWhitespace(in);
        if (*in == EOF) {
            break;
        } else if (*in == '('){
            // clear from/to to start a new symmetry
            from.clear(); to.clear();
            while(*in != '\n'){ // parse one symmetry
                cycle.clear();
                ++in; // skipping '('
                ++in; // skipping ' '
                while(*in != ')'){ // parse one cycle
                    int parsed_lit = parseInt(in);
                    int var = abs(parsed_lit)-1;
                    Lit l = (parsed_lit > 0) ? mkLit(var) : ~mkLit(var);
                    cycle.push(l);
                    ++in; // skipping ' '
                }
                // add cycle to from/to
                for(int i=0; i<cycle.size()-1; ++i){
                    from.push(cycle[i]);
                    to.push(cycle[i+1]);
                }
                if(cycle.size()>0){
                    from.push(cycle.last());
                    to.push(cycle[0]);
                }
                ++in; // skipping ')'
                ++in; // skipping ' '
            }
            ++in; // skipping '/n'
            S.addSymmetry(from,to);
        } else if (*in == 'r'){
            // interchangeability matrix
            ++in; ++in; ++in; ++in; // skipping "rows"
            int nbRows = parseInt(in);
            ++in; ++in; ++in; ++in; ++in; ++in; ++in; ++in; // skipping " columns"
            int nbColumns = parseInt(in);
            matrix<Lit> symmat(nbRows,nbColumns);
            for(int i=0; i<nbRows; ++i){
                for(int j=0; j<nbColumns; ++j){
                    int parsed_lit = parseInt(in);
                    int var = abs(parsed_lit)-1;
                    Lit l = (parsed_lit > 0) ? mkLit(var) : ~mkLit(var);
                    symmat.set(i,j,l);
                }
            }    
            for(int i=0; i<symmat.nr; ++i){
                for(int j=i+1; j<symmat.nr; ++j){
                    from.clear(); to.clear();
                    for(int k=0; k<symmat.nc; ++k){
                        from.push(symmat.get(i,k)); from.push(symmat.get(j,k));
                        to.push(symmat.get(j,k));   to.push(symmat.get(i,k));
                    }
                    S.addSymmetry(from,to);
                }
            }
            ++in; // skipping "\n"
        } else {
            skipLine(in);
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
