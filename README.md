csv is a tool that manipulates csv data and outputs the results to stdout.

to build: g++ -std=c++11 main.cpp -o csv

NOTE:

Tested with:

-Apple LLVM version 6.1.0 (clang-602.0.49) (based on LLVM 3.6.0svn)
-Target: x86_64-apple-darwin14.1.0


usage: cat file.csv | ./csv <options>
 where <options> may include any/all of: 
       -cols n_1 n_2 n_3 n_4 (where n_{1,2,3,4} are valid column indices)
       -addcol col_expr where col_expr is an expression consisting of:
                             -colN where N is the column index
                             -N where N is some integer
                             -OP where OP is a binary operator from {+, *, -, /}
       -aggcol N {avg,med,min,max} where N is a valid column index 
                             avg - get average over column N
                             med - get the median over column N
                             min - get the min over column N
                             max - get the max over column N
       -innerjoin, -outerjoin N file.csv where N is a column index and file.csv is a csv file
                             (inner or outer) join the rows from stdin with the rows in file.csv.
                             Joined rows' columns can be referenced in -aggcol,-addcol,and -cols expressions.

NOTE: This command line tool expects csv input from stdin, so this documentation expects that you are running it
on a UNIX-like system where 'a | b'  means send std out from 'a' to stdin of 'b'.

examples:

simplest example

$ cat small_data.csv | ./csv
| 31 |  24 | 81 | 94 |
| 22 |  46 | 35 | 57 |
| 80 |   8 | 39 | 92 |
| 65 |  67 | 57 | 46 |
| 78 |  76 |  5 | 63 |
| 31 |  68 | 95 | 29 |
| 31 |  42 | 27 | 22 |
| 12 |  41 | 74 | 91 |
| 12 |  71 | 13 | 63 |
...

here's a projection

$ cat small_data.csv | ./csv -cols 1 3
|  24 | 94 |
|  46 | 57 |
|   8 | 92 |
|  67 | 46 |
|  76 | 63 |
|  68 | 29 |
|  42 | 22 |
|  41 | 91 |
|  71 | 63 |
...

here's a column expression

$ cat small_data.csv | ./csv -addcol col1+col2
| 31 |  24 | 81 | 94 | 105 | 105 |
| 22 |  46 | 35 | 57 |  81 |  81 |
| 80 |   8 | 39 | 92 |  47 |  47 |
| 65 |  67 | 57 | 46 | 124 | 124 |
| 78 |  76 |  5 | 63 |  81 |  81 |
| 31 |  68 | 95 | 29 | 163 | 163 |
| 31 |  42 | 27 | 22 |  69 |  69 |
| 12 |  41 | 74 | 91 | 115 | 115 |
| 12 |  71 | 13 | 63 |  84 |  84 |
| 55 | 100 | 47 | 96 | 147 | 147 |
...

Here we redirect to stderr to /dev/null because warnings are emitted if a null column is used in an expression

$ cat small_data.csv | ./csv -outerjoin 1 more_small_data.csv -addcol col1+col2 2>/dev/null
| 31 |  24 | 81 | 94 | 105 |105 |
| 22 |  46 | 35 | 57 |  81 | 70 |  46 | 99 |  33 |  81 |
| 22 |  46 | 35 | 57 |  81 | 63 |  46 | 68 |  52 |  81 |
| 22 |  46 | 35 | 57 |  81 | 84 |  46 | 17 |  83 |  81 |
| 80 |   8 | 39 | 92 |  47 | 13 |   8 | 39 |  14 |  47 |
| 65 |  67 | 57 | 46 | 124 |  9 |  67 | 13 |   9 | 124 |
| 65 |  67 | 57 | 46 | 124 | 47 |  67 | 16 |  24 | 124 |
| 65 |  67 | 57 | 46 | 124 | 28 |  67 | 32 |  27 | 124 |
| 78 |  76 |  5 | 63 |  81 | 75 |  76 | 38 |  87 |  81 |
| 78 |  76 |  5 | 63 |  81 | 89 |  76 |  5 |  97 |  81 |
| 78 |  76 |  5 | 63 |  81 | 41 |  76 | 31 |  23 |  81 |
| 31 |  68 | 95 | 29 | 163 |163 |
| 31 |  42 | 27 | 22 |  69 | 96 |  42 |  7 |  27 |  69 |
| 31 |  42 | 27 | 22 |  69 |  4 |  42 | 75 |  48 |  69 |
| 31 |  42 | 27 | 22 |  69 | 97 |  42 | 19 |  59 |  69 |
| 31 |  42 | 27 | 22 |  69 |  9 |  42 | 31 |  22 |  69 |
| 12 |  41 | 74 | 91 | 115 |  2 |  41 | 54 |  81 | 115 |
| 12 |  41 | 74 | 91 | 115 | 85 |  41 | 76 |  50 | 115 |
| 12 |  41 | 74 | 91 | 115 | 11 |  41 | 18 |  43 | 115 |
| 12 |  71 | 13 | 63 |  84 | 84 |
...
|    |     |    |    |     | 63 |  46 | 68 |  52 | <------------ NULL columns 1 and 2
|    |     |    |    |     | 85 |  41 | 76 |  50 |
|    |     |    |    |     | 59 |  64 | 55 |   6 |
|    |     |    |    |     | 47 |  67 | 16 |  24 |
|    |     |    |    |     |  8 |  34 | 55 |  36 |
|    |     |    |    |     | 38 |  21 | 45 |  39 |
|    |     |    |    |     | 75 |  36 | 29 |  96 |
|    |     |    |    |     | 22 |  98 | 22 |  87 |
|    |     |    |    |     | 28 |  95 | 37 |  75 |
|    |     |    |    |     | 94 |  52 | 81 |   3 |
|    |     |    |    |     | 93 |  56 | 66 |  90 |
|    |     |    |    |     | 46 |  50 | 41 |   1 |
...

And finally an aggregate:

$ cat small_data.csv | ./csv -aggcol avg 9 -outerjoin 1 more_small_data.csv -addcol col1+col2
|    |     |    |    |     | 28 |  38 | 30 |  64 |
|    |     |    |    |     | 65 |  14 | 70 |  61 |
|    |     |    |    |     | 41 |  76 | 31 |  23 |
|    |     |    |    |     | 69 |  98 | 42 |  89 |
|    |     |    |    |     | 14 |  15 | 89 |  36 |
avg: 91
