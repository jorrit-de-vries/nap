#include "timelinemodel.h"
#include <QtDebug>
#include <cassert>

using namespace napkin;

Event::Event(Track& parent, const QString& name, const qreal start, const qreal end)
		: mName(name), mRange(start, end), QObject(&parent)
{
	mColor = QColor(Qt::cyan);
}

void Event::setName(const QString& name)
{
	mName = name;
	changed(*this);
}

void Event::setStart(const qreal start)
{
	mRange.setStart(start);
	changed(*this);
}

void Event::setEnd(const qreal end)
{
	mRange.setEnd(end);
	changed(*this);
}

void Event::setColor(const QColor& col)
{
	mColor = col;
	changed(*this);
}

Track& Event::track() const
{ return *(Track*) parent(); }

void Event::setTrack(Track& track)
{
	setParent(&track);
}


qreal Event::length() const
{
	return mRange.length();
}

void Event::moveTo(qreal start)
{
	mRange.moveTo(start);
	changed(*this);
}

void Event::move(qreal delta)
{
	mRange.move(delta);
	changed(*this);
}

void Event::setRange(const Range& range)
{
	mRange.set(range);
	changed(*this);
}

qreal Event::minLength() const
{
	return track().timeline().minEventLength();
}

qreal Event::maxLength() const
{
	return track().timeline().maxEventLength();
}

Track::Track(QObject& parent, const QString& name) : mName(name), QObject(&parent)
{

	auto p = parentTrack();
	if (p) {
		connect(this, &Track::changed, p, &Track::changed);
	}
}

void Track::setName(const QString& name)
{
	mName = name;
	changed(*this);
}

Event* Track::addEvent(const QString& name, qreal start, qreal end)
{
	auto event = new Event(*this, name, start, end);
	mEvents << event;
//	connect(event, &Event::changed, [this](Event& evt) {
//		changed(*this);
//	});
	eventAdded(*event);
	changed(*this);
	return event;
}

Tick* Track::addTick(qreal time)
{
	auto tick = new Tick(*this, time);
	mTicks << tick;
//	connect(tick, &Tick::changed, [this](Tick& tick) {
//		changed(*this);
//	});
	tickAdded(*tick);
	return tick;
}

Timeline& Track::timeline() const
{
	auto timeline = dynamic_cast<Timeline*>(parent());
	if (timeline)
		return *timeline;

	auto parentTrack = dynamic_cast<Track*>(parent());
	assert(parentTrack);
	return parentTrack->timeline();
}

int Track::index()
{
	return parent()->children().indexOf(this);
}

Track* Track::addTrack(const QString& name)
{
	auto track = new Track(*this, name);
	mChildren << track;
	trackAdded(*track);
	changed(*this);
	return track;
}

Track* Track::parentTrack() const
{
	return dynamic_cast<Track*>(parent());
}

bool Track::range(qreal& start, qreal& end)
{
	QList<Event*> allEvents;
	eventsRecursive(allEvents);
	if (allEvents.empty()) return false;
	start = std::numeric_limits<qreal>::max();
	end   = std::numeric_limits<qreal>::min();
	for (const auto evt : allEvents)
	{
		start = qMin(start, evt->start());
		end   = qMax(end, evt->end());
	}
	return true;
}

void Track::eventsRecursive(QList<Event*>& events) const
{
	for (const auto evt : mEvents)
		events << evt;
	for (const auto track : mChildren)
		track->eventsRecursive(events);
}

Tick::Tick(Track& parent, qreal time) : mTime(time), QObject(&parent)
{

}

void Tick::setTime(qreal time)
{
	mTime = time;
	changed(*this);
}

Track* Timeline::addTrack(const QString& name, Track* parent)
{
	if (parent != nullptr)
		return parent->addTrack(name);

	auto track = new Track(*this, name);
	mTracks << track;
	trackAdded(*track);
	return track;
}

void Timeline::removeTrack(Track& track)
{
	trackRemoved(track);
	mTracks.removeOne(&track);
}
