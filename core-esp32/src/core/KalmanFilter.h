//
// Created by Ivan Kishchenko on 2/4/24.
//

#pragma once

#include <array>
#include <math.h>


template<int nostates, int nobsers>
class KalmanFilter {
public:
    std::array<float, nostates * nostates> A{};
    std::array<float, nostates * nostates> P{};
    std::array<float, nostates * nostates> Q{};
    std::array<float, nostates * nobsers> H{};

    std::array<float, nostates> xp{};
    std::array<float, nostates> xc{};

    std::array<float, nobsers * nobsers> R{};

    KalmanFilter() = default;

    void zeros() {
        std::fill(A.begin(), A.end(), 0.0f);
        std::fill(Q.begin(), Q.end(), 0.0f);
        std::fill(P.begin(), P.end(), 0.0f);
        std::fill(H.begin(), H.end(), 0.0f);
        std::fill(R.begin(), R.end(), 0.0f);
        std::fill(xp.begin(), xp.end(), 0.0f);
        std::fill(xc.begin(), xc.end(), 0.0f);

        A[0] = 1.f;
        H[0] = 1.0f;
        Q[0] = 1.6f;
        R[0] = 6.0f;
        P[0] = 1.0f;
    }

    float *predict() {
        mulmat(A.data(), xc.data(), xp.data(), nostates, nostates, 1);
        return xp.data();
    }

    float *correct(float *z) {
        float tmp0[nostates * nostates];
        float tmp1[nostates * nobsers];
        float tmp2[nobsers * nostates];
        float tmp3[nobsers * nobsers];
        float tmp4[nobsers * nobsers];
        float tmp5[nobsers * nobsers];
        float tmp6[nobsers];
        float Ht[nostates * nobsers];
        float At[nostates * nostates];
        float Pp[nostates * nostates];
        float K[nostates * nobsers];

        /* P_k = F_{k-1} P_{k-1} F^T_{k-1} + Q_{k-1} */
        mulmat(A.data(), P.data(), tmp0, nostates, nostates, nostates);
        transpose(A.data(), At, nostates, nostates);
        mulmat(tmp0, At, Pp, nostates, nostates, nostates);
        accum(Pp, Q.data(), nostates, nostates);

        /* G_k = P_k H^T_k (H_k P_k H^T_k + R)^{-1} */
        transpose(H.data(), Ht, nobsers, nostates);
        mulmat(Pp, Ht, tmp1, nostates, nostates, nobsers);

        mulmat(H.data(), Pp, tmp2, nobsers, nostates, nostates);
        mulmat(tmp2, Ht, tmp3, nobsers, nostates, nobsers);
        accum(tmp3, R.data(), nobsers, nobsers);

        if (cholsl(tmp3, tmp4, tmp5, nobsers)) {
            return nullptr;
        }
        mulmat(tmp1, tmp4, K, nostates, nobsers, nobsers);

        /* \hat{x}_k = \hat{x_k} + G_k(z_k - h(\hat{x}_k)) */
        mulmat(H.data(), xp.data(), tmp6, nobsers, nostates, nostates);
        sub(z, tmp6, tmp5, nobsers);
        mulvec(K, tmp5, tmp2, nostates, nobsers);
        add(xp.data(), tmp2, xc.data(), nostates);

        /* P_k = (I - G_k H_k) P_k */
        mulmat(K, H.data(), tmp0, nostates, nobsers, nostates);
        negate(tmp0, nostates, nostates);
        mat_addeye(tmp0, nostates);
        mulmat(tmp0, Pp, P.data(), nostates, nostates, nostates);

        return xc.data();
    }

private:
    static void zeros(float *a, int m, int n) {
        int j;
        for (j = 0; j < m * n; ++j)
            a[j] = 0;
    }


    static int choldc1(float *a, float *p, int n) {
        int i, j, k;
        float sum;

        for (i = 0; i < n; i++) {
            for (j = i; j < n; j++) {
                sum = a[i * n + j];
                for (k = i - 1; k >= 0; k--) {
                    sum -= a[i * n + k] * a[j * n + k];
                }
                if (i == j) {
                    if (sum <= 0) {
                        return 1; /* error */
                    }
                    p[i] = sqrt(sum);
                } else {
                    a[j * n + i] = sum / p[i];
                }
            }
        }

        return 0; /* success */
    }

    static int choldcsl(float *A, float *a, float *p, int n) {
        int i, j, k;
        float sum;
        for (i = 0; i < n; i++)
            for (j = 0; j < n; j++)
                a[i * n + j] = A[i * n + j];
        if (choldc1(a, p, n)) return 1;
        for (i = 0; i < n; i++) {
            a[i * n + i] = 1 / p[i];
            for (j = i + 1; j < n; j++) {
                sum = 0;
                for (k = i; k < j; k++) {
                    sum -= a[j * n + k] * a[k * n + i];
                }
                a[j * n + i] = sum / p[j];
            }
        }

        return 0; /* success */
    }

    static int cholsl(float *A, float *a, float *p, int n) {
        int i, j, k;
        if (choldcsl(A, a, p, n)) return 1;
        for (i = 0; i < n; i++) {
            for (j = i + 1; j < n; j++) {
                a[i * n + j] = 0.0;
            }
        }
        for (i = 0; i < n; i++) {
            a[i * n + i] *= a[i * n + i];
            for (k = i + 1; k < n; k++) {
                a[i * n + i] += a[k * n + i] * a[k * n + i];
            }
            for (j = i + 1; j < n; j++) {
                for (k = j; k < n; k++) {
                    a[i * n + j] += a[k * n + i] * a[k * n + j];
                }
            }
        }
        for (i = 0; i < n; i++) {
            for (j = 0; j < i; j++) {
                a[i * n + j] = a[j * n + i];
            }
        }

        return 0; /* success */
    }

    /* C <- A * B */
    static void mulmat(float *a, float *b, float *c, int arows, int acols, int bcols) {
        int i, j, l;

        for (i = 0; i < arows; ++i)
            for (j = 0; j < bcols; ++j) {
                c[i * bcols + j] = 0;
                for (l = 0; l < acols; ++l)
                    c[i * bcols + j] += a[i * acols + l] * b[l * bcols + j];
            }
    }

    static void mulvec(float *a, float *x, float *y, int m, int n) {
        int i, j;

        for (i = 0; i < m; ++i) {
            y[i] = 0;
            for (j = 0; j < n; ++j)
                y[i] += x[j] * a[i * n + j];
        }
    }

    static void transpose(float *a, float *at, int m, int n) {
        int i, j;

        for (i = 0; i < m; ++i)
            for (j = 0; j < n; ++j) {
                at[j * m + i] = a[i * n + j];
            }
    }

/* A <- A + B */
    static void accum(float *a, float *b, int m, int n) {
        int i, j;

        for (i = 0; i < m; ++i)
            for (j = 0; j < n; ++j)
                a[i * n + j] += b[i * n + j];
    }

/* C <- A + B */
    static void add(float *a, float *b, float *c, int n) {
        int j;

        for (j = 0; j < n; ++j)
            c[j] = a[j] + b[j];
    }


/* C <- A - B */
    static void sub(float *a, float *b, float *c, int n) {
        int j;

        for (j = 0; j < n; ++j)
            c[j] = a[j] - b[j];
    }

    static void negate(float *a, int m, int n) {
        int i, j;

        for (i = 0; i < m; ++i)
            for (j = 0; j < n; ++j)
                a[i * n + j] = -a[i * n + j];
    }

    static void mat_addeye(float *a, int n) {
        int i;
        for (i = 0; i < n; ++i)
            a[i * n + i] += 1;
    }
};