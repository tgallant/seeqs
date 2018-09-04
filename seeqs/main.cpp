//
//  main.cpp
//  seeqs
//
//  Created by Tim Gallant on 9/3/18.
//  Copyright © 2018 Tim Gallant. All rights reserved.
//

#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "RtMidi.h"

struct Note {
    int pitch;
    int velocity;
};

class Seeqs {
    RtMidiIn* midiin;
    RtMidiOut* midiout;
    int gate;
    int rate;
    int step;
    int stepIdx;
    int note;
    int clocks;
    std::vector<Note> notes;
    std::vector<int> steps;
public:
    Seeqs (RtMidiIn*, RtMidiOut*);
    void increment ();
    void newNote (int, int);
    Note nextNote ();
    void newStep (int);
    int nextStep ();
};

Seeqs::Seeqs (RtMidiIn* in, RtMidiOut* out) {
    midiin = in;
    midiout = out;
    gate = 100;
    rate = 3;
    step = 1;
    stepIdx = 0;
    clocks = 0;
}

void Seeqs::newNote (int pitch, int velocity) {
    Note n;
    n.pitch = pitch;
    n.velocity = velocity;
    notes.push_back(n);
}

Note Seeqs::nextNote () {
    int currentNote = note;
    if (note == notes.size() - 1) {
        note = 0;
    } else {
        note += 1;
    }
    return notes[currentNote];
}

void Seeqs::newStep (int length) {
    steps.push_back(length);
    if (steps.size() == 1) {
        step = nextStep();
    }
}

int Seeqs::nextStep () {
    int currentStep = stepIdx;
    if (stepIdx != 0 && stepIdx == steps.size() - 1) {
        stepIdx = 0;
    } else {
        stepIdx += 1;
    }
    return steps[currentStep];
}

void Seeqs::increment () {
    if (clocks == 0) {
        Note n = nextNote();
        std::vector<unsigned char> onmessage(3);
        std::vector<unsigned char> offmessage(3);
        // Note On: 144, 64, 90
        onmessage[0] = 144;
        onmessage[1] = n.pitch;
        onmessage[2] = n.velocity;
        midiout->sendMessage(&onmessage);
        std::this_thread::sleep_for(std::chrono::milliseconds(gate));
        // Note Off: 128, 64, 40
        offmessage[0] = 128;
        offmessage[1] = n.pitch;
        offmessage[2] = n.velocity;
        midiout->sendMessage(&offmessage);
    }
    if (clocks == (rate * step) - 1) {
        clocks = 0;
        step = nextStep();
    } else {
        clocks += 1;
    }
}

void mycallback (double deltatime, std::vector< unsigned char > *message, void *userData) {
    Seeqs* seeqs = static_cast<Seeqs*>(userData);
    unsigned int code = (int)message->at(0);
    
    if (code == 248) {
        seeqs->increment();
    }
}

int main() {
    RtMidiIn *midiin = new RtMidiIn();
    RtMidiOut *midiout = new RtMidiOut();
    
    midiin->openPort(0);
    midiout->openPort(0);

    // Don't ignore sysex, timing, or active sensing messages.
    midiin->ignoreTypes(true, false, true);
    
    Seeqs seeqs (midiin, midiout);
    
    seeqs.newNote(64, 90);
    seeqs.newNote(69, 90);
    seeqs.newNote(74, 90);
    seeqs.newNote(59, 90);
    seeqs.newNote(64, 90);
    
    seeqs.newStep(2);
    seeqs.newStep(3);
    seeqs.newStep(3);
    seeqs.newStep(2);
    seeqs.newStep(2);
    seeqs.newStep(3);
    
    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the
    // queue.
    midiin->setCallback(&mycallback, &seeqs);
    std::cout << "\nReading MIDI input ... press <enter> to quit.\n";
    char input;
    std::cin.get(input);
    // Clean up
cleanup:
    delete midiin;
    delete midiout;
    return 0;
}
