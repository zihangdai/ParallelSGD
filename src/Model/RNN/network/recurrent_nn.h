#ifndef __RECURRENT_NEURAL_NETWORK_H__
#define __RECURRENT_NEURAL_NETWORK_H__

#include <vector>

#include "common.h"

#include "rnn_layer.h"
#include "rnn_input_layer.h"
#include "rnn_softmax_layer.h"
#include "rnn_mse_layer.h"
#include "lstm_layer.h"

#include "rnn_connection.h"

#include "model.h"

using namespace std;

class RecurrentNN: public modelBase
{
public:
	RecurrentNN() {};
	virtual ~RecurrentNN() {};

	/* data */
	int m_numLayer;
	int m_maxSeqLen;

	int m_inputSize;
	int m_outputSize;	

	int *m_numNeuronList;
	string *m_layerTypeList;
	string *m_connTypeList;

	vector<RecurrentLayer *> m_vecLayers;
	vector<RNNConnection *> m_vecConnections;

	/* method */
	float virtual computeGrad (float *grad, float *params, float *data, float *label) {return 0.f;};
	void virtual initParams (float *params) {};
};

#endif