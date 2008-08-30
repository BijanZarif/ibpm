// RungeKutta2.cc
//
// Description:
// Implementation of the RungeKutta2 class
//
// Author(s):
// Clancy Rowley
// Steve Brunton
//
// Date: 28 Aug 2008
//
// $Revision: 105 $
// $LastChangedDate: 2008-08-27 23:14:34 -0400 (Thu, 28 Aug 2008) $
// $LastChangedBy: sbrunton $
// $HeadURL:// $Header$

#include "BoundaryVector.h"
#include "Geometry.h"
#include "Grid.h"
#include "NavierStokesModel.h"
#include "ProjectionSolver.h"
#include "TimeStepper.h"
#include "State.h"
#include "RungeKutta2.h"

RungeKutta2::RungeKutta2(const NavierStokesModel& model, double timestep) :
    TimeStepper(model, timestep),
    _linearTermEigenvalues( *model.getLambda() ),
    _x1( *model.getGrid(), *model.getGeometry() )
    {
    // compute eigenvalues of linear term on RHS
    _linearTermEigenvalues *= timestep/2.;
    _linearTermEigenvalues += 1;
  
    _solver = createSolver(timestep);
}

void RungeKutta2::advance(State& x) {
    // If the body is moving, update the positions of the bodies
    const Geometry* geom = _model->getGeometry();
    if ( ! geom->isStationary() ) {
        geom->moveBodies(x.time);
    }

    // First Projection Solve 

    // Evaluate Right-Hand-Side (a) for first equation of ProjectionSolver
    Scalar a = _model->S( x.gamma );
    a *= _linearTermEigenvalues;
    a = _model->Sinv( a );

    Scalar nonlinear = _model->nonlinear( x );
    a += _timestep * nonlinear;
    
    // Evaluate Right-Hand-Side (b) for second equation of ProjectionSolver
    BoundaryVector b = geom->getVelocities();
    BoundaryVector b0 = _model->getBaseFlowBoundaryVelocities();
    b -= b0;
    
    // Call the ProjectionSolver to determine the circulation and forces
    _solver->solve( a, b, _x1.gamma, _x1.f );
    
    // Compute the corresponding flux
    _model->computeFlux( _x1.gamma, _x1.q );

    // Second Projection Solve

    // Evaluate Right-Hand-Side (a) for first equation of ProjectionSolver
    a = _model->S( x.gamma );
    a *= _linearTermEigenvalues;
    a = _model->Sinv( a );

    nonlinear = _model->nonlinear( x ) + _model->nonlinear( _x1 );
    a += .5 * _timestep * nonlinear;
    
    // Evaluate Right-Hand-Side (b) for second equation of ProjectionSolver
    b = geom->getVelocities();
    b0 = _model->getBaseFlowBoundaryVelocities();
    b -= b0;
    
    // Call the ProjectionSolver to determine the circulation and forces
    _solver->solve( a, b, x.gamma, x.f );
    
    // Compute the corresponding flux
    _model->computeFlux( x.gamma, x.q );
 
   
    // Update the value of the time
    x.time += _timestep;
    ++x.timestep;
}