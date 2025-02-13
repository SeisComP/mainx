/***************************************************************************
 * Copyright (C) gempa GmbH                                                *
 * All rights reserved.                                                    *
 * Contact: gempa GmbH (seiscomp-dev@gempa.de)                             *
 *                                                                         *
 * GNU Affero General Public License Usage                                 *
 * This file may be used under the terms of the GNU Affero                 *
 * Public License version 3.0 as published by the Free Software Foundation *
 * and appearing in the file LICENSE included in the packaging of this     *
 * file. Please review the following information to ensure the GNU Affero  *
 * Public License version 3.0 requirements will be met:                    *
 * https://www.gnu.org/licenses/agpl-3.0.html.                             *
 *                                                                         *
 * Other Usage                                                             *
 * Alternatively, this file may be used in accordance with the terms and   *
 * conditions contained in a signed written agreement between you and      *
 * gempa GmbH.                                                             *
 ***************************************************************************/


#ifndef SEISCOMP_MAPVIEWX_PROCESSOR_H
#define SEISCOMP_MAPVIEWX_PROCESSOR_H


#include <seiscomp/core/recordsequence.h>
#include <seiscomp/processing/waveformprocessor.h>


namespace Seiscomp {
namespace MapViewX {


DEFINE_SMARTPOINTER(GroundMotionProcessor);
class GroundMotionProcessor : public Processing::WaveformProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		GroundMotionProcessor();
		//! D'tor
		~GroundMotionProcessor();


	// ----------------------------------------------------------------------
	//  Ground motion interface
	// ----------------------------------------------------------------------
	public:
		double amplitude() const { return _amplitude; }
		const Core::Time &timestamp() const { return _amplitudeTimeStamp; }

		RecordSequence *rawData() const { return _rawData; }
		RecordSequence *velocityData() const { return _velocityData; }
		RecordSequence *processedData() const { return _processedData; }

		double dataScale() const { return _scaleToGroundMotion; }

		Filter *createVelocityFilter() const;


	// ----------------------------------------------------------------------
	//  WaveformProcessor interface
	// ----------------------------------------------------------------------
	public:
		virtual void reset();
		virtual void initFilter(double fsamp);
		virtual bool setup(const Processing::Settings &settings);
		virtual void fill(size_t n, double *samples);
		virtual void process(const Record *record,
		                     const DoubleArray &filteredData);
		bool handleGap(Filter *filter, const Core::TimeSpan&,
		               double lastSample, double nextSample,
		               size_t missingSamples);


	private:
		double      _scaleToGroundMotion{0};
		double      _amplitude;
		Core::Time  _amplitudeTimeStamp;

		RingBuffer *_rawData{nullptr}; //!< Raw data
		RingBuffer *_velocityData{nullptr}; //!< Data converted to velocity
		RingBuffer *_processedData{nullptr}; //!< Velocity converted to max amplitudes

		Core::SmartPointer<Filter>::Impl _velocityFilter;
		Core::SmartPointer<Filter>::Impl _maxAmpFilter;
};


}
}


#endif
