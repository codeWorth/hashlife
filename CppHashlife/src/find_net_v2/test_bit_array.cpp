#include "avx_bit_array.h"
#include "constants.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {

    int size = 256;

    AvxBitArray bitArray(size);
    cout << "zeros: " << bitArray.toString() << endl;
    
    AvxBitArray onesArray(size, true);
    cout << "ones: " << onesArray.toString() << endl;

    for (size_t i = 0; i < size; i++) {
        bitArray.set(i, i % 3 == 0);
    }
    cout << "every 3: " << bitArray.toString() << endl;

    cout << "every 3 (get): ";
    for (size_t i = 0; i < size; i++) {
        cout << (bitArray.get(i) ? '1' : '0');
    }
    cout << endl;

    cout << bitArray.toString() << " all zero: " << (bitArray.none() ? "yes" : "no") << endl;
    bitArray.zero();
    cout << bitArray.toString() << " all zero: " << (bitArray.none() ? "yes" : "no") << endl;

    
    AvxBitArray shiftTests(size);
    for (size_t i = 0; i < size; i++) {
        shiftTests.set(i, i % 3 == 0);
    }

    cout << "shifting left (creation): " << endl;
    for (size_t i = size - 32; i < size; i++) {
        cout << "\t" << i << "\t" << (shiftTests << i).toString(32) << endl;
    }
    cout << endl;

    cout << "shifting right (assignment): " << endl;
    for (size_t i = 0; i < 32; i++) {
        AvxBitArray test(shiftTests);
        test >>= i;
        cout << "\t" << i << "\t" << test.toString(32) << endl;
    }
    cout << endl;

    AvxBitArray cmpTest(size);
    cout << cmpTest.toString() << (cmpTest == shiftTests ? " == " : " != ") << shiftTests.toString() << endl;

    for (size_t i = 0; i < size; i++) {
        cmpTest.set(i, shiftTests.get(i));
    }
    cout << cmpTest.toString() << (cmpTest == shiftTests ? " == " : " != ") << shiftTests.toString() << endl;
}