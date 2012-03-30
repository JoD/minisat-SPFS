/****************************************************************************************[Solver.h]
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

#ifndef Minisat_Solver_h
#define Minisat_Solver_h

#include "minisat/mtl/Vec.h"
#include "minisat/mtl/Heap.h"
#include "minisat/mtl/Alg.h"
#include "minisat/mtl/IntMap.h"
#include "minisat/utils/Options.h"
#include "minisat/core/SolverTypes.h"
#include <set>


namespace Minisat {

class Symmetry;

//=================================================================================================
// Solver -- the main class:

class Solver {
public:

    // Constructor/Destructor:
    //
    Solver();
    virtual ~Solver();

    // Problem specification:
    //
    Var     newVar    (lbool upol = l_Undef, bool dvar = true); // Add a new variable with parameters specifying variable mode.
    void    releaseVar(Lit l);                                  // Make literal true and promise to never refer to variable again.

    bool    addClause (const vec<Lit>& ps);                     // Add a clause to the solver. 
    bool    addEmptyClause();                                   // Add the empty clause, making the solver contradictory.
    bool    addClause (Lit p);                                  // Add a unit clause to the solver. 
    bool    addClause (Lit p, Lit q);                           // Add a binary clause to the solver. 
    bool    addClause (Lit p, Lit q, Lit r);                    // Add a ternary clause to the solver. 
    bool    addClause_(      vec<Lit>& ps);                     // Add a clause to the solver without making superflous internal copy. Will
                                                                // change the passed vector 'ps'.

    // Solving:
    //
    bool    simplify     ();                        // Removes already satisfied clauses.
    bool    solve        (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions.
    lbool   solveLimited (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
    bool    solve        ();                        // Search without assumptions.
    bool    solve        (Lit p);                   // Search for a model that respects a single assumption.
    bool    solve        (Lit p, Lit q);            // Search for a model that respects two assumptions.
    bool    solve        (Lit p, Lit q, Lit r);     // Search for a model that respects three assumptions.
    bool    okay         () const;                  // FALSE means solver is in a conflicting state

    bool    implies      (const vec<Lit>& assumps, vec<Lit>& out);

    // Iterate over clauses and top-level assignments:
    ClauseIterator clausesBegin() const;
    ClauseIterator clausesEnd()   const;
    TrailIterator  trailBegin()   const;
    TrailIterator  trailEnd  ()   const;

    void    toDimacs     (FILE* f, const vec<Lit>& assumps);            // Write CNF to file in DIMACS-format.
    void    toDimacs     (const char *file, const vec<Lit>& assumps);
    void    toDimacs     (FILE* f, Clause& c, vec<Var>& map, Var& max);

    // Convenience versions of 'toDimacs()':
    void    toDimacs     (const char* file);
    void    toDimacs     (const char* file, Lit p);
    void    toDimacs     (const char* file, Lit p, Lit q);
    void    toDimacs     (const char* file, Lit p, Lit q, Lit r);
    
    // Variable mode:
    // 
    void    setPolarity    (Var v, lbool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
    void    setDecisionVar (Var v, bool b);  // Declare if a variable should be eligible for selection in the decision heuristic.

    // Read state:
    //
    lbool   value      (Var x) const;       // The current value of a variable.
    lbool   value      (Lit p) const;       // The current value of a literal.
    lbool   modelValue (Var x) const;       // The value of a variable in the last model. The last call to solve must have been satisfiable.
    lbool   modelValue (Lit p) const;       // The value of a literal in the last model. The last call to solve must have been satisfiable.
    int     nAssigns   ()      const;       // The current number of assigned literals.
    int     nClauses   ()      const;       // The current number of original clauses.
    int     nLearnts   ()      const;       // The current number of learnt clauses.
    int     nVars      ()      const;       // The current number of variables.
    int     nFreeVars  ()      const;
    void    printStats ()      const;       // Print some current statistics to standard output.

    // Resource contraints:
    //
    void    setConfBudget(int64_t x);
    void    setPropBudget(int64_t x);
    void    budgetOff();
    void    interrupt();          // Trigger a (potentially asynchronous) interruption of the solver.
    void    clearInterrupt();     // Clear interrupt indicator flag.

    // Memory managment:
    //
    virtual void garbageCollect();
    void    checkGarbage(double gf);
    void    checkGarbage();

    // Extra results: (read-only member variable)
    //
    vec<lbool> model;             // If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>   conflict;          // If problem is unsatisfiable (possibly under assumptions),
                                  // this vector represent the final conflict clause expressed in the assumptions.

    // Mode of operation:
    //
    int       verbosity;
    double    var_decay;
    double    clause_decay;
    double    random_var_freq;
    double    random_seed;
    bool      luby_restart;
    int       ccmin_mode;         // Controls conflict clause minimization (0=none, 1=basic, 2=deep).
    int       phase_saving;       // Controls the level of phase saving (0=none, 1=limited, 2=full).
    bool      rnd_pol;            // Use random polarities for branching heuristics.
    bool      rnd_init_act;       // Initialize variable activities with a small random value.
    double    garbage_frac;       // The fraction of wasted memory allowed before a garbage collection is triggered.

    int       restart_first;      // The initial restart limit.                                                                (default 100)
    double    restart_inc;        // The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
    double    learntsize_factor;  // The intitial limit for learnt clauses is a factor of the original clauses.                (default 1 / 3)
    double    learntsize_inc;     // The limit for learnt clauses is multiplied with this factor each restart.                 (default 1.1)

    int       learntsize_adjust_start_confl;
    double    learntsize_adjust_inc;

    bool	addPropagationClauses;
    bool	addConflictClauses;
    bool	varOrderOptimization;
    bool	inactivePropagationOptimization;

    // Statistics: (read-only member variable)
    //
    uint64_t solves, starts, decisions, rnd_decisions, propagations, conflicts;
	uint64_t dec_vars, num_clauses, num_learnts, clauses_literals, learnts_literals, max_literals, tot_literals;
    uint64_t sympropagations, symconflicts, invertingSyms;

	// Symmetry methods:
	//
	void	addSymmetry(vec<Lit>& from, vec<Lit>& to);	// Add a symmetry to the solver.
	bool	isDecision(Lit l){return decisionVars[var(l)];}
	void	notifySymmetries(Lit p);
	CRef 	propagateSymmetrical(Symmetry* sym, Lit l);
	bool 	hasLowerLevel(Lit first, Lit second){ return level(var(first))<level(var(second)); }
	bool 	canPropagate(Symmetry* sym, Clause& cl);
	int		nSymmetries(){return symmetries.size();}
	int		nInvertingSymmetries(){return invertingSyms;}

	bool	testSymmetry(Symmetry* sym);
	bool 	testActivityForSymmetries();
	bool	testPropagationClause(Symmetry* sym, Lit l, vec<Lit>& reason);
	bool	testConflictClause(Symmetry* sym, Lit l, vec<Lit>& reason);
	void	testPrintSymmetricalClauseInfo(Symmetry* sym, Lit l, vec<Lit>& reason);
	void	testPrintTrail();
	void	testPrintClause(vec<Lit>& reason);
	void	testPrintClause(CRef clause);
	void 	testPrintValue(Lit l);
	void 	testPrintClauseDimacs(CRef clause);
	int		toDimacs(Lit l);

protected:

    // Helper structures:
    //
    struct VarData { CRef reason; int level; };
    static inline VarData mkVarData(CRef cr, int l){ VarData d = {cr, l}; return d; }

    struct Watcher {
        CRef cref;
        Lit  blocker;
        Watcher(CRef cr, Lit p) : cref(cr), blocker(p) {}
        bool operator==(const Watcher& w) const { return cref == w.cref; }
        bool operator!=(const Watcher& w) const { return cref != w.cref; }
    };

    struct WatcherDeleted
    {
        const ClauseAllocator& ca;
        WatcherDeleted(const ClauseAllocator& _ca) : ca(_ca) {}
        bool operator()(const Watcher& w) const { return ca[w.cref].mark() == 1; }
    };

    struct VarOrderLt {
        const IntMap<Var, double>&  activity;
        bool operator () (Var x, Var y) const { return activity[x] > activity[y]; }
        VarOrderLt(const IntMap<Var, double>&  act) : activity(act) { }
    };

    struct ShrinkStackElem {
        uint32_t i;
        Lit      l;
        ShrinkStackElem(uint32_t _i, Lit _l) : i(_i), l(_l){}
    };

    // Solver state:
    //
    vec<CRef>           clauses;          // List of problem clauses.
    vec<CRef>           learnts;          // List of learnt clauses.
    vec<Lit>            trail;            // Assignment stack; stores all assigments made in the order they were made.
    vec<int>            trail_lim;        // Separator indices for different decision levels in 'trail'.
    vec<Lit>            assumptions;      // Current set of assumptions provided to solve by the user.

    VMap<double>        activity;         // A heuristic measurement of the activity of a variable.
    VMap<lbool>         assigns;          // The current assignments.
    VMap<char>          polarity;         // The preferred polarity of each variable.
    VMap<lbool>         user_pol;         // The users preferred polarity of each variable.
    VMap<char>          decision;         // Declares if a variable is eligible for selection in the decision heuristic.
    VMap<VarData>       vardata;          // Stores reason and level for each variable.
    OccLists<Lit, vec<Watcher>, WatcherDeleted, MkIndexLit>
                        watches;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).

    Heap<Var,VarOrderLt>order_heap;       // A priority queue of variables ordered with respect to the variable activity.

    bool                ok;               // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    double              cla_inc;          // Amount to bump next clause with.
    double              var_inc;          // Amount to bump next variable with.
    int                 qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    int                 simpDB_assigns;   // Number of top-level assignments since last execution of 'simplify()'.
    int64_t             simpDB_props;     // Remaining number of propagations that must be made before next execution of 'simplify()'.
    double              progress_estimate;// Set by 'search()'.
    bool                remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.
    Var                 next_var;         // Next variable to be created.
    ClauseAllocator     ca;

    vec<Var>            released_vars;
    vec<Var>            free_vars;

    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    VMap<char>          seen;
    vec<ShrinkStackElem>analyze_stack;
    vec<Lit>            analyze_toclear;
    vec<Lit>            add_tmp;

    double              max_learnts;
    double              learntsize_adjust_confl;
    int                 learntsize_adjust_cnt;

    // Resource contraints:
    //
    int64_t             conflict_budget;    // -1 means no budget.
    int64_t             propagation_budget; // -1 means no budget.
    bool                asynch_interrupt;

    // Main internal methods:
    //
    void     insertVarOrder   (Var x);                                                 // Insert a variable in the decision order priority queue.
    Lit      pickBranchLit    ();                                                      // Return the next decision variable.
    void     newDecisionLevel ();                                                      // Begins a new decision level.
    void     uncheckedEnqueue (Lit p, CRef from = CRef_Undef);                         // Enqueue a literal. Assumes value of literal is undefined.
    bool     enqueue          (Lit p, CRef from = CRef_Undef);                         // Test if fact 'p' contradicts current state, enqueue otherwise.
    CRef     propagate        ();                                                      // Perform unit propagation. Returns possibly conflicting clause.
    void     cancelUntil      (int level);                                             // Backtrack until a certain level.
    void     analyze          (CRef confl, vec<Lit>& out_learnt, int& out_btlevel);    // (bt = backtrack)
    void     analyzeFinal     (Lit p, vec<Lit>& out_conflict);                         // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
    bool     litRedundant     (Lit p);                                                 // (helper method for 'analyze()')
    lbool    search           (int nof_conflicts);                                     // Search for a given number of conflicts.
    lbool    solve_           ();                                                      // Main solve method (assumptions given in 'assumptions').
    void     reduceDB         ();                                                      // Reduce the set of learnt clauses.
    void     removeSatisfied  (vec<CRef>& cs);                                         // Shrink 'cs' to contain only non-satisfied clauses.
    void     rebuildOrderHeap ();

    // Maintaining Variable/Clause activity:
    //
    void     varDecayActivity ();                      // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    void     varBumpActivity  (Var v, double inc);     // Increase a variable with the current 'bump' value.
    void     varBumpActivity  (Var v);                 // Increase a variable with the current 'bump' value.
    void     claDecayActivity ();                      // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
    void     claBumpActivity  (Clause& c);             // Increase a clause with the current 'bump' value.

    // Operations on clauses:
    //
    void     attachClause     (CRef cr);               // Attach a clause to watcher lists.
    void     detachClause     (CRef cr, bool strict = false); // Detach a clause to watcher lists.
    void     removeClause     (CRef cr);               // Detach and free a clause.
    bool     isRemoved        (CRef cr) const;         // Test if a clause has been removed.
    bool     locked           (const Clause& c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
    bool     satisfied        (const Clause& c) const; // Returns TRUE if a clause is satisfied in the current state.

    // Misc:
    //
    int      decisionLevel    ()      const; // Gives the current decisionlevel.
    uint32_t abstractLevel    (Var x) const; // Used to represent an abstraction of sets of decision levels.
    CRef     reason           (Var x) const;
    int      level            (Var x) const;
    double   progressEstimate ()      const; // DELETE THIS ?? IT'S NOT VERY USEFUL ...
    bool     withinBudget     ()      const;
    void     relocAll         (ClauseAllocator& to);

	// Symmetry data structures:
	//
	vec<Symmetry*>		symmetries;		  	// List of all symmetries in the solver.
	vec<bool>			decisionVars;		// map mapping vars to a bool which if true iff the lit is a decision lit.
	vec<vec<Symmetry*> > watcherSymmetries; // List of symmetries which should be notified know when a certain literal  becomes true (index is lit)
	vec<Lit> 			implic;				// used when constructing clauses
	const static bool	debug=false; 		// if true the slow test methods are enabled

    // Static helpers:
    //

    // Returns a random float 0 <= x < 1. Seed must never be 0.
    static inline double drand(double& seed) {
        seed *= 1389796;
        int q = (int)(seed / 2147483647);
        seed -= (double)q * 2147483647;
        return seed / 2147483647; }

    // Returns a random integer 0 <= x < size. Seed must never be 0.
    static inline int irand(double& seed, int size) {
        return (int)(drand(seed) * size); }

	friend class Symmetry;

};


//=================================================================================================
// Implementation of inline methods:

inline CRef Solver::reason(Var x) const { return vardata[x].reason; }
inline int  Solver::level (Var x) const { return vardata[x].level; }

inline void Solver::insertVarOrder(Var x) {
    if (!order_heap.inHeap(x) && decision[x]) order_heap.insert(x); }

inline void Solver::varDecayActivity() { var_inc *= (1 / var_decay); }
inline void Solver::varBumpActivity(Var v) { varBumpActivity(v, var_inc); }
inline void Solver::varBumpActivity(Var v, double inc) {
    if ( (activity[v] += inc) > 1e100 ) {
        // Rescale:
        for (int i = 0; i < nVars(); i++)
            activity[i] *= 1e-100;
        var_inc *= 1e-100; }

    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(v))
        order_heap.decrease(v); }

inline void Solver::claDecayActivity() { cla_inc *= (1 / clause_decay); }
inline void Solver::claBumpActivity (Clause& c) {
        if ( (c.activity() += cla_inc) > 1e20 ) {
            // Rescale:
            for (int i = 0; i < learnts.size(); i++)
                ca[learnts[i]].activity() *= 1e-20;
            cla_inc *= 1e-20; } }

inline void Solver::checkGarbage(void){ return checkGarbage(garbage_frac); }
inline void Solver::checkGarbage(double gf){
    if (ca.wasted() > ca.size() * gf)
        garbageCollect(); }

// NOTE: enqueue does not set the ok flag! (only public methods do)
inline bool     Solver::enqueue         (Lit p, CRef from)      { return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true); }
inline bool     Solver::addClause       (const vec<Lit>& ps)    { ps.copyTo(add_tmp); return addClause_(add_tmp); }
inline bool     Solver::addEmptyClause  ()                      { add_tmp.clear(); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p)                 { add_tmp.clear(); add_tmp.push(p); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q)          { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q, Lit r)   { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); add_tmp.push(r); return addClause_(add_tmp); }
inline bool     Solver::isRemoved       (CRef cr)         const { return ca[cr].mark() == 1; }
inline bool     Solver::locked          (const Clause& c) const { return value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && ca.lea(reason(var(c[0]))) == &c; }
inline void     Solver::newDecisionLevel()                      { trail_lim.push(trail.size()); }

inline int      Solver::decisionLevel ()      const   { return trail_lim.size(); }
inline uint32_t Solver::abstractLevel (Var x) const   { return 1 << (level(x) & 31); }
inline lbool    Solver::value         (Var x) const   { return assigns[x]; }
inline lbool    Solver::value         (Lit p) const   { return assigns[var(p)] ^ sign(p); }
inline lbool    Solver::modelValue    (Var x) const   { return model[x]; }
inline lbool    Solver::modelValue    (Lit p) const   { return model[var(p)] ^ sign(p); }
inline int      Solver::nAssigns      ()      const   { return trail.size(); }
inline int      Solver::nClauses      ()      const   { return num_clauses; }
inline int      Solver::nLearnts      ()      const   { return num_learnts; }
inline int      Solver::nVars         ()      const   { return next_var; }
// TODO: nFreeVars() is not quite correct, try to calculate right instead of adapting it like below:
inline int      Solver::nFreeVars     ()      const   { return (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]); }
inline void     Solver::setPolarity   (Var v, lbool b){ user_pol[v] = b; }
inline void     Solver::setDecisionVar(Var v, bool b) 
{ 
    if      ( b && !decision[v]) dec_vars++;
    else if (!b &&  decision[v]) dec_vars--;

    decision[v] = b;
    insertVarOrder(v);
}
inline void     Solver::setConfBudget(int64_t x){ conflict_budget    = conflicts    + x; }
inline void     Solver::setPropBudget(int64_t x){ propagation_budget = propagations + x; }
inline void     Solver::interrupt(){ asynch_interrupt = true; }
inline void     Solver::clearInterrupt(){ asynch_interrupt = false; }
inline void     Solver::budgetOff(){ conflict_budget = propagation_budget = -1; }
inline bool     Solver::withinBudget() const {
    return !asynch_interrupt &&
           (conflict_budget    < 0 || conflicts < (uint64_t)conflict_budget) &&
           (propagation_budget < 0 || propagations < (uint64_t)propagation_budget); }

// FIXME: after the introduction of asynchronous interrruptions the solve-versions that return a
// pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
// all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.
inline bool     Solver::solve         ()                    { budgetOff(); assumptions.clear(); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p)               { budgetOff(); assumptions.clear(); assumptions.push(p); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q)        { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q, Lit r) { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); assumptions.push(r); return solve_() == l_True; }
inline bool     Solver::solve         (const vec<Lit>& assumps){ budgetOff(); assumps.copyTo(assumptions); return solve_() == l_True; }
inline lbool    Solver::solveLimited  (const vec<Lit>& assumps){ assumps.copyTo(assumptions); return solve_(); }
inline bool     Solver::okay          ()      const   { return ok; }

inline ClauseIterator Solver::clausesBegin() const { return ClauseIterator(ca, &clauses[0]); }
inline ClauseIterator Solver::clausesEnd  () const { return ClauseIterator(ca, &clauses[clauses.size()]); }
inline TrailIterator  Solver::trailBegin  () const { return TrailIterator(&trail[0]); }
inline TrailIterator  Solver::trailEnd    () const { 
    return TrailIterator(&trail[decisionLevel() == 0 ? trail.size() : trail_lim[0]]); }

inline void     Solver::toDimacs     (const char* file){ vec<Lit> as; toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p){ vec<Lit> as; as.push(p); toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p, Lit q){ vec<Lit> as; as.push(p); as.push(q); toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p, Lit q, Lit r){ vec<Lit> as; as.push(p); as.push(q); as.push(r); toDimacs(file, as); }


//=================================================================================================
// Debug etc:

//=================================================================================================
// Symmetry -- a class to represent a symmetry:

class Symmetry{
private:
	vec<Lit> sym;
	vec<Lit> inv;
	Solver* s;
	int id;
	vec<Lit> notifiedLits;
	int amountNeededForActive;
	int nextToPropagate;
	Lit reasonOfPermInactive;


public:


	Symmetry(Solver* solver, vec<Lit>& from, vec<Lit>& to, int id):
		s(solver),id(id){
		sym.growTo(s->nVars()*2);
		inv.growTo(s->nVars()*2);
		for(int i=sym.size()-1; i>=0; --i){
			sym[i]=toLit(i);
			inv[i]=toLit(i);
		}
		for(int i=0; i<from.size(); ++i){
			sym[toInt(from[i])]=to[i];
			inv[toInt(to[i])]=from[i];
		}
		amountNeededForActive=0;
		reasonOfPermInactive=lit_Undef;
		nextToPropagate=0;
	}

	void print(){
		printf("Symmetry: %i - neededForActive: %i\n",getId(),amountNeededForActive);
		for(int i=0; i<sym.size(); ++i){
			if(sym[i]!=toLit(i)){
				printf("%i->%i | ",i,toInt(sym[i]));
			}
		}printf("\n notifiedLits: ");
		for(int i=0; i<notifiedLits.size(); ++i){
			printf("%i:",toInt(notifiedLits[i]));
			s->testPrintValue(notifiedLits[i]);
			printf(" | ");
		}printf("\n");
		printf("amountNeededForActive: %i | firstNotPropagated: %i\n",amountNeededForActive,nextToPropagate);
	}

	int getId(){
		return id;
	}

	bool getSymmetricalClause(vec<Lit>& in_clause, vec<Lit>& out_clause){
		out_clause.clear();
		in_clause.copyTo(out_clause);
		bool changed = false;
		for(int i=in_clause.size()-1; i>=0; --i){
			if(getSymmetrical(in_clause[i])!=out_clause[i]){
				out_clause[i]=getSymmetrical(in_clause[i]);
				changed = true;
			}
		}
		return changed;
	}

	void getSymmetricalClause(Clause& in_clause, vec<Lit>& out_clause){
		out_clause.clear();
		out_clause.growTo(in_clause.size());
		for(int i=in_clause.size()-1; i>=0; --i){
			out_clause[i]=getSymmetrical(in_clause[i]);
		}
	}

	//	@pre:	in_clause is clause without true literals
	//	@post: 	out_clause is one of three options, depending on the number of unknown literals:
	//			all false literals with first most recent and second second recent
	//			first literal unknown, rest false and second most recent
	//			first two literals unknown
	void getSortedSymmetricalClause(Clause& in_clause, vec<Lit>& out_clause){
		assert(in_clause.size()>=2);
		int first=0;
		int second=1;
		out_clause.clear();
		out_clause.growTo(in_clause.size());
		out_clause[0]=getSymmetrical(in_clause[0]);
		for(int i=1; i<in_clause.size(); ++i){
			out_clause[i]=getSymmetrical(in_clause[i]);
			assert(s->value(out_clause[i])!=l_True);
			if(		 s->value(out_clause[first])!=l_Undef &&
					(s->value(out_clause[i])==l_Undef || s->hasLowerLevel(out_clause[first],out_clause[i])) ){
				second = first; first=i;
			}else if(s->value(out_clause[second])!=l_Undef &&
					(s->value(out_clause[i])==l_Undef || s->hasLowerLevel(out_clause[second],out_clause[i])) ){
				second = i;
			}
		}

		// note: swapping the final literals to their place is pretty tricky. Imagine for instance the case where first==0 or second==1 ;)
		if(first!=0){
			Lit temp = out_clause[0]; out_clause[0]=out_clause[first]; out_clause[first]=temp;
		}
		assert(second!=first);
		if(second==0){ second=first; }
		if(second!=1){
			Lit temp = out_clause[1];
			out_clause[1]=out_clause[second];
			out_clause[second]=temp;
		}
	}

	Lit getSymmetrical(Lit l){
		return sym[toInt(l)];
	}

	Lit getInverse(Lit l){
		return inv[toInt(l)];
	}

	Lit getNextToPropagate(){
		if(!isActive() && !s->inactivePropagationOptimization){
			return lit_Undef;
		}
		while( 	nextToPropagate<notifiedLits.size() &&
				(s->isDecision(notifiedLits[nextToPropagate]) ||
				 s->value(getSymmetrical(notifiedLits[nextToPropagate]))==l_True) ){
			++nextToPropagate;
		}
		if(nextToPropagate==notifiedLits.size()){
			return lit_Undef;
		}else if(isActive()){
			return notifiedLits[nextToPropagate];
		}else{
			assert(s->inactivePropagationOptimization);
			int nextToPropagateInactive = nextToPropagate;
			while( 	nextToPropagateInactive<notifiedLits.size() &&
					!canPropagate(notifiedLits[nextToPropagateInactive]) ){
				++nextToPropagateInactive;
			}
			if(nextToPropagateInactive==notifiedLits.size()){
				return lit_Undef;
			}else{
				return notifiedLits[nextToPropagateInactive];
			}
		}
	}

	bool canPropagate(Lit l){
		if(s->isDecision(l) || s->value(getSymmetrical(l))==l_True){
			return false;
		}
		if(s->level(var(l))==0){
			return true;
		}
		Clause& cl = s->ca[s->reason(var(l))];
		bool noUndefYet = true;
		for(int i=0; i<cl.size(); ++i){
			if(s->value(getSymmetrical(cl[i]))==l_True){
				return false;
			}else if(s->value(getSymmetrical(cl[i]))==l_Undef){
				if(noUndefYet){
					noUndefYet=false;
				}else{
					return false;
				}
			}
		}
		return true;
	}

	void notifyEnqueued(Lit l){
		assert(getSymmetrical(l)!=l);
		assert(s->value(l)==l_True);
		notifiedLits.push(l);
		if(isPermanentlyInactive()){
			return;
		}
		Lit inverse = getInverse(l);
		Lit symmetrical = getSymmetrical(l);
		if(s->isDecision(inverse)){
			if(s->value(inverse)==l_True){ //invar: value(l)==l_True
				--amountNeededForActive;
			}else{
				assert(s->value(inverse)==l_False);
				reasonOfPermInactive=l;
			}
		}
		if(s->isDecision(l)){
			if( s->value(symmetrical)==l_Undef ){
				++amountNeededForActive;
			}else if(s->value(symmetrical)==l_False){
				reasonOfPermInactive=l;
			}
			// else s->value(symmetrical)==l_True
		}
	}

	void notifyBacktrack(Lit l){
		assert(getSymmetrical(l)!=l);
		assert(s->value(var(l))!=l_Undef);
		assert(notifiedLits.size()>0 && notifiedLits.last()==l);
		notifiedLits.pop();
		nextToPropagate=0;
		if(isPermanentlyInactive()){
			if(reasonOfPermInactive==l){
				reasonOfPermInactive=lit_Undef;
			}else{
				return;
			}
		}
		if( s->isDecision(l) && s->value(getSymmetrical(l))==l_Undef ){
			--amountNeededForActive;
		}
		if( s->isDecision(getInverse(l)) && s->value(getInverse(l))==l_True){
			++amountNeededForActive;
		}
	}

	bool isActive(){
		return amountNeededForActive==0 && !isPermanentlyInactive(); // Laatste test is nodig voor phase change symmetries
	}

	bool isPermanentlyInactive(){
		return reasonOfPermInactive!=lit_Undef;
	}

	bool testIsActive(vec<Lit>& trail){
		std::set<Lit> trailCopy;
		for(int i=0; i<trail.size(); ++i){
			trailCopy.insert(trail[i]);
		}
		for(int i=0; i<trail.size(); ++i){
			Lit l = trail[i];
			if(s->isDecision(l)){
				if(!trailCopy.count(getSymmetrical(l))){
					return false;
				}
			}
		}
		return true;
	}

	bool testIsPermanentlyInactive(vec<Lit>& trail){
		std::set<Lit> trailCopy;
		for(int i=0; i<trail.size(); ++i){
			trailCopy.insert(trail[i]);
		}
		for(int i=0; i<trail.size(); ++i){
			Lit l = trail[i];
			if(s->isDecision(l)){
				if(trailCopy.count(~getSymmetrical(l))){
					return true;
				}
			}
		}
		return false;
	}
};


//=================================================================================================
}

#endif
