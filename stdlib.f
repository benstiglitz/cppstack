: dup ( n -- n n ) 0 pick ;
: over ( n m -- n m n ) 1 pick ;
: < ( n m -- n<m ) swap > ;
: <= ( n m -- n<=m ) > not ;
: >= ( n m -- n>=m ) < not ;
: 1+ ( n -- n+1 ) 1 + ;
: 1- ( n -- n-1 ) 1 - ;
: times ( n {} -- { } -- ) r< r< { r> r@ 0 > swap r< } { r> r> r> rot r< swap r< dup r< call r> r> 1- r> rot r< swap r< r< } while r> r> drop drop ;
: upto ( n m {} -- {n..m} -- ) r< over - { r> r@ swap r< call } times r> drop ;
: do dup call while ;
: . print ;
: print-stack sbase sp { @ print } upto ;
: / divmod drop ;
: % divmod swap drop ;
: = over over < { drop drop 0 } { > { 0 } { 1 } if } if ;
: <> = not ;
( : fib dup 0 = { 0 drop } { dup 1 = { 1 drop } { dup 1 - fib swap 2 - fib + } if } if ; add recursion support )

( stack operations )
: cell 4 ;
: cells cell * ; 
: s:depth sp sbase - cell / ;
: s:each sbase sp 8 - rot { rot rot over over < } { rot dup 3 pick swap call rot cell + rot rot } while drop drop drop ;
: s:print { @ print } s:each ;
: s:pick 2 + cell * sp swap - @ ;
: s:drop s:depth { drop } times ;

( string operations )
: .s print-string ;
: string-length 0 { over @c 0 <> } { 1+ swap 1+ swap } while swap drop ;
