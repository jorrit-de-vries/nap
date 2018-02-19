#include "timelinemodel.h"

using namespace napkin;

Event::Event(Track& parent, const QString& name, const qreal start, const qreal end)
		: mName(name), mStart(start), mEnd(end), QObject(&parent) {
	mColor = QColor(Qt::cyan);
}

void Event::setName(const QString& name) {
	mName = name;
	changed(*this);
}

void Event::setStart(const qreal start) {
	mStart = start;
	changed(*this);
}

void Event::setEnd(const qreal end) {
	mEnd = end;
	changed(*this);
}

void Event::setColor(const QColor& col) {
	mColor = col;
	changed(*this);
}

Track& Event::track() const { return *(Track*) parent(); }

void Event::setTrack(Track& track)
{
	setParent(&track);
}


qreal Event::length() const {
	return mEnd - mStart;
}

Track::Track(Timeline& parent, const QString& name) : mName(name), QObject(&parent) {}

void Track::setName(const QString& name) {
	mName = name;
	changed(*this);
}

Event* Track::addEvent(const QString& name, qreal start, qreal end) {
	auto event = new Event(*this, name, start, end);
	mEvents << event;
	eventAdded(*event);
	return event;
}

Timeline& Track::timeline() const { return *(Timeline*) parent(); }

int Track::index() { return timeline().children().indexOf(this); }


Track* Timeline::addTrack(const QString& name) {
	auto track = new Track(*this, name);
	mTracks << track;
	trackAdded(*track);
	return track;
}

void Timeline::removeTrack(Track& track) {
	trackRemoved(track);
	mTracks.removeOne(&track);
}
