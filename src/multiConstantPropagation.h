// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef MULTI_CONSTANT_PROPAGATION_ANALYSIS_H
#define MULTI_CONSTANT_PROPAGATION_ANALYSIS_H

#include "VariableStateTransfer.h"
#include <set>

using namespace std;

extern int multiConstantPropagationAnalysisDebugLevel;

class MultiConstantPropagationLattice : public FiniteLattice
{
private:
    bool undefined;
    int value;
    short level;
    set<int> values;

public:
    static const short bottom = 0;

    static const short constantValue = 8;

    static const short oneValue = 1;
    static const short twoValues = 2;
    static const short threeeValues = 3;
    static const short fourValues = 4;
    static const short fiveValues = 5;
    static const short sixValues = 6;
    static const short sevenValues = 7;
    static const short eightValues = 8;
    static const short nineValues = 9;
    static const short tenValues = 10;
    static const short _11Values = 11;
    static const short _12Values = 12;
    static const short _13Values = 13;
    static const short _14Values = 14;
    static const short _15Values = 15;
    static const short _16Values = 16;
    static const short _17Values = 17;
    static const short _18Values = 18;
    static const short _19Values = 19;
    static const short _20Values = 20;
    static const short _21Values = 21;
    static const short _22Values = 22;
    static const short _23Values = 23;
    static const short _24Values = 24;
    static const short _25Values = 25;
    static const short _26Values = 26;
    static const short _27Values = 27;
    static const short _28Values = 28;
    static const short _29Values = 29;
    static const short _30Values = 30;
    static const short _31Values = 31;

    static const short top = 32;

public:
    MultiConstantPropagationLattice();

    MultiConstantPropagationLattice( int v );

    MultiConstantPropagationLattice(const MultiConstantPropagationLattice & X);

    int getValue() const;
    short getLevel() const;
    bool isConstant() const;
    set<int> getValues() const;

    bool addValue(int x);
    bool setValue(int x);
    bool setValues(set<int> x);
    bool setLevel(short x);
    bool capValues(int x);
    bool capBelowValues(int x);

    bool setBottom();
    bool setTop();

    void initialize();

    // returns a copy of this lattice
    Lattice* copy() const;

    // overwrites the state of "this" Lattice with "that" Lattice
    void copy(Lattice* that);

    bool operator==(Lattice* that);

    // computes the meet of this and that and saves the result in this
    // returns true if this causes this to change and false otherwise
    bool meetUpdate(Lattice* that);

    std::string str(std::string indent="");
};


class MultiConstantPropagationAnalysisTransfer : public VariableStateTransfer<MultiConstantPropagationLattice>
{
private:
    typedef void (MultiConstantPropagationAnalysisTransfer::*TransferOp)(MultiConstantPropagationLattice *, MultiConstantPropagationLattice *, MultiConstantPropagationLattice *);
    template <typename T> void transferArith(SgBinaryOp *sgn, T transferOp);
    template <class T> void visitIntegerValue(T *sgn);

public:
    void transferArith(SgBinaryOp *sgn, TransferOp transferOp);

    void transferIncrement(SgUnaryOp *sgn);
    void transferCompoundAdd(SgBinaryOp *sgn);

    //Ugly hack to make this easier to test.
    static bool transferAdditiveStatic(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat, bool isAddition);
    static bool transferMultiplicativeStatic(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat);
    static bool transferDivisionStatic(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat);
    static bool transferModStatic(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat);

    void transferAdditive(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat, bool isAddition);
    void transferMultiplicative(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat);
    void transferDivision(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat);
    void transferMod(MultiConstantPropagationLattice *arg1Lat, MultiConstantPropagationLattice *arg2Lat, MultiConstantPropagationLattice *resLat);

    //  void visit(SgNode *);
    void visit(SgForStatement* sgn);
    void visit(SgLongLongIntVal *sgn);
    void visit(SgLongIntVal *sgn);
    void visit(SgIntVal *sgn);
    void visit(SgShortVal *sgn);
    void visit(SgUnsignedLongLongIntVal *sgn);
    void visit(SgUnsignedLongVal *sgn);
    void visit(SgUnsignedIntVal *sgn);
    void visit(SgUnsignedShortVal *sgn);
    void visit(SgValueExp *sgn);
    void visit(SgPlusAssignOp *sgn);
    void visit(SgMinusAssignOp *sgn);
    void visit(SgMultAssignOp *sgn);
    void visit(SgDivAssignOp *sgn);
    void visit(SgModAssignOp *sgn);
    void visit(SgAddOp *sgn);
    void visit(SgSubtractOp *sgn);
    void visit(SgMultiplyOp *sgn);
    void visit(SgDivideOp *sgn);
    void visit(SgModOp *sgn);
    void visit(SgPlusPlusOp *sgn);
    void visit(SgMinusMinusOp *sgn);
    void visit(SgUnaryAddOp *sgn);
    void visit(SgMinusOp *sgn);

    bool finish();

    MultiConstantPropagationAnalysisTransfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);
};


class MultiConstantPropagationAnalysis : public IntraFWDataflow
{
protected:
    static std::map<varID, Lattice*> constVars;
    static bool constVars_init;

    // The LiveDeadVarsAnalysis that identifies the live/dead state of all application variables.
    // Needed to create a FiniteVarsExprsProductLattice.
    LiveDeadVarsAnalysis* ldva;

public:
    MultiConstantPropagationAnalysis(LiveDeadVarsAnalysis* ldva);

    // generates the initial lattice state for the given dataflow node, in the given function, with the given NodeState
    void genInitState(const Function& func, const DataflowNode& n, const NodeState& state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts);

    bool transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);

    boost::shared_ptr<IntraDFTransferVisitor> getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);
};


#endif
