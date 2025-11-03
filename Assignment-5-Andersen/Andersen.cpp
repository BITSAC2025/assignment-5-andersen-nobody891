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
        using namespace SVF;

    WorkList<NodeID> worklist;

    /// Step 1. 初始化：为空的 pts，并把所有节点入队
    for (auto it = consg->begin(); it != consg->end(); ++it)
    {
        NodeID id = it->first;
        pts[id] = {}; // 空集合
        worklist.push(id);
    }

    /// Step 2. 主循环
    while (!worklist.empty())
    {
        NodeID p = worklist.pop();
        ConstraintNode* node = consg->getConstraintNode(p);
        bool changed = false;

        /// ========== Address Rule ==========
        for (auto edge : node->getAddrOutEdges())
        {
            if (auto addrEdge = SVFUtil::dyn_cast<AddrCGEdge>(edge))
            {
                NodeID x = addrEdge->getDstID();
                if (pts[p].insert(x).second)
                    changed = true;
            }
        }

        if (changed)
        {
            worklist.push(p);
            changed = false;
        }

        /// ========== Copy Rule ==========
        for (auto edge : node->getCopyOutEdges())
        {
            if (auto copyEdge = SVFUtil::dyn_cast<CopyCGEdge>(edge))
            {
                NodeID q = copyEdge->getDstID();
                for (auto obj : pts[q])
                {
                    if (pts[p].insert(obj).second)
                        changed = true;
                }
            }
        }

        if (changed)
        {
            worklist.push(p);
            changed = false;
        }

        /// ========== Load Rule ==========
        for (auto edge : node->getLoadOutEdges())
        {
            if (auto loadEdge = SVFUtil::dyn_cast<LoadCGEdge>(edge))
            {
                NodeID q = loadEdge->getDstID();
                for (auto x : pts[q])
                {
                    ConstraintNode* xNode = consg->getConstraintNode(x);
                    if (!xNode) continue;
                    for (auto obj : pts[x])
                    {
                        if (pts[p].insert(obj).second)
                            changed = true;
                    }
                }
            }
        }

        if (changed)
        {
            worklist.push(p);
            changed = false;
        }

        /// ========== Store Rule ==========
        for (auto edge : node->getStoreOutEdges())
        {
            if (auto storeEdge = SVFUtil::dyn_cast<StoreCGEdge>(edge))
            {
                NodeID q = storeEdge->getDstID();
                for (auto x : pts[p])
                {
                    for (auto obj : pts[q])
                    {
                        if (pts[x].insert(obj).second)
                            changed = true;
                    }
                    if (changed)
                    {
                        worklist.push(x);
                        changed = false;
                    }
                }
            }
        }

        /// ========== Gep Rule ==========
        /// 对于结构体成员访问：p = q + offset
        /// ========== Gep Rule ==========
        for (auto edge : node->getGepOutEdges())
        {
            if (auto gepEdge = SVFUtil::dyn_cast<GepCGEdge>(edge))
            {
                NodeID q = gepEdge->getDstID();

                for (auto obj : pts[q])
                {
                    NodeID fieldObj = 0;

                    if (auto normalGep = SVFUtil::dyn_cast<NormalGepCGEdge>(gepEdge))
                    {
                        const SVF::APOffset &offset = normalGep->getConstantFieldIdx();
                        fieldObj = consg->getGepObjVar(obj, offset);
                    }
                    else
                    {
                        // Variant GEP：无固定偏移，可忽略
                        continue;
                    }

                    if (pts[p].insert(fieldObj).second)
                        changed = true;
                }
            }
        }

        if (changed)
            worklist.push(p);
    }
}