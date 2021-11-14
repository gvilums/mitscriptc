#include <vector>

#include "ir.h"

namespace IR {

class RegallocPass : public BasePass {

    std::vector<std::pair<size_t, size_t>> block_range;
    std::vector<std::vector<std::pair<size_t, size_t>>> machine_reg_ranges;
    
    
    

   public:
    virtual void apply_to(Program& prog) override;
    virtual ~RegallocPass() {};
};

};
