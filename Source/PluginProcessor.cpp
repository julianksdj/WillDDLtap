/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WillDDLtapAudioProcessor::WillDDLtapAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // reset circular buffer
    for (int i=0; i<10; i++)
    {
        readIndexL[i] = 0;
        readIndexR[i] = 0;
    }
    writeIndexL = 0;
    writeIndexR = 0;

    // no buffer yet because we don’t have a sample rate yet
    pBufferL = NULL;
    pBufferR = NULL;
    bufferSize = 0;
}

WillDDLtapAudioProcessor::~WillDDLtapAudioProcessor()
{
    // delete buffer if it exists
    if(pBufferL)
        delete [] pBufferL;
    if(pBufferR)
        delete [] pBufferR;
}

//==============================================================================
const juce::String WillDDLtapAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WillDDLtapAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WillDDLtapAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WillDDLtapAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WillDDLtapAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WillDDLtapAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int WillDDLtapAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WillDDLtapAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String WillDDLtapAudioProcessor::getProgramName (int index)
{
    return {};
}

void WillDDLtapAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void WillDDLtapAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    currentSampleRate = sampleRate;
    
    bufferSize = 20 * currentSampleRate;     // 2 seconds delay @ fs

    pBufferL = new float[bufferSize];
    pBufferR = new float[bufferSize];
    
    resetDelay();
    
    setFeedback(50.f);
    setDelayTime(1000.f);
    setWetLevel(50.f);
    cookVariables();
    
    cross = false;
    
    taps = 1;
}

void WillDDLtapAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WillDDLtapAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void WillDDLtapAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    //for (int channel = 0; channel < totalNumInputChannels; ++channel)
    //{
    
    // channel L
    auto* channelDataL = buffer.getWritePointer (0); // write pointer to channel L
    auto* channelDataR = buffer.getWritePointer (1); // write pointer to channel R
    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Read the Input
        float xnL = channelDataL[sample];
        float xnR = channelDataR[sample];
        // Read the output of the delay at readIndex
        float ynL=0.f, ynR=0.f;
        float snL=0.f, snR=0.f;
        if (cross)
        {
            for(int i=0; i<(taps-1); i++)
            {
                snL += pBufferR[readIndexR[i]];
                snR += pBufferL[readIndexL[i]];
            }
            ynL = pBufferR[readIndexR[taps-1]];
            ynR = pBufferL[readIndexL[taps-1]];
        }
        else
        {
            for(int i=0; i<(taps-1); i++)
            {
                snL += pBufferL[readIndexL[i]];
                snR += pBufferR[readIndexR[i]];
            }
            ynL = pBufferL[readIndexL[taps-1]];
            ynR = pBufferR[readIndexR[taps-1]];
        }
        
        // if zero delay, just pass the input to output
        if(delaySamples == 0)
        {
            ynL = xnL;
            ynR = xnR;
            snL = 0.f;
            snR = 0.f;
        }
        
        // write the input to the delay
        pBufferL[writeIndexL] = xnL + feedback*ynL;
        pBufferR[writeIndexR] = xnR + feedback*ynR;
        
        // create the wet/dry mix and write to the output buffer
        // dry = 1 - wet
        channelDataL[sample] = wetLevel * (ynL + snL) + (1.f - wetLevel)*xnL;
        channelDataR[sample] = wetLevel * (ynR + snR) + (1.f - wetLevel)*xnR;
        
        // incremnent the pointers and wrap if necessary
        writeIndexL++;
        writeIndexR++;
        if(writeIndexL >= bufferSize)
            writeIndexL = 0;
        if(writeIndexR >= bufferSize)
            writeIndexR = 0;
        for(int i=0; i<taps; i++)
        {
            readIndexL[i]++;
            readIndexR[i]++;
            if(readIndexL[i] >= bufferSize)
            {
                readIndexL[i] = 0;
            }
            if(readIndexR[i] >= bufferSize)
            {
                readIndexR[i] = 0;
            }
        }
    }
}

//==============================================================================
bool WillDDLtapAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* WillDDLtapAudioProcessor::createEditor()
{
    return new WillDDLtapAudioProcessorEditor (*this);
}

//==============================================================================
void WillDDLtapAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void WillDDLtapAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WillDDLtapAudioProcessor();
}
