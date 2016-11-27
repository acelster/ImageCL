// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include <iostream>
#include <fstream>
#include <vector>
#include <set>

#include "rose.h"

#include "genericDataflowCommon.h"
#include "variables.h"
#include "analysisCommon.h"
#include "functionState.h"
#include "latticeFull.h"
#include "analysis.h"
#include "dataflow.h"
#include "divAnalysis.h"
#include "saveDotAnalysis.h"

#include "globalremover.h"
#include "naivecoarsener.h"
#include "coarsener.h"
#include "indexcallinserter.h"
#include "imagememtransformer.h"
#include "arrayflattener.h"
#include "kernelfinder.h"
#include "wrappergenerator.h"
#include "argument.h"
#include "footprintfinder.h"
#include "localmemtransformer.h"
#include "constantPropagation.h"
#include "multiConstantPropagation.h"
#include "parameters.h"
#include "kernelinfo.h"
#include "boundryguardinserter.h"
#include "constantmemtransformer.h"
#include "loopunroller.h"
#include "settings.h"
#include "fastwrappergenerator.h"
#include "filehandler.h"
#include "indexchanger.h"
#include "astutil.h"

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;
using namespace CommandlineProcessing;


void patch_ast(SgProject* project)
{
    Rose_STL_Container<SgNode*> varRefs = NodeQuery::querySubTree(project, V_SgVarRefExp);
    //This has to be the ugliest hack ever, somehow needed to fix AST due to template types for image arrays
    //Should be moved to a separate function or something
    for(SgNode* node : varRefs){
        SgVarRefExp* varRef = isSgVarRefExp(node);

        if(varRef->get_symbol()->get_name() == "idx"){
            //cout << varRef->get_symbol()->get_name() << endl;

            SgScopeStatement* scope = getEnclosingScope(varRef);
            SgVarRefExp* newVarRef = buildVarRefExp("idx", scope);

            replaceExpression(varRef, newVarRef);
        }
        else if(varRef->get_symbol()->get_name() == "idy"){
            //cout << varRef->get_symbol()->get_name() << endl;

            SgScopeStatement* scope = getEnclosingScope(varRef);
            SgVarRefExp* newVarRef = buildVarRefExp("idy", scope);

            replaceExpression(varRef, newVarRef);
        }
    }

    Rose_STL_Container<SgNode*> tempDecl = NodeQuery::querySubTree(project, V_SgTemplateTypedefDeclaration);

    for(SgNode* node : tempDecl){
        SgTemplateTypedefDeclaration* temptypdef = isSgTemplateTypedefDeclaration(node);
        removeStatement(temptypdef);
    }
}

void generateCCode(SgProject* project, KernelInfo kernelInfo, Settings settings, Parameters params)
{
    GlobalVariableRemover globalVarRemover;
    globalVarRemover.traverseInputFiles(project, preorder);

    ArrayFlattener arrayFlattener(project, params, kernelInfo, settings);
    arrayFlattener.transform();

    ArgumentHandler argumentHandler(project, kernelInfo, params, settings);
    argumentHandler.addCArguments();

    Rose_STL_Container<SgNode*> tempDecl = NodeQuery::querySubTree(project, V_SgTemplateTypedefDeclaration);

    for(SgNode* node : tempDecl){
        SgTemplateTypedefDeclaration* temptypdef = isSgTemplateTypedefDeclaration(node);
        removeStatement(temptypdef);
    }

    project->unparse();
}

int main(int argc, char** argv)
{
    Rose_STL_Container<string> commandLineArgs = generateArgListFromArgcArgv(argc, argv);
    Rose_STL_Container<string> fileNames = generateSourceFilenames(commandLineArgs, false);
    if(fileNames.size() > 1){
        cout << "ERROR: can only handle one input file. Exiting..." << endl;
        exit(-1);
    }


    bool generateParamSpec;
    generateParamSpec = isOption(commandLineArgs, "-clite:", "g", false);

    bool generateC;
    generateC = isOption(commandLineArgs, "-clite:", "c", false);

    Settings settings;
    settings.readSettingsFromFile("settings.txt");
    settings.inputBaseName = FileHandler::getFileNameBase(fileNames[0]);
    settings.setGenerateC(generateC);
    settings.printSettings();

    FileHandler::fixFiles(fileNames, settings);
    char** newArgv = FileHandler::fixFileNames(argc, argv, fileNames);

    SgProject* project = frontend(argc, newArgv);

    KernelInfo kernelInfo(project, settings);
    kernelInfo.scanForInfo();
    kernelInfo.printKernelInfo();

    Parameters params;
    if(generateParamSpec){
        params.generateParameterSpecification(kernelInfo);
        generateDOT(*project);
        return 0;
    }

    params.setDefaultParameters();
    if(!generateC){
        params.readParametersFromFile(kernelInfo, "config.txt");
        params.setParametersFromPragmas(kernelInfo.getPragmas());
    }
    params.validateParameters(kernelInfo);
    params.printParameters();

    if(generateC){
        generateCCode(project, kernelInfo, settings, params);
        FileHandler::cleanUpFiles(fileNames, settings, false);
        return 0;
    }

    LoopUnroller loopUnroller(project, params, kernelInfo);
    loopUnroller.traverseInputFiles(project, postorder);
    loopUnroller.unrollLoops();

    GlobalVariableRemover globalVarRemover;
    globalVarRemover.traverseInputFiles(project, preorder);

    NaiveCoarsener naiveCoarsener(params, kernelInfo, settings);
    naiveCoarsener.traverseInputFiles(project, preorder);
    fixVariableReferences(project);

    if(params.useLocalMem()){
        LocalMemTransformer localMemTransformer(project, params, kernelInfo, settings, naiveCoarsener.getOriginalFunctionBody());
        localMemTransformer.transform();
    }

    IndexChanger indexChanger(kernelInfo, params, settings);
    if(settings.generateMPI || settings.generateOMP){
        indexChanger.replaceIndices(project);
    }

    BoundryGuardInserter boundryGuardInserter(kernelInfo, params, project, settings);
    boundryGuardInserter.insertBoundryGuards();

    if(settings.generateMPI || settings.generateOMP){
        indexChanger.addPaddings(project);
    }

    if(params.useImageMem()){
        ImageMemTransformer imageMemTransformer(project, params, kernelInfo, settings);
        imageMemTransformer.transform();
    }

    ArrayFlattener arrayFlattener(project, params, kernelInfo, settings);
    arrayFlattener.transform();


    if(params.useConstantMem()){
        ConstantMemTransformer constantMemTransformer(project, kernelInfo, params);
        constantMemTransformer.transform();
    }

    KernelFinder kernelFinder(project, params, kernelInfo);
    kernelFinder.transform();

    ArgumentHandler argumentHandler(project, kernelInfo, params, settings);
    vector<Argument>* arguments = argumentHandler.addAndGetArguments();

    patch_ast(project);

    if(settings.generateStandalone){
        WrapperGenerator wrapperGenerator(settings.inputBaseName + "_wrapper.c",arguments,params,kernelInfo, settings);
        wrapperGenerator.generate();
    }
    if(settings.generateFAST){
        FASTWrapperGenerator fastWrapperGenerator("FastWrapper", arguments, params, kernelInfo);
        fastWrapperGenerator.generate();
    }

    generateDOT(*project);
    project->unparse();

    FileHandler::cleanUpFiles(fileNames, settings, true);
}
