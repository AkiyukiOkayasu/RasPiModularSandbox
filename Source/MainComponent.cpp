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

    //I2C
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

    spiHandler = spiOpen(spiCS, spiBaudrate, spiFlag);
    if (spiHandler < 0)
    {
        std::cout << "Error: spiOpen()" << std::endl;
    }
    else
    {
        std::cout << "SPI port is opened" << std::endl;
    }

    th = std::thread([this] {
        char txBuf[3];
        while (isCVEnabled)
        {
            for (int i = 0; i < 8; ++i)
            {
                txBuf[0] = (0x18 + i) << 2;
                txBuf[1] = txBuf[2] = 0;
                spiXfer(spiHandler, txBuf, txBuf, 3);
                cv[i] = (txBuf[1] & 0xFF) << 4 | (txBuf[2] & 0xFF) >> 4;
                //sleep_for(milliseconds(10)); //10ms sleep
            }
            sleep_for(milliseconds(5)); //5ms sleep
            std::cout << "CV: " << cv[0] << ", " << cv[1] << ", " << cv[2] << ", " << cv[3] << ", " << cv[4] << ", " << cv[5] << ", " << cv[6] << ", " << cv[7] << std::endl;
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
#if JUCE_LINUX
    //AK4558 setup
    ak4558::ak4558Config(i2cHandler, AK4558RESET);
#endif //JUCE_LINUX
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill)
{
    //Input audio through output without any processing
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
