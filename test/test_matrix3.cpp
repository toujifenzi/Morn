// g++ -O2 -fopenmp test_matrix3.cpp -o test_matrix3.exe -lclapack -lopenblas -lf2c -lmorn
#define EIGEN_DONT_VECTORIZE
#include "eigen3/Eigen/Dense"
#include "morn_math.h"

#define T 10000

void test_transpose(int K)
{
    printf("matrix transpose, size: %d*%d\n",K,K);
    Eigen::MatrixXf m_a(K,K);
    MMatrix *mat_a = mMatrixCreate(K,K);

    for(int j=0;j<K;j++)for(int i=0;i<K;i++)
    {
        float v=(float)mRand()/(float)mRand();
        m_a(j,i)=v;
        mat_a->data[j][i]=v;
    }

    Eigen::MatrixXf m_b;
    mTimerBegin("eigen");
    for(int i=0;i<T;i++)
        m_b = m_a.transpose();
    mTimerEnd("eigen");

    MMatrix *mat_b=mMatrixCreate();
    mTimerBegin("Morn");
    for(int i=0;i<T;i++)
        mMatrixTranspose(mat_a,mat_b);
    mTimerEnd("Morn");
    mMatrixRelease(mat_b);
    
    mMatrixRelease(mat_a);
}

void test_mul(int K)
{
    printf("matrix mul, size: %d*%d\n",K,K);
    Eigen::MatrixXf m_a(K,K);
    Eigen::MatrixXf m_b(K,K);
    MMatrix *mat_a = mMatrixCreate(K,K);
    MMatrix *mat_b = mMatrixCreate(K,K);
    
    for(int j=0;j<K;j++)for(int i=0;i<K;i++)
    {
        float v;
        v=mRand(-1000,1000)/1000.0;
        m_a(j,i)=v;
        mat_a->data[j][i]=v;

        v=mRand(-1000,1000)/1000.0;
        m_b(j,i)=v;
        mat_b->data[j][i]=v;
    }

    Eigen::MatrixXf m_mul;
    mTimerBegin("eigen");
    for(int i=0;i<T;i++)
        m_mul = m_a*m_b;
    mTimerEnd("eigen");
    // for(int i=0;i<K;i++) printf("%f,",m_mul(0,i)); printf("\n");

    MMatrix *mat_mul = mMatrixCreate();
    mTimerBegin("Morn");
    for(int i=0;i<T;i++)
        mMatrixMul(mat_a,mat_b,mat_mul);
    mTimerEnd("Morn");
    // for(int i=0;i<K;i++) printf("%f,",mat_mul->data[0][i]); printf("\n");
    mMatrixRelease(mat_mul);
    
    mMatrixRelease(mat_a);
    mMatrixRelease(mat_b);
}

void test_determinant(int K)
{
    printf("matrix determinant, size: %d*%d\n",K,K);
    Eigen::MatrixXf m_a(K,K);
    MMatrix *mat = mMatrixCreate(K,K);

    for(int j=0;j<K;j++)for(int i=0;i<K;i++)
    {
        float v=mRand(-1000,1000)/500.0;
        m_a(j,i)=v;
        mat->data[j][i]=v;
    }
    float det;

    mTimerBegin("eigen");
    for(int i=0;i<T;i++)
        det=m_a.determinant();
    mTimerEnd("eigen");
    // printf("det=%f\n",det);

    mTimerBegin("Morn");
    for(int i=0;i<T;i++)
        det=mMatrixDetValue(mat);
    mTimerEnd("Morn");
    // printf("det=%f\n",det);

    mMatrixRelease(mat);
}

void test_inverse(int K)
{
    printf("matrix inverse, size: %d*%d\n",K,K);
    Eigen::MatrixXf m_a(K,K);
    MMatrix *mat = mMatrixCreate(K,K);

    for(int j=0;j<K;j++)for(int i=0;i<K;i++)
    {
        float v=mRand(-1000,1000)/1000.0;
        m_a(j,i)=v;
        mat->data[j][i]=v;
    }

    Eigen::MatrixXf m_inv;
    mTimerBegin("eigen");
    for(int i=0;i<T;i++)
        m_inv=m_a.inverse();
    mTimerEnd("eigen");
    // for(int i=0;i<K;i++) printf("%f,",m_inv(2,i)); printf("\n");

    MMatrix *inv = mMatrixCreate(K,K);
    mTimerBegin("Morn");
    for(int i=0;i<T;i++)
        mMatrixInverse(mat,inv);
    mTimerEnd("Morn");
    // for(int i=0;i<K;i++) printf("%f,",inv->data[2][i]);printf("\n");
    mMatrixRelease(inv);

    mMatrixRelease(mat);
}

extern "C" int sgesv_(long *n, long *nrhs, float *a, long *lda,long *ipiv, float *b, long *ldb, long *info);
void test_linear_equation(int K)
{
    printf("linear equation, x num: %d\n",K);
    Eigen::MatrixXf m_a(K,K);
    Eigen::VectorXf v_b(K);
    MMatrix *mat = mMatrixCreate(K,K+1);

    float v;
    for(int i=0;i<K;i++)
    {
        for(int j=0;j<K;j++)
        {
            v=mRand(-1000,1000)/1000.0;
            m_a(i,j)=v;
            mat->data[i][j]=v;
        }
        v=mRand(-1000,1000);
        v_b(i)=v;
        mat->data[i][K]=0-v;
    }

    Eigen::VectorXf v_x;
    // mTimerBegin("Eigen QR");
    // for(int i=0;i<T;i++)
    //     v_x = m_a.colPivHouseholderQr().solve(v_b);
    // mTimerEnd("Eigen QR");
    // for(int i=0;i<K;i++) printf("%f,",v_x(i));printf("\n");

    mTimerBegin("Eigen");
    for(int i=0;i<T;i++)
        v_x = m_a.lu().solve(v_b);
    mTimerEnd("Eigen");
    // for(int i=0;i<K;i++) printf("%f,",v_x(i));printf("\n");

    float *b=(float *)malloc(K*sizeof(float));
    mTimerBegin("Morn");
    for(int i=0;i<T;i++)
        mLinearEquation(mat,b);
    mTimerEnd("Morn");
    // for(int i=0;i<K;i++) printf("%f,",b[i]);printf("\n");
    free(b);
    
    mMatrixRelease(mat);
}

// void test_eigenvalue(int K)
// {
//     printf("matrix eigen value, size: %d*%d\n",K,K);
//     arma::mat A(K,K);
//     MMatrix *a=mMatrixCreate(K,K);

//     for(int j=0;j<K;j++)for(int i=0;i<K;i++)
//     {
//         float v=mRand(-1000,1000)/1000.0;
//         A(j,i)=v;
//         a->data[j][i]=v;
//     }

//     arma::mat B = A.t()*A;
//     MMatrix *b=mMatrixCreate(K,K);
//     mMatrixTranspose(a,b);
//     mMatrixMul(b,a,b);

//     arma::vec eigval;
//     arma::mat eigvec;
//     mTimerBegin("armadillo");
//     for(int i=0;i<T;i++)
//         arma::eig_sym(eigval, eigvec, B);
//     mTimerEnd("armadillo");
//     // eigval.print("eigval:");
//     // eigvec.print("eigvec:");

//     MList *eigenvalue =mListCreate();
//     MList *eigenvector=mListCreate();
//     mTimerBegin("Morn");
//     for(int i=0;i<T;i++)
//         mMatrixEigenValue(b,eigenvalue,eigenvector);
//     mTimerEnd("Morn");
//     // for(int i=0;i<eigenvalue->num;i++)
//     // {
//         // printf("eigenvalue=%f\neigenvector=",*(float *)(eigenvalue->data[i]));
//         // float *p=(float *)eigenvector->data[i];
//         // for(int j=0;j<K;j++) printf("%f,",p[j]);
//         // printf("\n");
//     // }
//     mListRelease(eigenvalue);
//     mListRelease(eigenvector);

//     mMatrixRelease(a);
//     mMatrixRelease(b);
// }

int main()
{
    printf("\neigen use simd %s\n",Eigen::SimdInstructionSetsInUse());
    test_transpose(10);
    test_transpose(20);
    test_transpose(50);
    test_transpose(100);
    test_transpose(200);
//     test_transpose(500);

    printf("\neigen use simd %s\n",Eigen::SimdInstructionSetsInUse());
    test_mul(10);
    test_mul(20);
    test_mul(50);
    test_mul(100);
    test_mul(200);
//     test_mul(500);

    printf("\neigen use simd %s\n",Eigen::SimdInstructionSetsInUse());
    test_determinant(10);
    test_determinant(20);
    test_determinant(50);
    test_determinant(100);

    printf("\neigen use simd %s\n",Eigen::SimdInstructionSetsInUse());
    test_inverse(10);
    test_inverse(20);
    test_inverse(50);
    test_inverse(100);
    test_inverse(200);
//     test_inverse(500);

    printf("\neigen use simd %s\n",Eigen::SimdInstructionSetsInUse());
    test_linear_equation(10);
    test_linear_equation(20);
    test_linear_equation(50);
    test_linear_equation(100);
    test_linear_equation(200);
//     test_linear_equation(500);

    // printf("\n");
    // test_eigenvalue(10);
    // test_eigenvalue(20);
    // test_eigenvalue(50);
    // test_eigenvalue(100);
    
    return 0;
}
