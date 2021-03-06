#include <iostream>
#include <cmath>
#include "rnn_translator.h"

using namespace std;

RNNTranslator::RNNTranslator(ConfReader *confReader, int minibatchSize, string prefix) {
	// init encoder and decoder
	m_nMinibatchSize = minibatchSize;
	m_reverseEncoder = confReader->getInt(prefix + "reverse_encoder");
	m_encoder = new RNN_LSTM(confReader, m_nMinibatchSize, "encoder_");
	m_decoder = new RNN_LSTM(confReader, m_nMinibatchSize, "decoder_");
	
	// compute paramSize
	m_nParamSize = 0;
	m_nParamSize += m_encoder->m_nParamSize;
	m_nParamSize += m_decoder->m_nParamSize;
	m_nParamSize += m_decoder->m_inputSize * m_encoder->m_outputSize;
}

RNNTranslator::~RNNTranslator() {
	if (m_encoder != NULL) {
		delete m_encoder;
	}
	if (m_decoder != NULL) {
		delete m_decoder;
	}
}

void RNNTranslator::initParams (float *params) {
	float *paramsCursor = params;
	
	m_encoder->initParams(paramsCursor);
	paramsCursor += m_encoder->m_nParamSize;

	m_decoder->initParams(paramsCursor);
	paramsCursor += m_decoder->m_nParamSize;

	float multiplier = 0.08;
	for (int i=0; i<m_decoder->m_inputSize * m_encoder->m_outputSize; i++) {
		paramsCursor[i] = multiplier * SYM_UNIFORM_RAND;
	}
}

void RNNTranslator::bindWeights(float *params, float *grad) {
	float *paramsCursor = params;
	float *gradCursor = grad;
	
	m_encoder->bindWeights(paramsCursor, gradCursor);
	paramsCursor += m_encoder->m_nParamSize;
	gradCursor += m_encoder->m_nParamSize;

	m_decoder->bindWeights(paramsCursor, gradCursor);
	paramsCursor += m_decoder->m_nParamSize;
	gradCursor += m_decoder->m_nParamSize;

	m_encodingW = paramsCursor;
	m_gradEncodingW = gradCursor;
}

float RNNTranslator::computeGrad (float *grad, float *params, float *data, float *target) {
	float error = 0.f;
	
	memset(grad, 0x00, sizeof(float)*m_nParamSize);
	bindWeights(params, grad);

	float *sampleData = data;
	float *sampleTarget = target;

	/*** feed forward and feed backward ***/
	for (int dataIdx=0; dataIdx<m_nMinibatchSize; ++dataIdx) {
		// TODO
		int encoderSeqLen = m_encoder->m_maxSeqLen;
		int decoderSeqLen = m_decoder->m_maxSeqLen;

		/* reset internal states of LSTM layers */
		m_encoder->resetStates(encoderSeqLen);
		m_decoder->resetStates(decoderSeqLen);
		
		/****************************************************************
		*                      Feed Forward Phase                       *
		****************************************************************/
		// *** encoder ***
		float *dataCursor = sampleData;		
		RecurrentLayer *enInputLayer  = m_encoder->m_vecLayers[0];
		RecurrentLayer *enOutputLayer = m_encoder->m_vecLayers[m_encoder->m_numLayer-1];
		// bind input sequence to m_inputActs of the input layer of the encoder
		if (m_reverseEncoder) {
			for (int seqIdx=encoderSeqLen; seqIdx>=1; --seqIdx) {
				memcpy(enInputLayer->m_inputActs[seqIdx], dataCursor, sizeof(float)*m_encoder->m_inputSize);
				dataCursor += m_encoder->m_inputSize;
			}
		} else {			
			for (int seqIdx=1; seqIdx<=encoderSeqLen; ++seqIdx) {
				memcpy(enInputLayer->m_inputActs[seqIdx], dataCursor, sizeof(float)*m_encoder->m_inputSize);
				dataCursor += m_encoder->m_inputSize;
			}
		}
		m_encoder->feedForward(encoderSeqLen);

		// *** decoder ***
		float *targetCursor = sampleTarget;
		RecurrentLayer *deInputLayer  = m_decoder->m_vecLayers[0];
		RecurrentLayer *deOutputLayer = m_decoder->m_vecLayers[m_decoder->m_numLayer-1];
		// bind input sequence to m_inputActs of the input layer of the decoder
		dot(deInputLayer->m_inputActs[1], m_encodingW, m_decoder->m_inputSize, m_encoder->m_outputSize, 
			enOutputLayer->m_outputActs[encoderSeqLen], m_encoder->m_outputSize, 1);
		for (int seqIdx=2; seqIdx<=decoderSeqLen; ++seqIdx) {
			if (m_decoder->m_taskType == "classification") {
				for (int classIdx=0; classIdx<m_decoder->m_outputSize; ++classIdx) {
					if ( abs(targetCursor[classIdx] - 1.f) < 0.00001 ) {
						memcpy(deInputLayer->m_inputActs[seqIdx], sampleData+m_decoder->m_inputSize*classIdx, sizeof(float)*m_decoder->m_inputSize);
						// printf("classification: %f, %d\n", targetCursor[classIdx], classIdx);
					}					
				}				
			} else if (m_decoder->m_taskType == "regression") {
				memcpy(deInputLayer->m_inputActs[seqIdx], targetCursor, sizeof(float)*m_decoder->m_inputSize);
			}
			targetCursor += m_decoder->m_outputSize;
		}
		// set the internal states of the decoder at t = 0 to the internal states of encoder at the last step
		#pragma omp parallel for
		for (int layerIdx=0; layerIdx<m_encoder->m_numLayer; layerIdx++) {
			LSTMLayer *enLayer = dynamic_cast<LSTMLayer*>(m_encoder->m_vecLayers[layerIdx]);
			LSTMLayer *deLayer = dynamic_cast<LSTMLayer*>(m_decoder->m_vecLayers[layerIdx]);
			memcpy(deLayer->m_states[0], enLayer->m_states[encoderSeqLen], sizeof(float) * deLayer->m_numNeuron);
			memcpy(deLayer->m_outputActs[0], enLayer->m_outputActs[encoderSeqLen], sizeof(float) * deLayer->m_numNeuron);
		}
		
		// decoder feedforward
		m_decoder->feedForward(decoderSeqLen);		

		// ******** compute error phase ******** //
		error += m_decoder->computeError(sampleTarget, decoderSeqLen);
		
		/****************************************************************
		*                      Feed Backword Phase                      *
		****************************************************************/
		// *** decoder ***		
		targetCursor = sampleTarget; // reset the target cursor to sample target
		// bind target sequence to m_outputErrs of the output layer of the decoder
		for (int seqIdx=1; seqIdx<=decoderSeqLen; ++seqIdx) {
			memcpy(deOutputLayer->m_outputErrs[seqIdx], targetCursor, sizeof(float)*m_decoder->m_outputSize);
			targetCursor += m_decoder->m_outputSize;
		}
		m_decoder->feedBackward(decoderSeqLen);

		// *** encoder ***
		// set the spatial error signal of encoder
		trans_dot(enOutputLayer->m_outputErrs[encoderSeqLen], m_encodingW, m_decoder->m_inputSize, m_encoder->m_outputSize, 
			deInputLayer->m_inputErrs[0], m_decoder->m_inputSize, 1);
		#pragma omp parallel for
		for (int layerIdx=0; layerIdx<m_encoder->m_numLayer; layerIdx++) {
			LSTMLayer *enLayer = dynamic_cast<LSTMLayer*>(m_encoder->m_vecLayers[layerIdx]);
			LSTMLayer *deLayer = dynamic_cast<LSTMLayer*>(m_decoder->m_vecLayers[layerIdx]);
			memcpy(enLayer->m_cellStateErrs[encoderSeqLen+1], deLayer->m_cellStateErrs[1], sizeof(float) * deLayer->m_numNeuron);
			memcpy(enLayer->m_inGateDelta[encoderSeqLen+1], deLayer->m_inGateDelta[1], sizeof(float) * deLayer->m_numNeuron);
			memcpy(enLayer->m_forgetGateDelta[encoderSeqLen+1], deLayer->m_forgetGateDelta[1], sizeof(float) * deLayer->m_numNeuron);
			memcpy(enLayer->m_outGateDelta[encoderSeqLen+1], deLayer->m_outGateDelta[1], sizeof(float) * deLayer->m_numNeuron);
			memcpy(enLayer->m_preGateStateDelta[encoderSeqLen+1], deLayer->m_preGateStateDelta[1], sizeof(float) * deLayer->m_numNeuron);
			memcpy(enLayer->m_forgetGateActs[encoderSeqLen+1], deLayer->m_forgetGateActs[1], sizeof(float) * deLayer->m_numNeuron);
		}
		// encoder feed backward
		m_encoder->feedBackward(encoderSeqLen);

		// compute m_gradEncodingW
		outer(m_gradEncodingW, deInputLayer->m_inputErrs[0], m_decoder->m_inputSize, 
			enOutputLayer->m_outputActs[encoderSeqLen], m_encoder->m_outputSize);

		// move cursor to new position
		sampleData += encoderSeqLen * m_encoder->m_inputSize;
		sampleTarget += decoderSeqLen * m_decoder->m_outputSize;
	}

	// normalization by number of input sequences and clip gradients to [-1, 1]
	float normFactor = 1.f / (float) m_nMinibatchSize;
	for (int dim=0; dim<m_nParamSize; ++dim) {
		grad[dim] *= normFactor;
		if (grad[dim] < -1.f) {
			grad[dim] = -1.f;
		} else if (grad[dim] > 1.f) {
			grad[dim] = 1.f;
		}
	}
	error *= normFactor;
	printf("Error: %f\n", error);

	return error;
}