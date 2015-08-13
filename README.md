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