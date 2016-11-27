// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef BOUNDRYGUARDINSERTER_H
#define BOUNDRYGUARDINSERTER_H

#include "rose.h"
#include "kernelinfo.h"
#include "parameters.h"
#include "settings.h"
#include "uniquenamegenerator.h"

enum BoundaryGuardDirection {BOUNDARY_GUARD_DIR_X, BOUNDARY_GUARD_DIR_Y, BOUNDARY_GUARD_DIR_Z, BOUNDARY_GUARD_DIR_ALL};

enum GridPosition {CENTRAL = 0, WEST = 1, EAST = 2, CENTRAL_EAST_WEST = 3,
                   SOUTH = 4, SOUTH_WEST = 5, SOUTH_EAST = 6, SOUTH_EAST_WEST = 7,
                   NORTH = 8, NORTH_WEST = 9, NORTH_EAST = 10, NORTH_EAST_WEST = 11,
                   CENTRAL_NORTH_SOUTH = 12, WEST_NORTH_SOUTH = 13, EAST_NORTH_SOUTH = 14, NORTH_SOUTH_EAST_WEST = 15};

class BoundryGuardInserter
{
public:
    BoundryGuardInserter(KernelInfo kernelInfo, Parameters params, SgProject* project, Settings settings);

    void wrapWithGuards(SgPntrArrRefExp* arrRef, string arrayName, SgScopeStatement* scope, bool dirX, bool dirY);
    void insertBoundryGuards();
    static bool needsBoundaryGuard(string arrayName, KernelInfo kernelInfo, Parameters params, Settings settings, BoundaryGuardDirection direction=BOUNDARY_GUARD_DIR_ALL);
    bool needsBoundaryGuardX(string arrayName);
    bool needsBoundaryGuardY(string arrayName);

    void wrapWithGuardsConstant(SgExpression* xExpression, SgVarRefExp* height, SgVarRefExp* width, bool dirY, bool dirX, SgExpression* yExpression, SgPntrArrRefExp* arrRef);
    void wrapWithGuardsClamped(SgPntrArrRefExp* arrRef, SgExpression* yExpression, SgVarRefExp* width, SgVarRefExp* height, SgExpression* xExpression, bool dirY, bool dirX, SgScopeStatement* scope);
    void insertClampedGuard(SgScopeStatement* scope, SgExpression* varExpression, SgStatement* parentStatement, SgVarRefExp* dim);
    void insertClampedDistGuards(SgVarRefExp* dim, SgScopeStatement* scope, SgExpression* varExpression, SgStatement* parentStatement, GridPosition minDir, GridPosition maxDir);
private:
    KernelInfo kernelInfo;
    Parameters params;
    SgProject* project;
    Settings settings;
};

#endif // BOUNDRYGUARDINSERTER_H
