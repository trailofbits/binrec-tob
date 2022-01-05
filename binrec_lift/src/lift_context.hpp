#ifndef BINREC_LIFT_CONTEXT_HPP
#define BINREC_LIFT_CONTEXT_HPP

#include <llvm/Passes/PassBuilder.h>

namespace binrec {

class LiftContext {
public:
    bool link_prep_1;
    bool link_prep_2;
    bool clean;
    bool lift;
    bool optimize;
    bool optimize_better;
    bool compile;
    bool skip_link;
    bool clean_names;
    bool trace_calls;
    std::string trace_filename;
    std::string destination;

    LiftContext() : link_prep_1{false}, link_prep_2{false}, clean{false}, lift{false},
                    optimize{false}, optimize_better{false}, compile{false},
                    skip_link{false}, clean_names{false}, trace_calls(false),
                    trace_filename{}, destination{} {
    }

    LiftContext(const LiftContext &) = delete;
    LiftContext(LiftContext &&) = delete;
    auto operator=(const LiftContext &) -> LiftContext & = delete;
    auto operator=(LiftContext &&) -> LiftContext & = delete;
};

}

#endif
