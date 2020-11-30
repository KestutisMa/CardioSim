#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include "matrix_exponential.hpp"
#include "r8lib.hpp"

using namespace std;
// void expm(double *p, double *Q);

// gaussian slover (for steady state calc)
double *gauss(double b[4], double Q[4 * 4])
{
    double a[4 * (4 + 1)];
    int i, j, k, n = 4; // declare variables and matrixes as input
    double bb;
    double *x = new double[4];

    for (j = 0; j < n; j++)
        for (i = 0; i < n + 1; i++)
            a[j * (4 + 1) + i] = (i < n) ? Q[j * 4 + i] : b[j];
    //to find the elements of diagonal matrix
    for (j = 0; j < n; j++)
    {
        for (i = 0; i < n; i++)
        {
            if (i != j)
            {
                bb = a[i * (n + 1) + j] / a[j * (n + 1) + j];
                for (k = 1; k <= n + 1; k++)
                {
                    a[i * (n + 1) + k] = a[i * (n + 1) + k] - bb * a[j * (n + 1) + k];
                }
            }
        }
    }
    for (i = 0; i < n; i++)
    {
        x[i] = a[i * (n + 1) + n + 1] / a[i * (n + 1) + i];
    }
    return x;
}

void printM_col(double *A, int n, int m)
{
    // col major
    cout << endl;
    for (int i = 0; i < n; i++)
    {
        // printf("\n");
        for (int j = 0; j < m; j++)
        {
            // printf("%f ", A[i+j*n]);
            cout << A[i + j * n] << " ";
        }
        cout << endl;
    }
}

void printM_row(double *A, int n, int m)
{
    // row mjr
    cout << endl;
    for (int i = 0; i < n; i++)
    {
        // printf("\n");
        for (int j = 0; j < m; j++)
        {
            // printf("%f ", A[i+j*n]);
            cout << A[i * m + j] << "\t\t";
        }
        cout << endl;
    }
}

// C(n,m) = A(n,x) dot B(x,m) , n m x - dimensions
void matmul(int n, int m, int x, double *A, double *B, double *C)
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
        {
            C[i * m + j] = 0;
            for (int k = 0; k < x; k++)
                C[i * m + j] += A[i * x + k] * B[k * m + j];
        }
}

int factorial(int a)
{
    int f = 1;
    for (int i = 2; i <= a; i++)
        f *= i;
    return f;
}

// mat[dim,dim] exp: E = e^A
void expm(double *A, double *E, int dim)
{
    // printf("\nhi\n");
    const int prec = 10;                   // number of series elements (precision)
    double *A_acc = new double[dim * dim]; // accumulator for power
    double *A_next = new double[dim * dim];
    for (int i = 0; i < dim; i++)
        for (int j = 0; j < dim; j++)
        {
            E[i * dim + j] = i == j ? 1.f : 0.f;
            A_acc[i * dim + j] = A[i * dim + j];
        }
    for (int p = 1; p < prec; p++)
    {
        for (int i = 0; i < dim; i++)
            for (int j = 0; j < dim; j++)
                E[i * dim + j] += A_acc[i * dim + j] / factorial(p);
        matmul(dim, dim, dim, A, A_acc, A_next); // ar gerai?
        // printf("%d\n", p);
        // printM(A_next, 2,2);
        for (int i = 0; i < dim; i++) // perrasom A_acc, isvalom A_next, (galbut galima tik keist pointer'ius?)
            for (int j = 0; j < dim; j++)
            {
                A_acc[i * dim + j] = A_next[i * dim + j];
                A_next[i * dim + j] = 0;
            }
    }
}

// void main1() {
//     #define n 3
//     #define m 4
//     // double a[n*m] = {//row major
//     //     1, 2, -4, 2,
//     //     7, 6, -2, -5,
//     //     0, -3, -5, -8
//     //     };
//     double a[n*m] = { //column major
//         1,7,0,
//         2,6,-3,
//         -4,-2,-5,
//         2,-5,-8
//                     };
//     //double a[4*4] = {.1, .2, .3, .4,
//     //             .1, .2, .3, .4,
//     //             .1, .2, .3, .4,
//     //             .1, .2, .3, .4 };

// //     // double AA[2][2] = {{2,0},{0,3}};
// //     // double AA[2][2] = {{1,2},{3,4}};//{{2,0},{0,3}};
// //     // double BB[2][2] = {{5,6},{7,8}};
// //     double *A = new double[n*m];
// //     // double *B = new double[n*m];
// //     double *C = new double[4*4];
// //     for (int i = 0; i < n; i++)
// //             for (int j = 0; j < m; j++) {
// //                 A[i*m+j] = a[i*4+j];
// //                 // B[i*m+j] = BB[i][j];
// //                 C[i*m+j] = 0;
// //             }
// //     // matmul(2, 2, 2, A, B, C);
// //     // expm(A, C, 2);
// //     C = r8mat_expm1(4,a);
//     // printM(a, n, m);
//     r8mat_solve(3,1,a);
//     cout << "\nDone.";
//     // printM(a,n,m);
//     cout << "\nDone.";
// }

void solve(int nn, double a[], double b[], double x[])
{
    int i, j;
    double s;

    for (i = 0; i < nn; i++)
    {
        s = 0;
        for (j = 0; j < i; j++)
        {
            s = s + a[i * nn + j] * x[j];
        }
        x[i] = (b[i] - s) / a[i * nn + i];
    }
}

void gauss1(int n, double mat[], double x[])
{
    // const int n = 4;
    int i, j, k;

    // mat[n*(n+1)];

    for (i = 0; i < n; i++)
    {
        for (j = i + 1; j < n; j++)
        {
            if (abs(mat[i * (n + 1) + i]) < abs(mat[j * (n + 1) + i]))
            {
                for (k = 0; k < n + 1; k++)
                {
                    /* swapping mat[i*(n+1)+k] and mat[j*(n+1)+k] */
                    mat[i * (n + 1) + k] = mat[i * (n + 1) + k] + mat[j * (n + 1) + k];
                    mat[j * (n + 1) + k] = mat[i * (n + 1) + k] - mat[j * (n + 1) + k];
                    mat[i * (n + 1) + k] = mat[i * (n + 1) + k] - mat[j * (n + 1) + k];
                }
            }
        }
    }

    /* performing Gaussian elimination */
    for (i = 0; i < n - 1; i++)
    {
        for (j = i + 1; j < n; j++)
        {
            double f = mat[j * (n + 1) + i] / mat[i * (n + 1) + i];
            for (k = 0; k < n + 1; k++)
            {
                mat[j * (n + 1) + k] = mat[j * (n + 1) + k] - f * mat[i * (n + 1) + k];
            }
        }
    }
    /* Backward substitution for discovering values of unknowns */
    for (i = n - 1; i >= 0; i--)
    {
        x[i] = mat[i * (n + 1) + n];

        for (j = i + 1; j < n; j++)
        {
            if (i != j)
            {
                x[i] = x[i] - mat[i * (n + 1) + j] * x[j];
            }
        }
        x[i] = x[i] / mat[i * (n + 1) + i];
    }
}

double mc4sm_ss(double Vj, double *par, double *p_steady_state, double limit)
{
    // returns gj, *p_steady_state
    // % Vj gating parameters
    double lamda1 = par[0 * 7 + 0];  // lamda1 = par(1,1);  % opening and closing intensity rates when V=V0, in s^-1
    double A_alfa1 = par[0 * 7 + 1]; // A_alfa1 = par(1,2); % closing rate sensitivities to voltage, in mV^-1
    double A_beta1 = par[0 * 7 + 2]; // A_beta1 = par(1,3); % opening rate sensitivities to voltage, in mV^-1
    double V_01 = par[0 * 7 + 3];    // V_01 = par(1,4);    % voltages, when  opening rate equals closing rate, in mV
    double P_g1 = par[0 * 7 + 4];    // P_g1 = par(1,5);
    double G_o1 = par[0 * 7 + 5];    // G_o1 = par(1,6);  % maximum open state conductances, in nS
    double G_c1 = par[0 * 7 + 6];    // G_c1 = par(1,7);  % minimum closed state conductances, in nS

    double lamda2 = par[1 * 7 + 0];  // lamda1 = par(1,1);  % opening and closing intensity rates when V=V0, in s^-1
    double A_alfa2 = par[1 * 7 + 1]; // A_alfa1 = par(1,2); % closing rate sensitivities to voltage, in mV^-1
    double A_beta2 = par[1 * 7 + 2]; // A_beta1 = par(1,3); % opening rate sensitivities to voltage, in mV^-1
    double V_02 = par[1 * 7 + 3];    // V_01 = par(1,4);    % voltages, when  opening rate equals closing rate, in mV
    double P_g2 = par[1 * 7 + 4];    // P_g1 = par(1,5);
    double G_o2 = par[1 * 7 + 5];    // G_o1 = par(1,6);  % maximum open state conductances, in nS
    double G_c2 = par[1 * 7 + 6];    // G_c1 = par(1,7);  % minimum closed state conductances, in nS

    double V[4][2] = {{Vj * G_o2 / (G_o1 + G_o2), -Vj * G_o1 / (G_o1 + G_o2)},
                      {Vj * G_c2 / (G_o1 + G_c2), -Vj * G_o1 / (G_o1 + G_c2)},
                      {Vj * G_o2 / (G_c1 + G_o2), -Vj * G_c1 / (G_c1 + G_o2)},
                      {Vj * G_c2 / (G_c1 + G_c2), -Vj * G_c1 / (G_c1 + G_c2)}};

    double Q[4 * 4] = {
        -lamda2 * exp(A_beta2 * P_g2 * (V[0][1] - V_02)) - lamda1 * exp(A_beta1 * P_g1 * (V[0][0] - V_01)), lamda2 * exp(A_beta2 * P_g2 * (V[0][1] - V_02)), lamda1 * exp(A_beta1 * P_g1 * (V[0][0] - V_01)), 0,
        lamda2 * exp(-A_alfa2 * P_g2 * (V[1][1] - V_02)), -lamda2 * exp(-A_alfa2 * P_g2 * (V[1][1] - V_02)) - lamda1 * exp(A_beta1 * P_g1 * (V[1][0] - V_01)), 0, lamda1 * exp(A_beta1 * P_g1 * (V[1][0] - V_01)),
        lamda1 * exp(-A_alfa1 * P_g1 * (V[2][0] - V_01)), 0, -lamda1 * exp(-A_alfa1 * P_g1 * (V[2][0] - V_01)) - lamda2 * exp(A_beta2 * P_g2 * (V[2][1] - V_02)), lamda2 * exp(A_beta2 * P_g2 * (V[2][1] - V_02)),
        0, lamda1 * exp(-A_alfa1 * P_g1 * (V[3][0] - V_01)), lamda2 * exp(-A_alfa2 * P_g2 * (V[3][1] - V_02)), -lamda1 * exp(-A_alfa1 * P_g1 * (V[3][0] - V_01)) - lamda2 * exp(-A_alfa2 * P_g2 * (V[3][1] - V_02))};

    //Q=Q-Q.*(Q>limit)+limit*(Q>limit);
    // for (int i = 0; i < 4 * 4; i++)
    //     if (Q[i] > limit)
    //     {
    //         cout << "Q[i]=" << Q[i];
    //         Q[i] = limit;
    //     }

    //Q=Q+diag(-sum(Q'));
    // double Q_sum[4] = {};
    // for (int i = 0; i < 4; i++)
    //     for (int j = 0; j < 4; j++)
    //         Q_sum[i] += Q[i * 4 + j];
    // for (int i = 0; i < 4; i++)
    //     for (int j = 0; j < 4; j++)
    //         if (i == j)
    //             Q[i * 4 + j] -= Q_sum[i];

    double ggg[4] = {G_o1 * G_o2 / (G_o1 + G_o2), G_o1 * G_c2 / (G_o1 + G_c2), G_c1 * G_o2 / (G_c1 + G_o2), G_c1 * G_c2 / (G_c1 + G_c2)};

    //p_steady_state = StacionariosTikimybesQ(Q);
    // 3 -1 7
    // 2 3 1

    // double b[4] = {7,1};
    double x[4] = {};
    // double Q_[3*4] = {
    //     3,-1e-6,6,7,
    //     2,3,9,1,
    //     1,2,3,2,
    //     };
    // printM_row(Q_, 3, 4);
    // gauss1(3,Q_,x);
    // printM_row(x, 1, 3);
    // exit(0);
    double b[4] = {1, 0, 0, 0};
    double Q_[4 * 5] = {};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 5; j++)
        {
            if (i == 0)
                Q_[i * 5 + j] = 1;
            else if (j == 4)       // maybe only: if (i != 4) Q_[i*5+j] = Q[j*4+i]; else is not nescessary, cause Q is initalized to 0
                Q_[i * 5 + j] = 0; //b[i];// equivalent (i == 1) ? 1 : 0;
            else
                Q_[i * 5 + j] = Q[j * 4 + i];
        }

    // printM_row(Q_, 4, 5);
    gauss1(4, Q_, x);
    // printM_row(x, 1, 4);
    // exit(0);
    // double tmp[4*5] = {}; //construct matrix for r8mat_solve
    // for (int i = 0; i < 4; i++)
    //     for (int j = 0; j < 5; j++)
    //         if (j == 4 || j == 0)
    //             tmp[i+j*4] = 1;
    //         else
    //             tmp[i+j*4] = Q[i*4+j];
    // printM_row(Q, 4, 4);
    // printM_col(tmp, 4, 5);
    // r8mat_solve(4,1,tmp);//gauss(b, Q);
    // printM_col(tmp, 4, 5);
    // system("pause");
    for (int i = 0; i < 4; i++)
        p_steady_state[i] = x[i]; //tmp[i*4];
    // printM_row(p_steady_state, 1, 4);

    double gj = 0; // gj=p*ggg';
    for (int i = 0; i < 4; i++)
        gj += p_steady_state[i] * ggg[i];

    return gj;
}

double mc4sm(double Vj, double dt, double *par, double *p, double limit)
{ // function [gj, p_next] = MC4SM(Vj, dt, par, p)
    // % Vj gating parameters
    double lamda1 = par[0 * 7 + 0];  // lamda1 = par(1,1);  % opening and closing intensity rates when V=V0, in s^-1
    double A_alfa1 = par[0 * 7 + 1]; // A_alfa1 = par(1,2); % closing rate sensitivities to voltage, in mV^-1
    double A_beta1 = par[0 * 7 + 2]; // A_beta1 = par(1,3); % opening rate sensitivities to voltage, in mV^-1
    double V_01 = par[0 * 7 + 3];    // V_01 = par(1,4);    % voltages, when  opening rate equals closing rate, in mV
    double P_g1 = par[0 * 7 + 4];    // P_g1 = par(1,5);
    double G_o1 = par[0 * 7 + 5];    // G_o1 = par(1,6);  % maximum open state conductances, in nS
    double G_c1 = par[0 * 7 + 6];    // G_c1 = par(1,7);  % minimum closed state conductances, in nS

    double lamda2 = par[1 * 7 + 0];  // lamda1 = par(1,1);  % opening and closing intensity rates when V=V0, in s^-1
    double A_alfa2 = par[1 * 7 + 1]; // A_alfa1 = par(1,2); % closing rate sensitivities to voltage, in mV^-1
    double A_beta2 = par[1 * 7 + 2]; // A_beta1 = par(1,3); % opening rate sensitivities to voltage, in mV^-1
    double V_02 = par[1 * 7 + 3];    // V_01 = par(1,4);    % voltages, when  opening rate equals closing rate, in mV
    double P_g2 = par[1 * 7 + 4];    // P_g1 = par(1,5);
    double G_o2 = par[1 * 7 + 5];    // G_o1 = par(1,6);  % maximum open state conductances, in nS
    double G_c2 = par[1 * 7 + 6];    // G_c1 = par(1,7);  % minimum closed state conductances, in nS

    double V[4][2] = {{Vj * G_o2 / (G_o1 + G_o2), -Vj * G_o1 / (G_o1 + G_o2)},
                      {Vj * G_c2 / (G_o1 + G_c2), -Vj * G_o1 / (G_o1 + G_c2)},
                      {Vj * G_o2 / (G_c1 + G_o2), -Vj * G_c1 / (G_c1 + G_o2)},
                      {Vj * G_c2 / (G_c1 + G_c2), -Vj * G_c1 / (G_c1 + G_c2)}};

    double Q[4 * 4] = {-lamda2 * exp(A_beta2 * P_g2 * (V[0][1] - V_02)) - lamda1 * exp(A_beta1 * P_g1 * (V[0][0] - V_01)), lamda2 * exp(A_beta2 * P_g2 * (V[0][1] - V_02)), lamda1 * exp(A_beta1 * P_g1 * (V[0][0] - V_01)), 0,
                       lamda2 * exp(-A_alfa2 * P_g2 * (V[1][1] - V_02)), -lamda2 * exp(-A_alfa2 * P_g2 * (V[1][1] - V_02)) - lamda1 * exp(A_beta1 * P_g1 * (V[1][0] - V_01)), 0, lamda1 * exp(A_beta1 * P_g1 * (V[1][0] - V_01)),
                       lamda1 * exp(-A_alfa1 * P_g1 * (V[2][0] - V_01)), 0, -lamda1 * exp(-A_alfa1 * P_g1 * (V[2][0] - V_01)) - lamda2 * exp(A_beta2 * P_g2 * (V[2][1] - V_02)), lamda2 * exp(A_beta2 * P_g2 * (V[2][1] - V_02)),
                       0, lamda1 * exp(-A_alfa1 * P_g1 * (V[3][0] - V_01)), lamda2 * exp(-A_alfa2 * P_g2 * (V[3][1] - V_02)), -lamda1 * exp(-A_alfa1 * P_g1 * (V[3][0] - V_01)) - lamda2 * exp(-A_alfa2 * P_g2 * (V[3][1] - V_02))};

    // Q = Q - Q.*(Q > limit) + limit * (Q > limit);
    for (int i = 0; i < 4 * 4; i++)
        if (Q[i] > limit)
        {
            cout << "Q[i]=" << Q[i];
            Q[i] = limit;
        }

    //Q=Q+diag(-sum(Q'));
    // double Q_sum[4] = {};
    // for (int i = 0; i < 4; i++)
    //     for (int j = 0; j < 4; j++)
    //         Q_sum[i] += Q[i * 4 + j];
    // for (int i = 0; i < 4; i++)
    //     for (int j = 0; j < 4; j++)
    //         if (i == j)
    //             Q[i * 4 + j] -= Q_sum[i];

    // int i = exp(2);

    // P=expm(Q*dt);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            Q[i * 4 + j] *= dt;
    double *P;
    P = r8mat_expm1(4, Q);
    double *p_next = new double[4];
    for (int k = 0; k < 4; k++)
        p_next[k] = 0;
    //r8mat_mm(1,4,4,p,P,p_next); //p_next = p*P;
    for (int k = 0; k < 4; k++)
        for (int j = 0; j < 4; j++)
            p_next[k] += p[j] * P[j * 4 + k];

    // printM_row(P,4,4);
    // printM_row(p,1,4);
    // printM_row(p_next,1,4);
    // exit(0);

    // % laidumas
    // ggg=[G_o1*G_o2/(G_o1+G_o2) G_o1*G_c2/(G_o1+G_c2) G_c1*G_o2/(G_c1+G_o2) G_c1*G_c2/(G_c1+G_c2)];
    double ggg[4] = {G_o1 * G_o2 / (G_o1 + G_o2), G_o1 * G_c2 / (G_o1 + G_c2), G_c1 * G_o2 / (G_c1 + G_o2), G_c1 * G_c2 / (G_c1 + G_c2)};
    // gj=p*ggg';
    double gj = 0;
    for (int i = 0; i < 4; i++)
        // for (int j = 0; j < 4; j++)
        gj += p[i] * ggg[i];

    // printf("\n%f\n",gj);

    for (int i = 0; i < 4; i++)
        p[i] = p_next[i];

    return gj;
}

void main()
{
    // Cx43/45 hetero
    double gmax = 1.0e-9;                                                                        //.e-9;
    double par[14] = {0.0001, 0.0239, 0.0912, -19.0232, -1, 4 * 2 * gmax, 4 * 2 * 0.0773 * gmax, // left side
                      0.1115, 0.0179, 0.1051, -1.6781, -1, 2 * gmax, 2 * 0.0112 * gmax};         // right side
                                                                                                 // limit = 4.9653;     % threshold transition rate

    // double pars[5] = {0.1522, 0.0320, 0.2150, -34.2400, 0.2570};

    // Old
    // double gmax = 1.;
    // double par[14] = {pars[0], pars[1], pars[2], pars[3], -1, 2 * gmax, pars[4] * gmax,  //  % left side
    //                   pars[0], pars[1], pars[2], pars[3], -1, 2 * gmax, pars[4] * gmax}; // % right side

    double dt = 0.9;
    double T = 100;
    int N = T / dt;
    // double t[int(40/0.01)];
    double *V = new double[N];
    double *gj = new double[N];
    double p[4] = {};
    ofstream outFile;
    ifstream vj_file;
    outFile.open("MC4SM_out.csv");
    vj_file.open("vj.csv");
    outFile << "t,vj,gj" << endl;
    V[0] = 0;
    double limit = 10000;
    gj[0] = mc4sm_ss(V[0], par, p, limit);
    for (int i = 0; i < N; i++)
    {
        V[i] = 200. * (i)*dt / T - 100.;
        // if (i*dt > 10 && i*dt < 20)
        //     V[i] = 100;
        // else
        // if (i*dt >= 20 && i*dt < 30)
        //     V[i] = -100;
        // else
        //     V[i] = 0;
        // gj[i] = mc4sm_ss(V[i], par, p, limit);
        gj[i] = mc4sm(V[i], dt, par, p, limit);
        outFile << i * dt << ',' << V[i] << ',' << gj[i] << endl;
        // printf("i=%d\n",i);
    }
    outFile.close();
    cout << "Done.";
}