Failing Tests:

private:
- test12-manyArgs.mit
  - fails with segfault
- test13-exprAsArg.mit
  - fails with division by zero, although no division by zero should occur
- test20-mapReduce.mit
  - infinite loop
- test21-list.mit
  - infinite loop
- test31-usingGlobal.mit
  - memory corruption