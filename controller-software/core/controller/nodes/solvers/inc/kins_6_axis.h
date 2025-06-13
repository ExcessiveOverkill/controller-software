#pragma once
#include <array>

class InverseKinematics {
public:

    struct DHParam {
        float a;            ///< Link length (meters)
        float alpha;        ///< Link twist (radians)
        float d;            ///< Link offset (meters)
        float thetaOffset;  ///< Constant joint angle offset (radians)
    };

    struct IKErrorDetail {
        int    iterationCount;   ///< Number of iterations performed
        float  finalErrorNorm;   ///< Final Euclidean norm of the [pos;orient] error
    };

    /// Return codes for solve()
    enum class ErrorCode {
        Success = 0,       ///< Converged within tolerance
        NoConvergence,     ///< Exceeded maximum iterations without converging
        SingularJacobian   ///< Jacobian matrix was singular (non-invertible)
    };

    /**
     * @brief Construct the solver with DH parameters for all six joints.
     */
    InverseKinematics();

    /**
     * @brief Solve inverse kinematics via Newton–Raphson.
     * @param targetPos     Desired end-effector position [x, y, z] (m).
     * @param targetEuler   Desired end-effector ZYX Euler angles [φ, θ, ψ] (rad).
     * @param initGuess     Current measured joint angles [θ₁…θ₆] (rad).
     * @param cartTwist     Desired end-effector twist [vx, vy, vz, ωx, ωy, ωz].
     * @param outSolution   Returned joint angles solution (rad).
     * @param outVelocities Returned joint velocities (rad/s) = J⁻¹ · cartTwist.
     * @param detail        Populated only on error with iterationCount & finalErrorNorm.
     * @param maxIter       Maximum Newton iterations (default = 50).
     * @param tolerance     Convergence tolerance on error norm (default = 1e-4).
     * @return ErrorCode    Indicates success or specific failure.
     */
    ErrorCode solve(
        const std::array<float,3>&   targetPos,
        const std::array<float,3>&   targetEuler,
        const std::array<float,6>&   initGuess,
        const std::array<float,6>&   cartTwist,
        std::array<float,6>&         outSolution,
        std::array<float,6>&         outVelocities,
        IKErrorDetail&               detail,
        int                          maxIter    = 50,
        float                        tolerance  = 1e-4f
    ) const;

    void setDHParameters(
        const std::array<DHParam,6>& dhParams
    ) {
        dh_ = dhParams;
    }

    inline void forwardKinematicsF32(
        const std::array<float,6>& angles,
        std::array<float,3>&       posOut,
        std::array<float,9>&       rotOut
    ) const {
        forwardKinematics<float>(angles.data(), posOut.data(), rotOut.data());
    }

    inline void forwardKinematicsF64(
        const std::array<double,6>& angles,
        std::array<double,3>&       posOut,
        std::array<double,9>&       rotOut
    ) const {
        forwardKinematics<double>(angles.data(), posOut.data(), rotOut.data());
    }

    void benchmark();   // for performance testing, not meant for production use

private:
    std::array<DHParam,6> dh_;  ///< Denavit–Hartenberg parameters for 6 joints

    /// Templated core solver (single or double precision)
    template<typename T>
    ErrorCode solveT(
        const T        targetPos[3],
        const T        targetEuler[3],
        const T        initGuess[6],
        const T        cartTwist[6],
        T              outSol[6],
        T              outVel[6],
        IKErrorDetail& detail,
        int            maxIter,
        T              tolerance
    ) const;

    /// Forward kinematics: compute end-effector pos & rotation matrix
    template<typename T>
    void forwardKinematics(
        const T angles[6],
        T       posOut[3],
        T       rotOut[9]
    ) const;

    /// Analytic geometric Jacobian: 6×6 matrix [Jp; Jω]
    template<typename T>
    void computeAnalyticJacobian(
        const T angles[6],
        T       Jout[6][6]
    ) const;

    /// Solve A·x = b for x via Gaussian elimination with partial pivoting
    template<typename T>
    bool solveLinearSystem(
        const T A[6][6],
        const T b[6],
        T       x[6]
    ) const;
};
