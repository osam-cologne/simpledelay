/*
 * Simple Delay audio efffect based on DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2018 Christopher Arndt <info@chrisarndt.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <math.h>

#include "PluginSimpleDelay.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

PluginSimpleDelay::PluginSimpleDelay()
    : Plugin(paramCount, 1, 0)  // paramCount params, 1 program(s), 0 states
{
    smooth_delay = new CParamSmooth(PARAM_SMOOTH_TIME, getSampleRate());
    loadProgram(0);
}

PluginSimpleDelay::~PluginSimpleDelay() {
    if (buffer != nullptr) {
        delete[] buffer;
        buffer = nullptr;
    }
    if (smooth_delay != nullptr) {
        delete smooth_delay;
        smooth_delay = nullptr;
    }
}


// -----------------------------------------------------------------------
// Init

void PluginSimpleDelay::initParameter(uint32_t index, Parameter& parameter) {
    if (index >= paramCount)
        return;

    parameter.ranges.min = 0.0f;
    parameter.ranges.max = 1.0f;
    parameter.ranges.def = 0.1f;
    parameter.hints = kParameterIsAutomable | kParameterIsLogarithmic;

    switch (index) {
        case paramDelay:
            parameter.name = "Delay";
            parameter.symbol = "delay";
            parameter.ranges.max = 5000.0f;
            parameter.ranges.def =  240.0f;
            parameter.unit = "ms";
            break;
        case paramFeedback:
            parameter.name = "Feedback";
            parameter.symbol = "feedback";
            parameter.ranges.max = 100.0f;
            parameter.ranges.def =  20.0f;
            parameter.hints |= kParameterIsInteger;
            break;
        case paramMix:
            parameter.name = "Dry/Wet Mix";
            parameter.symbol = "mix";
            parameter.ranges.min = -100.0f;
            parameter.ranges.max =  100.0f;
            parameter.ranges.def =    0.0f;
            parameter.unit = "%";
            parameter.hints |= kParameterIsInteger;
            break;
    }
}

/**
  Set the name of the program @a index.
  This function will be called once, shortly after the plugin is created.
*/
void PluginSimpleDelay::initProgramName(uint32_t index, String& programName) {
    switch (index) {
        case 0:
            programName = "Default";
            break;
    }
}

// -----------------------------------------------------------------------
// Internal data

/**
  Optional callback to inform the plugin about a sample rate change.
*/
//~void PluginSimpleDelay::sampleRateChanged(double newSampleRate) {
    //~allocateBuffer(newSampleRate);
//~}

/**
  Get the current value of a parameter.
*/
float PluginSimpleDelay::getParameterValue(uint32_t index) const {
    return fParams[index];
}

/**
  Change a parameter value.
*/
void PluginSimpleDelay::setParameterValue(uint32_t index, float value) {
    fParams[index] = value;

    switch (index) {
        case paramDelay:
            delaylen = value / 1000 * getSampleRate();
            break;
        case paramFeedback:
            feedback = value / 100;
            break;
        case paramMix:
            drywetmix = (value + 100) / 201;
            break;
    }
}

/**
  Load a program.
  The host may call this function from any context,
  including realtime processing.
*/
void PluginSimpleDelay::loadProgram(uint32_t index) {
    switch (index) {
        case 0:
            setParameterValue(paramDelay, 240.f);
            setParameterValue(paramFeedback, 20.f);
            setParameterValue(paramMix, 0.0f);
            break;
    }
}

void PluginSimpleDelay::allocateBuffer(double sampleRate) {
    buflen = (MAX_DELAY_TIME / 1000) * (uint32_t)sampleRate;
    buffer = new (std::nothrow) AmpVal [buflen];

    if (buffer == nullptr) {
        buflen = 0;
    }
    else {
        std::memset(buffer, 0, sizeof(AmpVal) * buflen);
        writepos = 0;
    }
}

// -----------------------------------------------------------------------
// Process

void PluginSimpleDelay::activate() {
    double fs = getSampleRate();
    // plugin is activated
    allocateBuffer(fs);
    smooth_delay->initialize(PARAM_SMOOTH_TIME, fs);
}

void PluginSimpleDelay::deactivate() {
    if (buffer != nullptr) {
        delete[] buffer;
        buffer = nullptr;
    }
}

void PluginSimpleDelay::run(const AmpVal** inputs, AmpVal** outputs,
                                  uint32_t frames) {
    if (buflen > 0) {
        // get the left and right audio inputs
        const AmpVal* const inpL = inputs[0];
        const AmpVal* const inpR = inputs[1];

        // get the left and right audio outputs
        AmpVal* const outL = outputs[0];
        AmpVal* const outR = outputs[1];

        // apply gain against all samples
        for (uint32_t i=0; i < frames; ++i) {
            // Read from delay line with fixed delay length in samples
            int32_t readpos = writepos - smooth_delay->process(delaylen);

            // wrap around sample read position index
            if (readpos < 0) {
                readpos = buflen + readpos;
            }

            // read delayed sample from delay buffer
            AmpVal out = buffer[readpos];

            // read left and write sample from input buffer,
            AmpVal left = inpL[i];
            AmpVal right = inpR[i];
            // mix them with delay according to dry/wet mix param
            // and set the left and right output samples
            AmpVal wetsignal = out * drywetmix;
            outL[i] = left * (1 - drywetmix) + wetsignal;
            outR[i] = right * (1 - drywetmix) + wetsignal;
            // save mono mix of input mixed with delay, attenuated by feedback,
            // to delay line at current write position and increase delay
            // position pointer
            buffer[writepos++] = (left + right) / 2 + out * feedback;

            if (writepos >= buflen) {
                writepos = 0;
            }
        }
    } // buflen > 0
}

// -----------------------------------------------------------------------

Plugin* createPlugin() {
    return new PluginSimpleDelay();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
