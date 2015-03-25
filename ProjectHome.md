**New version iProver v0.8.1 (post CASC-J5, 2010) is released!!!**

iProver is an automated reasoning system for first-order logic.


---

Easy to install: 1) ./configure 2)  make

---


We assume OCaml v3.10 >= is installed.



---

Easy to use:  iproveropt problem.p

---



where problem.p is a problem in the TPTP format:
http://www.tptp.org

iproveropt --help for more options


---

iProver **won** the EPR (effectively propositional) division at
CASC-J5 2010, CASC-22(2009) and CASC-J4 (2008).

---


iProver based on an instantiation calculus,
which is complete for first-order logic.
One of the distinctive features of iProver
is a modular combination of first-order reasoning with ground reasoning.
In particular, iProver currently integrates MiniSat
for reasoning with ground abstractions of first-order clauses.
iProver combines instantiation with resolution and implements a number of redundancy elimination methods.

For more details:
http://www.cs.man.ac.uk/~korovink/iprover/


Please, send your comments, suggestions and bug reports to korovin at cs.man.ac.uk

If you require a different license please contact korovin at cs.man.ac.uk

To receive (infrequent) announcements about major releases you can subscribe to
iProver google group:  http://groups.google.com/group/iprover/