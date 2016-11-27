// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "multiConstantPropagation.h"

#include "boost/bind.hpp"
#include "boost/mem_fn.hpp"

#include <set>

using namespace std;

int multiConstantPropagationAnalysisDebugLevel = 2;


MultiConstantPropagationLattice::MultiConstantPropagationLattice()
{
    this->values = set<int>();
    this->value = 0;
    this->level = bottom;
}

MultiConstantPropagationLattice::MultiConstantPropagationLattice( int v )
{
    this->values = set<int>();
    this->values.insert(v);
    this->value = v;
    this->level = oneValue;
}

MultiConstantPropagationLattice::MultiConstantPropagationLattice (const MultiConstantPropagationLattice & X)
{
    this->values = X.values;
    this->value = X.value;
    this->level = X.level;
}

int MultiConstantPropagationLattice::getValue() const
{
    return value;
}

set<int> MultiConstantPropagationLattice::getValues() const
{
    return values;
}

short MultiConstantPropagationLattice::getLevel() const
{
    return level;
}

bool MultiConstantPropagationLattice::isConstant() const
{
    return level > bottom && level < top;
}

bool MultiConstantPropagationLattice::setValue(int x)
{
    bool modified = this->level != oneValue || this->values.count(x) != 1;
    this->value = x;
    this->values.clear();
    this->values.insert(x);
    level = oneValue;
    return modified;
}

bool MultiConstantPropagationLattice::capValues(int x)
{
    set<int> cappedValues;
    for(int i : this->values){
        if(i <= x){
            cappedValues.insert(i);
        }
    }
    return this->setValues(cappedValues);
}

bool MultiConstantPropagationLattice::capBelowValues(int x)
{
    set<int> cappedValues;
    for(int i : this->values){
        if( i >= x){
            cappedValues.insert(i);
        }
    }
    return this->setValues(cappedValues);
}

bool MultiConstantPropagationLattice::setValues(set<int> x)
{
    if(x.size() == this->values.size()){
        bool equal = true;
        for(int i : x){
            equal = equal && (this->values.count(i) == 1);
        }
        if(equal){
            return false;
        }
        else{
            this->values = x;
            this->level = values.size();
            return true;
        }
    }
    else{
        this->values = x;
        this->level = values.size();
        return true;
    }
}

bool MultiConstantPropagationLattice::addValue(int x)
{
    auto r = this->values.insert(x);

    if(r.second){
        level++;
        return true;
    }
    return false;
}

bool MultiConstantPropagationLattice::setLevel(short x)
{
    bool modified = this->level != x;
    level = x;
    return modified;
}

bool MultiConstantPropagationLattice::setBottom()
{
    bool modified = this->level != bottom;
    this->value = 0;
    level = bottom;
    return modified;
}

bool MultiConstantPropagationLattice::setTop()
{
    bool modified = this->level != bottom;
    this->value = 0;
    level = top;
    return modified;
}

void MultiConstantPropagationLattice::initialize()
{
}


// returns a copy of this lattice
Lattice* MultiConstantPropagationLattice::copy() const
{
    return new MultiConstantPropagationLattice(*this);
}


// overwrites the state of "this" Lattice with "that" Lattice
void MultiConstantPropagationLattice::copy(Lattice* X)
{
    MultiConstantPropagationLattice* that = dynamic_cast<MultiConstantPropagationLattice*>(X);

    this->values = that->values;
    this->value = that->value;
    this->level = that->level;
}


bool MultiConstantPropagationLattice::operator==(Lattice* X)
{
    MultiConstantPropagationLattice* that = dynamic_cast<MultiConstantPropagationLattice*>(X);
    return (value == that->value) && (level == that->level);
}


string MultiConstantPropagationLattice::str(string indent)
{
    ostringstream outs;
    if(level == bottom)
        outs << indent << "[level: bottom]";
    else if(level > bottom && level < top){
        outs << indent << "[level: " << level << "constant; values = ";
        for(int i : values){
            outs << i << ",";
        }
        outs << " ]";
    }
    else if(level == top)
        outs << indent << "[level: top]";

    return outs.str();
}

// Computes the join (aka union) of the two lattices, despite the name
// We store the the new lattice in "this", and return true if "this" has changed
bool MultiConstantPropagationLattice::meetUpdate(Lattice* X)
{
    MultiConstantPropagationLattice* that = dynamic_cast<MultiConstantPropagationLattice*>(X);

    if(this->level == bottom){
        this->level = that->level;
        this->value = that->value;
        this->values = that->values;
        return (that->level != bottom);
    }
    else if( this->level > bottom && this->level < top){

        if(that->level == bottom)
            return false;
        else if(that->level == top){
            this->level = top;
            return true;
        }
        else{
            set<int> newSet;
            for(int i : this->values){
                newSet.insert(i);
            }
            for(int i : that->values){
                newSet.insert(i);
            }
            if(newSet.size() != this->values.size()){
                if(newSet.size() < top){
                    this->level = newSet.size();
                    this->values = newSet;
                    return true;
                }
                else{
                    this->level = top;
                    return true;
                }
            }
            else{
                return false;
            }
        }
    }
    else if(this->level == top){
        return false;
    }
}



template <typename T>
void MultiConstantPropagationAnalysisTransfer::transferArith(SgBinaryOp *sgn, T transferOp)
{
    MultiConstantPropagationLattice *arg1Lat, *arg2Lat, *resLat;
    if (getLattices(sgn, arg1Lat, arg2Lat, resLat))
    {
        transferOp(this, arg1Lat, arg2Lat, resLat);
        if (isSgCompoundAssignOp(sgn))
            arg1Lat->copy(resLat);
    }
}

void MultiConstantPropagationAnalysisTransfer::transferArith(SgBinaryOp *sgn, TransferOp transferOp)
{
    transferArith(sgn, boost::mem_fn(transferOp));
}

void MultiConstantPropagationAnalysisTransfer::transferIncrement(SgUnaryOp *sgn)
{
    MultiConstantPropagationLattice *arg1Lat, *arg2Lat = NULL, *resLat;
    if (getLattices(sgn, arg1Lat, arg2Lat, resLat))
        transferAdditive(arg1Lat, arg2Lat, resLat, isSgPlusPlusOp(sgn));
    delete arg2Lat; // Allocated by getLattices
    arg1Lat->copy(resLat);
}


bool MultiConstantPropagationAnalysisTransfer::transferAdditiveStatic(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat, bool isAddition)
{
    if(arg1Lat->getLevel() == MultiConstantPropagationLattice::bottom || arg2Lat->getLevel() == MultiConstantPropagationLattice::bottom){
        return resLat->setLevel(MultiConstantPropagationLattice::bottom);
    }
    else if(arg1Lat->isConstant() && arg2Lat->isConstant()){
        set<int> resSet;
        for(int i : arg1Lat->getValues()){
            for(int j : arg2Lat->getValues()){
                resSet.insert(isAddition ? i + j : i - j);
            }
        }
        if(resSet.size() < MultiConstantPropagationLattice::top){
            return(resLat->setValues(resSet));
        }
        else{
            return resLat->setLevel(MultiConstantPropagationLattice::top);
        }
    }
    else{
        return resLat->setLevel(MultiConstantPropagationLattice::top);
    }
}



bool MultiConstantPropagationAnalysisTransfer::transferMultiplicativeStatic(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat)
{
    if(arg1Lat->getLevel() == MultiConstantPropagationLattice::bottom || arg2Lat->getLevel() == MultiConstantPropagationLattice::bottom){
        return resLat->setLevel(MultiConstantPropagationLattice::bottom);
    }
    else if(arg1Lat->isConstant() && arg2Lat->isConstant()){
        set<int> resSet;
        for(int i : arg1Lat->getValues()){
            for(int j : arg2Lat->getValues()){
                resSet.insert(i*j);
            }
        }
        if(resSet.size() < MultiConstantPropagationLattice::top){
            return(resLat->setValues(resSet));
        }
        else{
            return resLat->setLevel(MultiConstantPropagationLattice::top);
        }
    }
    else{
        return resLat->setLevel(MultiConstantPropagationLattice::top);
    }
}

bool MultiConstantPropagationAnalysisTransfer::transferDivisionStatic(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat)
{
    if(arg1Lat->getLevel() == MultiConstantPropagationLattice::bottom || arg2Lat->getLevel() == MultiConstantPropagationLattice::bottom){
        return resLat->setLevel(MultiConstantPropagationLattice::bottom);
    }
    else if(arg1Lat->isConstant() && arg2Lat->isConstant()){
        set<int> resSet;
        for(int i : arg1Lat->getValues()){
            for(int j : arg2Lat->getValues()){
                resSet.insert(i/j);
            }
        }
        if(resSet.size() < MultiConstantPropagationLattice::top){
            return(resLat->setValues(resSet));
        }
        else{
            return resLat->setLevel(MultiConstantPropagationLattice::top);
        }
    }
    else{
        return resLat->setLevel(MultiConstantPropagationLattice::top);
    }
}

bool MultiConstantPropagationAnalysisTransfer::transferModStatic(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat)
{
    if(arg1Lat->getLevel() == MultiConstantPropagationLattice::bottom || arg2Lat->getLevel() == MultiConstantPropagationLattice::bottom){
        return resLat->setLevel(MultiConstantPropagationLattice::bottom);
    }
    else if(arg1Lat->isConstant() && arg2Lat->isConstant()){
        set<int> resSet;
        for(int i : arg1Lat->getValues()){
            for(int j : arg2Lat->getValues()){
                resSet.insert(i%j);
            }
        }
        if(resSet.size() < MultiConstantPropagationLattice::top){
            return(resLat->setValues(resSet));
        }
        else{
            return resLat->setLevel(MultiConstantPropagationLattice::top);
        }
    }
    else{
        return resLat->setLevel(MultiConstantPropagationLattice::top);
    }
}



void MultiConstantPropagationAnalysisTransfer::transferAdditive(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat, bool isAddition)
{
    updateModified(transferAdditiveStatic(arg1Lat, arg2Lat,resLat, isAddition));
}


void MultiConstantPropagationAnalysisTransfer::transferMultiplicative(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat)
{
    updateModified(transferMultiplicativeStatic(arg1Lat, arg2Lat, resLat));
}

void MultiConstantPropagationAnalysisTransfer::transferDivision(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat)
{
    updateModified(transferDivisionStatic(arg1Lat, arg2Lat, resLat));
}

void MultiConstantPropagationAnalysisTransfer::transferMod(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat)
{
    updateModified(transferModStatic(arg1Lat, arg2Lat, resLat));
}

void MultiConstantPropagationAnalysisTransfer::visit(SgForStatement* forStatement)
{
    SgExpression* test = forStatement->get_test_expr();

    SgBinaryOp* binOp = isSgBinaryOp(test);

    SgLessThanOp* less = isSgLessThanOp(test);
    SgLessOrEqualOp* lessEq = isSgLessOrEqualOp(test);
    SgGreaterThanOp* great = isSgGreaterThanOp(test);
    SgGreaterOrEqualOp* greatEq = isSgGreaterOrEqualOp(test);

    SgVarRefExp* lhsVar;
    SgIntVal* lhsInt;
    SgVarRefExp* rhsVar;
    SgIntVal* rhsInt;
    if(binOp){
        lhsVar = isSgVarRefExp(binOp->get_lhs_operand());
        lhsInt = isSgIntVal(binOp->get_rhs_operand());
        rhsVar = isSgVarRefExp(binOp->get_rhs_operand());
        rhsInt = isSgIntVal(binOp->get_rhs_operand());
    }

    if(less){
        if(lhsVar && rhsInt){ // i < 5
            getLattice(lhsVar)->capValues(rhsInt->get_value() -1);
        }
        if(rhsVar && lhsInt){ // 5 < i
            getLattice(rhsVar)->capBelowValues(lhsInt->get_value() +1);
        }
    }
    if(lessEq){
        if(lhsVar && rhsInt){ // i <= 5
            getLattice(lhsVar)->capValues(rhsInt->get_value());
        }
        if(rhsVar && lhsInt){ // 5 <= i
            getLattice(rhsVar)->capBelowValues(lhsInt->get_value());
        }
    }
    if(great){
        if(lhsVar && rhsInt){ // i > 5
            getLattice(lhsVar)->capBelowValues(rhsInt->get_value() + 1);
        }
        if(rhsVar && lhsInt){ // 5 > i
            getLattice(rhsVar)->capValues(lhsInt->get_value() -1);
        }
    }
    if(greatEq){
        if(lhsVar && rhsInt){ // i >= 5
            getLattice(lhsVar)->capBelowValues(rhsInt->get_value());
        }
        if(rhsVar && lhsInt){ // 5 >= i
            getLattice(rhsVar)->capValues(lhsInt->get_value());
        }
    }


}

void MultiConstantPropagationAnalysisTransfer::visit(SgLongLongIntVal *sgn)
   {
   }

void MultiConstantPropagationAnalysisTransfer::visit(SgLongIntVal *sgn)
   {
   }

void MultiConstantPropagationAnalysisTransfer::visit(SgIntVal *sgn)
{
    ROSE_ASSERT(sgn != NULL);
    MultiConstantPropagationLattice* resLat = getLattice(sgn);
    ROSE_ASSERT(resLat != NULL);
    resLat->setValue(sgn->get_value());
    //resLat->setLevel(MultiConstantPropagationLattice::constantValue);
}

void MultiConstantPropagationAnalysisTransfer::visit(SgShortVal *sgn)
   {
   }

void MultiConstantPropagationAnalysisTransfer::visit(SgUnsignedLongLongIntVal *sgn)
   {
   }

void MultiConstantPropagationAnalysisTransfer::visit(SgUnsignedLongVal *sgn)
   {
   }

void MultiConstantPropagationAnalysisTransfer::visit(SgUnsignedIntVal *sgn)
   {
   }

void MultiConstantPropagationAnalysisTransfer::visit(SgUnsignedShortVal *sgn)
   {
   }

void MultiConstantPropagationAnalysisTransfer::visit(SgValueExp *sgn)
   {
   }

void MultiConstantPropagationAnalysisTransfer::visit(SgPlusAssignOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferAdditive, _1, _2, _3, _4, true ));
}

void MultiConstantPropagationAnalysisTransfer::visit(SgMinusAssignOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferAdditive, _1, _2, _3, _4, false));
}

void MultiConstantPropagationAnalysisTransfer::visit(SgMultAssignOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferMultiplicative, _1, _2, _3, _4 ));
}

void MultiConstantPropagationAnalysisTransfer::visit(SgDivAssignOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferDivision, _1, _2, _3, _4 ));
}

void MultiConstantPropagationAnalysisTransfer::visit(SgModAssignOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferMod, _1, _2, _3, _4 ));
}

void MultiConstantPropagationAnalysisTransfer::visit(SgAddOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferAdditive, _1, _2, _3, _4, true ));
}

void MultiConstantPropagationAnalysisTransfer::visit(SgSubtractOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferAdditive, _1, _2, _3, _4, false));
}

void MultiConstantPropagationAnalysisTransfer::visit(SgMultiplyOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferMultiplicative, _1, _2, _3, _4 ));
}

void
MultiConstantPropagationAnalysisTransfer::visit(SgDivideOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferDivision, _1, _2, _3, _4 ));
}

void
MultiConstantPropagationAnalysisTransfer::visit(SgModOp *sgn)
{
    transferArith(sgn, boost::bind(&MultiConstantPropagationAnalysisTransfer::transferMod, _1, _2, _3, _4 ));
}

void
MultiConstantPropagationAnalysisTransfer::visit(SgPlusPlusOp *sgn)
{
    transferIncrement(sgn);
}

void
MultiConstantPropagationAnalysisTransfer::visit(SgMinusMinusOp *sgn)
{
    transferIncrement(sgn);
}

void
MultiConstantPropagationAnalysisTransfer::visit(SgUnaryAddOp *sgn)
{
    MultiConstantPropagationLattice* resLat = getLattice(sgn);
    resLat->copy(getLattice(sgn->get_operand()));
}

void MultiConstantPropagationAnalysisTransfer::visit(SgMinusOp *sgn)
{
    MultiConstantPropagationLattice* resLat = getLattice(sgn);

    // This sets the level
    resLat->copy(getLattice(sgn->get_operand()));

    // This fixes up the values if it is relevant (where level is neither top not bottom).
    set<int> negValues;
    for(int i : resLat->getValues()){
        negValues.insert(-i);
    }
    resLat->setValues(negValues);
}

bool MultiConstantPropagationAnalysisTransfer::finish()
{
    return modified;
}

MultiConstantPropagationAnalysisTransfer::MultiConstantPropagationAnalysisTransfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo)
    : VariableStateTransfer<MultiConstantPropagationLattice>(func, n, state, dfInfo, multiConstantPropagationAnalysisDebugLevel)
{
}



MultiConstantPropagationAnalysis::MultiConstantPropagationAnalysis(LiveDeadVarsAnalysis* ldva)
{
    this->ldva = ldva;
}

// generates the initial lattice state for the given dataflow node, in the given function, with the given NodeState
void MultiConstantPropagationAnalysis::genInitState(const Function& func, const DataflowNode& n, const NodeState& state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts)
{
    // ???
    // vector<Lattice*> initLattices;
    map<varID, Lattice*> emptyM;
    FiniteVarsExprsProductLattice* l = new FiniteVarsExprsProductLattice((Lattice*)new MultiConstantPropagationLattice(), emptyM/*genConstVarLattices()*/,
                                                                         (Lattice*)NULL, ldva, /*func, */n, state);
    //Dbg::dbg << "DivAnalysis::genInitState, returning l="<<l<<" n=<"<<Dbg::escape(n.getNode()->unparseToString())<<" | "<<n.getNode()->class_name()<<" | "<<n.getIndex()<<">\n";
    //Dbg::dbg << "    l="<<l->str("    ")<<"\n";
    initLattices.push_back(l);
}


bool MultiConstantPropagationAnalysis::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo)
{
    assert(0);
    return false;
}

boost::shared_ptr<IntraDFTransferVisitor> MultiConstantPropagationAnalysis::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo)
{
    // Why is the boost shared pointer used here? Hell if I know!
    return boost::shared_ptr<IntraDFTransferVisitor>(new MultiConstantPropagationAnalysisTransfer(func, n, state, dfInfo));
}

