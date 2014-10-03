
// Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
// Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright 
//     notice, this list of conditions and the following disclaimer in the 
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of the Computer Graphics Systems Group at the 
//     Hasso-Plattner-Institute (HPI), Germany nor the names of its 
//     contributors may be used to endorse or promote products derived from 
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.

#include "brightstars.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <algorithm>

#include <assert.h>


namespace
{
    const unsigned int NUM_BRIGHTSTARS(9093);
}


namespace osgHimmel
{

BrightStars::BrightStars(const char *fileName)
:   m_stars(NULL)
,   m_numStars(0)
{
    fromFile(fileName);
}


BrightStars::~BrightStars()
{
    delete[] m_stars;
}


const BrightStars::s_BrightStar *BrightStars::stars() const
{
    return m_stars;
}


const unsigned int BrightStars::numStars() const
{
    return m_numStars;
}


unsigned int BrightStars::fromFile(const char *fileName)
{
    if(m_stars)
        delete[] m_stars;

    // Retrieve file size.

    FILE *f;
#ifdef __GNUC__
    f = std::fopen(fileName, "r");
#else // __GNUC__
    fopen_s(&f, fileName, "r");
#endif // __GNUC__

    if(!f)
        return 0;
    
    std::fseek(f, 0, SEEK_END);
    const long fileSize = std::ftell(f);

    std::fclose(f);


    m_numStars = fileSize / sizeof(s_BrightStar);
    //assert(NUM_BRIGHTSTARS == numStars);

    m_stars = new s_BrightStar[m_numStars];

    std::ifstream instream(fileName, std::ios::binary);

    instream.read(reinterpret_cast<char*>(m_stars), fileSize);

    return m_numStars;
}


unsigned int BrightStars::toFile(const char *fileName) const
{
    std::ofstream outstream(fileName, std::ios::binary);

    outstream.write((char*)&m_stars, sizeof(m_stars) * m_numStars);
    outstream.close();

    return m_numStars;
}

} // namespace osgHimmel
