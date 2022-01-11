/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class WillDDLtapAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    WillDDLtapAudioProcessor();
    ~WillDDLtapAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    float getDelayTime(){
        return delayTime;
    };
    float getFeedback(){
        return feedbackParam;
    };
    float getWetLevel(){
        return wetLevelParam;
    };
    
    void setDelayTime(float dt){
        delayTime = dt;
    };
    void setFeedback(float fb){
        feedbackParam = fb;
    };
    void setWetLevel(float wl){
        wetLevelParam = wl;
    };
    
    void cookVariables(){
        delaySamples = delayTime*(currentSampleRate/1000.0);
        feedback = feedbackParam/100.0;
        wetLevel = wetLevelParam/100.0;
        for (int i=0; i<taps; i++)
        {
            readIndexL[i] = writeIndexL - (int)delaySamples*(i+1);
            readIndexR[i] = writeIndexR - (int)delaySamples*(i+1);
            // check and wrap BACKWARDS if the index is negative
            if(readIndexL[i] < 0)
                readIndexL[i] += bufferSize;     // amount of wrap is Read + Length
            if(readIndexR[i] < 0)
                readIndexR[i] += bufferSize;     // amount of wrap is Read + Length
        }
    };
    
    void resetDelay()
    {
        // flush buffer
        if(pBufferL)
            memset(pBufferL, 0, bufferSize*sizeof(float));
        if(pBufferR)
            memset(pBufferR, 0, bufferSize*sizeof(float));
        // init read/write indices
        writeIndexL = 0; // reset the Write index to top
        writeIndexR = 0;
        for (int i=0; i<10; i++)
        {
            readIndexL[i] = 0; // reset the Read index to top
            readIndexR[i] = 0;
        }
    };
    
    void switchCross()
    {
        cross = !cross;
    };
    bool getCross()
    {
        return cross;
    };

    void setTaps(int t)
    {
        taps = t;
    };
    int getTaps()
    {
        return taps;
    };
private:
    
    float currentSampleRate;
    float delayTime;
    float delaySamples;
    float feedback, feedbackParam;
    float wetLevel, wetLevelParam;
    
    // circular buffer
    float* pBufferL;
    float* pBufferR;
    int readIndexL[10];
    int readIndexR[10];
    int writeIndexL;
    int writeIndexR;
    int bufferSize;
    
    //cross button enable/disable
    bool cross;
    
    //taps
    int taps;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WillDDLtapAudioProcessor)
};
