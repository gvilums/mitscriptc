
#include "bc-parser.h"
#include "bc-lexer.h"
#include <iostream>
#include "prettyprinter.h"

using namespace std;

int main(int argc, char** argv)
{
  void* scanner;
  bclex_init(&scanner);
  bcset_in(stdin, scanner);
  
  Function* output;
  int rvalue = bcparse(scanner, output);
  if(rvalue == 1){

	  cout<<"Parsing failed"<<endl;
	  return 1;
  }

  PrettyPrinter printer;

  printer.print(*output, std::cout);

  return 0;
}
