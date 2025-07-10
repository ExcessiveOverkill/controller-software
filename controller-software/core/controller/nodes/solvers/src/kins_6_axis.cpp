#include "kins_6_axis.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>
#include <chrono>

// Constructor
InverseKinematics::InverseKinematics()
{}

// Public solve: first float pass, then refine with double if needed
InverseKinematics::ErrorCode
InverseKinematics::solve(
    const std::array<float,3>&   targetPosF,
    const std::array<float,3>&   targetEulerF,
    const std::array<float,6>&   initGuessF,
    const std::array<float,6>&   cartTwistF,
    std::array<float,6>&         outSolutionF,
    std::array<float,6>&         outVelF,
    IKErrorDetail&               detail,
    int                           maxIter,
    float                         tolerance
) const {
    // 1) Single-precision
    std::array<float,6> solF = initGuessF;
    std::array<float,6> velF = {0,0,0,0,0,0};
    auto ec = solveT<float>(
        targetPosF.data(),
        targetEulerF.data(),
        initGuessF.data(),
        cartTwistF.data(),
        solF.data(),
        velF.data(),
        detail,
        maxIter/2, // Use half iterations for first pass
        tolerance
    );

    if(ec == ErrorCode::Success && detail.finalErrorNorm < tolerance) {
        // complete and in tolerance, no need for double precision
        outSolutionF = solF;
        outVelF      = velF;
        return ErrorCode::Success;
    }


    // 2) Double-precision refinement
    std::array<double,3> tgtPosD, tgtEulD;
    std::array<double,6> initGuessD, cartTwistD, solD, velD;
    for(int i=0;i<3;i++) {
        tgtPosD[i]   = targetPosF[i];
        tgtEulD[i]   = targetEulerF[i];
    }
    for(int i=0;i<6;i++) {
        initGuessD[i] = solF[i];
        cartTwistD[i] = cartTwistF[i];
    }
    IKErrorDetail detailD;
    auto ec2 = solveT<double>(
        tgtPosD.data(),
        tgtEulD.data(),
        initGuessD.data(),
        cartTwistD.data(),
        solD.data(),
        velD.data(),
        detailD,
        maxIter/2,
        static_cast<double>(tolerance)
    );
    // Cast back
    for(int i=0;i<6;i++) {
        outSolutionF[i] = static_cast<float>(solD[i]);
        outVelF     [i] = static_cast<float>(velD[i]);
    }
    detail = detailD;
    return ec2;
}

// -----------------------------------------------------------------------------
// Templated definitions (must be visible to linker)

// Forward kinematics using DH parameters
template<typename T>
void InverseKinematics::forwardKinematics(
    const T angles[6], T posOut[3], T rotOut[9]
) const {
    // Full 4×4 accumulator (row-major)
    T Tmat[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    for(int i=0;i<6;i++){
        T theta = angles[i] + dh_[i].thetaOffset;
        T cth   = std::cos(theta), sth = std::sin(theta);
        T calp  = std::cos(dh_[i].alpha), salp = std::sin(dh_[i].alpha);
        // Single-joint DH transform
        T A[16] = {
            cth,        -sth*calp,   sth*salp,   dh_[i].a*cth,
            sth,         cth*calp,  -cth*salp,   dh_[i].a*sth,
            0,            salp,       calp,       dh_[i].d,
            0,              0,          0,         1
        };
        // Tmat = Tmat * A
        T tmp[16];
        for(int r=0;r<4;r++){
            for(int c=0;c<4;c++){
                T sum = 0;
                for(int k=0;k<4;k++) sum += Tmat[r*4+k] * A[k*4+c];
                tmp[r*4+c] = sum;
            }
        }
        std::copy(tmp, tmp+16, Tmat);
    }
    // Extract position
    posOut[0] = Tmat[3];
    posOut[1] = Tmat[7];
    posOut[2] = Tmat[11];
    // Extract rotation (row-major 3×3)
    rotOut[0] = Tmat[0]; rotOut[1] = Tmat[1]; rotOut[2] = Tmat[2];
    rotOut[3] = Tmat[4]; rotOut[4] = Tmat[5]; rotOut[5] = Tmat[6];
    rotOut[6] = Tmat[8]; rotOut[7] = Tmat[9]; rotOut[8] = Tmat[10];
}

// Analytic Jacobian: geometric form
template<typename T>
void InverseKinematics::computeAnalyticJacobian(
    const T angles[6], T Jout[6][6]
) const {
    // Store frame origins and z-axes
    std::array<std::array<T,3>,7> p;
    std::array<std::array<T,3>,7> z;
    // Base frame
    p[0] = {T(0),T(0),T(0)};
    z[0] = {T(0),T(0),T(1)};
    // Accumulate transforms
    T Tmat[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    for(int i=0;i<6;i++){
        T theta = angles[i] + dh_[i].thetaOffset;
        T cth   = std::cos(theta), sth = std::sin(theta);
        T calp  = std::cos(dh_[i].alpha), salp = std::sin(dh_[i].alpha);
        T A[16] = {
            cth,        -sth*calp,   sth*salp,   dh_[i].a*cth,
            sth,         cth*calp,  -cth*salp,   dh_[i].a*sth,
            0,            salp,       calp,       dh_[i].d,
            0,              0,          0,         1
        };
        T tmp[16];
        for(int r=0;r<4;r++){
            for(int c=0;c<4;c++){
                T sum=0;
                for(int k=0;k<4;k++) sum += Tmat[r*4+k] * A[k*4+c];
                tmp[r*4+c] = sum;
            }
        }
        std::copy(tmp, tmp+16, Tmat);
        // Frame origin
        p[i+1] = { Tmat[3], Tmat[7], Tmat[11] };
        // z-axis = third column
        z[i+1] = { Tmat[2], Tmat[6], Tmat[10] };
    }
    // End-effector origin
    auto& pe = p[6];
    // Populate Jacobian columns
    for(int j=0;j<6;j++){
        // Linear component Jp = z_j × (p6 - p_j)
        T rx = z[j][1]*(pe[2]-p[j][2]) - z[j][2]*(pe[1]-p[j][1]);
        T ry = z[j][2]*(pe[0]-p[j][0]) - z[j][0]*(pe[2]-p[j][2]);
        T rz = z[j][0]*(pe[1]-p[j][1]) - z[j][1]*(pe[0]-p[j][0]);
        Jout[0][j] = rx;
        Jout[1][j] = ry;
        Jout[2][j] = rz;
        // Angular component Jω = z_j
        Jout[3][j] = z[j][0];
        Jout[4][j] = z[j][1];
        Jout[5][j] = z[j][2];
    }
}

// Gaussian elimination solver
template<typename T>
bool InverseKinematics::solveLinearSystem(
    const T A[6][6], const T b[6], T x[6]
) const {
    const int N = 6;
    T aug[N][7];
    // Build augmented matrix
    for(int i=0;i<N;i++){
        for(int j=0;j<N;j++) aug[i][j] = A[i][j];
        aug[i][N] = b[i];
    }
    // Forward elimination
    for(int i=0;i<N;i++){
        // Partial pivot
        int piv = i;
        T maxv = std::abs(aug[i][i]);
        for(int k=i+1;k<N;k++){
            T v = std::abs(aug[k][i]);
            if(v > maxv){ maxv=v; piv=k; }
        }
        if(maxv < std::numeric_limits<T>::epsilon()) return false;
        if(piv != i) for(int j=i;j<=N;j++) std::swap(aug[i][j], aug[piv][j]);
        // Normalize row i
        T diag = aug[i][i];
        for(int j=i;j<=N;j++) aug[i][j] /= diag;
        // Eliminate below
        for(int k=i+1;k<N;k++){
            T factor = aug[k][i];
            for(int j=i;j<=N;j++) aug[k][j] -= factor * aug[i][j];
        }
    }
    // Back-substitution
    for(int i=N-1;i>=0;i--){
        T sum = aug[i][N];
        for(int j=i+1;j<N;j++) sum -= aug[i][j] * x[j];
        x[i] = sum;
    }
    return true;
}

// Core templated solver
template<typename T>
InverseKinematics::ErrorCode
InverseKinematics::solveT(
    const T targetPos[3],
    const T targetEuler[3],
    const T initGuess[6],
    const T cartTwist[6],
    T       outSol[6],
    T       outVel[6],
    IKErrorDetail& detail,
    int     maxIter,
    T       tol
) const {
    // Current estimate
    T theta[6];
    for(int i=0;i<6;i++) theta[i] = initGuess[i];

    T J[6][6], err[6], delta[6];
    T bestErr = std::numeric_limits<T>::infinity();

    // Precompute desired rotation matrix from ZYX Euler
    T phi = targetEuler[0], th = targetEuler[1], psi = targetEuler[2];
    T c1 = std::cos(phi), s1 = std::sin(phi);
    T c2 = std::cos(th),  s2 = std::sin(th);
    T c3 = std::cos(psi), s3 = std::sin(psi);
    // Rz(φ)·Ry(θ)·Rx(ψ)
    T Rg[9] = {
        c1*c2,            c1*s2*s3 - s1*c3,    c1*s2*c3 + s1*s3,
        s1*c2,            s1*s2*s3 + c1*c3,    s1*s2*c3 - c1*s3,
        -s2,              c2*s3,               c2*c3
    };

    for(int iter=0; iter<maxIter; iter++){
        // Forward kinematics
        T pos[3], Rcur[9];
        forwardKinematics<T>(theta, pos, Rcur);
        // Position error
        err[0] = targetPos[0] - pos[0];
        err[1] = targetPos[1] - pos[1];
        err[2] = targetPos[2] - pos[2];
        // Orientation error: R_err = Rg · Rcur^T
        T Rerr[9];
        for(int i=0;i<3;i++){
            for(int j=0;j<3;j++){
                Rerr[i*3+j] =
                  Rg[i*3+0]*Rcur[j*3+0] +
                  Rg[i*3+1]*Rcur[j*3+1] +
                  Rg[i*3+2]*Rcur[j*3+2];
            }
        }
        // Skew-symmetric extraction
        err[3] = T(0.5) * (Rerr[7] - Rerr[5]);
        err[4] = T(0.5) * (Rerr[2] - Rerr[6]);
        err[5] = T(0.5) * (Rerr[3] - Rerr[1]);
        // Norm
        T normErr = T(0);
        for(int i=0;i<6;i++) normErr += err[i]*err[i];
        normErr = std::sqrt(normErr);
        if(normErr < tol){
            detail.iterationCount  = iter;
            detail.finalErrorNorm  = static_cast<float>(normErr);
            // Fill solution
            for(int i=0;i<6;i++) outSol[i] = theta[i];
            // Compute joint velocities: J·vel = cartTwist
            computeAnalyticJacobian<T>(theta, J);
            solveLinearSystem<T>(J, cartTwist, outVel);
            return ErrorCode::Success;
        }
        if(normErr < bestErr) bestErr = normErr;
        // Jacobian
        computeAnalyticJacobian<T>(theta, J);
        // Solve for delta
        if(!solveLinearSystem<T>(J, err, delta)){
            detail.iterationCount  = iter;
            detail.finalErrorNorm  = static_cast<float>(normErr);
            return ErrorCode::SingularJacobian;
        }
        // Update
        for(int i=0;i<6;i++) theta[i] += delta[i];
    }
    // No convergence
    detail.iterationCount = maxIter;
    detail.finalErrorNorm = static_cast<float>(bestErr);
    return ErrorCode::NoConvergence;
}

void InverseKinematics::benchmark() {

    // examle DH parameters for a 6-axis robot arm (motoman UP6)
    std::array<DHParam,6> dhParams = {{
        {0.150f, -M_PI_2, 0.0f, 0.0f}, // Joint 1
        {0.570f, 0.0f, 0.0f, -M_PI_2f}, // Joint 2
        {0.130f, -M_PI_2, 0.0f, 0.0f}, // Joint 3
        {0.0f, M_PI_2, 0.640f, 0.0f}, // Joint 4
        {0.0f, -M_PI_2, 0.0f, -M_PI_2}, // Joint 5
        {0.0f, 0.0f, 0.095f, 0.0f} // Joint 6
    }};

    const std::array<float, 3> endPos = {0.5, 0.0, 0.5};
    uint32_t interpolationSteps = 1000;

    setDHParameters(dhParams);
    std::array<float, 6> currentAngles = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    std::array<float, 3> currentPos;
    std::array<float, 9> currentRot;

    // get initial starting state (and benchmark forward kinematics)
    forwardKinematicsF32(currentAngles, currentPos, currentRot);

    auto startTime = std::chrono::high_resolution_clock::now();
    for(uint32_t i = 0; i < interpolationSteps; i++) {
        // Forward kinematics to get the current position
        forwardKinematicsF32(currentAngles, currentPos, currentRot);
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    

    std::cout << "Initial forward kinematics benchmark completed in "
              << std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count()
              << " us for " << interpolationSteps << " steps." << std::endl;

    std::array<float, 3> interpolatedPos;

    std::array<float, 3> targetEuler = {0.0, 0.0, 0.0}; // No rotation for simplicity
    std::array<float, 6> cartTwist = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // No twist
    std::array<float, 6> outSolution;
    std::array<float, 6> outVelocities;
    IKErrorDetail detail;

    
    startTime = std::chrono::high_resolution_clock::now();
    for(uint32_t i = 0; i < interpolationSteps; i++) {

        // Interpolate position
        for(int j = 0; j < 3; j++) {
            interpolatedPos[j] = currentPos[j] + (endPos[j] - currentPos[j]) * (static_cast<float>(i) / interpolationSteps);
        }

        // Solve IK for interpolated position
        
        ErrorCode ec = solve(
            interpolatedPos, targetEuler, currentAngles, cartTwist,
            outSolution, outVelocities, detail, 50, 1e-4
        );
        if(ec != ErrorCode::Success) {
            std::cerr << "IK solve failed at step " << i << ": "
                      << static_cast<int>(ec) << ", iterations: "
                      << detail.iterationCount << ", error norm: "
                      << detail.finalErrorNorm << std::endl;
            continue;
        }
        
    }
    endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cout << "Benchmark completed in " << duration << " us for "
              << interpolationSteps << " steps." << std::endl;

}