/*
  ==============================================================================

    BoolHolder.h
    Created: 12 Dec 2019 1:56:40pm
    Author:  MC

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
class BoolHolder
{
public:
	BoolHolder(bool* state) : state(state) {}
	bool GetState() const { return *state; } // read only
	bool& GetState() { return *state; } // read/write

private:
	bool* state;
};
