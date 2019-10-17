/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#if JUCE_LINUX
#include <pigpio.h>
#include "AK4558.h"
#include "CVUtil.h"
#endif

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public AudioAppComponent
{
public:
  //==============================================================================
  MainComponent();
  ~MainComponent();

  //==============================================================================
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override;
  void releaseResources() override;

  //==============================================================================
  void paint(Graphics &g) override;
  void resized() override;
  void showAudioSettings();
  void updateSynthVoices();

private:
  //==============================================================================
  // GPIO pin for AK4558 Reset
  static constexpr unsigned int AK4558RESET = 5;

  //I2C
  int i2cHandler;
  static constexpr unsigned int i2cAddr = 0b0010000; //AK4558 I2C address
  static constexpr unsigned int i2cPort = 1;         //Raspberry Pi I2C Port 1(0 or 1)
  static constexpr unsigned int i2cFlag = 0;

  //SPI
  int spiHandler;
  static constexpr int spiCS = 0; //SPI chip select(0 or 1)
  static constexpr int spiBaudrate = 200000;
  static constexpr int spiFlag = 0;

  //CV
  std::thread th; //Thread for getting CV value via SPI
  bool isCVEnabled = true;
  std::array<std::atomic<int>, 8> cv;
  static constexpr int NUM_ADC_CHANNELS = 8;

  //Oscillators
  enum class synthVoiceIndex
  {
    osc = 0,
    gain
  };
  using synthVoice = dsp::ProcessorChain<dsp::Oscillator<float>, dsp::Gain<float>>;
  static constexpr int NUM_VOICES = 4;
  dsp::ProcessorChain<dsp::Oscillator<float>, dsp::Gain<float>> processorChain;
  std::array<synthVoice, NUM_VOICES> synthVoices;
  static constexpr float OSC_FREQ_MIN = 110.0;
  static constexpr float OSC_FREQ_MAX = 880.0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
