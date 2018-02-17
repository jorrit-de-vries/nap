#pragma once

#include <QGraphicsScene>
#include <QtWidgets/QGraphicsItemGroup>
#include "timelinemodel.h"

namespace napkin {

	class TrackItem;
	class EventItem;




	class TimelineScene : public QGraphicsScene {
	public:
		TimelineScene();
		void setModel(Timeline* timeline);
		Timeline* timeline() const { return mTimeline; }
	private:
		void onTrackAdded(Track& track);
		void onTrackRemoved(Track& track);
		void onEventAdded(Event& event);
		void onEventRemoved(Event& event);

		TrackItem* trackItem(Track& track);

		EventItem* eventItem(Event& event);


		Timeline* mTimeline = nullptr;

		QGraphicsItemGroup mTrackGroup;
		QGraphicsItemGroup mEventGroup;


	};
}