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

#pragma once

#include "typedefs.h"

#include <boost/timer/timer.hpp>
#include <time.h>

namespace osgHimmel {
	// TimeF manages an osg::Timer and features an interface for floating time
	// in the closed interval [0;1] representing a full day and standard c
	// time (time_t) simultaneously. The time updates have to be requested
	// explicitly, thus simplifying usage between multiple recipients.
	// The time starts cycling automatically, but can also be paused, stopped,
	// or set to a specific value.
	class TimeF {
	public:
		static long utcOffset();

		TimeF(t_longf time = 0.0);

		TimeF(const time_t &time, const time_t &utcOffset);

		~TimeF();

		// Float time in the intervall [0;1]
		inline const t_longf getf() const {
			return m_timef[1];
		}

		t_longf getf();

		// Sets only time, date remains unchanged.
		t_longf setf(t_longf time);

		// Elapsed float time from initialized time.
		t_longf getNonModf();

		// Time in seconds from initial time.
		inline const time_t gett() const {
			return m_time[1] + utcOffset();
		}

		time_t gett();

		time_t sett(const time_t &time);

		time_t getUtcOffset() const;

		time_t setUtcOffset(const time_t &utcOffset /* In Seconds. */);

	protected:
		static inline const t_longf secondsTof(const time_t &time);

		static inline const time_t fToSeconds(t_longf time);

		void initialize();

	protected:
		time_t m_utcOffset;

		time_t m_time[3];       // [2] is for stop
		t_longf m_timef[3]; // [2] is for stop

		t_longf m_offset;
	};

} // namespace osgHimmel
