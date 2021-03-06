#include "MainComponent.hpp"

//==============================================================================
MainComponent::MainComponent()
  : audioGraph(std::make_unique<juce::AudioProcessorGraph>()), audioSettings(deviceManager) {
  setSize(600, 400);
  startTimerHz(30);

  // tell the ProcessorPlayer what audio callback function to play (.get() needed since audioGraph
  // is a unique_ptr)

  processorPlayer.setProcessor(audioGraph.get());
  // simplest way to start audio device. Uses whichever device the current system (mac/pc/linux
  // machine) uses
  deviceManager.initialiseWithDefaultDevices(2, 2);
  // Tell the processor player to keep moving every time the device requests more data
  deviceManager.addAudioCallback(&processorPlayer);
  // set up the graph
  audioGraph->clear();  // likely not needed but won't hurt
  // a node that passes in input from your device (not currently used)
  audioInputNode =
    audioGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
      juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
  // a node that passes audio out to your device
  audioOutputNode =
    audioGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
      juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
  // a simple sine tone generator node
  testToneNode = audioGraph->addNode(std::make_unique<ToneGenerator>());
  // set the details of the processor (io and samplerate/buffersize
  testToneNode->getProcessor()->setPlayConfigDetails(
    0, 2, deviceManager.getAudioDeviceSetup().sampleRate,
    deviceManager.getAudioDeviceSetup().bufferSize);
  // connect the 'left' channel
  audioGraph->addConnection({{testToneNode->nodeID, 0}, {audioOutputNode->nodeID, 0}});
  // connect the 'right' channel
  audioGraph->addConnection({{testToneNode->nodeID, 1}, {audioOutputNode->nodeID, 1}});

  // Spectrogram
  spectrogramNode = audioGraph->addNode(std::make_unique<SpectrogramComponent>());

  spectrogramNode->getProcessor()->setPlayConfigDetails(
    1, 0, deviceManager.getAudioDeviceSetup().sampleRate,
    deviceManager.getAudioDeviceSetup().bufferSize);
  audioGraph->addConnection({{audioInputNode->nodeID, 0}, {spectrogramNode->nodeID, 0}});
  // This is the best way I've found to get the editor and be able to display it. Just have your
  // mainComponent own a pointer to an editor, then point it to the editor when it's created.
  spectrogramEditor =
    std::unique_ptr<juce::AudioProcessorEditor>(spectrogramNode->getProcessor()->createEditor());

  audioSettings.button->setBounds(getLocalBounds().removeFromTop(50));
  addAndMakeVisible(audioSettings.button.get());
  addAndMakeVisible(spectrogramEditor.get());
}
// TrackGroup* trackGroupPtr = dynamic_cast<TrackGroup*>(graphElementList.data()[groupID].get());
//==============================================================================
void MainComponent::timerCallback() {
  // Should be able to set the timer and call repaint from within the processorEditor itself, but it
  // gives segfaults when I try, so I'm setting up the timer and repaint call from here in the
  // mainComponent because it seems to work.
  spectrogramEditor->repaint();
}

void MainComponent::paint(juce::Graphics& g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
  // g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  // g.setFont(juce::Font(16.0f));
  // g.setColour(juce::Colours::white);
  // g.drawText("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized() {
  // This is called when the MainComponent is resized.
  // If you add any child components, this is where you should
  // update their positions.
  audioSettings.button->setBounds(getLocalBounds().removeFromTop(50));
}

MainComponent::~MainComponent() {
  deviceManager.closeAudioDevice();
  // unfortunately the naming convention to de-allocate a unique pointer is .reset()
  audioGraph.reset();
  deleteAllChildren();
}
