#ifndef __RECURRENT_LAYER_H__
#define __RECURRENT_LAYER_H__

#include <vector>

#include "matrix.h"
#include "nonlinearity.h"

#include "common.h"

using namespace std;

/****************************************************************
* Recurrent Layer
****************************************************************/
class RecurrentLayer
{
public:
	RecurrentLayer(int numNeuron, int maxSeqLen, int inputSize);
	virtual ~RecurrentLayer();

	/* data */
	int m_numNeuron;
	int m_inputSize;
	int m_maxSeqLen;

	int m_nParamSize;

	vector<float *> m_inputActs;
	vector<float *> m_outputActs;

	vector<float *> m_inputErrs;
	vector<float *> m_outputErrs;
	
	/* method */
	void virtual initParams(float *params) {};

	void virtual feedForward(int inputSeqLen) {};
	void virtual feedBackward(int inputSeqLen) {};

	void virtual forwardStep(int seqIdx) {};
	void virtual backwardStep(int seqIdx) {};

	void virtual bindWeights(float *params, float *grad) {};

	void virtual resetStates(int inputSeqLen);
	void virtual reshape(int newSeqLen);	

private:
	void resize (int newSeqLen);
	void releaseMem (int seqIdx);
	void allocateMem (int seqIdx);
};

#endif