# Space-Efficient B Trees via Load-Balancing

Implementation of the B tree variant explained in [1].
An example execution is written in `main.cpp`. The same execution is done with `std::set` in `stdset.cpp`.
We also have an additional variant called `expand` implemented, which works better on smaller input sizes.

In the executable `main.cpp`, the following parameters can be set.

- `q` : number of siblings considered for key shifting
- `t` : capacity of internal nodes except for marginal nodes
- `t'` : capacity of marginal nodes
- `b` : number of keys a leaf can store

An automatic benchmark is given in `evaluate.sh`.
The produced output can be interpreted with [https://github.com/koeppl/sqlplot](sqlplot). 



[1] Tomohiro I, Dominik Köppl:
Space-Efficient B Trees via Load-Balancing.  Proc. IWOCA, LNCS 13270, pages 327–340. (2022)
