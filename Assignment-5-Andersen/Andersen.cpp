/**
 * Andersen.cpp
 * @author kisslune
 */

#include "A5Header.h"

using namespace llvm;
using namespace std;

int main(int argc, char** argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    SVF::LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVF::SVFIRBuilder builder;
    auto pag = builder.build();
    auto consg = new SVF::ConstraintGraph(pag);
    consg->dump();

    Andersen andersen(consg);

    // TODO: complete the following method
    andersen.runPointerAnalysis();

    andersen.dumpResult();
    SVF::LLVMModuleSet::releaseLLVMModuleSet();
	return 0;
}


void Andersen::runPointerAnalysis()
{
    // TODO: complete this method. Point-to set and worklist are defined in A5Header.h
    //  The implementation of constraint graph is provided in the SVF library
    WorkList<unsigned> worklist;

    // 初始化所有节点
    for (auto it = consg->begin(); it != consg->end(); ++it)
    {
        unsigned id = it->first;
        worklist.push(id);
        pts[id];  // 确保有条目
    }

    while (!worklist.empty())
    {
        unsigned src = worklist.pop();

        SVF::ConstraintNode* node = consg->getConstraintNode(src);
        if (!node) continue;

        for (auto eit = node->OutEdgeBegin(); eit != node->OutEdgeEnd(); ++eit)
        {
            SVF::ConstraintEdge* edge = *eit;

            // ---------- Copy -----------
            if (auto cpy = SVF::SVFUtil::dyn_cast<SVF::CopyCGEdge>(edge))
            {
                unsigned dst = cpy->getDstID();
                bool changed = false;

                for (unsigned obj : pts[src])
                {
                    if (pts[dst].insert(obj).second)
                        changed = true;
                }

                if (changed)
                    worklist.push(dst);
            }

            // ---------- Gep -----------
            else if (auto gep = SVF::SVFUtil::dyn_cast<SVF::GepCGEdge>(edge))
            {
                unsigned y = gep->getSrcID();
                unsigned x = gep->getDstID();
                bool changed = false;

                for (unsigned obj : pts[y])
                {
                    unsigned fld = consg->getGepObjVar(obj, gep);
                    if (pts[x].insert(fld).second)
                        changed = true;
                }

                if (changed)
                    worklist.push(x);
            }
        }
    }
}