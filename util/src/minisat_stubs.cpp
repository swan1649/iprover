/*----------------------------------------------------------------------(C)-*/
/* Copyright (C) 2006-2010 Konstantin Korovin and The University of Manchester. 
   This file is part of iProver - a theorem prover for first-order logic.

   iProver is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   iProver is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
   See the GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with iProver.  If not, see <http://www.gnu.org/licenses/>.         */
/*----------------------------------------------------------------------[C]-*/

/*
  
  Created: 2011-12-07 Christoph Sticksel

 */

extern "C" {

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/custom.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

}

/* -D flags in MiniSat mtl/template.mk */
#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

/* includes from MiniSat simp/Main.cc */
#include <errno.h>

#include <signal.h>
#include <zlib.h>
#include <sys/resource.h>

#include <utils/System.h>
#include <utils/ParseUtils.h>
#include <utils/Options.h>
#include <core/Dimacs.h>
#include <simp/SimpSolver.h>

/* 'a option = None */
#define Val_none Val_int(0)

/* 'a option = Some of 'a */
static inline value Val_some( value v )
{   
    CAMLparam1 (v);
    CAMLlocal1 (some);
    some = caml_alloc(1, 0);
    Store_field (some, 0, v);
    CAMLreturn (some);
}

/* Switch to MiniSat namespace */
using namespace Minisat;

/* Custom OCaml operations for MiniSat literal 
   
 None of the default operations are defined. 

 TODO: think about defining some of them 
 - finalisation is not needed
 - comparing and hashing would be nice 
 - serialisation is not needed 

*/
static struct custom_operations minisat_lit_custom_ops = {
    identifier: "Minisat::Lit",
    finalize:    custom_finalize_default,
    compare:     custom_compare_default,
    hash:        custom_hash_default,
    serialize:   custom_serialize_default,
    deserialize: custom_deserialize_default
};

/* Copy a MiniSat literal into a newly allocated OCaml custom tag */
static inline value copy_minisat_lit( Lit *lit )
{
    CAMLparam0();
    CAMLlocal1(v);
    v = caml_alloc_custom( &minisat_lit_custom_ops, sizeof(Lit), 0, 1);
    memcpy( Data_custom_val(v), lit, sizeof(Lit) );
    CAMLreturn(v);
}


/* Create and return a MiniSat solver instance 

   external minisat_create_solver : unit -> minisat_solver = "minisat_create_solver" 

   The solver is created in the C++ heap, OCaml gets only a pointer in
   an Abstract_tag.

*/
extern "C" value minisat_create_solver(value unit)
{

  // Declare parameters 
  CAMLparam1 (unit);

  // Initialise MiniSat instance 
  Solver* s = new Solver;

  // Allocate abstract datatype for MiniSat instance 
  value res = caml_alloc(1, Abstract_tag);
  Store_field(res, 0, (value) s); 

  // Return MiniSat instance 
  CAMLreturn(res);

}

/* Add a variable to MiniSat

   external minisat_add_var : minisat_solver -> int -> unit = "minisat_add_var"

   Variables are integers, the first is 0. Integers do nat have to be
   allocated for OCaml.

   Each variable has to be allocated by calling newVar().
   minisat_create_lit does this on literal creation if the variable
   has not been allocated.

 */
extern "C" value minisat_add_var (value solver_in, value var_id_in)
{  

  // Declare parameters 
  CAMLparam2 (solver_in, var_id_in);
  Solver* solver = (Solver*) Field(solver_in, 0);
  int var_id = Int_val(var_id_in);

  // Declare variable in MiniSat
  while (var_id + 1 >= solver->nVars()) solver->newVar();

  // Return 
  CAMLreturn(Val_unit);

}

/* Create and return a literal of a variable 

   external minisat_create_lit : minisat_solver -> int -> bool -> minisat_lit = "minisat_create_lit" 

   Variables are integers, the first is 0. Use true for a positive
   literal and false for a negative one.

   A literal has to be created with the mkLit function, it is a custom
   datatype stored on the OCaml heap.

 */
extern "C" value minisat_create_lit(value solver_in, value sign_in, value var_id_in)
{
  
  // Declare parameters 
  CAMLparam3 (solver_in, sign_in, var_id_in);
  CAMLlocal1 (res);

  Solver* solver = (Solver*) Field(solver_in, 0);
  int var_id = Int_val(var_id_in);
  bool sign = Bool_val(sign_in);

  // First declare variable in MiniSat
  while (var_id >= solver->nVars()) solver->newVar();

  // Must use mkLit to create literals 
  Lit lit = sign ? mkLit(var_id) : ~mkLit(var_id);

#ifdef DEBUG
  printf("Created literal %d from %s%d\n", toInt(lit), sign ? "" : "~", var_id);
#endif

  // Allocate and copy MiniSat literal to OCaml
  res = copy_minisat_lit(&lit);

  // Return literal
  CAMLreturn(res);

}

/* Assert a clause given as a list of literals, return false if the
   clause set immediately becomes unsatisfiable, true otherwise.

   external minisat_add_clause : minisat_solver -> minisat_lit list -> bool = "minisat_add_clause" 

*/
extern "C" value minisat_add_clause(value solver_in, value clause_in)
{	

  // Declare parameters 
  CAMLparam2 (solver_in, clause_in);
  CAMLlocal1(head);

  Solver* solver = (Solver*) Field(solver_in, 0);
  head = clause_in;

  // Clause to be asserted
  vec<Lit> lits;

#ifdef DEBUG
  printf("Asserting clause ");
#endif

  // Iterate list of literals
  while (head != Val_emptylist) 
    {

      // Get head element of list 
      value lit_in = Field(head, 0);

      // Get MiniSat literal from value
      Lit* lit = (Lit*) Data_custom_val(lit_in);

#ifdef DEBUG
      printf("%d ", toInt(*lit));
#endif

      // Add literal to clause 
      lits.push(*lit);

      // Continue with tail of list
      head = Field(head, 1);

    }

#ifdef DEBUG
  printf("\n");
#endif

  // Add clause to solver
  if (solver->addClause(lits))
    {

      // Not immediately unsatisfiable 
      CAMLreturn (Val_true);

    }
  else
    {

#ifdef DEBUG
      printf("Unsatisfiable with added clause\n");
#endif

      // Immediately unsatisfiable with added clause
      CAMLreturn (Val_false);

    }

}


/* Test the given clause set for satisfiability. Return true if
   satisfiable, false if unsatisfiable.

   external minisat_solve : minisat_solver -> bool = "minisat_solve" 

*/
extern "C" value minisat_solve(value solver_in)
{
    
  // Declare parameters 
  CAMLparam1(solver_in);
  Solver* solver = (Solver*) Field(solver_in, 0);

#ifdef DEBUG
  printf("Solving without assumptions\n");
#endif

  // Run MiniSat
  int res = solver->solve();

  // Return result
  CAMLreturn(Val_bool(res));
}


/* Test the given clause set for satisfiability when the given
   literals are to be made true. Return l_True = 0 if the clause set
   is satisfiable with assumptions, l_Undef = 2 if the clause set is
   immediately unsatisfiable without assumptions and l_False = 1 if
   the clause set is unsatisfiable with assumptions.

   external minisat_solve_assumptions : minisat_solver -> minisat_lit list -> lbool = "minisat_solve_assumptions" 

*/
extern "C" value minisat_solve_assumptions(value solver_in, value assumptions_in)
{

  // Declare parameters 
  CAMLparam2 (solver_in, assumptions_in);
  CAMLlocal1(head);

  Solver* solver = (Solver*) Field(solver_in, 0);
  head = assumptions_in;

  // Assumptions for solving
  vec<Lit> lits;
  lits.clear();

  // Only if satisfiable after simplifications
  if (solver->simplify())
    {

#ifdef DEBUG
      printf("Assuming ");
#endif

      // Iterate list of literals
      while (head != Val_emptylist) 
	{
	  
	  // Get head element of list 
	  value lit_in = Field(head, 0);
	  
	  // Get MiniSat literal from value
	  Lit* lit = (Lit*) Data_custom_val(lit_in);
	  
#ifdef DEBUG
	  printf("%s%d ", 
		 var(*lit) ? "" : "~",
		 toInt(*lit));
#endif
	  
	  // Add literal to assumptions
	  lits.push(*lit);
	  
	  // Continue with tail of list
	  head = Field(head, 1);
	  
	}
      
#ifdef DEBUG
      printf("\n");
#endif

      // Solve with literal assumptions
      if (solver->solve(lits))
	{
	  
#ifdef DEBUG
	  printf("Satisfiable under assumptions\n");
#endif

	  // Satisfiable under assumptions
	  CAMLreturn(Val_int(toInt(l_True)));

	}

      else
	{

#ifdef DEBUG
	  printf("Unsatisfiable under assumptions\n");
#endif

	  // Unsatisfiable under assumptions
	  CAMLreturn(Val_int(toInt(l_False)));

	}

    }

  else  
    {

#ifdef DEBUG
      printf("Unsatisfiable without assumptions\n");
#endif

      // Unsatisfiable without assumptions
      CAMLreturn(Val_int(toInt(l_Undef)));
    }
	
}

/* Test the given clause set for satisfiability in a limited search
   when the given literals are to be made true.

   This is similar to minisat_solve_assumptions above, but the search
   is limited to the given number of conflicts. 

   Return None if satisfiability could not be determined under the
   conflict limit. Return Some l_True = Some 0 if the clause set is
   satisfiable with assumptions, Some l_Undef = Some 2 if the clause
   set is immediately unsatisfiable without assumptions and Some
   l_False = Some 1 if the clause set is unsatisfiable with
   assumptions.

   external minisat_fast_solve : minisat_solver -> minisat_lit list -> int -> lbool option = "minisat_fast_solve"

*/
extern "C" value minisat_fast_solve(value solver_in, value assumptions_in, value max_conflicts_in)
{

  // Declare parameters 
  CAMLparam3 (solver_in, assumptions_in, max_conflicts_in);
  CAMLlocal1(head);

  Solver* solver = (Solver*) Field(solver_in, 0);
  int max_conflicts = Int_val(max_conflicts_in);

  head = assumptions_in;

  // Assumptions for solving
  vec<Lit> lits;
  lits.clear();

  // Only if satisfiable after simplifications
  if (solver->simplify())
    {

#ifdef DEBUG
      printf("Assuming ");
#endif

      // Iterate list of literals
      while (head != Val_emptylist) 
	{
	  
	  // Get head element of list 
	  value lit_in = Field(head, 0);
	  
	  // Get MiniSat literal from value
	  Lit* lit = (Lit*) Data_custom_val(lit_in);
	  
#ifdef DEBUG
	  printf("%s%d ", 
		 sign(*lit) ? "" : "~",
		 var(*lit));
#endif
	  
	  // Add literal to assumptions
	  lits.push(*lit);
	  
	  // Continue with tail of list
	  head = Field(head, 1);
	  
	}
      
#ifdef DEBUG
      printf("\n");

      if (!lits.size()) printf("No assumptions\n");
#endif

      // Set budget for number of conflicts
      solver->setConfBudget(max_conflicts);

      // Solve with literal assumptions 
      lbool res = solver->solveLimited(lits);

      if (res == l_True) 
	{
#ifdef DEBUG
	  printf("Satisfiable with assumptions (fast solve)\n");
#endif

	  CAMLreturn(Val_some(Val_int(toInt(l_True))));
	}

      if (res == l_False) 
	{
#ifdef DEBUG
	  printf("Unsatisfiable with assumptions (fast solve)\n");
#endif

	  CAMLreturn(Val_some(Val_int(toInt(l_True))));
	}

      if (res == l_Undef) 
	{
#ifdef DEBUG
	  printf("Unknown (fast solve)\n");
#endif

	  CAMLreturn(Val_none);
	}
      
    }

  else
    {

#ifdef DEBUG
      printf("Unsatisfiable without assumptions (fast solve)\n");
#endif

      // Unsatisfiable without assumptions
      CAMLreturn(Val_some(Val_int(toInt(l_Undef))));
      
    }

}


/* Return the truth value of the literal in the current model: Some
    true if the literal is true, Some false if the literal is false
    and None if the literal value is undefined

  external minisat_model_value : minisat_solver -> minisat_lit -> int = "minisat_model_value"

*/
extern "C" value minisat_model_value (value solver_in, value lit_in)
{

  // Declare parameters 
  CAMLparam2 (solver_in, lit_in);
  Solver* solver = (Solver*) Field(solver_in, 0);
  Lit* lit = (Lit*) Data_custom_val(lit_in);

  lbool val = solver->modelValue(*lit);

#ifdef DEBUG
  printf ("Model value %s%d: %s (%d)\n", 
	  sign(*lit) ? "" : "~",
	  var(*lit),
	  val == l_True ? "l_True" : (val == l_False ? "l_False" : "l_Undef"),
	  val);
#endif

  if (val == l_True) 
    { 
      CAMLreturn(Val_int(toInt(l_True)));
    }
  else if (val == l_False) 
    { 
      CAMLreturn(Val_int(toInt(l_False)));
    }
  else
    {
      CAMLreturn(Val_int(toInt(l_Undef)));
    }
  
}


/* Return the propositional variable in the literal

   external minisat_lit_var : minisat_solver -> minisat_lit -> int = "minisat_lit_to_int"

*/
extern "C" value minisat_lit_var(value solver_in, value lit_in)
{

  // Declare parameters 
  CAMLparam2 (solver_in, lit_in);
  Solver* solver = (Solver*) Field(solver_in, 0);
  Lit* lit = (Lit*) Data_custom_val(lit_in);
  
  value res = Val_int(var(*lit));
  CAMLreturn(res);

}


/* Return the sign of the literal, true for a positive and false
   for a negative literal 
   
   external minisat_lit_sign : minisat_solver -> minisat_lit -> bool = "minisat_lit_to_int"
    
*/
extern "C" value minisat_lit_sign(value solver_in, value lit_in)
{

  // Declare parameters 
  CAMLparam2 (solver_in, lit_in);
  Solver* solver = (Solver*) Field(solver_in, 0);
  Lit* lit = (Lit*) Data_custom_val(lit_in);
  
  value res = Val_bool(sign(*lit));
  CAMLreturn(res);

}


/* Return the number of propositional variables

  external minisat_stat_vars : minisat_solver -> int = "minisat_stat_vars" 

*/
extern "C" value minisat_stat_vars(value solver_in)
{

  // Declare parameters 
  CAMLparam1 (solver_in);
  Solver* solver = (Solver*) Field(solver_in, 0);

  // Read number of variables 
  int vars = solver->nVars();

  // Return integer
  value res = Val_int(vars);
  CAMLreturn(res);

}


/* Return the number of clauses
  
  external minisat_stat_clauses : minisat_solver -> int = "minisat_stat_clauses" 
*/
extern "C" value minisat_stat_clauses(value solver_in)
{

  // Declare parameters 
  CAMLparam1 (solver_in);
  Solver* solver = (Solver*) Field(solver_in, 0);

  // Read number of clauses 
  int vars = solver->nClauses();

  // Return integer
  value res = Val_int(vars);
  CAMLreturn(res);

}