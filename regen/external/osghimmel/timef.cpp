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

#include "timef.h"
#include "mathmacros.h"
#include <cmath>

using namespace osgHimmel;

long TimeF::utcOffset() {
	long tz;

#ifdef __GNUC__
	tz = timezone;
#else // __GNUC__
	_get_timezone(&tz);
#endif // __GNUC__

	return tz;
}

TimeF::TimeF(const t_longf time)
		: m_utcOffset(0), m_offset(0.f) {
	initialize();
	setf(time);
}

TimeF::TimeF(const time_t &time, const time_t &utcOffset)
		: m_utcOffset(utcOffset), m_offset(0.f) {
	initialize();
	sett(time);
}

void TimeF::initialize() {
	//m_lastModeChangeTime = elapsedSeconds(m_timer);

	m_timef[0] = 0.f;
	m_timef[1] = 0.f;
	m_timef[2] = 0.f;

	m_time[0] = 0;
	m_time[1] = 0;
	m_time[2] = 0;
}

TimeF::~TimeF() {
}

t_longf TimeF::getf() {
	return m_timef[1];
}

t_longf TimeF::setf(t_longf timef) {
	timef = _frac(timef);

	if (1.f == timef)
		timef = 0.f;

	m_timef[0] = timef;
	m_timef[2] = m_timef[0];

	m_offset = 0;

	const time_t seconds(fToSeconds(timef));

#ifdef __GNUC__
	struct tm lcl(*localtime(&m_time[1]));
#else // __GNUC__
	struct tm lcl;
	localtime_s(&lcl, &m_time[1]);
#endif // __GNUC__      

	lcl.tm_hour = seconds / 3600;
	lcl.tm_min = seconds % 3600 / 60;
	lcl.tm_sec = seconds % 60;

	time_t mt = mktime(&lcl);
	if (mt == -1)
		m_time[0] = m_time[2] = 0;
	else
		m_time[0] = m_time[2] = mktime(&lcl) - utcOffset();

	return getf();
}

t_longf TimeF::getNonModf() {
	return secondsTof(gett());
}

time_t TimeF::gett() {
	return m_time[1] + utcOffset();
}

time_t TimeF::sett(const time_t &time) {
	time_t t = time - utcOffset();

	m_time[0] = t;
    m_time[1] = fToSeconds(m_offset) + static_cast<t_longf>(m_time[0]);
	m_time[2] = m_time[0];

	m_timef[0] = _frac(secondsTof(t));
    m_timef[1] = _frac(m_timef[0] + m_offset);
	m_timef[2] = m_timef[0];

	m_offset = 0;

	return gett();
}

inline const t_longf TimeF::secondsTof(const time_t &time) {
	return static_cast<t_longf>((time) / (60.0 * 60.0 * 24.0));
}

inline const time_t TimeF::fToSeconds(const t_longf time) {
	return static_cast<time_t>(time * 60.0 * 60.0 * 24.0 + 0.1);
}

time_t TimeF::getUtcOffset() const {
	return m_utcOffset;
}

time_t TimeF::setUtcOffset(const time_t &utcOffset) {
	m_utcOffset = utcOffset;

	return getUtcOffset();
}
