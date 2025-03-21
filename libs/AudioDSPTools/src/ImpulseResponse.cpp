//
//  ImpulseResponse.cpp
//  NeuralAmpModeler-macOS
//
//  Created by Steven Atkinson on 12/30/22.
//

#include "AudioDSPTools/dsp/Resample.h"
#include "AudioDSPTools/dsp/wav.h"

#include "AudioDSPTools/dsp/ImpulseResponse.h"

dsp::ImpulseResponse::ImpulseResponse(const char* fileName, const double sampleRate)
: mWavState(dsp::wav::LoadReturnCode::ERROR_OTHER)
, mSampleRate(sampleRate)
{
  // Try to load the WAV
  this->mWavState = dsp::wav::Load(fileName, this->mRawAudio, this->mRawAudioSampleRate);
  if (this->mWavState != dsp::wav::LoadReturnCode::SUCCESS)
  {
    std::stringstream ss;
    ss << "Failed to load IR at " << fileName << std::endl;
  }
  else
    // Set the weights based on the raw audio.
    this->_SetWeights();
}

dsp::ImpulseResponse::ImpulseResponse(const IRData& irData, const double sampleRate)
: mWavState(dsp::wav::LoadReturnCode::SUCCESS)
, mSampleRate(sampleRate)
{
  this->mRawAudio = irData.mRawAudio;
  this->mRawAudioSampleRate = irData.mRawAudioSampleRate;
  this->_SetWeights();
}

double** dsp::ImpulseResponse::Process(double** inputs, const size_t numChannels, const size_t numFrames)
{
  this->_PrepareBuffers(numChannels, numFrames);
  this->_UpdateHistory(inputs, numChannels, numFrames);

  for (size_t i = 0, j = this->mHistoryIndex - this->mHistoryRequired; i < numFrames; i++, j++)
  {
    auto input = Eigen::Map<const Eigen::VectorXf>(&this->mHistory[j], this->mHistoryRequired + 1);
    this->mOutputs[0][i] = (double)this->mWeight.dot(input);
  }
  // Copy out for more-than-mono.
  for (size_t c = 1; c < numChannels; c++)
    for (size_t i = 0; i < numFrames; i++)
      this->mOutputs[c][i] = this->mOutputs[0][i];

  this->_AdvanceHistoryIndex(numFrames);
  return this->_GetPointers();
}

void dsp::ImpulseResponse::_SetWeights()
{
  if (this->mRawAudioSampleRate == mSampleRate)
  {
    this->mResampled.resize(this->mRawAudio.size());
    memcpy(this->mResampled.data(), this->mRawAudio.data(), sizeof(float) * this->mResampled.size());
  }
  else
  {
    // Cubic resampling
    std::vector<float> padded;
    padded.resize(this->mRawAudio.size() + 2);
    padded[0] = 0.0f;
    padded[padded.size() - 1] = 0.0f;
    memcpy(padded.data() + 1, this->mRawAudio.data(), sizeof(float) * this->mRawAudio.size());
    dsp::ResampleCubic<float>(padded, this->mRawAudioSampleRate, mSampleRate, 0.0, this->mResampled);
  }
  // Simple implementation w/ no resample...
  const size_t irLength = std::min(this->mResampled.size(), this->mMaxLength);
  this->mWeight.resize(irLength);
  // Gain reduction.
  // https://github.com/sdatkinson/NeuralAmpModelerPlugin/issues/100#issuecomment-1455273839
  // Add sample rate-dependence
  const float gain = pow(10, -18 * 0.05) * 48000 / mSampleRate;
  for (size_t i = 0, j = irLength - 1; i < irLength; i++, j--)
    this->mWeight[j] = gain * this->mResampled[i];
  this->mHistoryRequired = irLength - 1;
}

dsp::ImpulseResponse::IRData dsp::ImpulseResponse::GetData()
{
  IRData irData;
  irData.mRawAudio = this->mRawAudio;
  irData.mRawAudioSampleRate = this->mRawAudioSampleRate;
  return irData;
}
