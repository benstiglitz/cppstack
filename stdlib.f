( bootstrapping )
";" ( word def -- ) { dp @ rot 1 pick ! 4 + 0 pick rot swap ! 4 + dp ! } dp @ rot 1 pick ! 4 + 0 pick rot swap ! 4 + dp !

( "r<" { r> swap 1 swap r<? 1 swap r<? } ; )
"call" { 4 - r< } ;
"call?" { 4 - r<? } ;
"dup" ( n -- n n ) { 0 pick } ;
"over" ( n m -- n m n ) { 1 pick } ;
"if" ( n {} {} -- ) { rot { swap } call? swap drop call } ;
"while" ( {} {} -- ) { r< dup r< call r> swap r> rot r< dup r< swap { call r> 56 - r> r> swap rot r< } call? drop r> r> drop drop } ;
"<" ( n m -- n<m ) { swap > } ;
"<=" ( n m -- n<=m ) { > not } ;
">=" ( n m -- n>=m ) { < not } ;
"1+" ( n -- n+1 ) { 1 + } ;
"1-" ( n -- n-1 ) { 1 - } ;
"times" ( n {} -- { } -- ) { r< r< { r> r@ 0 > swap r< } { r> r> r> rot r< swap r< dup r< call r> r> 1- r> rot r< swap r< r< } while r> r> drop drop } ;
"upto" ( n m {} -- {n..m} -- ) { r< over - { r> r@ swap r< call } times r> drop } ;
"do" ( {} {} -- ) { dup call while } ;
"." ( n -- ) { print } ;
"print-stack" ( ... -- ) { sbase @ sp @ { @ print 32 emit } upto } ;
"/" ( n m -- quo ) { remquo swap drop } ;
"%" ( n m -- rem ) { remquo drop } ;
"=" ( n m -- eql? ) { over over < { drop drop 0 } { > { 0 } { 1 } if } if } ;
"<>" ( n m -- !eql? ) { = not } ;
"and" ( n m -- n && m ) { 0 <> swap 0 <> & } ;
"or" ( n m -- n || m ) { 0 <> swap 0 <> | } ;
( : fib dup 0 = { 0 drop } { dup 1 = { 1 drop } { dup 1 - fib swap 2 - fib + } if } if ; add recursion support )

"toggle" { dup @ not swap ! } ;

( stack operations )
"cell" ( -- cellsize ) { 4 } ;
"cells" ( n -- m ) { cell * } ; 
"s:depth" ( ... -- ... count ) { sp @ sbase @ - cell / } ;
"s:each" ( ... {} -- ... ) { sbase @ sp @ 8 - rot { rot rot over over < } { rot dup 3 pick swap call rot cell + rot rot } while drop drop drop } ;
"s:print" ( ... -- ... ) { { @ print 32 emit } s:each } ;
"s:pick" ( ... -- ... ...[-n] ) { 2 + cell * sp @ swap - @ } ;
"s:drop" ( ... -- ) { sbase @ sp ! } ;

( string operations )
".s" ( "" -- ) { { dup c@ dup 0 <> } { emit 1+ } while drop drop } ;
"string-length" ( "" -- len ) {  0 { over c@ 0 <> } { 1+ swap 1+ swap } while swap drop } ;
"s="  { { over over c@ 0 <> swap c@ 0 <> and { over over c@ swap c@ = { 1 } { 0 0 } if } { over over c@ 0 = swap c@ 0 = and 0 } if } { 1+ swap 1+ swap } while rot rot drop drop } ;
"cr" ( -- ) { 10 emit } ;

( debugging )
"d:to-constant" { 4 * 1 + } ;
"d:halt" { 0 r< } ;
"d:continue" { r> drop r> drop } ;
"d:break" ( addr -- ) { { "Hit breakpoint at " .s r@ . d:halt r> drop 0 0 pc ! } swap over over @ swap 7 cells + ! over over d:to-constant swap 8 cells + ! ! } ;

( memory )
"here" { heap @ } ;
"allot" { here + heap ! } ;
"bell" { 7 emit } ;
"read-line" { here { key dup 13 <> } { dup 127 = { 8 32 8 emit emit emit drop dup here <> { -1 allot } { bell } if } { dup emit here ! 1 allot } if } while drop 0 here ! 1 allot } ;

( dictionary )
"entry" ( -- entrysize ) { 8 } ;
"entries" ( -- entrysize ) { entry * } ;
"d:dict" { dbase @ dp @ { over over < } { entry - dup @ .s cr } while drop drop } ;

( initialization )
{ "signal " .s . 0 r< } interrupt-handler !
