//
//  wav.cpp
//  NeuralAmpModeler-macOS
//
//  Created by Steven Atkinson on 12/31/22.
//

#include <cstring> // strncmp
#include <cmath> // pow
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "AudioDSPTools/dsp/wav.h"

bool idIsNotJunk(char* id)
{
  return strncmp(id, "RIFF", 4) == 0 || strncmp(id, "WAVE", 4) == 0 || strncmp(id, "fmt ", 4) == 0
         || strncmp(id, "data", 4) == 0;
}

bool ReadChunkAndSkipJunk(std::ifstream& file, char* chunkID)
{
  file.read(chunkID, 4);
  while (!idIsNotJunk(chunkID) && file.good())
  {
    int junkSize;
    file.read(reinterpret_cast<char*>(&junkSize), 4);
    file.ignore(junkSize);
    // Unused byte if junkSize is odd
    if ((junkSize % 2) == 1)
      file.ignore(1);
    // And now we should be ready for data...
    file.read(chunkID, 4);
  }
  return file.good();
}

std::string dsp::wav::GetMsgForLoadReturnCode(LoadReturnCode retCode)
{
  std::stringstream message;

  switch (retCode)
  {
    case (LoadReturnCode::ERROR_OPENING):
      message << "Failed to open file (is it being used by another "
                 "program?)";
      break;
    case (LoadReturnCode::ERROR_NOT_RIFF): message << "File is not a WAV file."; break;
    case (LoadReturnCode::ERROR_NOT_WAVE): message << "File is not a WAV file."; break;
    case (LoadReturnCode::ERROR_MISSING_FMT): message << "File is missing expected format chunk."; break;
    case (LoadReturnCode::ERROR_INVALID_FILE): message << "WAV file contents are invalid."; break;
    case (LoadReturnCode::ERROR_UNSUPPORTED_FORMAT_ALAW): message << "Unsupported file format \"A-law\""; break;
    case (LoadReturnCode::ERROR_UNSUPPORTED_FORMAT_MULAW): message << "Unsupported file format \"mu-law\""; break;
    case (LoadReturnCode::ERROR_UNSUPPORTED_FORMAT_EXTENSIBLE):
      message << "Unsupported file format \"extensible\"";
      break;
    case (LoadReturnCode::ERROR_NOT_MONO): message << "File is not mono."; break;
    case (LoadReturnCode::ERROR_UNSUPPORTED_BITS_PER_SAMPLE): message << "Unsupported bits per sample"; break;
    case (dsp::wav::LoadReturnCode::ERROR_OTHER): message << "???"; break;
    default: message << "???"; break;
  }

  return message.str();
}

dsp::wav::LoadReturnCode dsp::wav::Load(const char* fileName, std::vector<float>& audio, double& sampleRate)
{
  // FYI: https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
  // Open the WAV file for reading
  std::ifstream wavFile(fileName, std::ios::binary);

  // Check if the file was opened successfully
  if (!wavFile.is_open())
  {
    std::cerr << "Error opening WAV file" << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_OPENING;
  }

  // WAV file has 3 "chunks": RIFF ("RIFF"), format ("fmt ") and data ("data").
  // Read the WAV file header
  char chunkId[4];
  if (!ReadChunkAndSkipJunk(wavFile, chunkId))
  {
    std::cerr << "Error while reading for next chunk." << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
  }

  if (strncmp(chunkId, "RIFF", 4) != 0)
  {
    std::cerr << "Error: File does not start with expected RIFF chunk. Got" << chunkId << " instead." << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_NOT_RIFF;
  }

  int chunkSize;
  wavFile.read(reinterpret_cast<char*>(&chunkSize), 4);

  char format[4];
  wavFile.read(format, 4);
  if (strncmp(format, "WAVE", 4) != 0)
  {
    std::cerr << "Error: Files' second chunk (format) is not expected WAV. Got" << format << " instead." << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_NOT_WAVE;
  }

  // Read the format chunk
  char subchunk1Id[4];
  if (!ReadChunkAndSkipJunk(wavFile, subchunk1Id))
  {
    std::cerr << "Error while reading for next chunk." << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
  }
  if (strncmp(subchunk1Id, "fmt ", 4) != 0)
  {
    std::cerr << "Error: Invalid WAV file missing expected fmt section; got " << subchunk1Id << " instead."
              << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_MISSING_FMT;
  }

  int subchunk1Size;
  wavFile.read(reinterpret_cast<char*>(&subchunk1Size), 4);
  if (subchunk1Size < 16)
  {
    std::cerr << "WAV chunk 1 size is " << subchunk1Size
              << ", which is smaller than the requried 16 to fit the expected "
                 "information."
              << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
  }

  unsigned short audioFormat;
  wavFile.read(reinterpret_cast<char*>(&audioFormat), 2);
  const short AUDIO_FORMAT_PCM = 1;
  const short AUDIO_FORMAT_IEEE = 3;
  std::unordered_set<short> supportedFormats{AUDIO_FORMAT_PCM, AUDIO_FORMAT_IEEE};
  if (supportedFormats.find(audioFormat) == supportedFormats.end())
  {
    std::cerr << "Error: Unsupported WAV format detected. ";
    switch (audioFormat)
    {
      case 6: std::cerr << "(Got: A-law)" << std::endl; return dsp::wav::LoadReturnCode::ERROR_UNSUPPORTED_FORMAT_ALAW;
      case 7:
        std::cerr << "(Got: mu-law)" << std::endl;
        return dsp::wav::LoadReturnCode::ERROR_UNSUPPORTED_FORMAT_MULAW;
      case 65534:
        std::cerr << "(Got: Extensible)" << std::endl;
        return dsp::wav::LoadReturnCode::ERROR_UNSUPPORTED_FORMAT_EXTENSIBLE;
      default:
        std::cerr << "(Got unknown format " << audioFormat << ")" << std::endl;
        return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
    }
  }

  short numChannels;
  wavFile.read(reinterpret_cast<char*>(&numChannels), 2);
  // HACK
  if (numChannels != 1)
  {
    std::cerr << "Require mono (using for IR loading)" << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_NOT_MONO;
  }

  int iSampleRate;
  wavFile.read(reinterpret_cast<char*>(&iSampleRate), 4);
  // Store in format we assume (SR is double)
  sampleRate = (double)iSampleRate;

  int byteRate;
  wavFile.read(reinterpret_cast<char*>(&byteRate), 4);

  short blockAlign;
  wavFile.read(reinterpret_cast<char*>(&blockAlign), 2);

  short bitsPerSample;
  wavFile.read(reinterpret_cast<char*>(&bitsPerSample), 2);

  // The default is for there to be 16 bytes in the fmt chunk, but sometimes
  // it's different.
  if (subchunk1Size > 16)
  {
    const int extraBytes = subchunk1Size - 16;
    const int skipChars = extraBytes / 4 * 4; // truncate to dword size
    wavFile.ignore(skipChars);
    const int remainder = extraBytes % 4;
    wavFile.read(reinterpret_cast<char*>(&byteRate), remainder);
  }

  // Read the data chunk
  char subchunk2Id[4];
  if (!ReadChunkAndSkipJunk(wavFile, subchunk2Id))
  {
    std::cerr << "Error while reading for next chunk." << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
  }
  if (strncmp(subchunk2Id, "data", 4) != 0)
  {
    std::cerr << "Error: Invalid WAV file" << std::endl;
    return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
  }

  // Size of the data chunk, in bits.
  int subchunk2Size;
  wavFile.read(reinterpret_cast<char*>(&subchunk2Size), 4);

  if (audioFormat == AUDIO_FORMAT_IEEE)
  {
    if (bitsPerSample == 32)
      dsp::wav::_LoadSamples32(wavFile, subchunk2Size, audio);
    else
    {
      std::cerr << "Error: Unsupported bits per sample for IEEE files: " << bitsPerSample << std::endl;
      return dsp::wav::LoadReturnCode::ERROR_UNSUPPORTED_BITS_PER_SAMPLE;
    }
  }
  else if (audioFormat == AUDIO_FORMAT_PCM)
  {
    if (bitsPerSample == 16)
      dsp::wav::_LoadSamples16(wavFile, subchunk2Size, audio);
    else if (bitsPerSample == 24)
      dsp::wav::_LoadSamples24(wavFile, subchunk2Size, audio);
    else if (bitsPerSample == 32)
      dsp::wav::_LoadSamples32(wavFile, subchunk2Size, audio);
    else
    {
      std::cerr << "Error: Unsupported bits per sample for PCM files: " << bitsPerSample << std::endl;
      return dsp::wav::LoadReturnCode::ERROR_UNSUPPORTED_BITS_PER_SAMPLE;
    }
  }

  // Close the WAV file
  wavFile.close();

  // Print the number of samples
  // std::cout << "Number of samples: " << samples.size() << std::endl;

  return dsp::wav::LoadReturnCode::SUCCESS;
}

void dsp::wav::_LoadSamples16(std::ifstream& wavFile, const int chunkSize, std::vector<float>& samples)
{
  // Allocate an array to hold the samples
  std::vector<short> tmp(chunkSize / 2); // 16 bits (2 bytes) per sample

  // Read the samples from the file into the array
  wavFile.read(reinterpret_cast<char*>(tmp.data()), chunkSize);

  // Copy into the return array
  const float scale = 1.0 / ((double)(1 << 15));
  samples.resize(tmp.size());
  for (auto i = 0; i < samples.size(); i++)
    samples[i] = scale * ((float)tmp[i]); // 2^16
}

void dsp::wav::_LoadSamples24(std::ifstream& wavFile, const int chunkSize, std::vector<float>& samples)
{
  // Allocate an array to hold the samples
  std::vector<int> tmp(chunkSize / 3); // 24 bits (3 bytes) per sample
  // Read in and convert the samples
  for (int& x : tmp)
  {
    x = dsp::wav::_ReadSigned24BitInt(wavFile);
  }

  // Copy into the return array
  const float scale = 1.0 / ((double)(1 << 23));
  samples.resize(tmp.size());
  for (auto i = 0; i < samples.size(); i++)
    samples[i] = scale * ((float)tmp[i]);
}

int dsp::wav::_ReadSigned24BitInt(std::ifstream& stream)
{
  // Read the three bytes of the 24-bit integer.
  std::uint8_t bytes[3];
  stream.read(reinterpret_cast<char*>(bytes), 3);

  // Combine the three bytes into a single integer using bit shifting and
  // masking. This works by isolating each byte using a bit mask (0xff) and then
  // shifting the byte to the correct position in the final integer.
  int value = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16);

  // The value is stored in two's complement format, so if the most significant
  // bit (the 24th bit) is set, then the value is negative. In this case, we
  // need to extend the sign bit to get the correct negative value.
  if (value & (1 << 23))
  {
    value |= ~((1 << 24) - 1);
  }

  return value;
}

void dsp::wav::_LoadSamples32(std::ifstream& wavFile, const int chunkSize, std::vector<float>& samples)
{
  // NOTE: 32-bit is float.
  samples.resize(chunkSize / 4); // 32 bits (4 bytes) per sample
  // Read the samples from the file into the array
  wavFile.read(reinterpret_cast<char*>(samples.data()), chunkSize);
}
