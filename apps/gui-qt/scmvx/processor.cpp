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


#define SEISCOMP_COMPONENT MapView
#include <seiscomp/logging/log.h>
#include <seiscomp/math/filter.h>
#include <seiscomp/math/filter/chainfilter.h>
#include <seiscomp/math/filter/iirdifferentiate.h>
#include <seiscomp/math/filter/iirintegrate.h>
#include <seiscomp/math/filter/minmax.h>
#include <seiscomp/math/filter/abs.h>

#include "processor.h"
#include "settings.h"


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace Seiscomp {
namespace MapViewX {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GroundMotionProcessor::GroundMotionProcessor() {
	setUsedComponent(Vertical);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GroundMotionProcessor::~GroundMotionProcessor() {
	if ( _rawData ) {
		delete _rawData;
	}
	if ( _velocityData ) {
		delete _velocityData;
	}
	if ( _processedData ) {
		delete _processedData;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool GroundMotionProcessor::setup(const Processing::Settings &settings) {
	if ( !WaveformProcessor::setup(settings) ) {
		return false;
	}

	setSaturationCheckEnabled(false);

	const Processing::Stream &stream = streamConfig(VerticalComponent);
	SignalUnit unit;
	if ( !unit.fromString(stream.gainUnit.c_str()) ) {
		SEISCOMP_ERROR("%s.%s.%s.%s: no or invalid signal unit: '%s'",
		               settings.networkCode.c_str(), settings.stationCode.c_str(),
		               settings.locationCode.c_str(), settings.channelCode.c_str(),
		               stream.gainUnit.c_str());
		return false;
	}

	if ( stream.gain == 0 ) {
		SEISCOMP_ERROR("%s.%s.%s.%s: invalid gain: %f",
		               settings.networkCode.c_str(), settings.stationCode.c_str(),
		               settings.locationCode.c_str(), settings.channelCode.c_str(),
		               stream.gain);
		return false;
	}

	_scaleToGroundMotion = fabs(1.0 / stream.gain);

	Filter *f = Filter::Create(global.filter);
	if ( !f ) {
		SEISCOMP_ERROR("Could not create filter: %s", global.filter.c_str());
		return false;
	}

	Math::Filtering::ChainFilter<double> *chain = new Math::Filtering::ChainFilter<double>;

	// We want M/S
	switch ( unit ) {
		case Meter:
		{
			chain->add(new Math::Filtering::IIRDifferentiate<double>);
			break;
		}
		case MeterPerSecond:
			break;
		case MeterPerSecondSquared:
		{
			chain->add(new Math::Filtering::IIRIntegrate<double>);
			break;
		}
		default:
			break;
	}

	chain->add(f);

	_velocityFilter = chain;

	chain = new Math::Filtering::ChainFilter<double>;

	// Compute absolute values
	chain->add(new Math::Filtering::AbsFilter<double>);
	// Compute maximum amplitude of past 10 seconds
	chain->add(new Math::Filtering::Max<double>(static_cast<double>(global.maximumAmplitudeTimeSpan)));

	_maxAmpFilter = chain;

	_rawData = new RingBuffer(Core::TimeSpan(global.ringBuffer));
	_velocityData = new RingBuffer(Core::TimeSpan(global.ringBuffer));
	_processedData = new RingBuffer(Core::TimeSpan(global.ringBuffer));

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GroundMotionProcessor::reset() {
	WaveformProcessor::reset();
	_amplitude = -1;
	_amplitudeTimeStamp = Core::Time();
	_velocityFilter = _velocityFilter->clone();
	_maxAmpFilter = _maxAmpFilter->clone();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GroundMotionProcessor::initFilter(double fsamp) {
	if ( _velocityFilter ) _velocityFilter->setSamplingFrequency(fsamp);
	if ( _maxAmpFilter ) _maxAmpFilter->setSamplingFrequency(fsamp);
	WaveformProcessor::initFilter(fsamp);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GroundMotionProcessor::fill(size_t n, double *samples) {
	WaveformProcessor::fill(n, samples);
	if ( _velocityFilter ) {
		_velocityFilter->apply(n, samples);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GroundMotionProcessor::process(const Record *record,
                                    const DoubleArray &filteredData) {
	GenericRecordPtr rec = new GenericRecord(*record);
	rec->setData(filteredData.copy(Array::FLOAT));
	_velocityData->feed(rec.get());

	if ( _maxAmpFilter ) {
		_maxAmpFilter->apply(filteredData.size(), const_cast<double*>(filteredData.typedData()));
	}

	rec = new GenericRecord(*record);
	rec->setData(filteredData.copy(Array::FLOAT));
	_processedData->feed(rec.get());

	int sample = filteredData.size() - 1;
	if ( sample < 0 ) {
		return;
	}

	_amplitude = filteredData[sample] * _scaleToGroundMotion;
	_amplitudeTimeStamp = record->startTime() + Core::TimeSpan(sample / record->samplingFrequency());

	_rawData->feed(record);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool GroundMotionProcessor::handleGap(Filter *filter, const Core::TimeSpan&,
                                      double lastSample, double nextSample,
                                      size_t missingSamples) {
	reset();
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GroundMotionProcessor::Filter *GroundMotionProcessor::createVelocityFilter() const {
	return _velocityFilter ? _velocityFilter->clone() : nullptr;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
