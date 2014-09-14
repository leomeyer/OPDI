// main_io: common IO functions

#ifndef __MAIN_IO_H
#define __MAIN_IO_H

#include <stdio.h>
#include <iostream>
#include <conio.h>

const char *read_line (char *buf, size_t length, FILE *f);

typedef struct OutputType
{
	// adapted from: http://stackoverflow.com/questions/1134388/stdendl-is-of-unknown-type-when-overloading-operator

    template <typename T>
    OutputType& operator<<(const T& x)
    {
        std::cout << x;

        return *this;
    }

    // function that takes a custom stream, and returns it
    typedef OutputType& (*MyStreamManipulator)(OutputType&);

    // take in a function with the custom signature
    OutputType& operator<<(MyStreamManipulator manip)
    {
        // call the function, and return it's value
        return manip(*this);
    }

    // define the custom endl for this stream.
    // note how it matches the `MyStreamManipulator`
    // function signature
    static OutputType& endl(OutputType& stream)
    {
        // print a new line
        std::cout << std::endl;

        // do other stuff with the stream
        // std::cout, for example, will flush the stream
        stream << "Called OutputType::endl!" << std::endl;

        return stream;
    }

    // this is the type of std::cout
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;

    // this is the function signature of std::endl
    typedef CoutType& (*StandardEndLine)(CoutType&);

    // define an operator<< to take in std::endl
    OutputType& operator<<(StandardEndLine manip)
    {
        // call the function, but we cannot return it's value
        manip(std::cout);

        return *this;
    }

} OUTPUT_TYPE;

// global output stream instance
extern OUTPUT_TYPE output;

std::string getLine(std::string message);


#endif
