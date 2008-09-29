: dup 0 pick ;
: over 1 pick ;
: < swap > ;
: <= > not ;
: >= < not ;
: 1+ 1 + ;
: upto { rot rot over over <= } { rot dup 3 pick swap call rot 1+ rot rot } while drop drop drop ;
: do dup call while ;
: p print ;
: print-stack sbase sp { print } upto ;

