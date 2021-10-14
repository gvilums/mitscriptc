#include "types.h"
#include <string>
#include <memory>

class VM {
private:

public:
    VM(Function prog);
    void exec();
    void step();
};