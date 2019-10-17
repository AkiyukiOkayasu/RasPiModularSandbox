/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize(800, 600);

    //DSP module
    for (auto& s: synthVoices) 
    {
        auto& osc s.get<synthVoiceIndex::osc>();
        auto& gain s.get<synthVoiceIndex::gain>();
        osc.initialise([](float x)){return std::sin(x)}, 128);
    }

#if JUCE_LINUX
    using std::chrono::milliseconds;
    using std::this_thread::sleep_for;

    /*
    Configure PIGPIO clock for I2S audio
    gpioCfgClock(macro, PWM/PCM, deprecated param)
    macro: 5
    PWM/PCM: 0:PWM, 1:PCM
    pigpio with using I2S audio. You have to set PWM mode before call gpioInitialise()
    */
    gpioCfgClock(5, 0, 0);

    //gpioinit
    if (gpioInitialise() < 0)
    {
        //pigpio init failed
        std::cout << "Error: pigpio init failed" << std::endl;
        jassertfalse;
    }
    else
    {
        //pigpio init succeed
        std::cout << "pigpio is initialized" << std::endl;
    }

    //gpio 5 to high>low
    gpioSetMode(AK4558RESET, PI_OUTPUT);

    // I2C
    i2cHandler = i2cOpen(i2cPort, i2cAddr, i2cFlag);
    if (i2cHandler < 0)
    {
        std::cout << "Error: i2cOpen()" << std::endl;
        std::cout << i2cHandler << std::endl;
        std::cout << "BADI2CBUS: " << PI_BAD_I2C_BUS << ", BADI2CADDR: " << PI_BAD_I2C_ADDR << ", NOHANDLE: " << PI_NO_HANDLE << ", I2COPENFAILED: " << PI_I2C_OPEN_FAILED << std::endl;
    }
    else
    {
        std::cout << "I2C port is opened" << std::endl;
    }

    // SPI
    spiHandler = spiOpen(spiCS, spiBaudrate, spiFlag);
    if (spiHandler < 0)
    {
        std::cout << "Error: spiOpen()" << std::endl;
    }
    else
    {
        std::cout << "SPI port is opened" << std::endl;
    }

    // Get CV Value
    char txBuf[3];
    for (int i = 0; i < NUM_ADC_CHANNELS; ++i)
    {
        txBuf[0] = (0x18 + i) << 2;
        txBuf[1] = txBuf[2] = 0;
        spiXfer(spiHandler, txBuf, txBuf, 3);
        cv[i] = (txBuf[1] & 0xFF) << 4 | (txBuf[2] & 0xFF) >> 4;
    }

    updateSynthVoices();

    // CV update thread
    th = std::thread([this] {
        char txBuffer[3];
        while (isCVEnabled)
        {
            for (int i = 0; i < NUM_ADC_CHANNELS; ++i)
            {
                txBuffer[0] = (0x18 + i) << 2;
                txBuffer[1] = txBuffer[2] = 0;
                spiXfer(spiHandler, txBuffer, txBuffer, 3);
                cv[i] = (txBuffer[1] & 0xFF) << 4 | (txBuffer[2] & 0xFF) >> 4;                
            }
            sleep_for(milliseconds(5)); //5ms sleep
        }
    });
#endif //JUCE_LINUX

    // Some platforms require permissions to open input channels so request that here
    if (RuntimePermissions::isRequired(RuntimePermissions::recordAudio) && !RuntimePermissions::isGranted(RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request(RuntimePermissions::recordAudio,
                                    [&](bool granted) { if (granted)  setAudioChannels (2, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels(2, 2);
    }

    showAudioSettings();
}

MainComponent::~MainComponent()
{
#if JUCE_LINUX
    if (i2cClose(i2cHandler))
    {
        std::cout << "Error: i2cClose()" << std::endl;
    }

    isCVEnabled = false;
    th.join();
    if (spiClose(spiHandler))
    {
        std::cout << "Error: spiClose()" << std::endl;
    }
    gpioTerminate();
#endif //JUCE_LINUX

    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    //DSP module
    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.numChannels = 2;
    spec.maximumBlockSize = samplesPerBlockExpected;
    for (auto& s: synthVoices) 
    {
        s.prepare(spec);
    }

#if JUCE_LINUX
    //AK4558 setup
    AK4558::config(i2cHandler, AK4558RESET);
#endif //JUCE_LINUX
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill)
{    
    updateSynthVoices();
    dsp::AudioBlock<float> audioBlock(*bufferToFill.buffer);
	dsp::ProcessContextReplacing<float> context(audioBlock);
    for (auto& s: synthVoices) 
    {
        s.process(context);
    }	
}

void MainComponent::releaseResources()
{
}

//==============================================================================
void MainComponent::paint(Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
}

void MainComponent::showAudioSettings()
{
    AudioDeviceSelectorComponent audioSettingsComp(deviceManager,
                                                   0, 2,  //InputChannels: min/max
                                                   0, 2,  //OutputChannels: min/max
                                                   false, //Show MIDI input options
                                                   false, //Show MIDI output selector
                                                   false, //Stereo pair
                                                   false  //Hide advanced option with button
    );

    audioSettingsComp.setSize(450, 250);
    DialogWindow::LaunchOptions o;
    o.content.setNonOwned(&audioSettingsComp);
    o.dialogTitle = "Audio Settings";
    o.componentToCentreAround = this;
    o.dialogBackgroundColour = getLookAndFeel().findColour(ResizableWindow::backgroundColourId);
    o.escapeKeyTriggersCloseButton = false;
    o.useNativeTitleBar = true;
    o.resizable = false;
    o.runModal();
}

void MainComponent::updateSynthVoices()
{
    for (int i = 0; i < NUM_VOICES; ++i) 
    {
        auto& osc synthVoices[i].get<synthVoiceIndex::osc>();
        auto& gain synthVoices[i].get<synthVoiceIndex::gain>();        
        osc.setFrequency(CVUtil::scale(cv[i], OSC_FREQ_MIN, OSC_FREQ_MAX));
        gain.setGainLinear(CVUtil::normalize(cv[i + 1]));
    }    
}
