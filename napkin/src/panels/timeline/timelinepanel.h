#pragma once

#include "timelinemodel.h"
#include "timelineview.h"
#include "timelinescene.h"
#include "gridview.h"

#include <cassert>

#include <QPaintEvent>
#include <QPainter>
#include <QtGui>
#include <QGraphicsView>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QStyleOptionFrame>
#include <QStylePainter>
#include <QVBoxLayout>
#include <QWidget>
#include <QGraphicsItem>

namespace napkin {



	class RulerWidget : public QWidget {
	public:
		RulerWidget();


	};


	class OutlineHeader : public QWidget {
	public:
		OutlineHeader() : QWidget() {}

	protected:
		void paintEvent(QPaintEvent* event) override;

	};


	class TLOutlineItem : public QWidget {
	public:
		TLOutlineItem(Track& track, QWidget* parent);

		Track& track() const { return mTrack; }
		void setHeight(int height);

	protected:
		void paintEvent(QPaintEvent* event) override;

	private:
		QHBoxLayout mLayout;
		QLabel mLabel;
		Track& mTrack;
	};

	class TimelineOutline : public QWidget {
	public:
		TimelineOutline();

		void setModel(Timeline* timeline);
		void setHeaderHeight(int height);

	protected:
		void paintEvent(QPaintEvent* event) override;

	private:
		void onTrackAdded(Track& track);
		void onTrackRemoved(Track& track);
		TLOutlineItem* widget(Track& track);

		QWidget mHolder;
		QVBoxLayout mLayout;
		OutlineHeader mHeader;
		QList<TLOutlineItem*> mTracks;
		Timeline* mTimeline = nullptr;
	};


	class TimelineWidget : public QWidget {
	Q_OBJECT
	public:
		TimelineWidget();

		void setModel(Timeline* timeline);
		void setHeaderHeight(int height);
		int headerHeight() const { return mRuler.minimumHeight(); }

	private:

		QVBoxLayout mLayout;
		TimelineView mView;
		TimelineScene mScene;
		RulerWidget mRuler;

	};

	class TimelinePanel : public QWidget {
	public:
		TimelinePanel();

		void setTimeline(Timeline* timeline);
		void setHeaderHeight(int height);
		void demo();

	private:

		QVBoxLayout mLayout;
		QSplitter mSplitter;
		TimelineWidget mTimeline;
		TimelineOutline mOutline;
	};

}