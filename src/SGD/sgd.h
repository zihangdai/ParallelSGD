#ifndef __SGD_H__
#define __SGD_H__

#include <stdio.h>
#include <map>
#include <mpi.h>
#include <math.h>
#include "confreader.h"

class sgdBase
{
public:
    sgdBase() {};
    virtual ~sgdBase() {};

    /* data */

    /* method */
    void virtual updateParams (float *params, float *grad, int rank) {};

protected:
    /* data */
    int m_useMomentum;
    int m_nParamSize;
    float m_learningRate;
    int m_stepCount;

    /* method */
    //TODO void truncate (float);
    void printInfo (float *buffer) {
        float sum = 0.f;
        for (int i=0; i<m_nParamSize; ++i) {
            sum += buffer[i];
        }
        printf("sum: %f\n", sum);
    }
};

/****************************************************************
* BASIC SGD
****************************************************************/
class sgdBasic: public sgdBase
{
public:
    sgdBasic(ConfReader *confReader, int paramSize);
    ~sgdBasic();

    /* data */

    /* method */
    void updateParams (float *params, float *grad, int rank);
};

/****************************************************************
* ADAGRAD
****************************************************************/
class adagrad: public sgdBase
{
public:
    adagrad(ConfReader *confReader, int paramSize);
    ~adagrad();

    /* data */

    /* method */
    void updateParams (float *params, float *grad, int rank);

private:
    /* data */
    float *m_histSquareGrad;
};

/****************************************************************
* DELAYED ADAGRAD
****************************************************************/
class delayedAdagrad: public sgdBase
{
public:
    delayedAdagrad(ConfReader *confReader, int paramSize);
    ~delayedAdagrad();

    /* data */
    int m_nSlave;

    /* method */
    void updateParams (float *params, float *grad, int rank);

private:
    /* data */
    float *m_histSquareGrad;
    std::map<int, float*> m_mapHistSquareGrad;    
};

/****************************************************************
* Future ADAGRAD
****************************************************************/
class futureAdagrad: public sgdBase
{
public:
    futureAdagrad(ConfReader *confReader, int paramSize);
    ~futureAdagrad();

    /* data */
    int m_nSlave;

    /* method */
    void updateParams (float *params, float *grad, int rank);

private:
    /* data */
    float *m_histSquareGrad;
    std::map<int, float*> m_mapHistSquareGrad;    
};

/****************************************************************
* ADADELTA
****************************************************************/
class adadelta: public sgdBase
{
public:
    adadelta(ConfReader *confReader, int paramSize);
    ~adadelta();

    /* data */

    /* method */
    void updateParams (float *params, float *grad, int rank);

private:
    /* data */
    float m_decayFactor;
    float m_stableConst;

    float *m_ESquareGrad;
    float *m_ESquareDelta;    
};

/****************************************************************
* KERNEL ADADELTA
****************************************************************/
class kernelAdadelta: public sgdBase
{
public:
    kernelAdadelta(ConfReader *confReader, int paramSize);
    ~kernelAdadelta();

    /* data */

    /* method */
    void updateParams (float *params, float *grad, int rank);

private:
    /* data */
    int m_nSlave;

    float m_decayFactor;
    float m_stableConst;

    float *m_ESquareGrad;
    float *m_ESquareDelta;

    std::map<int, float> m_factor;
    std::map<int, float*> m_mapESquareGrad;
    std::map<int, float*> m_mapESquareDelta;
};

/****************************************************************
* RMSPROP
****************************************************************/
class rmsprop: public sgdBase
{
public:
    rmsprop(ConfReader *confReader, int paramSize);
    ~rmsprop();

    /* data */

    /* method */
    void updateParams (float *params, float *grad, int rank);

private:
    /* data */
    float m_decayFactor;

    float *m_meanSquareGrad;    
};

/****************************************************************
* delayed ADADELTA
****************************************************************/
class DelayedAdadelta: public sgdBase
{
public:
    DelayedAdadelta(ConfReader *confReader, int paramSize);
    ~DelayedAdadelta();

    /* data */

    /* method */
    void updateParams (float *params, float *grad, int rank);

private:
    /* data */
    int m_numSlave;
    float m_decayFactor;
    float m_stableConst;

    float *m_ESquareGrad;
    float *m_ESquareDelta;

    std::map<int, float*> m_mapHistSquareGrad;
    std::map<int, float*> m_mapHistSquareDelta;
};
#endif