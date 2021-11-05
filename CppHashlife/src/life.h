#pragma once

class Life {
public:
	virtual void setup() = 0;
	virtual void tick() = 0;
	virtual void draw(char* pixelBuffer) = 0;
	
};