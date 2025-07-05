#pragma once
#include <array>
#include <cmath>

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

struct quat_rot {
    float w,x,y,z;  ///< Quaternion components (w, x, y, z) representing rotation
};
struct euler_rot {
    float roll;   ///< Roll angle (x-axis rotation)
    float pitch;  ///< Pitch angle (y-axis rotation)
    float yaw;    ///< Yaw angle (z-axis rotation)
};

static quat_rot quatFromZYX(float z, float y, float x) {
    float c1 = cosf(z/2), s1 = sinf(z/2);
    float c2 = cosf(y/2), s2 = sinf(y/2);
    float c3 = cosf(x/2), s3 = sinf(x/2);

    quat_rot q;
    q.w = c1*c2*c3 + s1*s2*s3;
    q.x = c1*c2*s3 - s1*s2*c3;
    q.y = c1*s2*c3 + s1*c2*s3;
    q.z = s1*c2*c3 - c1*s2*s3;
    return q;
}

static void toEulerZYX(const quat_rot* q, float* yaw, float* pitch, float* roll) {
    float sinθ =  2*(q->w*q->y - q->z*q->x);
    *pitch = asinf( fmin(fmax(sinθ, -1.0f), 1.0f)) ;

    *yaw = atan2f( 2*(q->w*q->z + q->x*q->y),
                1 - 2*(q->y*q->y + q->z*q->z) );

    *roll = atan2f( 2*(q->w*q->x + q->y*q->z),
                1 - 2*(q->x*q->x + q->y*q->y) );
}

static quat_rot multiply(const quat_rot &A, const quat_rot &B) {
    return quat_rot{
        A.w*B.w - A.x*B.x - A.y*B.y - A.z*B.z,
        A.w*B.x + A.x*B.w + A.y*B.z - A.z*B.y,
        A.w*B.y - A.x*B.z + A.y*B.w + A.z*B.x,
        A.w*B.z + A.x*B.y - A.y*B.x + A.z*B.w
    };
}

static quat_rot inverse(const quat_rot &q) {
    return quat_rot{q.w, -q.x, -q.y, -q.z};  // conjugate for unit quaternions
}

static float wrapPi(float a) {
    a = std::fmod(a + M_PI, 2*M_PI);
    if(a < 0) a += 2*M_PI;
    return a - M_PI;
}

// Build a 4×4 from pos[3] and ZYX Euler [yaw, pitch, roll]
struct Mat4 { float m[4][4]; };

static Mat4 poseToMatrix(const std::array<float,3>& pos,
                  const std::array<float,3>& zyx) {
    float φ = zyx[0], θ = zyx[1], ψ = zyx[2];
    float c1 = cosf(φ), s1 = sinf(φ);
    float c2 = cosf(θ), s2 = sinf(θ);
    float c3 = cosf(ψ), s3 = sinf(ψ);

    // R = Rz(φ) * Ry(θ) * Rx(ψ)
    Mat4 T{};
    T.m[0][0] = c1*c2;
    T.m[0][1] = c1*s2*s3 - s1*c3;
    T.m[0][2] = c1*s2*c3 + s1*s3;
    T.m[1][0] = s1*c2;
    T.m[1][1] = s1*s2*s3 + c1*c3;
    T.m[1][2] = s1*s2*c3 - c1*s3;
    T.m[2][0] =    -s2;
    T.m[2][1] =     c2*s3;
    T.m[2][2] =     c2*c3;

    // translation
    T.m[0][3] = pos[0];
    T.m[1][3] = pos[1];
    T.m[2][3] = pos[2];

    // bottom row
    T.m[3][0] = T.m[3][1] = T.m[3][2] = 0.0f;
    T.m[3][3] = 1.0f;
    return T;
}

// Extract pos + ZYX Euler from 4×4 matrix
static void matrixToPose(const Mat4& T,
                  std::array<float,3>& pos,
                  std::array<float,3>& zyx) {
    pos = { T.m[0][3], T.m[1][3], T.m[2][3] };
    // pitch = asin(-r20)
    float sy = -T.m[2][0];
    zyx[1] = asinf(fmin(fmax(sy, -1.0f), 1.0f));
    // yaw = atan2(r10, r00)
    zyx[0] = atan2f(T.m[1][0], T.m[0][0]);
    // roll = atan2(r21, r22)
    zyx[2] = atan2f(T.m[2][1], T.m[2][2]);
    // wrap all three
    for(auto &a : zyx) a = wrapPi(a);
}

// Multiply A·B → C
static Mat4 mul(const Mat4 &A, const Mat4 &B) {
    Mat4 C{};
    for(int i=0;i<4;i++){
      for(int j=0;j<4;j++){
        float s=0;
        for(int k=0;k<4;k++) s += A.m[i][k]*B.m[k][j];
        C.m[i][j]=s;
      }
    }
    return C;
}

// Invert a 4×4 rigid‐body transform (R|p) easily:
static Mat4 invertRigid(const Mat4 &T) {
    Mat4 Rinv{}, Tout{};
    // transpose upper 3×3
    for(int i=0;i<3;i++) for(int j=0;j<3;j++) Rinv.m[i][j] = T.m[j][i];
    // new translation = -R^T * p
    for(int i=0;i<3;i++){
      float s=0;
      for(int j=0;j<3;j++) s += -Rinv.m[i][j]*T.m[j][3];
      Rinv.m[i][3] = s;
    }
    // bottom row
    Rinv.m[3][0] = Rinv.m[3][1] = Rinv.m[3][2] = 0;
    Rinv.m[3][3] = 1;
    return Rinv;
}

// --- User‐facing APIs ---

/// @param flangePos   flange XYZ
/// @param flangeEul   flange ZYX [yaw,pitch,roll]
/// @param toolPos     tool offset XYZ in flange frame
/// @param toolEul     tool offset ZYX Euler
/// @param tcpPos      (out) TCP XYZ in world
/// @param tcpEul      (out) TCP Euler in world
static void applyTool(const std::array<float,3>& flangePos,
               const std::array<float,3>& flangeEul,
               const std::array<float,3>& toolPos,
               const std::array<float,3>& toolEul,
               std::array<float,3>& tcpPos,
               std::array<float,3>& tcpEul)
{
    auto T_flange = poseToMatrix(flangePos, flangeEul);
    auto T_tool   = poseToMatrix(toolPos,   toolEul);
    auto T_tcp    = mul(T_flange, T_tool);
    matrixToPose(T_tcp, tcpPos, tcpEul);
}

/// @param tcpPos      desired TCP XYZ in world
/// @param tcpEul      desired TCP ZYX [yaw,pitch,roll]
/// @param toolPos     tool offset XYZ in flange frame
/// @param toolEul     tool offset ZYX Euler
/// @param flangePos   (out) computed flange XYZ in world (to feed IK)
/// @param flangeEul   (out) computed flange ZYX Euler
static void removeTool(const std::array<float,3>& tcpPos,
                const std::array<float,3>& tcpEul,
                const std::array<float,3>& toolPos,
                const std::array<float,3>& toolEul,
                std::array<float,3>& flangePos,
                std::array<float,3>& flangeEul)
{
    auto T_tcp    = poseToMatrix(tcpPos, tcpEul);
    auto T_tool   = poseToMatrix(toolPos,   toolEul);
    auto T_flange = mul(T_tcp, invertRigid(T_tool));
    matrixToPose(T_flange, flangePos, flangeEul);
}
