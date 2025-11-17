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
    consg->dump("res.dot");

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

    // ---- Step 1: 初始化：处理 AddrOut edges ----
    for (auto &nodePair : *consg)
    {
        unsigned src = nodePair.first;
        SVF::ConstraintNode *node = nodePair.second;

        for (auto edge : node->getAddrOutEdges())
        {
            if (auto addr = SVF::SVFUtil::dyn_cast<SVF::AddrCGEdge>(edge))
            {
                unsigned dst = addr->getDstID();
                if (pts[src].insert(dst).second)
                    worklist.push(src);
            }
        }
    }

    // ---- Step 2: Worklist propagation ----
    while (!worklist.empty())
    {
        unsigned n = worklist.pop();
        SVF::ConstraintNode *node = consg->getConstraintNode(n);

        // ---------- COPY edges ----------
        for (auto edge : node->getCopyOutEdges())
        {
            if (auto cp = SVF::SVFUtil::dyn_cast<SVF::CopyCGEdge>(edge))
            {
                unsigned dst = cp->getDstID();
                bool changed = false;
                for (unsigned p : pts[n])
                {
                    if (pts[dst].insert(p).second)
                        changed = true;
                }
                if (changed) worklist.push(dst);
            }
        }

        // ---------- LOAD edges : n --load--> x  means  x = *n ----------
        for (auto edge : node->getLoadOutEdges())
        {
            if (auto ld = SVF::SVFUtil::dyn_cast<SVF::LoadCGEdge>(edge))
            {
                unsigned x = ld->getDstID();
                bool changed = false;

                // For every pointee o ∈ pts[n], propagate pts[o] into pts[x]
                for (unsigned o : pts[n])
                {
                    for (unsigned p : pts[o])
                    {
                        if (pts[x].insert(p).second)
                            changed = true;
                    }
                }

                if (changed) worklist.push(x);
            }
        }

        // ---------- STORE edges : n --store--> m  means  *m = n ----------
        for (auto edge : node->getStoreOutEdges())
        {
            if (auto st = SVF::SVFUtil::dyn_cast<SVF::StoreCGEdge>(edge))
            {
                unsigned m = st->getDstID();
                bool changed = false;

                // For every o ∈ pts[m], propagate pts[n] → pts[o]
                for (unsigned o : pts[m])
                {
                    for (unsigned p : pts[n])
                    {
                        if (pts[o].insert(p).second)
                            changed = true;
                    }
                }

                if (changed)
                {
                    // push all affected 'o'
                    for (auto o : pts[m])
                        worklist.push(o);
                }
            }
        }

        // ---------- GEP edges: n --gep--> x ----------
        for (auto edge : node->getGepOutEdges())
        {
            if (auto gep = SVF::SVFUtil::dyn_cast<SVF::GepCGEdge>(edge))
            {
                unsigned x = gep->getDstID();
                bool changed = false;

                for (unsigned obj : pts[n])
                {
                    // 根据接口：getGepObjVar(对象ID, gepEdge)
                    unsigned fieldObj = consg->getGepObjVar(obj, gep);
                    if (pts[x].insert(fieldObj).second)
                        changed = true;
                }

                if (changed) worklist.push(x);
            }
        }
    }
}