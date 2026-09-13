# References

Lyraform is a research-oriented implementation project. This reading list
records intellectual provenance; it is not a claim that the project
reimplements or is endorsed by any cited work.

## Compiler construction and intermediate representation

- Chris Lattner and Vikram Adve, “LLVM: A Compilation Framework for Lifelong
  Program Analysis & Transformation,” 2004. [LLVM documentation](https://llvm.org/docs/)
  and [paper record](https://llvm.org/pubs/2004-01-30-CGO-LLVM.html).
- Alfred V. Aho, Monica S. Lam, Ravi Sethi, and Jeffrey D. Ullman, *Compilers:
  Principles, Techniques, and Tools*, 2nd ed., Pearson, 2006.

## Graphs, ordering, and structured computation

- Arthur B. Kahn, “Topological sorting of large networks,” *Communications of
  the ACM* 5(11), 1962, DOI
  [10.1145/368996.369025](https://doi.org/10.1145/368996.369025).
- Corrado Böhm and Giuseppe Jacopini, “Flow diagrams, Turing machines and
  languages with only two formation rules,” *Communications of the ACM* 9(5),
  1966, DOI
  [10.1145/355592.365646](https://doi.org/10.1145/355592.365646).
- Edsger W. Dijkstra, “Go To Statement Considered Harmful,” *Communications
  of the ACM* 11(3), 1968, DOI
  [10.1145/362929.362947](https://doi.org/10.1145/362929.362947).

## Types, effects, and explicit computation

- Robin Milner, “A Theory of Type Polymorphism in Programming,” *Journal of
  Computer and System Sciences* 17(3), 1978, DOI
  [10.1016/0022-0000(78)90014-4](https://doi.org/10.1016/0022-0000(78)90014-4).
- Eugenio Moggi, “Notions of Computation and Monads,” *Information and
  Computation* 93(1), 1991, DOI
  [10.1016/0890-5401(91)90052-4](https://doi.org/10.1016/0890-5401(91)90052-4).
- Michael J. Gordon, “The Denotational Description of Programming Languages,”
  Springer, 1979, DOI
  [10.1007/978-1-4684-9467-7](https://doi.org/10.1007/978-1-4684-9467-7).

## Provenance and governed systems

- James Cheney, Laura Chiticariu, and Wang-Chiew Tan, “Provenance in
  Databases: Why, How, and Where,” *Foundations and Trends in Databases* 1(4),
  2009, DOI
  [10.1561/1900000006](https://doi.org/10.1561/1900000006).
- Peter Buneman, Sanbhu K. Das, and Wang-Chiew Tan, “Why and Where: A
  Characterization of Data Provenance,” in *Database Theory — ICDT 2001*,
  DOI [10.1007/3-540-44503-X_20](https://doi.org/10.1007/3-540-44503-X_20).

## Project-adjacent standards and tools

- [LLVM language reference](https://llvm.org/docs/LangRef.html), for the
  backend IR boundary used by the current native path.
- [CMake documentation](https://cmake.org/documentation/) and
  [CTest documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html),
  for reproducible project configuration and verification.
- [POSIX.1-2017](https://pubs.opengroup.org/onlinepubs/9699919799/), for the
  host capability contracts exercised by selected native examples.

The repository does not distribute copyrighted PDFs. Links point to publisher,
DOI, standards, or project documentation pages.
